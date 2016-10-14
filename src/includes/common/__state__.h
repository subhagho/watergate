//
// Created by Subhabrata Ghosh on 31/08/16.
//

#ifndef WATERGATE_STATE_H
#define WATERGATE_STATE_H

#include <stdexcept>

#include "base_error.h"
#include "common.h"

#define CHECK_STATE_AVAILABLE(s) do{ \
    if (!s.is_available()) { \
        throw BASE_ERROR("Instance not in available state. [state=%s]", s.get_state_string().c_str()); \
    } \
} while(0);

#define CHECK_STATE_AVAILABLE_P(s) do{ \
    if (!s->is_available()) { \
        throw BASE_ERROR("Instance not in available state. [state=%s]", s->get_state_string().c_str()); \
    } \
} while(0);

namespace com {
    namespace watergate {
        namespace common {
            enum __state_enum {
                Unknown, Initialized, Available, Disposed, Exception
            };

            class __state__ {
            private:
                __state_enum state = Unknown;
                const exception *error = NULL;

            public:
                const __state_enum get_state() const {
                    return state;
                }

                string get_state_string() const {
                    switch (state) {
                        case Unknown:
                            return "Unknown";
                        case Initialized:
                            return "Initialized";
                        case Available:
                            return "Available";
                        case Disposed:
                            return "Disposed";
                        case Exception:
                            return "Exception";
                        default:
                            return EMPTY_STRING;
                    }
                }

                void set_state(__state_enum s) {
                    state = s;
                }

                void set_error(const exception *e) {
                    error = e;
                    state = Exception;
                }

                const exception *get_error() const {
                    if (state == Exception) {
                        return error;
                    }
                    return NULL;
                }

                bool has_error() const {
                    return (state == Exception);
                }

                bool is_available() const {
                    return (state == Available);
                }

                bool is_disposed() const {
                    return (state == Disposed);
                }
            };
        }
    }
}

#endif //WATERGATE_STATE_H
