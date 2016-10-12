//
// Created by Subhabrata Ghosh on 16/09/16.
//

#include <includes/common/lock_record_def.h>
#include <includes/core/control.h>
#include <includes/common/alarm.h>
#include "includes/core/resource_factory.h"


using namespace com::watergate::core;

void com::watergate::core::_semaphore::create(const _app *app, const ConfigValue *config, bool server) {
    CHECK_NOT_NULL(app);
    CHECK_NOT_NULL(config);

    const ConfigValue *r_node = config->find(CONST_SEM_CONFIG_NODE_RESOURCE);
    if (IS_NULL(r_node)) {
        throw CONFIG_ERROR("Required configuration node not found. [node=%s]", CONST_SEM_CONFIG_NODE_RESOURCE);
    }

    const BasicConfigValue *cn = Config::get_value(CONST_SEM_CONFIG_PARAM_RESOURCE_CLASS, r_node);
    if (IS_NULL(cn)) {
        throw ERROR_MISSING_CONFIG(CONST_SEM_CONFIG_PARAM_RESOURCE_CLASS);
    }
    const string r_class = cn->get_value();
    if (IS_EMPTY(r_class)) {
        throw CONFIG_ERROR("NULL/Empty configuration value for node. [node=%s]", CONST_SEM_CONFIG_PARAM_RESOURCE_CLASS);
    }


    this->resource = resource_factory::get_resource(r_class, r_node);

    this->name = new string(*this->resource->get_resource_name());

    this->is_server = server;

    const BasicConfigValue *v_prior = Config::get_value(CONST_SEM_CONFIG_NODE_PRIORITIES, config);
    if (IS_NULL(v_prior)) {
        throw CONFIG_ERROR("Required configuration node not found. [node=%s]", CONST_SEM_CONFIG_NODE_PRIORITIES);
    }
    this->priorities = v_prior->get_int_value(this->priorities);
    if (this->priorities > MAX_PRIORITY_ALLOWED) {
        throw CONFIG_ERROR("Invalid configuration value. [%s=%d][MAX=%d]", CONST_SEM_CONFIG_NODE_PRIORITIES,
                           this->priorities, MAX_PRIORITY_ALLOWED);
    }
    this->max_concurrent = this->resource->get_control_size();
    if (this->max_concurrent > SEM_VALUE_MAX) {
        throw CONFIG_ERROR("Invalid configuration value. [Max Concurrency=%d][MAX=%d]",
                           this->max_concurrent, SEM_VALUE_MAX);
    }
    const BasicConfigValue *v_mode = Config::get_value(CONST_SEM_CONFIG_NODE_MODE, config);
    if (NOT_NULL(v_mode)) {
        this->mode = v_mode->get_short_value(DEFAULT_SEM_MODE);
    }

    semaphores = (sem_t **) malloc(this->priorities * sizeof(sem_t *));
    memset(semaphores, 0, this->priorities * sizeof(sem_t *));

    for (int ii = 0; ii < this->priorities; ii++) {
        create_sem(ii);
    }

    for (int ii = 0; ii < this->priorities; ii++) {
        sem_t *ptr = semaphores[ii];
        if (IS_VALID_SEM_PTR(ptr))
            LOG_INFO("Created semaphore handle [name=%s][index=%d]", name->c_str(), ii);
        else
            throw CONTROL_ERROR("Invalid semaphore handle. [name=%s][index=%d]", name->c_str(), ii);
    }
}

void com::watergate::core::_semaphore::create_sem(int index) {
    CHECK(index >= 0 && index < priorities);

    string sem_name = common_utils::format("%s::%s::%d", CONTROL_LOCK_PREFIX, name->c_str(), index);

    sem_t *ptr = sem_open(sem_name.c_str(), O_CREAT, mode, max_concurrent);
    if (!IS_VALID_SEM_PTR(ptr)) {
        throw CONTROL_ERROR("Error creating semaphore. [name=%s][errno=%s]", sem_name.c_str(), strerror(errno));
    }

    semaphores[index] = ptr;
}

void com::watergate::core::_semaphore::delete_sem(int index) {
    CHECK(index >= 0 && index < priorities);

    if (IS_VALID_SEM_PTR(semaphores[index])) {
        if (!owner) {
            if (sem_close(semaphores[index]) != 0) {
                LOG_ERROR("Error disposing semaphore. [index=%s][errno=%s]", index, strerror(errno));
            }
        } else {
            string sem_name = common_utils::format("%s::%s::%d", CONTROL_LOCK_PREFIX, name->c_str(), index);
            if (sem_unlink(sem_name.c_str()) != 0) {
                LOG_ERROR("Error disposing semaphore. [index=%s][errno=%s]", index, strerror(errno));
            }
        }
        semaphores[index] = nullptr;
    }
}

com::watergate::core::_semaphore::~_semaphore() {
    CHECK_AND_FREE(this->name);
    CHECK_AND_FREE(resource);
    CHECK_AND_FREE(table);
}

lock_acquire_enum com::watergate::core::_semaphore_client::try_lock(int priority, int base_priority, bool wait) {
    CHECK(priority >= 0 && priority < priorities);
    CHECK(base_priority >= 0 && base_priority < priorities);

            ASSERT(NOT_NULL(semaphores));

    std::lock_guard<std::mutex> guard(counts[priority]->priority_lock);

    thread_lock_ptr *t_ptr = nullptr;
    thread_lock_record *t_rec = get_thread_lock();
    if (NOT_NULL(t_rec)) {
        t_ptr = t_rec->get_thread_ptr();
        if (t_ptr->priority_lock_index[priority]->id != counts[priority]->index) {
            t_ptr->priority_lock_index[priority]->id = -1;
        }
    } else {
        string tid = thread_lock_record::get_current_thread();
        if (IS_EMPTY(tid)) {
            tid = "UNKNOWN-THREAD";
        }
        throw CONTROL_ERROR("No lock record found for thread.[thread=%s]", tid.c_str());
    }

    lock_acquire_enum ls = client->has_valid_lock(priority);
    if (ls == Locked) {
        counts[priority]->count++;
        t_rec->increment(priority);
        t_ptr->priority_lock_index[priority]->id = counts[priority]->index;
        t_ptr->priority_lock_index[priority]->acquired_time = time_utils::now();
        return ls;
    } else if (ls == Expired) {
        LOG_DEBUG("Lock expired. [resetting all semaphores][priority=%d][base priority=%d]", priority, base_priority);
        reset_locks(priority);
    }

    sem_t *lock = get(priority);
    if (IS_VALID_SEM_PTR(lock)) {
        LOG_DEBUG("Waiting for semaphore. [name=%s][priority=%d]", this->name->c_str(), priority);
        int r = 0;
        if (wait) {
            r = sem_wait(lock);
        } else {
            r = sem_trywait(lock);
        }
        if (r == 0) {

            counts[priority]->count++;
            t_rec->increment(priority);
            t_ptr->priority_lock_index[priority]->id = counts[priority]->index;
            t_ptr->priority_lock_index[priority]->acquired_time = time_utils::now();
            client->update_lock(priority);

            LOG_DEBUG("Acquired semaphore. [name=%s][priority=%d][base priority=%d][lock count=%d]",
                      this->name->c_str(), priority,
                      base_priority, counts[priority]->count);

            return Locked;
        } else {
            if (wait) {
                LOG_DEBUG("Failed to acquire semaphore. [name=%s][priority=%d][base priority=%d][error=%s]",
                          this->name->c_str(), priority, base_priority,
                          strerror(errno));
                return Error;
            } else {
                return Retry;
            }
        }
    }
    throw CONTROL_ERROR("No lock found for the specified priority. [lock=%s][priority=%d]", this->name->c_str(),
                        priority);
}

lock_acquire_enum com::watergate::core::_semaphore_client::try_lock_base(double quota, int base_priority, bool wait) {
    CHECK(base_priority >= 0 && base_priority < priorities);

            ASSERT(NOT_NULL(semaphores));

    std::lock_guard<std::mutex> guard(counts[BASE_PRIORITY]->priority_lock);
    thread_lock_ptr *t_ptr = nullptr;
    thread_lock_record *t_rec = get_thread_lock();
    if (NOT_NULL(t_rec)) {
        t_ptr = t_rec->get_thread_ptr();
        if (t_ptr->priority_lock_index[BASE_PRIORITY]->id != counts[BASE_PRIORITY]->index) {
            t_ptr->priority_lock_index[BASE_PRIORITY]->id = -1;
        }
    } else {
        string tid = thread_lock_record::get_current_thread();
        if (IS_EMPTY(tid)) {
            tid = "UNKNOWN-THREAD";
        }
        throw CONTROL_ERROR("No lock record found for thread.[thread=%s]", tid.c_str());
    }

    lock_acquire_enum ls = client->check_and_lock(quota);
    if (ls == Locked) {
        counts[BASE_PRIORITY]->count++;
        t_rec->increment(BASE_PRIORITY);
        t_ptr->priority_lock_index[BASE_PRIORITY]->id = counts[BASE_PRIORITY]->index;
        t_ptr->priority_lock_index[BASE_PRIORITY]->acquired_time = time_utils::now();
        return ls;
    } else if (ls == QuotaReached) {
        return ls;
    } else if (ls == Expired) {
        LOG_DEBUG("Lock expired. [resetting all semaphores][priority=%d][base priority=%d]", BASE_PRIORITY,
                  base_priority);
        reset_locks(BASE_PRIORITY);
    }

    sem_t *lock = get(BASE_PRIORITY);
    if (IS_VALID_SEM_PTR(lock)) {
        int r = 0;
        if (wait) {
            r = sem_wait(lock);
        } else {
            r = sem_trywait(lock);
        }
        if (r == 0) {

            counts[BASE_PRIORITY]->count++;
            t_rec->increment(BASE_PRIORITY);
            t_ptr->priority_lock_index[BASE_PRIORITY]->id = counts[BASE_PRIORITY]->index;
            t_ptr->priority_lock_index[BASE_PRIORITY]->acquired_time = time_utils::now();
            client->update_lock(BASE_PRIORITY);
            client->update_quota(quota, base_priority);

            LOG_DEBUG("Acquired semaphore. [name=%s][priority=%d][base priority=%d][lock count=%d]",
                      this->name->c_str(),
                      BASE_PRIORITY, base_priority, counts[BASE_PRIORITY]->count);

            return Locked;
        } else if (errno == EAGAIN) {
            return Timeout;
        } else {
            if (wait) {
                LOG_DEBUG("Failed to acquire semaphore. [name=%s][priority=%d][base_priority=%d][error=%s]",
                          this->name->c_str(),
                          BASE_PRIORITY, base_priority,
                          strerror(errno));
                return Error;
            } else {
                return Retry;
            }
        }
    }
    throw CONTROL_ERROR("No lock found for the specified priority. [lock=%s][priority=%d]", this->name->c_str(),
                        BASE_PRIORITY);
}

bool com::watergate::core::_semaphore_client::release_lock_base(int base_priority) {
    CHECK(base_priority >= 0 && base_priority < priorities);

            ASSERT(NOT_NULL(semaphores));

    std::lock_guard<std::mutex> guard(counts[BASE_PRIORITY]->priority_lock);
    thread_lock_ptr *t_ptr = nullptr;
    thread_lock_record *t_rec = get_thread_lock();
    if (NOT_NULL(t_rec)) {
        t_ptr = t_rec->get_thread_ptr();
        if (t_ptr->priority_lock_index[BASE_PRIORITY]->id != counts[BASE_PRIORITY]->index) {
            t_ptr->priority_lock_index[BASE_PRIORITY]->id = -1;
            LOG_WARN(
                    "Lock index out-of-sync. [thread=%s][priority=%d][base priority=%d][current index=%d][new index=%d]",
                    t_ptr->thread_id.c_str(), BASE_PRIORITY, base_priority,
                    t_ptr->priority_lock_index[BASE_PRIORITY]->id,
                    counts[BASE_PRIORITY]->index);
            return true;
        }
    } else {
        string tid = thread_lock_record::get_current_thread();
        if (IS_EMPTY(tid)) {
            tid = "UNKNOWN-THREAD";
        }
        throw CONTROL_ERROR("No lock record found for thread.[thread=%s]", tid.c_str());
    }

    lock_acquire_enum ls = client->has_valid_lock(BASE_PRIORITY);
    if (ls == Locked) {
        counts[BASE_PRIORITY]->count--;
        t_rec->decremet(BASE_PRIORITY);

        if (counts[BASE_PRIORITY]->count <= 0) {
            t_ptr->priority_lock_index[BASE_PRIORITY]->id = -1;
            t_ptr->priority_lock_index[BASE_PRIORITY]->acquired_time = 0;

            sem_t *lock = get(BASE_PRIORITY);
            if (IS_VALID_SEM_PTR(lock)) {
                if (sem_post(lock) != 0) {
                    throw CONTROL_ERROR(
                            "Semaphores in invalid state. [name=%s][priority=%d][base priority=%d][error=%s]",
                            this->name->c_str(),
                            BASE_PRIORITY, base_priority, strerror(errno));
                }
                LOG_DEBUG("Released semaphore [name=%s][priority=%d][base priority=%d]", this->name->c_str(),
                          BASE_PRIORITY, base_priority);


                client->release_lock(ls, BASE_PRIORITY);
                return true;
            } else {
                throw CONTROL_ERROR("No semaphore found for the specified priority. [lock=%s][priority=%d]",
                                    this->name->c_str(),
                                    BASE_PRIORITY);
            }
        } else {
            LOG_DEBUG("Lock count pending. [priority=%d][base priority=%d][count=%d]", BASE_PRIORITY, base_priority,
                      counts[BASE_PRIORITY]->count);
        }
    } else if (ls == Expired) {
        LOG_DEBUG("Lock expired. [Lock should be retried][priority=%d][base priority=%d]", BASE_PRIORITY,
                  base_priority);
        reset_locks(BASE_PRIORITY);
        return true;
    } else {
        LOG_DEBUG("Lock already released. [state=%s][priority=%d][base priority=%d]",
                  record_utils::get_lock_acquire_enum_string(ls).c_str(), BASE_PRIORITY, base_priority);
    }
    LOG_DEBUG("Invalid lock state. [state=%s][priority=%d][base priority=%d]",
              record_utils::get_lock_acquire_enum_string(ls).c_str(), BASE_PRIORITY, base_priority);
    return false;
}

bool com::watergate::core::_semaphore_client::release_lock(int priority, int base_priority) {
    CHECK(priority >= 0 && priority < priorities);
    CHECK(base_priority >= 0 && base_priority < priorities);

            ASSERT(NOT_NULL(semaphores));

    std::lock_guard<std::mutex> guard(counts[priority]->priority_lock);
    thread_lock_ptr *t_ptr = nullptr;
    thread_lock_record *t_rec = get_thread_lock();
    if (NOT_NULL(t_rec)) {
        t_ptr = t_rec->get_thread_ptr();
        if (t_ptr->priority_lock_index[priority]->id != counts[priority]->index) {
            t_ptr->priority_lock_index[priority]->id = -1;
            LOG_WARN(
                    "Lock index out-of-sync. [thread=%s][priority=%d][base priority=%d][current index=%d][new index=%d]",
                    t_ptr->thread_id.c_str(), priority, base_priority, t_ptr->priority_lock_index[priority]->id,
                    counts[priority]->index);
            return true;
        }
    } else {
        string tid = thread_lock_record::get_current_thread();
        if (IS_EMPTY(tid)) {
            tid = "UNKNOWN-THREAD";
        }
        throw CONTROL_ERROR("No lock record found for thread.[thread=%s]", tid.c_str());
    }

    lock_acquire_enum ls = client->has_valid_lock(priority);
    if (ls == Locked) {
        counts[priority]->count--;
        t_rec->decremet(priority);

        if (counts[priority]->count <= 0) {
            t_ptr->priority_lock_index[priority]->id = -1;
            t_ptr->priority_lock_index[priority]->acquired_time = 0;

            sem_t *lock = get(priority);
            if (IS_VALID_SEM_PTR(lock)) {
                if (sem_post(lock) != 0) {
                    throw CONTROL_ERROR(
                            "Semaphores in invalid state. [name=%s][priority=%d][base priority=%d][error=%s]",
                            this->name->c_str(),
                            priority, base_priority, strerror(errno));
                }
                LOG_DEBUG("Released semaphore [name=%s][priority=%d]", this->name->c_str(),
                          priority);

                client->release_lock(ls, priority);
                return true;
            } else {
                throw CONTROL_ERROR(
                        "No semaphore found for the specified priority. [lock=%s][priority=%d][base priority=%d]",
                        this->name->c_str(),
                        priority, base_priority);
            }
        } else {
            LOG_DEBUG("Lock count pending. [priority=%d][base priority=%d][count=%d]", priority, base_priority,
                      counts[priority]->count);
        }
    } else if (ls == Expired) {
        LOG_DEBUG("Lock expired. [resetting all semaphores] [priority=%d][base priority=%d]", priority, base_priority);
        reset_locks(priority);
        return true;
    } else {
        LOG_DEBUG("Lock already released. [state=%s][priority=%d][base priority=%d]",
                  record_utils::get_lock_acquire_enum_string(ls).c_str(), priority, base_priority);
    }
    LOG_DEBUG("Invalid lock state. [state=%s][priority=%d][base priority=%d]",
              record_utils::get_lock_acquire_enum_string(ls).c_str(), priority, base_priority);
    return false;
}

void com::watergate::core::_semaphore_owner::reset() {
    LOG_DEBUG("[name=%s] Resetting all semaphores...", name->c_str());
    clear_locks();
}

void com::watergate::core::_semaphore_owner::check_expired_locks(uint64_t expiry_time) {
    CHECK(expiry_time > 0);

    uint32_t counts[MAX_PRIORITY_ALLOWED];
    memset(counts, 0, (MAX_PRIORITY_ALLOWED * sizeof(uint32_t)));

    lock_table_manager *tm = get_table_manager();
    CHECK_NOT_NULL(tm);

    tm->check_expired_locks(expiry_time, counts);
    for (int ii = 0; ii < MAX_PRIORITY_ALLOWED; ii++) {
        if (counts[ii] > 0) {
            LOG_WARN("Force releasing semaphore locks. [name=%s][priority=%d][count=%d]", name->c_str(), ii,
                     counts[ii]);
            sem_t *sem = get(ii);
            if (IS_VALID_SEM_PTR(sem)) {
                for (int jj = 0; jj < counts[ii]; jj++) {
                    sem_post(sem);
                }
            } else {
                LOG_ERROR("Invalid semaphore pointer. [name=%s][priority=%d]", name->c_str(), ii);
            }
        }
    }
}

void com::watergate::core::_semaphore_owner::check_expired_records(uint64_t expiry_time) {
    CHECK(expiry_time > 0);

    lock_table_manager *tm = get_table_manager();
    CHECK_NOT_NULL(tm);
    tm->reset_expired_records(expiry_time);
}