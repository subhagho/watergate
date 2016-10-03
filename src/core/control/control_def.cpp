//
// Created by Subhabrata Ghosh on 31/08/16.
//

#include "includes/core/control_def.h"


using namespace com::watergate::core;


void com::watergate::core::control_def::create(const _app *app, const ConfigValue *config, bool server) {
    try {
        assert(NOT_NULL(config));

        if (config->get_type() == Node) {
            add_resource_lock(app, config, server);
        } else if (config->get_type() == List) {
            const ListConfigValue *list = static_cast<const ListConfigValue *>(config);
            const vector<ConfigValue *> values = list->get_values();
            if (!IS_EMPTY(values)) {
                for (auto v : values) {
                    add_resource_lock(app, v, server);
                }
            }
        }

        state.set_state(Available);
    } catch (const exception &e) {
        state.set_error(&e);
        throw CONTROL_ERROR("Error creating control handle. \n\tNested : %s", e.what());
    }
}

void com::watergate::core::control_def::add_resource_lock(const _app *app, const ConfigValue *config, bool server) {
    _semaphore *sem = nullptr;
    if (server) {
        sem = new _semaphore_owner();
    } else {
        sem = new _semaphore_client();
    }
    sem->init(app, config);

    semaphores.insert(make_pair(*sem->get_name(), sem));

    LOG_INFO("Created new semaphore handle. [name=%s]...", sem->get_name()->c_str());
}

com::watergate::core::control_def::~control_def() {
    state.set_state(Disposed);
    if (!IS_EMPTY(semaphores)) {
        for (auto kv : semaphores) {
            if (NOT_NULL(kv.second)) {
                free(kv.second);
            }
        }
        semaphores.clear();
    }
}

lock_acquire_enum com::watergate::core::control_client::try_lock(string name, int priority, double quota) {
    CHECK_STATE_AVAILABLE(state);

    _semaphore *sem = get_lock(name);
    if (IS_NULL(sem)) {
        throw CONTROL_ERROR("No registered lock with specified name. [name=%s]", name.c_str());
    }
    _semaphore_client *sem_c = static_cast<_semaphore_client *>(sem);

    if (IS_BASE_PRIORITY(priority))
        return sem_c->try_lock_base(quota, false);
    else
        return sem_c->try_lock(priority, false);
}

lock_acquire_enum
com::watergate::core::control_client::wait_lock(string name, int priority, double quota) {
    CHECK_STATE_AVAILABLE(state);

    _semaphore *sem = get_lock(name);
    if (IS_NULL(sem)) {
        throw CONTROL_ERROR("No registered lock with specified name. [name=%s]", name.c_str());
    }

    _semaphore_client *sem_c = static_cast<_semaphore_client *>(sem);

    if (IS_BASE_PRIORITY(priority))
        return sem_c->try_lock_base(quota, true);
    else
        return sem_c->try_lock(priority, true);
}

bool com::watergate::core::control_client::release_lock(string name, int priority) {
    CHECK_STATE_AVAILABLE(state);

    _semaphore *sem = get_lock(name);
    if (IS_NULL(sem)) {
        throw CONTROL_ERROR("No registered lock with specified name. [name=%s]", name.c_str());
    }

    _semaphore_client *sem_c = static_cast<_semaphore_client *>(sem);

    if (IS_BASE_PRIORITY(priority))
        return sem_c->release_lock_base();
    else
        return sem_c->release_lock(priority);
}

lock_acquire_enum
com::watergate::core::control_client::lock_get(string name, int priority, double quota, long timeout, int *err) {

    timer t;
    t.start();

    lock_acquire_enum ret = wait_lock(name, priority, quota);
    if (ret == Error) {
        throw CONTROL_ERROR("Error acquiring base lock. [name=%s]", name.c_str());
    } else if (ret == QuotaReached) {
        return ret;
    }
    if (t.get_current_elapsed() > timeout && (priority != 0)) {
        release_lock(name, priority);
        *err = ERR_CORE_CONTROL_TIMEOUT;
        return Timeout;
    }

    com::watergate::common::alarm a(DEFAULT_LOCK_LOOP_SLEEP_TIME * (priority + 1));
    for (int ii = priority - 1; ii >= 0; ii--) {
        while (true) {
            if (t.get_current_elapsed() > timeout) {
                for (int jj = ii - 1; jj >= 0; jj--) {
                    release_lock(name, jj);
                }
                *err = ERR_CORE_CONTROL_TIMEOUT;
                return Timeout;
            }

            ret = this->try_lock(name, ii, quota);
            if (ret == Locked || ret == QuotaReached || ret == Error) {
                break;
            } else {
                if (!a.start()) {
                    break;
                }
            }
        }
    }

    _semaphore *sem = get_lock(name);
    if (IS_NULL(sem)) {
        throw CONTROL_ERROR("No registered lock with specified name. [name=%s]", name.c_str());
    }

    _semaphore_client *sem_c = static_cast<_semaphore_client *>(sem);

    t.stop();
    long ts = t.get_elapsed();
    sem_c->update_metrics(priority, ts);

    return ret;
}

bool com::watergate::core::control_client::release(string name, int priority) {
    CHECK_STATE_AVAILABLE(state);

    bool r = true;
    for (int ii = priority; ii >= 0; ii--) {
        if (!release_lock(name, ii)) {
            r = false;
        }
    }
    return r;
}
