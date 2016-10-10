//
// Created by Subhabrata Ghosh on 21/09/16.
//

#ifndef WATERGATE_LOCK_TABLE_H
#define WATERGATE_LOCK_TABLE_H

#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <includes/common/lock_record_def.h>
#include <thread>
#include <mutex>

#include "includes/common/_app.h"
#include "includes/common/timer.h"
#include "includes/common/config.h"
#include "includes/common/log_utils.h"
#include "exclusive_lock.h"
#include "includes/common/lock_record_def.h"
#include "includes/core/resource_def.h"

#define CONST_LOCKT_ERROR_PREFIX "Lock Table Error : "
#define LOCK_TABLE_LOCK_PREFIX "t_lock_"

#define LOCK_TABLE_ERROR(fmt, ...) lock_table_error(__FILE__, __LINE__, common_utils::format(fmt, ##__VA_ARGS__))
#define LOCK_TABLE_ERROR_PTR(fmt, ...) new lock_table_error(__FILE__, __LINE__, common_utils::format(fmt, ##__VA_ARGS__))

#define RELEASE_LOCK_RECORD(rec, priority) do {\
    LOG_DEBUG("[priority=%d] Resetting lock record...", priority); \
    rec->lock.locks[priority].has_lock = false; \
    rec->lock.locks[priority].acquired_time = 0; \
    if (IS_BASE_PRIORITY(priority)) rec->lock.quota_used = 0; \
}while(0)

#define SHARED_LOCK_NAME(pid) common_utils::format("proc_%d", pid)

namespace com {
    namespace watergate {
        namespace core {
            class lock_table_error : public base_error {
            public:
                lock_table_error(char const *file, const int line, string mesg) : base_error(file, line,
                                                                                             CONST_LOCKT_ERROR_PREFIX,
                                                                                             mesg) {
                }
            };

            class lock_table {
            protected:
                string name;
                int shm_fd = -1;
                void *mem_ptr = nullptr;
                exclusive_lock *lock = nullptr;

            protected:
                _state state;

                void create(string name, resource_def *resource, bool server);


                const _lock_record *get_record(int index) {
                    CHECK_STATE_AVAILABLE(state);

                    assert(index >= 0 && index < DEFAULT_MAX_RECORDS);
                    _lock_table *ptr = (_lock_table *) mem_ptr;

                    return &ptr->records[index];
                }

                void remove_record(int index);

                _lock_record *create_new_record(string app_name, string app_id, pid_t pid);

            public:
                lock_table() {

                }

                virtual ~lock_table() {
                    state.set_state(Disposed);
                    CHECK_AND_FREE(lock);
                    if (shm_fd >= 0 && NOT_NULL(mem_ptr)) {
                        shm_unlink(name.c_str());
                    }

                }

                const string get_name() const {
                    return this->name;
                }

                const _state get_state() const {
                    return state;
                }


                void remove_record(_lock_record *rec) {
                    assert(NOT_NULL(rec));
                    remove_record(rec->index);
                }

                void set_quota(double quota) {
                    CHECK_STATE_AVAILABLE(state);
                    _lock_table *ptr = (_lock_table *) mem_ptr;

                    ptr->quota = quota;
                }

                double get_quota() {
                    CHECK_STATE_AVAILABLE(state);
                    _lock_table *ptr = (_lock_table *) mem_ptr;

                    return ptr->quota;
                }

                uint64_t get_lock_lease_time() {
                    CHECK_STATE_AVAILABLE(state);
                    _lock_table *ptr = (_lock_table *) mem_ptr;

                    return ptr->lock_lease_time;
                }

                void set_lock_lease_time(long lease_time) {
                    CHECK_STATE_AVAILABLE(state);
                    _lock_table *ptr = (_lock_table *) mem_ptr;

                    ptr->lock_lease_time = lease_time;
                }
            };

            class lock_table_manager : public lock_table {

            public:
                void init(string name, resource_def *resource) {
                    create(name, resource, true);

                    lock->reset();

                    _lock_table *ptr = (_lock_table *) mem_ptr;

                    for (int ii = 0; ii < DEFAULT_MAX_RECORDS; ii++) {
                        ptr->free_indexes[ii] = ii;
                        _lock_record *rec = &ptr->records[ii];
                        RESET_RECORD(rec);
                        rec->index = ii;
                    }

                    long lt = resource->get_lease_time();
                    if (lt > 0) {
                        ptr->lock_lease_time = lt;
                    } else {
                        ptr->lock_lease_time = DEFAULT_LEASE_TIME;
                    }

                    double quota = resource->get_resource_quota();
                    if (quota > 0.0) {
                        ptr->quota = quota;
                    } else {
                        ptr->quota = DEFAULT_QUOTA;
                    }
                }

            };

            class lock_table_client : public lock_table {
            private:
                _lock_record *lock_record;
                exclusive_lock *shared_lock = nullptr;

                bool is_lock_active(int priority) {
                    if (lock_record->lock.locks[priority].has_lock) {
                        uint64_t now = time_utils::now();
                        if (now < (lock_record->lock.locks[priority].acquired_time + get_lock_lease_time())) {
                            return true;
                        }
                    }
                    return false;
                }

            public:
                ~lock_table_client() override {
                    state.set_state(Disposed);

                    if (NOT_NULL(lock_record)) {
                        remove_record(lock_record->index);
                        lock_record = nullptr;
                    }

                    CHECK_AND_FREE(shared_lock);
                }

                void init(const _app *app, string name, resource_def *resrouce) {
                    if (NOT_NULL(lock_record)) {
                        pid_t pid = getpid();
                        if (lock_record->app.proc_id != pid) {
                            throw new LOCK_TABLE_ERROR("Invalid lock record. [current pid=%d][app pid=%d]", pid,
                                                       lock_record->app.proc_id);
                        }
                    }
                    create(name, resrouce, false);
                    pid_t pid = getpid();

                    lock_record = create_new_record(app->get_name(), app->get_id(), pid);

                    string sl_name = SHARED_LOCK_NAME(pid);
                    shared_lock = new exclusive_lock(&sl_name);
                    shared_lock->create();
                    shared_lock->reset();

                    strncpy(lock_record->app.table_lock_name, sl_name.c_str(), sl_name.length());
                    LOG_DEBUG("Added shared lock entry. [name=%s]", lock_record->app.table_lock_name);
                }

                _lock_record *new_record(string app_name, string app_id, pid_t pid) {
                    CHECK_STATE_AVAILABLE(state);

                    if (NOT_NULL(lock_record)) {
                        pid_t pid = getpid();
                        if (lock_record->app.proc_id != pid) {
                            throw LOCK_TABLE_ERROR("Invalid lock record. [current pid=%d][app pid=%d]", pid,
                                                   lock_record->app.proc_id);
                        }
                    } else {
                        throw LOCK_TABLE_ERROR("Invalid lock table client state. Process record handle is null.");
                    }

                    return lock_record;
                }

                _lock_record *new_record_test(string app_name, string app_id, pid_t pid) {
                    CHECK_STATE_AVAILABLE(state);

                    return create_new_record(app_name, app_id, pid);
                }

                lock_acquire_enum has_valid_lock(int priority) {
                    CHECK_STATE_AVAILABLE(state);

                    if (!shared_lock->wait_lock()) {
                        lock_table_error te = LOCK_TABLE_ERROR(
                                "Error getting lock to update table. [name=%s][error=%s]",
                                *shared_lock->get_name()->c_str(),
                                strerror(errno));
                        LOG_CRITICAL(te.what());
                        state.set_error(&te);

                        throw te;
                    }
                    lock_acquire_enum r = None;
                    pid_t pid = getpid();
                    if (lock_record->app.proc_id != pid) {
                        lock_table_error te = LOCK_TABLE_ERROR(
                                "Lock record has been reset. [current owner=%d][proc id=%d]", lock_record->app.proc_id,
                                pid);
                    }
                    if (lock_record->lock.locks[priority].has_lock) {
                        if (!is_lock_active(priority)) {
                            lock_record->lock.locks[priority].has_lock = false;
                            lock_record->lock.locks[priority].acquired_time = 0;
                            r = Expired;
                        } else {
                            r = Locked;
                        }
                    }
                    if (!shared_lock->release_lock()) {
                        lock_table_error te = LOCK_TABLE_ERROR(
                                "Error getting lock to update table. [name=%s][error=%s]",
                                *shared_lock->get_name()->c_str(),
                                strerror(errno));
                        LOG_CRITICAL(te.what());
                        state.set_error(&te);

                        throw te;
                    }
                    return r;
                }


                lock_acquire_enum check_and_lock(double quota) {
                    CHECK_STATE_AVAILABLE(state);

                    lock_record->app.last_active_ts = time_utils::now();
                    lock_acquire_enum ls = has_valid_lock(BASE_PRIORITY);
                    if (ls == Expired) {
                        release_lock(ls, BASE_PRIORITY);
                    } else if (ls == Locked) {
                        do {
                            double q = get_quota();
                            if (q > 0) {
                                double aq = q - lock_record->lock.quota_used;
                                if (aq < quota) {
                                    return QuotaReached;
                                }
                                lock_record->lock.quota_used += quota;
                                lock_record->lock.quota_total += quota;
                                LOG_DEBUG("[priority=%d] Current quota used = %f", BASE_PRIORITY,
                                          lock_record->lock.quota_used);
                            }
                        } while (0);
                    }
                    return ls;
                }

                lock_acquire_enum quota_available(double quota) {
                    CHECK_STATE_AVAILABLE(state);

                    do {
                        double q = get_quota();
                        if (q > 0) {
                            double aq = q - lock_record->lock.quota_used;
                            if (aq < quota) {
                                LOG_DEBUG("Quota Reached %f", lock_record->lock.quota_used);
                                return QuotaReached;
                            }
                        }
                    } while (0);

                    return QuotaAvailable;
                }

                void update_lock(int priority) {
                    CHECK_STATE_AVAILABLE(state);

                    do {
                        lock_record->lock.locks[priority].has_lock = true;
                        lock_record->lock.locks[priority].acquired_time = time_utils::now();
                    } while (0);
                }

                void update_quota(double quota, int priority) {
                    CHECK_STATE_AVAILABLE(state);
                    do {
                        lock_record->lock.quota_used += quota;
                        lock_record->lock.quota_total += quota;
                        LOG_DEBUG("[priority=%d] Current quota used = %f", priority,
                                  lock_record->lock.quota_used);
                    } while (0);
                }

                void release_lock(lock_acquire_enum lock_state, int priority) {
                    CHECK_STATE_AVAILABLE(state);
                    if (lock_state == Expired || lock_state == Locked) {
                        if (lock_record->lock.locks[priority].has_lock) {
                            do {
                                RELEASE_LOCK_RECORD(lock_record, priority);
                            } while (0);
                        }
                    }
                }

                void dump() {
                    CHECK_STATE_AVAILABLE(state);

                    LOG_DEBUG("**************[LOCK TABLE:%s:%d]**************", get_name().c_str(), getpid());
                    LOG_DEBUG("\tquota=%f", get_quota());
                    LOG_DEBUG("\tlease time=%lu", get_lock_lease_time());

                    if (NOT_NULL(lock_record)) {
                        LOG_DEBUG("\t\tin use=%s", (lock_record->used ? "used" : "unused"));
                        LOG_DEBUG("\t\tindex=%d", lock_record->index);
                        LOG_DEBUG("\t\tapp name=%s", lock_record->app.app_name);
                        LOG_DEBUG("\t\tapp id=%s", lock_record->app.app_id);
                        LOG_DEBUG("\t\tproc id=%d", lock_record->app.proc_id);
                        LOG_DEBUG("\t\tregister time=%s",
                                  time_utils::get_time_string(lock_record->app.register_time).c_str());
                        LOG_DEBUG("\t\tlast active=%s",
                                  time_utils::get_time_string(lock_record->app.last_active_ts).c_str());
                        for (int ii = 0; ii < MAX_PRIORITY_ALLOWED; ii++) {
                            LOG_DEBUG("\t\t[%d] has lock=%s", ii,
                                      (lock_record->lock.locks[ii].has_lock ? "true" : "false"));
                            LOG_DEBUG("\t\t[%d] acquired time=%s", ii,
                                      (lock_record->lock.locks[ii].acquired_time > 0 ? time_utils::get_time_string(
                                              lock_record->lock.locks[ii].acquired_time).c_str() : "N/A"));
                        }
                        LOG_DEBUG("\t\tused quota=%f", lock_record->lock.quota_used);
                        LOG_DEBUG("\t\ttotal quota=%f", lock_record->lock.quota_total);
                    }

                    LOG_DEBUG("**************[LOCK TABLE:%s:%d]**************", get_name().c_str(), getpid());
                }
            };
        }
    }
}
#endif //WATERGATE_LOCK_TABLE_H
