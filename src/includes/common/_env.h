//
// Created by Subhabrata Ghosh on 31/08/16.
//

#ifndef WATERGATE_ENV_H
#define WATERGATE_ENV_H

#include <vector>

#include "config.h"
#include "includes/core/control_def.h"
#include "file_utils.h"
#include "_app.h"
#include "log_utils.h"

#define CONFIG_ENV_PARAM_TEMPDIR "env.config.tempdir"
#define CONFIG_ENV_PARAM_WORKDIR "env.config.workdir"

#define CONST_DEFAULT_DIR "/tmp/watergate"

#define CONST_CONFIG_ENV_PARAM_APPNAME "app.name"
#define CONST_CONFIG_ENV_PATH "env"

#define CHECK_ENV_STATE(env) do { \
    if (IS_NULL(env)) { \
        throw runtime_error("Environment handle is NULL"); \
    } \
    CHECK_STATE_AVAILABLE(env->get_state()); \
} while(0)


namespace com {
    namespace watergate {
        namespace common {
            class _env {
            private:
                _state state;
                Config *config;
                _app *app;

                Path *work_dir;
                Path *temp_dir;

                void setup_defaults() {
                    this->app = new _app(CONST_CONFIG_ENV_PARAM_APPNAME);

                    this->work_dir = new Path(CONST_DEFAULT_DIR);
                    string appdir = this->app->get_app_directory();
                    if (!IS_EMPTY(appdir)) {
                        this->work_dir->append(appdir);
                    }
                    this->work_dir->append("work");

                    if (!this->work_dir->exists()) {
                        this->work_dir->create(0755);
                    }

                    this->temp_dir = new Path(CONST_DEFAULT_DIR);
                    if (!IS_EMPTY(appdir)) {
                        this->temp_dir->append(appdir);
                    }
                    this->temp_dir->append("temp");

                    if (!this->temp_dir->exists()) {
                        this->temp_dir->create(0755);
                    }
                }

            public:
                ~_env();

                void create(string filename);

                const _state get_state() const {
                    return state;
                }

                Config *get_config() const {
                    CHECK_STATE_AVAILABLE(this->state);

                    return this->config;
                }

                const Path *get_work_dir() const;

                const Path *get_temp_dir() const;

                const _app *get_app() const {
                    return app;
                }

                Path *get_work_dir(string name, mode_t mode) const;

                Path *get_temp_dir(string name, mode_t mode) const;
            };
        }
    }
}

#endif //WATERGATE_ENV_H
