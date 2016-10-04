//
// Created by Subhabrata Ghosh on 21/09/16.
//

#ifndef WATERGATE_CONTROL_MANAGER_H
#define WATERGATE_CONTROL_MANAGER_H

#include "control_def.h"

namespace com {
    namespace watergate {
        namespace core {
            class control_manager : public control_def {
            public:
                void init(const _app *app, const ConfigValue *config) {
                    create(app, config, true);

                    clear_locks();
                }

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

                void dump() {

                }
            };
        }
    }
}
#endif //WATERGATE_CONTROL_MANAGER_H
