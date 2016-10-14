//
// Created by Subhabrata Ghosh on 21/09/16.
//

#ifndef WATERGATE_CONTROL_MANAGER_H
#define WATERGATE_CONTROL_MANAGER_H

#include <thread>

#include "control_def.h"

#define DEFAULT_LOCK_RESET_TIME "60s"
#define DEFAULT_RECORD_RESET_TIME "10m"

#define CONST_CM_CONFIG_LOCK_RESET_TIME "lock.reset.time"
#define CONST_CM_CONFIG_RECORD_RESET_TIME "record.reset.time"

#define DEFAULT_CONTROL_THREAD_SLEEP 5 * 1000 // 10 seconds

namespace com {
    namespace watergate {
        namespace core {
            class control_manager : public control_def {
            private:
                uint64_t lock_timeout;
                uint64_t record_timeout;
                thread *control_thread;


                void start() {
                    control_thread = new thread(control_manager::run, this);
                }

                void join() {
                    PRECONDITION(!state.is_available());

                    if (NOT_NULL(control_thread)) {
                        control_thread->join();
                    }
                }

                static void run(control_manager *owner);

            public:
                ~control_manager() override {
                    state.set_state(Disposed);
                    LOG_INFO("Disposing control manager. [state=%s]", state.get_state_string().c_str());
                    join();
                }

                void init(const __app *app, const ConfigValue *config) override;

                void clear_locks() {
                    if (!IS_EMPTY(semaphores)) {
                        unordered_map<string, _semaphore *>::iterator iter;
                        for (iter = semaphores.begin(); iter != semaphores.end(); iter++) {
                            _semaphore *sem = iter->second;
                            if (NOT_NULL(sem)) {
                                _semaphore_owner *c = static_cast<_semaphore_owner *>(sem);
                                c->reset();
                            }
                        }
                    }
                }

                uint64_t get_record_timeout() {
                    return record_timeout;
                }

                uint64_t get_lock_timeout() {
                    return lock_timeout;
                }
            };
        }
    }
}
#endif //WATERGATE_CONTROL_MANAGER_H
