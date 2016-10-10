//
// Created by Subhabrata Ghosh on 15/09/16.
//

#include <includes/common/lock_record_def.h>
#include "dummy_resource.h"
#include "includes/core/control.h"

void com::watergate::core::dummy_resource::init(const ConfigValue *config) {
    const BasicConfigValue *nn = Config::get_value(CONST_SEM_CONFIG_PARAM_RESOURCE_NAME, config);
    if (IS_NULL(nn)) {
        throw ERROR_MISSING_CONFIG_NODE(CONST_SEM_CONFIG_PARAM_RESOURCE_NAME);
    }
    const string name = nn->get_value();
    if (IS_EMPTY(name)) {
        throw CONFIG_ERROR("NULL/Empty string value. [name=%s]", CONST_SEM_CONFIG_PARAM_RESOURCE_NAME);
    }
    this->name = new string(name);

    const BasicConfigValue *sn = Config::get_value(CONST_DR_CONFIG_PARAM_SIZE, config);
    if (NOT_NULL(sn)) {
        int s = sn->get_int_value(-1);
        if (s > 0) {
            this->size = s;
        }
    }

    this->lease_time = 5000;
    this->resource_quota = 1000;
}

int com::watergate::core::dummy_resource::get_control_size() {
    return this->size;
}

const string *com::watergate::core::dummy_resource::get_resource_name() {
    return this->name;
}
