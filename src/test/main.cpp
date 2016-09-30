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

//#define CONFIG_FILE "/home/subho/dev/wookler/watergate/test/data/test-sem-conf.json"
#define CONTROL_DEF_CONFIG_PATH "/configuration/control/def"
#define LOCK_TABLE_NAME "Test lock-table"

using namespace com::watergate::core;

#define CONTROL_NAME "test-dummy-resource"
#define REQUIRE _assert

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

typedef struct {
    string thread_id;
    uint32_t thread_index;
    uint32_t priority;
    uint64_t elapsed_time;
    uint64_t lock_wait_time;
} thread_record;

void rund(control_client *control, int priority, thread_record *record) {
    try {
        string tid = thread_lock_record::get_current_thread();
        record->priority = priority;
        record->thread_id = string(tid);
        timer tg, tl;
        tg.start();
        int count = 0;
        for (int ii = 0; ii < 8; ii++) {
            int err = 0;
            int retry_count = 0;
            while (true) {
                tl.restart();
                lock_acquire_enum r = control->lock(CONTROL_NAME, priority, 200, 20000, &err);
                tl.pause();
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
                retry_count++;

                if (retry_count > 1000) {
                    throw BASE_ERROR("Exceeded max retry count. [thread=%s][priority=%d]", tid.c_str(), priority);
                }
            }
        }
        tg.stop();

        record->elapsed_time = tg.get_elapsed();
        record->lock_wait_time = tg.get_current_elapsed();

        LOG_INFO("[thread=%s][priority=%d] Finished execution...", tid.c_str(), priority);
    } catch (const exception &e) {
        cout << "ERROR : " << e.what() << "\n";
    } catch (...) {
        cout << "Unknown error...\n";
    }
}

void run(control_client *control, int priority, thread_record *record) {
    try {
        string tid = thread_lock_record::get_current_thread();
        record->priority = priority;
        record->thread_id = string(tid);
        timer tg, tl;
        tg.start();
        int count = 0;
        for (int ii = 0; ii < 8; ii++) {
            int err = 0;
            int retry_count = 0;
            while (true) {
                tl.restart();
                lock_acquire_enum r = control->lock(CONTROL_NAME, priority, 200, 5000 * (priority + 1), &err);
                tl.pause();
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
                retry_count++;

                if (retry_count > 1000) {
                    throw BASE_ERROR("Exceeded max retry count. [thread=%s][priority=%d]", tid.c_str(), priority);
                }
            }
        }
        tg.stop();

        record->elapsed_time = tg.get_elapsed();
        record->lock_wait_time = tg.get_current_elapsed();

        LOG_INFO("[thread=%s][priority=%d] Finished execution...", tid.c_str(), priority);
    } catch (const exception &e) {
        cout << "ERROR : " << e.what() << "\n";
    } catch (...) {
        cout << "Unknown error...\n";
    }
}

int main(int argc, char *argv[]) {

    try {
        if (argc < 1) {
            throw BASE_ERROR("Missing required argument. <config file>");
        }
        string cf(argv[1]);
        _assert(!IS_EMPTY(cf));

        _env *env = create_env(cf);
                REQUIRE(NOT_NULL(env));

        const Config *config = env->get_config();
                REQUIRE(NOT_NULL(config));

        const ConfigValue *c_config = config->find(CONTROL_DEF_CONFIG_PATH);
                REQUIRE(NOT_NULL(c_config));

        control_manager *manager = new control_manager();
        manager->init(env->get_app(), c_config);

        control_client *control = new control_client();
        control->init(env->get_app(), c_config);

        int t_count = 6;

        thread *threads[t_count];
        thread_record * records[t_count];
        for (int ii = 0; ii < t_count - 1; ii++) {
            thread_record * tr = new thread_record();
            tr->thread_index = ii;
            thread *t = new thread(run, control, ii % 2, tr);

            threads[ii] = t;
            records[ii] = tr;
        }
        {
            int ii = t_count - 1;
            thread_record * tr = new thread_record();
            tr->thread_index = ii;
            thread *t = new thread(rund, control, 1, tr);

            threads[ii] = t;
            records[ii] = tr;
        }
        control->dump();

        for (int ii = 0; ii < t_count; ii++) {
            threads[ii]->join();
        }


        control->dump();

        manager->dump();
        manager->clear_locks();

        for (int ii = 0; ii < t_count; ii++) {
            thread_record * tr = records[ii];
            if (NOT_NULL(tr)) {
                LOG_WARN("[thread:%s][priority=%d] total elapsed time = %d, total lock wait time = %d",
                         tr->thread_id.c_str(),
                         tr->priority, tr->elapsed_time, tr->lock_wait_time);
            }
        }

        CHECK_AND_FREE(manager);
        CHECK_AND_FREE(control);
        CHECK_AND_FREE(env);


    } catch (const exception &e) {
        cout << "ERROR : " << e.what() << "\n";
    } catch (...) {
        cout << "Unknown error...\n";
    }
}
