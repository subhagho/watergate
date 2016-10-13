//
// Created by Subhabrata Ghosh on 22/09/16.
//

#ifndef WATERGATE_LOCK_RECORD_DEF_H
#define WATERGATE_LOCK_RECORD_DEF_H

#include <unistd.h>
#include <thread>
#include <sstream>
#include <unordered_map>
#include <semaphore.h>

#include "includes/common/log_utils.h"
#include "includes/common/common.h"
#include "includes/common/base_error.h"

#define MAX_PRIORITY_ALLOWED 8
#define DEFAULT_MAX_RECORDS 1024
#define MAX_STRING_SIZE 64
#define FREE_INDEX_USED -999

#ifndef PSEMNAMLEN // For cases where the POSIX sem header is not visible.
#define PSEMNAMLEN 31
#endif //PSEMNAMLEN

#define BASE_PRIORITY 0

#define IS_BASE_PRIORITY(p) (p == BASE_PRIORITY)

#define RESET_RECORD(rec) com::watergate::core::record_utils::reset_record(rec)

typedef struct {
    char app_name[MAX_STRING_SIZE];
    char app_id[MAX_STRING_SIZE];
    pid_t proc_id;
    uint64_t register_time = 0;
    uint64_t last_active_ts = 0;
} _app_handle;

typedef struct {
    bool has_lock = false;
    uint64_t acquired_time = 0;
    bool force_released = false;
} _priority_lock;

typedef struct {
    _priority_lock locks[MAX_PRIORITY_ALLOWED];
    double quota_used;
    double quota_total;
} _lock_handle;

typedef struct {
    bool used = false;
    int index = -1;
    _app_handle app;
    _lock_handle lock;
} _lock_record;


typedef struct {
    uint64_t lock_lease_time;
    double quota = -1;
    int free_indexes[DEFAULT_MAX_RECORDS];
    _lock_record records[DEFAULT_MAX_RECORDS];
} _lock_table;

typedef struct {
    int base_priority = -1;
    uint64_t total_wait_time = 0;
    uint64_t max_wait_time = 0;
    uint64_t tries = 0;
} _lock_metrics;

typedef struct {
    uint64_t id = -1;
    uint64_t acquired_time = 0;
} _lock_id;
using namespace std;


using namespace com::watergate::common;

namespace com {
    namespace watergate {
        namespace core {

            enum lock_acquire_enum {
                Locked = 0, Expired, Retry, Timeout, QuotaReached, QuotaAvailable, Error, None
            };

            struct thread_lock_ptr {
                string thread_id;
                _lock_id **priority_lock_index = nullptr;
                int lock_priority = -1;
                _lock_metrics metrics;
            };

            class thread_lock_record {
            private:
                std::string thread_id;
                int *p_counts = nullptr;
                int priorities = 0;
                thread_lock_ptr *thread_ptr;

            public:
                thread_lock_record(thread_lock_ptr *thread_ptr, int priorities) {
                    this->thread_id = thread_ptr->thread_id;
                    this->p_counts = (int *) malloc(priorities * sizeof(int));
                    this->priorities = priorities;
                    for (int ii = 0; ii < this->priorities; ii++) {
                        p_counts[ii] = 0;
                    }
                    this->thread_ptr = thread_ptr;
                }

                ~thread_lock_record() {
                    if (NOT_NULL(p_counts)) {
                        free(p_counts);
                        p_counts = nullptr;
                    }
                    if (NOT_NULL(thread_ptr)) {
                        if (NOT_NULL(thread_ptr->priority_lock_index)) {
                            for (int ii = 0; ii < priorities; ii++) {
                                if (NOT_NULL(thread_ptr->priority_lock_index[ii])) {
                                    free(thread_ptr->priority_lock_index[ii]);
                                }
                            }
                            free(thread_ptr->priority_lock_index);
                        }
                        delete (thread_ptr);
                    }
                }

                thread_lock_ptr *get_thread_ptr() {
                    return thread_ptr;
                }

                int increment(int priority) {
                    _assert(priority >= 0 && priority < priorities);

                    p_counts[priority]++;

                    return p_counts[priority];
                }

                int decremet(int priority) {
                    _assert(priority >= 0 && priority < priorities);

                    p_counts[priority]--;

                    return p_counts[priority];
                }

                void reset(int priority) {
                    p_counts[priority] = 0;
                    thread_ptr->priority_lock_index[priority]->acquired_time = 0;
                    thread_ptr->priority_lock_index[priority]->id = -1;
                }

                void update_metrics(int priority, uint64_t lock_time) {
                    _assert(NOT_NULL(thread_ptr));

                    if (thread_ptr->metrics.base_priority < 0) {
                        thread_ptr->metrics.base_priority = priority;
                    }
                    thread_ptr->metrics.total_wait_time += lock_time;
                    thread_ptr->metrics.tries++;
                    if (thread_ptr->metrics.max_wait_time < lock_time) {
                        thread_ptr->metrics.max_wait_time = lock_time;
                    }
                }

                void dump() {
                    _assert(NOT_NULL(thread_ptr));
                    if (NOT_NULL(p_counts)) {
                        LOG_DEBUG("**************[THREAD:%s:%d]**************", thread_id.c_str(), getpid());
                        for (int ii = 0; ii < this->priorities; ii++) {
                            LOG_DEBUG("[priority=%d] count=%d", ii, p_counts[ii]);
                            double avg = ((double) thread_ptr->metrics.total_wait_time) / thread_ptr->metrics.tries;
                            LOG_DEBUG(
                                    "METRICS : [base priority=%d][average wait time=%f][max wait time=%lu][tries=%lu]",
                                    thread_ptr->metrics.base_priority, avg, thread_ptr->metrics.max_wait_time,
                                    thread_ptr->metrics.tries);
                        }
                        LOG_DEBUG("**************[THREAD:%s:%d]**************", thread_id.c_str(), getpid());
                    }
                }

                void test_assert() {
                    _assert(NOT_NULL(thread_ptr));
                    if (NOT_NULL(p_counts)) {
                        for (int ii = 0; ii < this->priorities; ii++) {
                            _assert(p_counts[ii] == 0);
                        }
                    }
                }

                static std::string get_current_thread() {
                    std::stringstream ss;
                    ss << std::this_thread::get_id();
                    return std::string(ss.str());
                }

                static thread_lock_ptr *create_new_ptr(int max_priority) {
                    thread_lock_ptr *ptr = new thread_lock_ptr();
                    ptr->thread_id = get_current_thread();
                    ptr->priority_lock_index = (_lock_id **) malloc(max_priority * sizeof(_lock_id *));
                    for (int ii = 0; ii < max_priority; ii++) {
                        ptr->priority_lock_index[ii] = (_lock_id *) malloc(sizeof(_lock_id));
                        ptr->priority_lock_index[ii]->acquired_time = 0;
                        ptr->priority_lock_index[ii]->id = -1;
                    }
                    return ptr;
                }
            };

            class record_utils {
            public:
                static string get_lock_acquire_enum_string(lock_acquire_enum value) {
                    switch (value) {
                        case lock_acquire_enum::Locked:
                            return string("LOCKED");
                        case lock_acquire_enum::None:
                            return string("NONE");
                        case lock_acquire_enum::Error:
                            return string("ERROR");
                        case lock_acquire_enum::Expired:
                            return string("EXPIRED");
                        case lock_acquire_enum::QuotaAvailable:
                            return string("QUOTA AVAILABLE");
                        case lock_acquire_enum::QuotaReached:
                            return string("QUOTA REACHED");
                        case lock_acquire_enum::Retry:
                            return string("RETRY");
                        case lock_acquire_enum::Timeout:
                            return string("TIMEOUT");
                        default:
                            return string("UNKNOWN");
                    }
                }

                static void reset_record(_lock_record *record) {
                    PRECONDITION(NOT_NULL(record));

                    record->used = false;

                    // Reset App handle
                    record->app.proc_id = 0;
                    memset(record->app.app_id, 0, MAX_STRING_SIZE);
                    memset(record->app.app_name, 0, MAX_STRING_SIZE);
                    record->app.last_active_ts = 0;
                    record->app.register_time = 0;

                    // Reset Lock handles.
                    record->lock.quota_total = 0;
                    record->lock.quota_used = 0;

                    for (int ii = 0; ii < MAX_PRIORITY_ALLOWED; ii++) {
                        record->lock.locks[ii].acquired_time = 0;
                        record->lock.locks[ii].has_lock = false;
                        record->lock.locks[ii].force_released = false;
                    }
                }
            };
        }
    }
}
#endif //WATERGATE_LOCK_RECORD_DEF_H
