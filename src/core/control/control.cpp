//
// Created by Subhabrata Ghosh on 16/09/16.
//

#include <includes/common/lock_record_def.h>
#include <includes/core/control.h>
#include "includes/core/resource_factory.h"

#define CONTROL_LOCK_PREFIX "/locks"

using namespace com::watergate::core;

void com::watergate::core::_semaphore::create(const _app *app, const ConfigValue *config, bool server) {

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

    this->name = this->resource->get_resource_name();

    this->is_server = server;

    const BasicConfigValue *v_prior = Config::get_value(CONST_SEM_CONFIG_NODE_PRIORITIES, config);
    if (IS_NULL(v_prior)) {
        throw CONFIG_ERROR("Required configuration node not found. [node=%s]", CONST_SEM_CONFIG_NODE_PRIORITIES);
    }
    this->priorities = v_prior->get_int_value(this->priorities);
    if (this->priorities > DEFAULT_MAX_PRIORITIES) {
        throw CONFIG_ERROR("Invalid configuration value. [%s=%d][MAX=%d]", CONST_SEM_CONFIG_NODE_PRIORITIES,
                           this->priorities, DEFAULT_MAX_PRIORITIES);
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

    if (this->is_server) {
        table = new lock_table_manager();
        lock_table_manager *mgr = get_table_manager();
        mgr->init(*name, resource);
    } else {
        table = new lock_table_client();
        lock_table_client *client = get_table_client();
        client->init(app, *name, resource);
    }
}

void com::watergate::core::_semaphore::create_sem(int index) {
    string *sem_name = common_utils::format("%s::%s::%d", CONTROL_LOCK_PREFIX, name->c_str(), index);

    sem_t *ptr = sem_open(sem_name->c_str(), O_CREAT, mode, max_concurrent);
    if (!IS_VALID_SEM_PTR(ptr)) {
        throw CONTROL_ERROR("Error creating semaphore. [name=%s][errno=%s]", sem_name->c_str(), strerror(errno));
    }

    semaphores[index] = ptr;
    CHECK_AND_FREE(sem_name);
}

void com::watergate::core::_semaphore::delete_sem(int index) {
    if (IS_VALID_SEM_PTR(semaphores[index])) {
        if (!owner) {
            if (sem_close(semaphores[index]) != 0) {
                LOG_ERROR("Error disposing semaphore. [index=%s][errno=%s]", index, strerror(errno));
            }
        } else {
            string *sem_name = common_utils::format("%s::%s::%d", CONTROL_LOCK_PREFIX, name->c_str(), index);
            if (sem_unlink(sem_name->c_str()) != 0) {
                LOG_ERROR("Error disposing semaphore. [index=%s][errno=%s]", index, strerror(errno));
            }
            CHECK_AND_FREE(sem_name);
        }
        semaphores[index] = nullptr;
    }
}

com::watergate::core::_semaphore::~_semaphore() {
    if (NOT_NULL(semaphores)) {
        for (int ii = 0; ii < priorities; ii++) {
            delete_sem(ii);
        }
        free(semaphores);
    }
    CHECK_AND_FREE(resource);
    CHECK_AND_FREE(table);
}

lock_acquire_enum com::watergate::core::_semaphore_client::try_lock(int priority, bool update, double quota) {
    assert(NOT_NULL(semaphores));

    lock_acquire_enum ls = client->check_and_lock(priority, quota);
    if (ls == Locked) {
        counts[priority]->count++;
        thread_lock_record *t_rec = get_thread_lock();
        if (NOT_NULL(t_rec)) {
            t_rec->increment(priority);
        }
        if (update) {
            client->update_quota(quota);
        }
        return ls;
    } else if (ls == QuotaReached) {
        return ls;
    } else if (ls == Expired) {
        LOG_DEBUG("Lock expired. [resetting all semaphores]");
        reset_locks();
    }

    sem_t *lock = get(priority);
    if (IS_VALID_SEM_PTR(lock)) {
        if (sem_trywait(lock) == 0) {
            LOG_DEBUG("Acquired semaphore. [name=%s][priority=%d]", this->name->c_str(), priority);
            counts[priority]->count++;
            thread_lock_record *t_rec = get_thread_lock();
            if (NOT_NULL(t_rec)) {
                t_rec->increment(priority);
            }
            if (update) {
                client->update_lock(update, priority, quota);
            }
            return Locked;
        } else if (errno == EAGAIN) {
            return Timeout;
        } else {
            return Error;
        }
    }
    throw CONTROL_ERROR("No lock found for the specified priority. [lock=%s][priority=%d]", this->name->c_str(),
                        priority);
}

lock_acquire_enum com::watergate::core::_semaphore_client::wait_lock(int priority, bool update, double quota) {
    assert(NOT_NULL(semaphores));

    lock_acquire_enum ls = client->check_and_lock(priority, quota);
    if (ls == Locked) {
        counts[priority]->count++;
        thread_lock_record *t_rec = get_thread_lock();
        if (NOT_NULL(t_rec)) {
            t_rec->increment(priority);
        }
        if (update) {
            client->update_quota(quota);
        }
        return ls;
    } else if (ls == QuotaReached) {
        return ls;
    } else if (ls == Expired) {
        LOG_DEBUG("Lock expired. [resetting all semaphores]");
        reset_locks();
    }

    sem_t *lock = get(priority);
    if (IS_VALID_SEM_PTR(lock)) {
        if (sem_wait(lock) == 0) {
            LOG_DEBUG("Acquired semaphore. [name=%s][priority=%d]", this->name->c_str(), priority);
            counts[priority]->count++;
            thread_lock_record *t_rec = get_thread_lock();
            if (NOT_NULL(t_rec)) {
                t_rec->increment(priority);
            }
            if (update) {
                client->update_lock(update, priority, quota);
            }
            return Locked;
        } else {
            return Error;
        }
    }
    throw CONTROL_ERROR("No lock found for the specified priority. [lock=%s][priority=%d]", this->name->c_str(),
                        priority);
}

bool com::watergate::core::_semaphore_client::release_lock(int priority) {
    assert(NOT_NULL(semaphores));

    lock_acquire_enum ls = client->has_valid_lock();
    if (ls == Locked) {
        counts[priority]->count--;
        thread_lock_record *t_rec = get_thread_lock();
        if (NOT_NULL(t_rec)) {
            t_rec->decremet(priority);
        }
        int count = 0;
        for (int ii = 0; ii < priorities; ii++) {
            count += counts[ii]->count;
        }

        if (count <= 0) {

            sem_t *lock = get(priority);
            if (IS_VALID_SEM_PTR(lock)) {
                if (sem_post(lock) != 0) {
                    throw CONTROL_ERROR("Semaphores in invalid state. [name=%s][priority=%d][errno=%d]",
                                        this->name->c_str(),
                                        priority, errno);
                }
                LOG_DEBUG("Released semaphore [name=%s][priority=%d]", this->name->c_str(),
                          priority);
                for (int ii = 0; ii < priorities; ii++) {
                    counts[ii]->count = 0;
                }
                client->release_lock(ls);
            } else {
                throw CONTROL_ERROR("No semaphore found for the specified priority. [lock=%s][priority=%d]",
                                    this->name->c_str(),
                                    priority);
            }
        }
        return true;
    } else if (ls == Expired) {
        LOG_DEBUG("Lock expired. [resetting all semaphores]");
        reset_locks();
    } else {
        LOG_DEBUG("Lock already released. [state=%d]", ls);
    }
    return false;
}

void com::watergate::core::_semaphore_owner::reset() {
    LOG_DEBUG("Resetting all semaphores...");
    clear_locks();
}
