//
// Created by Subhabrata Ghosh on 16/09/16.
//

#ifndef WATERGATE_LOG_UTILS_H
#define WATERGATE_LOG_UTILS_H

#include "_log.h"

#define LOG_DEBUG(fmt, ...) do { \
    if (com::watergate::common::log_utils::check_log_level(spdlog::level::debug)) { \
        string s = com::watergate::common::common_utils::format(fmt, ##__VA_ARGS__); \
        com::watergate::common::log_utils::debug(s); \
    } \
} while(0)
#define LOG_INFO(fmt, ...) do { \
    if (com::watergate::common::log_utils::check_log_level(spdlog::level::info)) { \
        string s = com::watergate::common::common_utils::format(fmt, ##__VA_ARGS__); \
        com::watergate::common::log_utils::info(s); \
    } \
} while(0)
#define LOG_WARN(fmt, ...) do { \
    if (com::watergate::common::log_utils::check_log_level(spdlog::level::warn)) { \
        string s = com::watergate::common::common_utils::format(fmt, ##__VA_ARGS__); \
        com::watergate::common::log_utils::warn(s); \
    } \
} while(0)
#define LOG_ERROR(fmt, ...) do { \
    if (com::watergate::common::log_utils::check_log_level(spdlog::level::err)) { \
        string s = com::watergate::common::common_utils::format(fmt, ##__VA_ARGS__); \
        com::watergate::common::log_utils::error(s); \
    } \
} while(0)
#define LOG_CRITICAL(fmt, ...) do { \
    if (com::watergate::common::log_utils::check_log_level(spdlog::level::critical)) { \
        string s = com::watergate::common::common_utils::format(fmt, ##__VA_ARGS__); \
        com::watergate::common::log_utils::critical(s); \
    } \
} while(0)
#define TRACE(fmt, ...) do { \
    if (com::watergate::common::log_utils::check_log_level(spdlog::level::trace)) { \
        string s = com::watergate::common::common_utils::format(fmt, ##__VA_ARGS__); \
        com::watergate::common::log_utils::trace(s); \
    } \
} while(0)

namespace spd = spdlog;

extern com::watergate::common::_log *LOG;

namespace com {
    namespace watergate {
        namespace common {
            class log_utils {
            public:
                static bool check_log_level(spd::level::level_enum level) {
                    return LOG->check_lock_level(level);
                }

                static void critical(string mesg) {
                    if (NOT_NULL(LOG)) {
                        bool console_enabled = LOG->console_enabled;
                        shared_ptr<spd::logger> console = LOG->get_console();
                        shared_ptr<spd::logger> logger = LOG->get_logger();

                        if (console_enabled || IS_NULL(logger)) {
                            if (NOT_NULL(console))
                                console->critical(mesg);
                        }
                        if (NOT_NULL(logger)) {
                            logger->critical(mesg);
                        }
                    }
                }

                static void error(string mesg) {
                    if (NOT_NULL(LOG)) {
                        bool console_enabled = LOG->console_enabled;
                        shared_ptr<spd::logger> console = LOG->get_console();
                        shared_ptr<spd::logger> logger = LOG->get_logger();

                        if (console_enabled || IS_NULL(logger)) {
                            if (NOT_NULL(console))
                                console->error(mesg);
                        }
                        if (NOT_NULL(logger)) {
                            logger->error(mesg);
                        }
                    }
                }

                static void warn(string mesg) {
                    if (NOT_NULL(LOG)) {
                        bool console_enabled = LOG->console_enabled;
                        shared_ptr<spd::logger> console = LOG->get_console();
                        shared_ptr<spd::logger> logger = LOG->get_logger();

                        if (console_enabled || IS_NULL(logger)) {
                            if (NOT_NULL(console))
                                console->warn(mesg);
                        }
                        if (NOT_NULL(logger)) {
                            logger->warn(mesg);
                        }
                    }
                }

                static void info(string mesg) {
                    if (NOT_NULL(LOG)) {
                        bool console_enabled = LOG->console_enabled;
                        shared_ptr<spd::logger> console = LOG->get_console();
                        shared_ptr<spd::logger> logger = LOG->get_logger();

                        if (console_enabled || IS_NULL(logger)) {
                            if (NOT_NULL(console))
                                console->info(mesg);
                        }
                        if (NOT_NULL(logger)) {
                            logger->info(mesg);
                        }
                    }
                }

                static void debug(string mesg) {
                    if (NOT_NULL(LOG)) {
                        bool console_enabled = LOG->console_enabled;
                        shared_ptr<spd::logger> console = LOG->get_console();
                        shared_ptr<spd::logger> logger = LOG->get_logger();

                        if (console_enabled || IS_NULL(logger)) {
                            if (NOT_NULL(console))
                                console->debug(mesg);
                        }
                        if (NOT_NULL(logger)) {
                            logger->debug(mesg);
                        }
                    }
                }

                static void trace(string mesg) {
                    if (NOT_NULL(LOG)) {
                        bool console_enabled = LOG->console_enabled;
                        shared_ptr<spd::logger> console = LOG->get_console();
                        shared_ptr<spd::logger> logger = LOG->get_logger();

                        if (console_enabled || IS_NULL(logger)) {
                            if (NOT_NULL(console))
                                console->trace(mesg);
                        }
                        if (NOT_NULL(logger)) {
                            logger->trace(mesg);
                        }
                    }
                }
            };
        }
    }
}
#endif //WATERGATE_LOG_UTILS_H
