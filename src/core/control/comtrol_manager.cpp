//
// Created by Subhabrata Ghosh on 11/10/16.
//

#include "includes/core/control_manager.h"


void com::watergate::core::control_manager::run(control_manager *owner) {
    CHECK_NOT_NULL(owner);

    try {
        LOG_INFO("Starting control manager thread...");
        com::watergate::common::alarm sw(DEFAULT_CONTROL_THREAD_SLEEP);
        while (NOT_NULL(owner) && owner->state.is_available()) {
            if (!sw.start()) {
                throw CONTROL_ERROR("Sleep state interrupted...");
            }
            if (!IS_EMPTY(owner->semaphores)) {
                unordered_map<string, _semaphore *>::iterator iter;
                for (iter = owner->semaphores.begin(); iter != owner->semaphores.end(); iter++) {
                    _semaphore *sem = iter->second;
                    if (NOT_NULL(sem)) {
                        _semaphore_owner *c = static_cast<_semaphore_owner *>(sem);
                        c->check_expired_locks(owner->lock_timeout);
                        c->check_expired_records(owner->record_timeout);
                    }
                }
            }
        }
        LOG_WARN("Shutting down control manager thread. [state=%s]", owner->state.get_state_string().c_str());
    } catch (const exception &e) {
        LOG_CRITICAL("[%d][name=control_manager] Control thread exited with error. [error=%s]", getpid(),
                     e.what());
    }
}

void com::watergate::core::control_manager::init(const _app *app, const ConfigValue *config) {
    create(app, config, true);

    clear_locks();


    string ss = DEFAULT_LOCK_RESET_TIME;
    {
        const BasicConfigValue *cn = Config::get_value(CONST_CM_CONFIG_LOCK_RESET_TIME, config);
        if (!IS_NULL(cn)) {
            const string sv = cn->get_value();
            if (!IS_EMPTY(sv)) {
                ss = string(sv);
            }
        }
    }

    lock_timeout = common_utils::parse_duration(ss);
    PRECONDITION(lock_timeout > 0);
    LOG_INFO("Using lock timeout value %lu msec.", lock_timeout);
    ss = DEFAULT_RECORD_RESET_TIME;
    {
        const BasicConfigValue *cn = Config::get_value(CONST_CM_CONFIG_RECORD_RESET_TIME, config);
        if (!IS_NULL(cn)) {
            const string sv = cn->get_value();
            if (!IS_EMPTY(sv)) {
                ss = string(sv);
            }
        }
    }

    record_timeout = common_utils::parse_duration(ss);
    PRECONDITION(record_timeout > 0);
    LOG_INFO("Using record reset timeout value %lu msec.", record_timeout);

    start();
}