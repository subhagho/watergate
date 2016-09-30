//
// Created by Subhabrata Ghosh on 22/09/16.
//

#ifndef WATERGATE_LOCK_RECORD_DEF_H
#define WATERGATE_LOCK_RECORD_DEF_H

#include <unistd.h>
#include <thread>
#include <sstream>
#include <unordered_map>

#include "includes/common/log_utils.h"
#include "includes/common/common.h"
#include "includes/common/base_error.h"

#define DEFAULT_MAX_RECORDS 4096
#define MAX_STRING_SIZE 64
#define FREE_INDEX_USED -999

#define RESET_RECORD(rec) do {\
    rec->used = false; \
    memset(&rec->app, 0, sizeof(_app_handle)); \
    memset(&rec->lock, 0, sizeof(_lock_handle)); \
} while(0)

typedef struct {
    char app_name[MAX_STRING_SIZE];
    char app_id[MAX_STRING_SIZE];
    pid_t proc_id;
    long register_time;
    long last_active_ts;
} _app_handle;

typedef struct {
    bool has_lock;
    int lock_priority;
    long acquired_time;
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
    long lock_lease_time;
    double quota = -1;
    int free_indexes[DEFAULT_MAX_RECORDS];
    _lock_record records[DEFAULT_MAX_RECORDS];
} _lock_table;

typedef struct {
    int base_priority = -1;
    long total_wait_time = 0;
    long max_wait_time = 0;
    long tries = 0;
} _lock_metrics;

using namespace std;
using namespace com::watergate::common;

namespace com {
    namespace watergate {
        namespace core {

            enum lock_acquire_enum {
                Locked = 0, Expired, Timeout, QuotaReached, Ignore, Error, None
            };

            class thread_lock_record {
            private:
                std::string thread_id;
                int *p_counts = nullptr;
                int priorities = 0;
                _lock_metrics metrics;

            public:
                thread_lock_record(std::string thread_id, int priorities) {
                    this->thread_id = thread_id;
                    this->p_counts = (int *) malloc(priorities * sizeof(int));
                    this->priorities = priorities;
                    for (int ii = 0; ii < this->priorities; ii++) {
                        p_counts[ii] = 0;
                    }
                }

                ~thread_lock_record() {
                    if (NOT_NULL(p_counts)) {
                        free(p_counts);
                        p_counts = nullptr;
                    }
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

                void reset() {
                    for (int ii = 0; ii < this->priorities; ii++) {
                        p_counts[ii] = 0;
                    }
                }

                void update_metrics(int priority, long lock_time) {
                    if (metrics.base_priority < 0) {
                        metrics.base_priority = priority;
                    }
                    metrics.total_wait_time += lock_time;
                    metrics.tries++;
                    if (metrics.max_wait_time < lock_time) {
                        metrics.max_wait_time = lock_time;
                    }
                }

                void dump() {
                    if (NOT_NULL(p_counts)) {
                        LOG_DEBUG("**************[THREAD:%s:%d]**************", thread_id.c_str(), getpid());
                        for (int ii = 0; ii < this->priorities; ii++) {
                            LOG_DEBUG("[priority=%d] count=%d", ii, p_counts[ii]);
                            double avg = ((double) metrics.total_wait_time) / metrics.tries;
                            LOG_DEBUG("METRICS : [base priority=%d][average wait time=%f][max wait time=%d][tries=%d]",
                                      metrics.base_priority, avg, metrics.max_wait_time, metrics.tries);
                        }
                        LOG_DEBUG("**************[THREAD:%s:%d]**************", thread_id.c_str(), getpid());
                    }
                }

                static std::string get_current_thread() {
                    std::stringstream ss;
                    ss << std::this_thread::get_id();
                    return std::string(ss.str());
                }
            };
        }
    }
}
#endif //WATERGATE_LOCK_RECORD_DEF_H
