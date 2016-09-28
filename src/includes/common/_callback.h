//
// Created by Subhabrata Ghosh on 20/09/16.
//

#ifndef WATERGATE_CALLBACK_H
#define WATERGATE_CALLBACK_H

#include "common.h"

namespace com {
    namespace watergate {
        namespace common {
            enum _callback_state_enum {
                UNKNOWN, SUCCESS, IGNORED, ERROR
            };

            class _callback_state {
            private:
                _callback_state_enum state = _callback_state_enum::UNKNOWN;
                exception *error = nullptr;

            public:
                ~_callback_state() {
                    dispose();
                }

                void set_state(_callback_state_enum state) {
                    this->state = state;
                }

                void set_error(exception *error) {
                    set_state(ERROR);
                    this->error = error;
                }

                const _callback_state_enum get_state() const {
                    return state;
                }

                const exception *get_error() const {
                    return this->error;
                }

                void dispose() {
                    CHECK_AND_FREE(error);
                }
            };

            class _callback {
            protected:
                void *context;
                _callback_state state;

            public:
                virtual ~_callback() {
                    state.dispose();
                }

                const _callback_state get_state() const {
                    return state;
                }

                template<typename T>
                T *get_context(T *t) const {
                    if (NOT_NULL(context)) {
                        T *tt = static_cast<T *>(context);
                        return tt;
                    }
                    return t;
                }

                void set_context(void *context) {
                    this->context = context;
                }

                void set_state(_callback_state_enum state) {
                    this->state.set_state(state);
                }

                void set_error(exception *error) {
                    this->state.set_error(error);
                }

                virtual void callback() = 0;
            };
        }
    }
}
#endif //WATERGATE_CALLBACK_H
