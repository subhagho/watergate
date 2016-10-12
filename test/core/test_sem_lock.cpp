//
// Created by Subhabrata Ghosh on 16/09/16.
//

#include <unistd.h>

#include "test_sem_lock.h"
#include "test_lock_client.h"
#include "includes/core/control_manager.h"

using namespace com::watergate::core;
using namespace com::watergate::common;

TEST_CASE("Basic control setup", "[com::watergate::core::control_def]") {
    _env *env = create_env(CONFIG_FILE);
    REQUIRE(NOT_NULL(env));

    const Config *config = env->get_config();
    REQUIRE(NOT_NULL(config));
    config->print();

    const ConfigValue *c_config = config->find(CONTROL_DEF_CONFIG_PATH);
    REQUIRE(NOT_NULL(c_config));

    control_manager *control = new control_manager();
    control->init(env->get_app(), c_config);

    CHECK_AND_FREE(control);
    CHECK_AND_FREE(env);
}

TEST_CASE("Basic lock operations", "[com::watergate::core::control_def]") {
    try {
        _env *env = create_env(CONFIG_FILE);
        REQUIRE(NOT_NULL(env));

        const Config *config = env->get_config();
        REQUIRE(NOT_NULL(config));

        const ConfigValue *c_config = config->find(CONTROL_DEF_CONFIG_PATH);
        REQUIRE(NOT_NULL(c_config));

        LOG_DEBUG("Creating Control Manager...");
        control_manager *manager = new control_manager();
        manager->init(env->get_app(), c_config);

        LOG_DEBUG("Creating Control Client...");
        control_client *control = new control_client();
        control->init(env->get_app(), c_config);

        thread_lock_ptr *tptr = control->register_thread(CONTROL_NAME);
        if (IS_NULL(tptr)) {
            LOG_ERROR("Error registering thread. [name=%s]", CONTROL_NAME);
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
        LOG_ERROR(e.what());
        REQUIRE(false);
    } catch (...) {
        LOG_ERROR("Unknown error.");
        REQUIRE(false);
    }
}


TEST_CASE("Fail lock operations", "[com::watergate::core::control_def]") {
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

    thread_lock_ptr *tptr = control->register_thread(CONTROL_NAME);
    if (IS_NULL(tptr)) {
        throw BASE_ERROR("Error registering thread. [name=%s]", CONTROL_NAME);
    }
    string tid = tptr->thread_id;

    int count = 0;
    for (int ii = 0; ii < 8; ii++) {
        int err = 0;
        lock_acquire_enum r = control->lock(CONTROL_NAME, 0, 500, 1000, &err);
        REQUIRE(err == 0);
        if (r == Locked) {
            LOG_INFO("Successfully acquired lock [name=%s][priority=%d][try=%d]", CONTROL_NAME, 0, ii);
            count++;
        } else
            LOG_ERROR("Filed to acquired lock [name=%s][priority=%d][try=%d]", CONTROL_NAME, 0, ii);
    }

    for (int ii = 0; ii < count; ii++) {
        bool r = control->release(CONTROL_NAME, 0);
        if (r)
            LOG_INFO("Successfully released lock [name=%s][priority=%d][index=%d]", CONTROL_NAME, 0, ii);
        else
            LOG_INFO("Lock already released [name=%s][priority=%d][index=%d]", CONTROL_NAME, 0, ii);
    }

    control->dump();
    control->test_assert();
    CHECK_AND_FREE(control);
    CHECK_AND_FREE(env);
}

TEST_CASE("Inter-process lock operations", "[com::watergate::core::control_def]") {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
        fprintf(stdout, "Current working dir: %s\n", cwd);
    else
        perror("getcwd() error");

    _env *env = create_env(CONFIG_FILE);
    REQUIRE(NOT_NULL(env));

    const Config *config = env->get_config();
    REQUIRE(NOT_NULL(config));

    const ConfigValue *c_config = config->find(CONTROL_DEF_CONFIG_PATH);
    REQUIRE(NOT_NULL(c_config));

    control_manager *manager = new control_manager();
    manager->init(env->get_app(), c_config);

    const ConfigValue *p_config = config->find(TLC_CONFIG_NODE);
    REQUIRE(NOT_NULL(p_config));

    const BasicConfigValue *pv = Config::get_value(TCL_CONFIG_VALUE_PROCESSES, p_config);
    REQUIRE(NOT_NULL(pv));
    int p_count = pv->get_int_value(-1);
    REQUIRE(p_count > 0);

    const BasicConfigValue *nv = Config::get_value(TCL_CONFIG_VALUE_PROCESS, p_config);
    REQUIRE(NOT_NULL(nv));
    const string p_name = nv->get_value();

    string proc(p_name);
    string c_param("--config=");
    c_param.append(CONFIG_FILE);

    pid_t pids[p_count];
    for (int ii = 0; ii < p_count; ii++) {
        pids[ii] = -1;
        string p = string("--priority=");
        p.append(to_string(ii % 3));

        string index = string("--index=");
        index.append(to_string(ii));

        pid_t pid = fork();
        switch (pid) {
            case -1: /* Error */
                LOG_ERROR("Uh-Oh! fork() failed. [index=%d]", ii);
                exit(1);
            case 0:
                LOG_INFO("Launching process [index=%d]. [%s %s %s %s]", ii, proc.c_str(), c_param.c_str(), p.c_str(),
                         index.c_str());

                execl(proc.c_str(), proc.c_str(), c_param.c_str(), p.c_str(), index.c_str(), (char *) nullptr);
                LOG_ERROR("Uh-Oh! execl() failed! [index=%d]", ii);/* execl doesn't return unless there's an error */
                exit(1);
            default:
                LOG_INFO("Launched child process. [cmd=%s][pid=%d]", p.c_str(), pid);
                pids[ii] = pid;
        }
    }
    for (int ii = 0; ii < p_count; ii++) {
        if (pids[ii] <= 0) {
            continue;
        }
        LOG_INFO("Waiting for process [%d]...", pids[ii]);
        int status;

        while (!WIFEXITED(status)) {
            waitpid(pids[ii], &status, 0); /* Wait for the process to complete */
        }

        LOG_INFO("Process [pid=%d] exited with status [%d]", pids[ii], status);
    }

    CHECK_AND_FREE(manager);
    CHECK_AND_FREE(env);
}
