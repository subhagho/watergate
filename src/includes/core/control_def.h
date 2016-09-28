//
// Created by Subhabrata Ghosh on 31/08/16.
//

#ifndef WATERGATE_CONTROL_OWNER_H
#define WATERGATE_CONTROL_OWNER_H

#include <stdio.h>
#include <mutex>

#include "includes/common/common_utils.h"
#include "includes/common/file_utils.h"
#include "includes/common/config.h"
#include "resource_def.h"
#include "control.h"
#include "includes/common/_state.h"
#include "control_errors.h"
#include "includes/common/alarm.h"

#define DEFAULT_MAX_TIMEOUT 30 * 1000
#define DEFAULT_LOCK_LOOP_SLEEP_TIME 10

namespace com {
    namespace watergate {
        namespace core {
            class control_def {
            protected:
                _state state;
                unordered_map<string, _semaphore *> semaphores;

                void add_resource_lock(const _app *app, const ConfigValue *config, bool server);

                _semaphore *get_lock(string name) {
                    CHECK_STATE_AVAILABLE(state);
                    if (!IS_EMPTY(name)) {
                        unordered_map<string, _semaphore *>::iterator iter = semaphores.find(name);
                        if (iter != semaphores.end()) {
                            return iter->second;
                        }
                    }
                    return nullptr;
                }

                void create(const _app *app, const ConfigValue *config, bool server);

            public:

                virtual ~control_def();

                virtual void init(const _app *app, const ConfigValue *config) = 0;

                const _state get_state() const {
                    return state;
                }

            };

            class control_client : public control_def {
            private:

                lock_acquire_enum try_lock(string name, int priority, double quota, bool update);

                lock_acquire_enum wait_lock(string name, int priority, double quota, bool update);

                bool release_lock(string name, int priority);

                lock_acquire_enum lock_get(string name, int priority, double quota, long timeout, int *err);


            public:
                void init(const _app *app, const ConfigValue *config) override {
                    create(app, config, false);
                }

                lock_acquire_enum lock(string name, int priority, double quota, int *err) {
                    return lock(name, priority, quota, DEFAULT_MAX_TIMEOUT, err);
                }

                lock_acquire_enum lock(string name, int priority, double quota, long timeout, int *err) {
                    CHECK_STATE_AVAILABLE(state);

                    timer t;
                    t.start();

                    lock_acquire_enum ret;
                    while (true) {
                        ret = lock_get(name, priority, quota, timeout, err);
                        if (ret != QuotaReached) {
                            break;
                        }
                        if (t.get_current_elapsed() > timeout && (priority != 0)) {
                            release_lock(name, priority);
                            *err = ERR_CORE_CONTROL_TIMEOUT;
                            return Timeout;
                        }
                    }
                    return ret;
                }

                bool release(string name, int priority);

                void dump() {
                    LOG_DEBUG("**************[REGISTERED CONTROLS:%d]**************", getpid());
                    if (!IS_EMPTY(semaphores)) {
                        unordered_map<string, _semaphore *>::iterator iter;
                        for (iter = semaphores.begin(); iter != semaphores.end(); iter++) {
                            _semaphore *sem = iter->second;
                            if (NOT_NULL(sem)) {
                                _semaphore_client *c = static_cast<_semaphore_client *>(sem);
                                c->dump();
                            }
                        }
                    }
                    LOG_DEBUG("**************[REGISTERED CONTROLS:%d]**************", getpid());
                }
            };
        }
    }
}

#endif //WATERGATE_CONTROL_OWNER_H
