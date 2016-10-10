//
// Created by Subhabrata Ghosh on 19/09/16.
//
#include <sys/types.h>
#include <unistd.h>

#include "test_lock_client.h"
#include "includes/common/alarm.h"
#include "includes/common/timer.h"

#define REQUIRE _assert

void com::watergate::tests::common::basic_lock_client::setup() {
    const Config *config = env->get_config();
    assert(NOT_NULL(config));

    const ConfigValue *c_config = config->find(CONTROL_DEF_CONFIG_PATH);
    assert(NOT_NULL(c_config));

    control = new control_client();
    control->init(env->get_app(), c_config);
    CHECK_STATE_AVAILABLE(control->get_state());

    const ConfigValue *t_config = config->find(TLC_CONFIG_NODE);
    if (IS_NULL(t_config)) {
        throw CONFIG_ERROR("Test Client configuration not specified. [node=%s]", TLC_CONFIG_NODE);
    }

    const BasicConfigValue *sv = Config::get_value(TCL_CONFIG_VALUE_SLEEP, t_config);
    if (IS_NULL(sv)) {
        throw CONFIG_ERROR("Test Client configuration not specified. [node=%s]", TCL_CONFIG_VALUE_SLEEP);
    }
    sleep_timeout = sv->get_long_value(-1);
    if (sleep_timeout <= 0)
        throw CONFIG_ERROR("Invalid Test Client configuration. [node=%s][value=%d]", TCL_CONFIG_VALUE_SLEEP,
                           sleep_timeout);

    const BasicConfigValue *tv = Config::get_value(TCL_CONFIG_VALUE_TRIES, t_config);
    if (IS_NULL(tv)) {
        throw CONFIG_ERROR("Test Client configuration not specified. [node=%s]", TCL_CONFIG_VALUE_TRIES);
    }
    lock_tries = tv->get_int_value(-1);
    if (lock_tries <= 0)
        throw CONFIG_ERROR("Invalid Test Client configuration. [node=%s][value=%d]", TCL_CONFIG_VALUE_TRIES,
                           lock_tries);
}

void com::watergate::tests::common::basic_lock_client::run() {
    pid_t pid = getpid();
    thread_lock_ptr *tptr = control->register_thread(CONTROL_NAME);
    if (IS_NULL(tptr)) {
        throw BASE_ERROR("Error registering thread. [name=%s]", CONTROL_NAME);
    }
    string tid = tptr->thread_id;
    int count = 0;
    LOG_INFO("Running lock control client. [pid=%d]", pid);

    timer t;

    usleep((priority + 1) * 50 * 1000);
    for (int ii = 0; ii < 8; ii++) {
        while (true) {
            int err = 0;
            t.restart();
            lock_acquire_enum r = control->lock(CONTROL_NAME, priority, 200, 5000, &err);
            t.pause();
            if (r == Locked && err == 0) {
                LOG_INFO("Successfully acquired lock [pid=%d][thread=%s][name=%s][priority=%d][try=%d]", pid,
                         tid.c_str(),
                         CONTROL_NAME, priority,
                         ii);
                count++;
                usleep((2 - priority) * 500 * 1000);
                bool r = control->release(CONTROL_NAME, priority);
                if (r) {
                    LOG_INFO("Successfully released lock [pid=%d][thread=%s][name=%s][priority=%d][try=%d]", pid,
                             tid.c_str(),
                             CONTROL_NAME, priority,
                             ii);
                }
                break;
            } else if (err != 0) {
                LOG_ERROR("Filed to acquired lock [thread=%s][name=%s][priority=%d][try=%d][response=%d][error=%d]",
                          tid.c_str(),
                          CONTROL_NAME, priority, ii, r, err);
            } else
                LOG_ERROR("Filed to acquired lock [thread=%s][name=%s][priority=%d][try=%d][response=%d]", tid.c_str(),
                          CONTROL_NAME, priority, ii, r);
        }
        bool b = false;
        START_ALARM(sleep_timeout * (priority + 1), b);
        REQUIRE(b);
    }

    LOG_DEBUG("[pid=%d][priority=%d] Finished executing. [execution time=%lu]", pid, priority, t.get_current_elapsed());

    control->dump();
}

int main(int argc, char *argv[]) {
    cout << "Starting lock client...\n";
    for (int ii = 0; ii < argc; ii++) {
        cout << "ARG[" << ii << "]=" << argv[ii] << "\n";
    }

    try {
        argc -= (argc > 0);
        argv += (argc > 0); // skip program name argv[0] if present
        option::Stats stats(usage, argc, argv);
        std::vector<option::Option> options(stats.options_max);
        std::vector<option::Option> buffer(stats.buffer_max);
        option::Parser parse(usage, argc, argv, &options[0], &buffer[0]);

        if (parse.error())
            return 1;

        if (options[HELP] || argc == 0) {
            option::printUsage(std::cout, usage);
            return 0;
        }

        _env *env = nullptr;
        option::Option o = options[CONFIG];
        if (o && o.count() > 0) {
            if (o.arg) {
                string configf = o.arg;
                if (!IS_EMPTY(configf)) {
                    env = new _env();
                    env->create(configf);
                } else
                    throw CONFIG_ERROR("NULL/empty configuration file path specified.");
            }
        } else {
            option::printUsage(std::cout, usage);
            return -1;
        }
        CHECK_ENV_STATE(env);

        int priority = -1;
        o = options[PRIORITY];
        if (o && o.count() > 0) {
            if (o.arg) {
                priority = atoi(o.arg);
            }
        } else {
            option::printUsage(std::cout, usage);
            return -1;
        }

        if (priority < 0)
            throw BASE_ERROR("Invalid priority specified. [priority=%d]", priority);

        com::watergate::tests::common::basic_lock_client *client = new com::watergate::tests::common::basic_lock_client(
                env, priority);
        client->setup();
        client->run();

        CHECK_AND_FREE(env);
    } catch (const exception &e) {
        cout << "ERROR : " << e.what() << "\n";
    } catch (...) {
        cout << "Unkown exception.\n";
    }
}
