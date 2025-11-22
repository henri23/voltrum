#include <core/logger.hpp>
#include <math/math.hpp>

#define expect_should_be(expected, actual)                                     \
    {                                                                          \
        if (actual != expected) {                                              \
            CORE_ERROR("--> Expected %lld, but got %lld. File: %s:%d",         \
                expected,                                                      \
                actual,                                                        \
                __FILE__,                                                      \
                __LINE__);                                                     \
            return false;                                                      \
        }                                                                      \
    }

#define expect_should_not_be(expected, actual)                                 \
    {                                                                          \
        if (actual == expected) {                                              \
            CORE_ERROR(                                                        \
                "--> Expected %d != %d, but they are equal. File: %s:%d",      \
                expected,                                                      \
                actual,                                                        \
                __FILE__,                                                      \
                __LINE__);                                                     \
            return false;                                                      \
        }                                                                      \
    }

#define expect_float_to_be(expected, actual)                                   \
    {                                                                          \
        if (math_abs_value(actual - expected) > 0.001f) {                      \
            CORE_ERROR("--> Expected %f, but fot %f. File: %s:%d",             \
                expected,                                                      \
                actual,                                                        \
                __FILE__,                                                      \
                __LINE__);                                                     \
            return false;                                                      \
        }                                                                      \
    }

#define expect_true(actual)                                                    \
    {                                                                          \
        if (actual != true) {                                                  \
            CORE_ERROR("--> Expected true, but got false. File: %s:%d",        \
                expected,                                                      \
                actual,                                                        \
                __FILE__,                                                      \
                __LINE__);                                                     \
            return false;                                                      \
        }                                                                      \
    }

#define expect_false(actual)                                                   \
    {                                                                          \
        if (actual == true) {                                                  \
            CORE_ERROR("--> Expected false, but got true. File: %s:%d",        \
                expected,                                                      \
                actual,                                                        \
                __FILE__,                                                      \
                __LINE__);                                                     \
            return false;                                                      \
        }                                                                      \
    }
