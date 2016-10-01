//
// Created by Subhabrata Ghosh on 19/09/16.
//

#ifndef WATERGATE_TIMER_H_H
#define WATERGATE_TIMER_H_H

#include <chrono>

#include "common.h"
#include "common_utils.h"

using namespace std::chrono;

namespace com {
    namespace watergate {
        namespace common {
            class time_utils {
            public:
                static uint64_t now() {
                    milliseconds nt = duration_cast<milliseconds>(
                            system_clock::now().time_since_epoch()
                    );
                    return nt.count();
                }

                static long get_delta_time(long start) {
                    milliseconds nt = duration_cast<milliseconds>(
                            system_clock::now().time_since_epoch()
                    );
                    return (nt.count() - start);
                }
            };

            class timer {
            private:
                time_point<system_clock> _t = system_clock::now().min();
                uint64_t elapsed = 0;

            public:
                void start() {
                    _t = system_clock::now();
                    elapsed = 0;
                }

                void pause() {
                    if (_t > system_clock::now().min()) {
                        milliseconds nt = duration_cast<milliseconds>(
                                system_clock::now().time_since_epoch()
                        );
                        milliseconds ot = duration_cast<milliseconds>(
                                _t.time_since_epoch()
                        );
                        milliseconds d = nt - ot;
                        elapsed += d.count();
                    }
                    _t = system_clock::now();
                }

                void restart() {
                    _t = system_clock::now();
                }

                void stop() {
                    if (_t > system_clock::now().min()) {
                        milliseconds nt = duration_cast<milliseconds>(
                                system_clock::now().time_since_epoch()
                        );
                        milliseconds ot = duration_cast<milliseconds>(
                                _t.time_since_epoch()
                        );
                        milliseconds d = nt - ot;
                        elapsed += d.count();
                    }
                    _t = system_clock::now();
                }

                uint64_t get_current_elapsed() {
                    if (_t > system_clock::now().min()) {
                        milliseconds nt = duration_cast<milliseconds>(
                                system_clock::now().time_since_epoch()
                        );
                        milliseconds ot = duration_cast<milliseconds>(
                                _t.time_since_epoch()
                        );
                        milliseconds d = nt - ot;
                        uint64_t e = elapsed + d.count();

                        return e;
                    }
                    return elapsed;
                }

                uint64_t get_elapsed() {
                    return elapsed;
                }
            };
        }
    }
}
#endif //WATERGATE_TIMER_H_H
