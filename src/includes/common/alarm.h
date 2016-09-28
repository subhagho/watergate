//
// Created by Subhabrata Ghosh on 20/09/16.
//

#ifndef WATERGATE_ALARM_H
#define WATERGATE_ALARM_H

#include <unistd.h>
#include <csignal>

#include "common_utils.h"
#include "_callback.h"
#include "base_error.h"

#define START_ALARM(t, r) do {\
    com::watergate::common::alarm a(t); \
    r = a.start(); \
} while(0)

#define START_ALARM_WITH_CALLBACK(t, c, r) do {\
    com::watergate::common::alarm a(t, c); \
    r = a.start(); \
} while(0)

namespace com {
    namespace watergate {
        namespace common {
            class alarm {
            private:
                _callback *_c = nullptr;
                long _d;

                static void signal_handler(int signal) {
                    throw BASE_ERROR("Alarm interrupted...");
                }

            public:
                alarm(long delta) {
                    this->_d = delta;
                }

                alarm(long delta, _callback *c) {
                    this->_d = delta;
                    this->_c = c;
                }

                alarm(string duration) {
                    assert(!IS_EMPTY(duration));
                    this->_d = common_utils::parse_duration(duration);
                    assert((this->_d > 0));
                }

                alarm(string duration, _callback *c) {
                    assert(!IS_EMPTY(duration));
                    this->_d = common_utils::parse_duration(duration);
                    assert((this->_d > 0));

                    this->_c = c;
                }

                bool start() {
                    assert(_d > 0);
                    signal(SIGINT, &signal_handler);

                    long s_time = _d * 1000;
                    usleep(s_time);

                    if (NOT_NULL(_c)) {
                        _c->set_state(_callback_state_enum::SUCCESS);
                        _c->callback();
                    }
                    return true;
                }
            };
        }
    }
}
#endif //WATERGATE_ALARM_H
