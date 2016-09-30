//
// Created by Subhabrata Ghosh on 19/09/16.
//
#include <unistd.h>
#include <iostream>
#include <thread>
#include <fstream>

#include "includes/common/common.h"
#include "includes/common/_env.h"
#include "includes/core/control_manager.h"

#define CONFIG_FILE "/home/subho/dev/wookler/watergate/test/data/test-sem-conf.json"
#define CONTROL_DEF_CONFIG_PATH "/configuration/control/def"
#define LOCK_TABLE_NAME "Test lock-table"

using namespace com::watergate::core;

#define CONTROL_NAME "test-dummy-resource"
#define REQUIRE assert

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

void run(control_client *control, int priority) {
    try {
        string tid = thread_lock_record::get_current_thread();
        int count = 0;
        for (int ii = 0; ii < 8; ii++) {
            int err = 0;
            while (true) {
                lock_acquire_enum r = control->lock(CONTROL_NAME, priority, 200, 5000, &err);
                if (r == Locked && err == 0) {
                    LOG_INFO("Successfully acquired lock [thread=%s][name=%s][priority=%d][try=%d]", tid.c_str(),
                             CONTROL_NAME, priority,
                             ii);
                    count++;
                    usleep(5 * 1000);
                    bool r = control->release(CONTROL_NAME, priority);
                    LOG_INFO("Successfully released lock [thread=%s][name=%s][priority=%d][index=%d]", tid.c_str(),
                             CONTROL_NAME, priority,
                             ii);
                    break;
                } else if (err != 0) {
                    LOG_ERROR("Filed to acquired lock [thread=%s][name=%s][priority=%d][try=%d][response=%d][error=%d]",
                              tid.c_str(),
                              CONTROL_NAME, priority, ii, r, err);
                } else
                    LOG_ERROR("Filed to acquired lock [thread=%s][name=%s][priority=%d][try=%d][response=%d]",
                              tid.c_str(),
                              CONTROL_NAME, priority, ii, r);
            }
        }

    } catch (const exception &e) {
        cout << "ERROR : " << e.what() << "\n";
    } catch (...) {
        cout << "Unknown error...\n";
    }
}

int main(int, char *[]) {

    try {

        _env *env = create_env(CONFIG_FILE);
                REQUIRE(NOT_NULL(env));

        const Config *config = env->get_config();
                REQUIRE(NOT_NULL(config));

        const ConfigValue *c_config = config->find(CONTROL_DEF_CONFIG_PATH);
                REQUIRE(NOT_NULL(c_config));

        control_manager *manager = new control_manager();
        manager->init(env->get_app(), c_config);

        control_client *control = new control_client();
        control->init(env->get_app(), c_config);

        int t_count = 5;
        thread *threads[t_count];
        for (int ii = 0; ii < t_count; ii++) {
            thread *t = new thread(run, control, (ii + 1) % 2);
            threads[ii] = t;
        }
        control->dump();

        for (int ii = 0; ii < t_count; ii++) {
            threads[ii]->join();
        }

        control->dump();

        manager->dump();
        manager->clear_locks();

        CHECK_AND_FREE(manager);
        CHECK_AND_FREE(control);
        CHECK_AND_FREE(env);


    } catch (const exception &e) {
        cout << "ERROR : " << e.what() << "\n";
    } catch (...) {
        cout << "Unknown error...\n";
    }
}
