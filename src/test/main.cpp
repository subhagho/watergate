//
// Created by Subhabrata Ghosh on 19/09/16.
//
#include <unistd.h>
#include <iostream>
#include <thread>

#include "includes/common/common.h"
#include "includes/common/_env.h"
#include "includes/core/control_manager.h"
#include "core/control/dummy_resource.h"
#include "includes/core/init_utils.h"

#define CONTROL_DEF_CONFIG_PATH "/configuration/control/def"
#define CONTROL_CONFIG_PATH "/configuration/control"

using namespace com::watergate::core;

#define CONTROL_NAME "dummy-resource-1"
#define REQUIRE _assert
#define METRIC_LOCK_TIME "lock.acquire.wait.time"
#define LOCK_TABLE_NAME "Test lock-table"

_env *create_env(const string file) {
    try {
        _env *env = new _env();
        env->create(file);
        CHECK_ENV_STATE(env);
        return env;
    } catch (exception &e) {
        cout << "error : " << e.what() << "\n";
        return nullptr;
    }
}

int main(int argc, char *argv[]) {

    try {
        _env *env = create_env(getenv("CONFIG_FILE_PATH"));
        REQUIRE(NOT_NULL(env));

        const Config *config = env->get_config();
        REQUIRE(NOT_NULL(config));

        control_manager *manager = init_utils::init_control_manager(env, CONTROL_CONFIG_PATH);
        REQUIRE(NOT_NULL(manager));

        control_client *control = init_utils::init_control_client(env, CONTROL_DEF_CONFIG_PATH);
        REQUIRE(NOT_NULL(control));

        thread_lock_ptr *tptr = control->register_thread(CONTROL_NAME);
        if (IS_NULL(tptr)) {
            throw BASE_ERROR("Error registering thread. [name=%s]", CONTROL_NAME);
        }
        string tid = tptr->thread_id;

        int err = 0;
        lock_acquire_enum r = control->lock(CONTROL_NAME, 0, 500, &err);
        REQUIRE(err == 0);
        REQUIRE(r == Locked);
        LOG_INFO("Successfully acquired lock [name=%s][priority=%d]", CONTROL_NAME, 0);

        bool b = control->release(CONTROL_NAME, 0);
        REQUIRE(b);
        LOG_INFO("Successfully released lock [name=%s][priority=%d]", CONTROL_NAME, 0);

        r = control->lock(CONTROL_NAME, 1, 500, &err);
        REQUIRE(err == 0);
        REQUIRE(r == Locked);
        LOG_INFO("Successfully acquired lock [name=%s][priority=%d]", CONTROL_NAME, 1);

        b = control->release(CONTROL_NAME, 1);
        REQUIRE(b);
        LOG_INFO("Successfully released lock [name=%s][priority=%d]", CONTROL_NAME, 1);

        control->dump();

        CHECK_AND_FREE(control);
        CHECK_AND_FREE(manager);
        CHECK_AND_FREE(env);
    } catch (const exception &e) {
        cout << "ERROR : " << e.what() << "\n";
    } catch (...) {
        cout << "Unknown error...\n";
    }
}
