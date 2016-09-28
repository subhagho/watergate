//
// Created by Subhabrata Ghosh on 19/09/16.
//

#ifndef WATERGATE_TEST_COMMON_H
#define WATERGATE_TEST_COMMON_H

_env *create_env(const string file) {
    try {
        _env *env = new _env();
        env->create(file);
        CHECK_ENV_STATE(env);
        return env;
    } catch (exception &e) {
        cout << "error : " << e.what() << "\n";
        return nullptr;
    }
}

#endif //WATERGATE_TEST_COMMON_H
