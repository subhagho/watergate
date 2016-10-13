//
// Created by Subhabrata Ghosh on 13/10/16.
//

#ifndef WATERGATE_INIT_UTILS_H
#define WATERGATE_INIT_UTILS_H

#include "includes/core/control_def.h"
#include "includes/core/control_manager.h"

namespace com {
    namespace watergate {
        namespace core {
            class init_utils {
            public:
                static control_manager *init_control_manager(const _env *env, const string path) {
                    CHECK_NOT_NULL(env);
                    CHECK_NOT_EMPTY(path);

                    const Config *config = env->get_config();
                    CHECK_NOT_NULL(config);

                    const ConfigValue *m_config = config->find(path);
                    CHECK_NOT_NULL(m_config);

                    control_manager *manager = new control_manager();
                    manager->init(env->get_app(), m_config);

                    return manager;
                }

                static control_client *init_control_client(const _env *env, const string path) {
                    CHECK_NOT_NULL(env);
                    CHECK_NOT_EMPTY(path);

                    const Config *config = env->get_config();
                    CHECK_NOT_NULL(config);

                    const ConfigValue *c_config = config->find(path);
                    CHECK_NOT_NULL(c_config);

                    control_client *control = new control_client();
                    control->init(env->get_app(), c_config);

                    return control;
                }
            };
        }
    }
}
#endif //WATERGATE_INIT_UTILS_H
