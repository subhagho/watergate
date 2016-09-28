//
// Created by Subhabrata Ghosh on 31/08/16.
//

#include "includes/common/_env.h"

com::watergate::common::_log *LOG = nullptr;

void com::watergate::common::_env::create(string filename) {
    try {
        this->config = new Config();
        this->config->create(filename);

        const ConfigValue *e_node = this->config->find(CONST_CONFIG_ENV_PATH);
        if (NOT_NULL(e_node)) {
            const ParamConfigValue *e_params = Config::get_params(e_node);
            if (NOT_NULL(e_params)) {
                const string app_name = e_params->get_string(CONST_CONFIG_ENV_PARAM_APPNAME);
                if (IS_EMPTY(app_name)) {
                    throw ERROR_MISSING_CONFIG(CONST_CONFIG_ENV_PARAM_APPNAME);
                }
                this->app = new _app(app_name);
                string appdir = this->app->get_app_directory();
                const string workdir = e_params->get_string(CONFIG_ENV_PARAM_WORKDIR);
                if (!IS_EMPTY(workdir)) {
                    this->work_dir = new Path(workdir);
                } else {
                    this->work_dir = new Path(CONST_DEFAULT_DIR);
                    if (!IS_EMPTY(appdir)) {
                        this->work_dir->append(appdir);
                    }
                    this->work_dir->append("work");
                }
                if (!this->work_dir->exists()) {
                    this->work_dir->create(0755);
                }

                const string tempdir = e_params->get_string(CONFIG_ENV_PARAM_TEMPDIR);
                if (!IS_EMPTY(tempdir)) {
                    this->temp_dir = new Path(tempdir);
                } else {
                    this->temp_dir = new Path(CONST_DEFAULT_DIR);
                    if (!IS_EMPTY(appdir)) {
                        this->temp_dir->append(appdir);
                    }
                    this->temp_dir->append("temp");
                }
                if (!this->temp_dir->exists()) {
                    this->temp_dir->create(0755);
                }

            } else {
                setup_defaults();
            }
            LOG = new _log();
            LOG->init(e_node, this->work_dir, this->app->get_name());
        } else {
            setup_defaults();
            LOG = new _log();
            LOG->init();
        }

        LOG_INFO("Initialized environement. [config=%s]", filename.c_str());
        state.set_state(Available);
    } catch (const exception &e) {
        state.set_error(&e);
        LOG_ERROR("Error initializing environment. [config=%s]", filename.c_str());
        LOG_ERROR("Error [%s]", e.what());
    } catch (...) {
        state.set_error(new runtime_error("Unknown error occurred while creating environment"));
        LOG_ERROR("Error initializing environment. [config=%s]", filename.c_str());
        LOG_ERROR("Error [%s]", state.get_error()->what());
    }
}

const Path *com::watergate::common::_env::get_temp_dir() const {
    CHECK_STATE_AVAILABLE(this->state);

    return this->temp_dir;
}

Path *com::watergate::common::_env::get_temp_dir(string name, mode_t mode) const {
    CHECK_STATE_AVAILABLE(this->state);
    assert(!name.empty());

    Path *pp = new Path(temp_dir->get_path());
    pp->append(name);

    file_utils::create_directory(pp->get_path(), mode);

    return pp;
}

const Path *com::watergate::common::_env::get_work_dir() const {
    CHECK_STATE_AVAILABLE(this->state);

    return this->work_dir;
}

Path *com::watergate::common::_env::get_work_dir(string name, mode_t mode) const {
    CHECK_STATE_AVAILABLE(this->state);
    assert(!name.empty());

    Path *pp = new Path(work_dir->get_path());
    pp->append(name);

    file_utils::create_directory(pp->get_path(), mode);

    return pp;
}

com::watergate::common::_env::~_env() {
    this->state.set_state(Disposed);
    CHECK_AND_FREE(this->config);
    CHECK_AND_FREE(this->app);
    CHECK_AND_FREE(temp_dir);
    CHECK_AND_FREE(work_dir);
    CHECK_AND_FREE(LOG);
}


