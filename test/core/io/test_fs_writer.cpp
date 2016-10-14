//
// Created by Subhabrata Ghosh on 14/10/16.
//

#include "test_fs_writer.h"

using namespace com::watergate::core;
using namespace com::watergate::common;

TEST_CASE("Basic locked write test", "[com::watergate::core::io::fs_writer]") {
init_utils::create_env(CONFIG_FILE);
const _env *env = init_utils::get_env();
REQUIRE(NOT_NULL(env));

const Config *config = init_utils::get_config();
REQUIRE(NOT_NULL(config));

config->

print();

control_manager *manager = init_utils::init_control_manager(env, CONTROL_CONFIG_PATH);
REQUIRE(NOT_NULL(manager));

const control_client *control = init_utils::init_control_client(env, CONTROL_DEF_CONFIG_PATH);
REQUIRE(NOT_NULL(control));

CHECK_AND_FREE(manager);

init_utils::dispose();

}
