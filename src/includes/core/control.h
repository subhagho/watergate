//
// Created by Subhabrata Ghosh on 15/09/16.
//

#ifndef WATERGATE_CONTROL_H
#define WATERGATE_CONTROL_H

#include <semaphore.h>
#include <vector>
#include <includes/common/lock_record_def.h>

#include "includes/common/common.h"
#include "includes/common/base_error.h"
#include "includes/common/common_utils.h"
#include "includes/common/_app.h"
#include "resource_def.h"
#include "lock_table.h"

#define DEFAULT_SEM_MODE 0760

#define CONST_SEM_CONFIG_NODE_RESOURCE "./resource"
#define CONST_SEM_CONFIG_PARAM_RESOURCE_NAME "name"
#define CONST_SEM_CONFIG_PARAM_RESOURCE_CLASS "class"
#define CONST_SEM_CONFIG_NODE_PRIORITIES "priorities"
#define CONST_SEM_CONFIG_NODE_THREAD_LOCKS "allow_thread_locks"
#define CONST_SEM_CONFIG_NODE_MODE "mode"

#define CONST_CONTROL_ERROR_PREFIX "Control Object Error : "

#define CONTROL_ERROR(fmt, ...) control_error(__FILE__, __LINE__, common_utils::format(fmt, ##__VA_ARGS__))
#define CONTROL_ERROR_PTR(fmt, ...) new control_error(__FILE__, __LINE__, common_utils::format(fmt, ##__VA_ARGS__))

#define IS_VALID_SEM_PTR(ptr) (NOT_NULL(ptr) && ptr != SEM_FAILED)


#define CONTROL_LOCK_PREFIX "/locks"

using namespace com::watergate::common;

namespace com {
    namespace watergate {
        namespace core {
            class control_error : public base_error {
            public:
                control_error(char const *file, const int line, string *mesg) : base_error(file, line,
                                                                                           CONST_CONTROL_ERROR_PREFIX,
                                                                                           mesg) {
                }
            };

            struct _struct_priority_record {
                int priority;
                mutex priority_lock;
                uint64_t count = 0;
                uint64_t index = 0;
                bool has_lock = false;
                uint64_t acquired_time = 0;
            };

            class _semaphore {
            private:
                mode_t mode = DEFAULT_SEM_MODE;
                bool is_server;

                void create_sem(int index);


            protected:
                string *name;
                int priorities;
                int max_concurrent;
                sem_t **semaphores;
                resource_def *resource;
                lock_table *table;
                bool owner = false;

                void delete_sem(int index);

                sem_t *get(int index) const {
                    if (index < 0 || index > priorities)
                        LOG_ERROR("Invalid index range specified. [index=%d]", index);
                    _assert(index >= 0 && index < priorities);

                    return semaphores[index];
                }

                lock_table_manager *get_table_manager() {
                    if (is_server) {
                        lock_table_manager *ptr = static_cast<lock_table_manager *>(table);
                        return ptr;
                    }
                    throw BASE_ERROR("Invalid call to get table manager. Instance not running in server mode.");
                }

                lock_table_client *get_table_client() {
                    if (!is_server) {
                        lock_table_client *ptr = static_cast<lock_table_client *>(table);
                        return ptr;
                    }
                    throw BASE_ERROR("Invalid call to get table client. Instance not running in client mode.");
                }

                bool is_priority_valid(int p) {
                    return (p >= 0 && p < this->priorities);
                }

                void create(const _app *app, const ConfigValue *config, bool server);

            public:
                virtual ~_semaphore();

                const string *get_name() const {
                    return this->name;
                }

                const int get_max_priority() const {
                    return priorities;
                }

                virtual void init(const _app *app, const ConfigValue *config) = 0;
            };

            class _semaphore_owner : public _semaphore {
            private:
                lock_table_manager *manager;

            public:
                _semaphore_owner() {
                    this->owner = true;
                }

                ~_semaphore_owner() {
                    if (NOT_NULL(semaphores)) {
                        for (int ii = 0; ii < priorities; ii++) {
                            delete_sem(ii);
                        }
                        free(semaphores);
                    }
                }

                void init(const _app *app, const ConfigValue *config) override {
                    create(app, config, true);

                    reset();

                    manager = get_table_manager();
                }

                void clear_locks() {
                    for (int ii = 0; ii < priorities; ii++) {
                        sem_t *sem = get(ii);
                        if (IS_VALID_SEM_PTR(sem)) {
                            LOG_INFO("Clearing semaphore lock [%s:%d]", name->c_str(), ii);
                            int count = 0;
                            while (true) {
                                if (sem_trywait(sem) == 0) {
                                    count++;
                                } else {
                                    break;
                                }
                            }
                            LOG_DEBUG("[%s]Available free lock count = %d", name->c_str(), count);
                            if (count < max_concurrent)
                                count = max_concurrent;
                            for (int jj = 0; jj < count; jj++) {
                                sem_post(sem);
                            }
                        }
                    }
                }

                void reset();
            };

            class _semaphore_client : public _semaphore {
            private:
                lock_table_client *client;
                vector<_struct_priority_record *> counts;
                unordered_map<string, thread_lock_record *> threads;


                lock_acquire_enum check_lock_state(int priority) {
                    _struct_priority_record *counter = counts[priority];
                    if (counter->has_lock) {
                        uint64_t now = time_utils::now();
                        uint64_t v_time = counter->acquired_time + client->get_lock_lease_time();
                        if (now <= v_time) {
                            return Locked;
                        } else {
                            return Expired;
                        }
                    }
                    return None;
                }

                void reset_locks(int priority) {
                    counts[priority]->count = 0;
                    counts[priority]->acquired_time = 0;
                    counts[priority]->has_lock = false;

                    reset_thread_locks(priority);
                    client->release_lock(Expired, priority);
                    sem_t *lock = get(priority);
                    if (IS_VALID_SEM_PTR(lock)) {
                        LOG_DEBUG("Force Released semaphore [name=%s][priority=%d]", this->name->c_str(), priority);
                        if (sem_post(lock) != 0) {
                            throw CONTROL_ERROR("Semaphores in invalid state. [name=%s][priority=%d][errno=%s]",
                                                this->name->c_str(),
                                                priority, strerror(errno));
                        }
                    } else {
                        throw CONTROL_ERROR("No lock found for the specified priority. [lock=%s][priority=%d]",
                                            this->name->c_str(),
                                            priority);
                    }
                }

                inline thread_lock_record *get_thread_lock() {
                    thread_lock_record *r = nullptr;
                    string id = thread_lock_record::get_current_thread();

                    unordered_map<string, thread_lock_record *>::iterator iter = threads.find(id);
                    if (iter != threads.end()) {
                        r = iter->second;
                    }
                    return r;
                }


                void reset_thread_locks(int priority) {
                    unordered_map<string, thread_lock_record *>::iterator iter;
                    for (iter = threads.begin(); iter != threads.end(); iter++) {
                        thread_lock_record *rec = iter->second;
                        if (NOT_NULL(rec)) {
                            rec->reset(priority);
                        }
                    }
                }

            public:
                mutex sem_lock;

                ~_semaphore_client() {
                    if (NOT_NULL(semaphores)) {
                        for (int ii = 0; ii < priorities; ii++) {
                            delete_sem(ii);
                        }
                        free(semaphores);
                    }
                    if (!IS_EMPTY(counts)) {
                        for (int ii = 0; ii < priorities; ii++) {
                            if (NOT_NULL(counts[ii])) {
                                delete (counts[ii]);
                            }
                        }
                    }
                    if (!IS_EMPTY(threads)) {
                        unordered_map<string, thread_lock_record *>::iterator iter;
                        for (iter = threads.begin(); iter != threads.end(); iter++) {
                            thread_lock_record *rec = iter->second;
                            if (NOT_NULL(rec)) {
                                delete (rec);
                            }
                        }
                    }
                }

                void init(const _app *app, const ConfigValue *config) override {
                    create(app, config, false);

                    for (int ii = 0; ii < priorities; ii++) {
                        _struct_priority_record *lc = new _struct_priority_record();
                        counts.push_back(lc);
                        counts[ii]->priority = ii;
                        counts[ii]->count = 0;
                    }
                    client = get_table_client();
                }


                inline thread_lock_record *register_thread() {
                    thread_lock_record *r = nullptr;

                    string id = thread_lock_record::get_current_thread();
                    unordered_map<string, thread_lock_record *>::iterator iter = threads.find(id);
                    if (iter != threads.end()) {
                        r = iter->second;
                    } else {
                        thread_lock_ptr *ptr = thread_lock_record::create_new_ptr(priorities);
                        r = new thread_lock_record(ptr, priorities);
                        threads.insert(make_pair(ptr->thread_id, r));
                    }
                    return r;
                }

                void update_metrics(int priority, long lock_time) {
                    thread_lock_record *rec = get_thread_lock();
                    if (NOT_NULL(rec)) {
                        rec->update_metrics(priority, lock_time);
                    }
                }

                lock_acquire_enum try_lock(int priority, int base_priority, bool wait);

                lock_acquire_enum try_lock_base(double quota, int base_priority, bool wait);

                bool release_lock(int priority, int base_priority);

                bool release_lock_base(int base_priority);

                void dump() {
                    LOG_DEBUG("**************[LOCK:%s:%d]**************", name->c_str(), getpid());
                    if (NOT_NULL(client)) {
                        client->dump();
                    }
                    if (!IS_EMPTY(counts)) {
                        for (int ii = 0; ii < priorities; ii++) {
                            LOG_DEBUG("\t[priority=%d] lock count=%d", ii,
                                      counts[ii]->count);
                        }
                    }
                    if (!IS_EMPTY(threads)) {
                        unordered_map<string, thread_lock_record *>::iterator iter;
                        for (iter = threads.begin(); iter != threads.end(); iter++) {
                            thread_lock_record *rec = iter->second;
                            if (NOT_NULL(rec)) {
                                rec->dump();
                            }
                        }
                    }
                    LOG_DEBUG("**************[LOCK:%s:%d]**************", name->c_str(), getpid());
                }
            };
        }
    }
}
#endif //WATERGATE_CONTROL_H
