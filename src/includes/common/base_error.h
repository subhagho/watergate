//
// Created by Subhabrata Ghosh on 08/09/16.
//

#ifndef WATERGATE_BASE_ERROR_H
#define WATERGATE_BASE_ERROR_H

#include "common_utils.h"

#define CONST_NOTF_ERROR_PREFIX "Data Not Found : "

#define BASE_ERROR(fmt, ...) base_error(__FILE__, __LINE__, common_utils::format(fmt, ##__VA_ARGS__))
#define BASE_ERROR_PTR(fmt, ...) new base_error(__FILE__, __LINE__, common_utils::format(fmt, ##__VA_ARGS__))

#define NOT_FOUND_ERROR(fmt, ...) not_found_error(__FILE__, __LINE__, common_utils::format(fmt, ##__VA_ARGS__))
#define NOT_FOUND_ERROR_PTR(fmt, ...) new not_found_error(__FILE__, __LINE__, common_utils::format(fmt, ##__VA_ARGS__))

#define _assert(e) do {\
    if (!(e)) { \
        LOG_ERROR("Assertion failed. [%s][%s]", #e, __PRETTY_FUNCTION__); \
        throw BASE_ERROR("Assertion failed. [%s][%s]", #e, __PRETTY_FUNCTION__); \
    } \
} while(0);

#define ASSERT _assert

namespace com {
    namespace watergate {
        namespace common {
            class base_error : public exception {
            private:
                string *mesg;
                string file;
                int lineno;
                string r_mesg;

                void set_r_mesg() {
                    stringstream ss;
                    if (!IS_EMPTY(file)) {
                        ss << file << "\t";
                    }
                    if (lineno >= 0) {
                        ss << lineno << "\t";
                    }
                    if (NOT_EMPTY_P(mesg)) {
                        ss << *mesg;
                    }

                    r_mesg = string(ss.str());
                }

            protected:
                base_error(char const *file, const int line, const string prefix, string mesg) {
                    this->file = string(file);
                    this->lineno = line;
                    this->mesg = new string(common_utils::format("%s %s", prefix.c_str(), mesg.c_str()));

                    set_r_mesg();
                }

            public:
                base_error(char const *file, const int line, string mesg) {
                    this->file = file;
                    this->lineno = line;
                    this->mesg = new string(mesg);

                    set_r_mesg();
                }

                ~base_error() {
                    if (NOT_NULL(mesg)) {
                        delete (mesg);
                    }
                }

                const string get_error() {
                    return *mesg;
                }

                const char *what() const throw() override {
                    return r_mesg.c_str();
                }

            };

            class not_found_error : public base_error {
            public:
                not_found_error(char const *file, const int line, string mesg) : base_error(file, line,
                                                                                             CONST_NOTF_ERROR_PREFIX,
                                                                                             mesg) {
                }
            };
        }
    }
}
#endif //WATERGATE_BASE_ERROR_H
