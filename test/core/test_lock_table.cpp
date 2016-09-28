//
// Created by Subhabrata Ghosh on 22/09/16.
//

#include "test_lock_table.h"

using namespace com::watergate::core;
using namespace com::watergate::common;

#define LOCK_TABLE_NAME "Test lock-table"

TEST_CASE("Test SHM based lock table.", "[com::watergate::core::lock_table]") {
    _env *env = create_env(CONFIG_FILE);
    REQUIRE(NOT_NULL(env));

    const Config *config = env->get_config();
    REQUIRE(NOT_NULL(config));
    config->print();

    lock_table_manager *t = new lock_table_manager();
    t->init(LOCK_TABLE_NAME, nullptr);

    lock_table_client *c = new lock_table_client();
    c->init(env->get_app(), LOCK_TABLE_NAME, nullptr);

    _lock_record *records[DEFAULT_MAX_RECORDS - 1];

    int count = 0;
    for (int ii = 0; ii < DEFAULT_MAX_RECORDS - 1; ii++) {
        string app_name("LOCK_TABLE_TEST_");
        app_name.append(to_string(ii));
        pid_t pid = 1000 + ii;
        string app_id = common_utils::uuid();

        _lock_record *rec = c->new_record_test(app_name, app_id, pid);
        if (IS_NULL(rec)) {
            LOG_ERROR("Null record returned for index [%d]", ii);
        }
        REQUIRE(NOT_NULL(rec));

        records[ii] = rec;
        if (ii % 100 == 0) {
            LOG_DEBUG("Added [%d] records...", ii);
        }
        count++;
    }

    LOG_DEBUG("Created [%d] lock records.", count);

    count = 0;
    for (int ii = 0; ii < DEFAULT_MAX_RECORDS - 1; ii++) {
        _lock_record *rec = records[ii];

        if (ii % 100 == 0) {
            LOG_DEBUG("[index=%d][application name:%s id:%s pid:%d] Registered at [%d]", ii, rec->app.app_name,
                      rec->app.app_id, rec->app.proc_id,
                      rec->app.register_time);
        }
        c->remove_record(rec);
        count++;
    }
    LOG_DEBUG("Released [%d] lock records.", count);

    CHECK_AND_FREE(c);
    CHECK_AND_FREE(t);
    CHECK_AND_FREE(env);
}
