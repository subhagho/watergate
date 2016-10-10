//
// Created by Subhabrata Ghosh on 10/10/16.
//

#ifndef WATERGATE_FILESYSTEM_DRIVER_H
#define WATERGATE_FILESYSTEM_DRIVER_H

#include <unordered_map>

#include "includes/core/resource_def.h"
#include "includes/common/file_utils.h"

using namespace com::watergate::core;

namespace com {
    namespace watergate {
        namespace core {
            namespace io {
                class fs_driver_constants {
                public:
                    static const string CONFIG_PARAM_ROOT_PATH;
                    static const string CONFIG_PARAM_QUOTA_BYTES;
                    static const string CONFIG_PARAM_QUOTA_LEASE_TIME;
                    static const string CONFIG_PARAM_MAX_CONCURRENCY;
                };

                class filesystem_driver : public resource_def {
                private:
                    Path *root_path;
                    int concurrency;

                public:
                    filesystem_driver() : resource_def(IO) {}

                    ~filesystem_driver() {
                        CHECK_AND_FREE(root_path);
                    }

                    void init(const ConfigValue *config) override;

                    int get_control_size() override {
                        return concurrency;
                    }

                    const string *get_resource_name() override;
                };
            }
        }
    }
}

#endif //WATERGATE_FILESYSTEM_DRIVER_H
