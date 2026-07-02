#pragma once

#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

struct Test {
    std::string name;
    std::function<bool()> fn;
};

extern std::vector<Test>& _allTests();

#define TEST(name)                                                              \
    static bool name();                                                         \
    static int _reg_##name = (_allTests().push_back({#name, name}), 0);         \
    static bool name()

#define ASSERT_TRUE(x)                                                          \
    do {                                                                        \
        if (!(x)) {                                                             \
            std::cerr << "  FAIL " << __FILE__ << ":" << __LINE__               \
                      << " ASSERT_TRUE(" << #x << ")" << std::endl;             \
            return false;                                                       \
        }                                                                       \
    } while (0)

#define ASSERT_EQ(a, b)                                                         \
    do {                                                                        \
        auto _a = (a);                                                          \
        auto _b = (b);                                                          \
        if (!(_a == _b)) {                                                      \
            std::cerr << "  FAIL " << __FILE__ << ":" << __LINE__               \
                      << " ASSERT_EQ(" << #a << ", " << #b << "): "             \
                      << _a << " != " << _b << std::endl;                       \
            return false;                                                       \
        }                                                                       \
    } while (0)

#define ASSERT_NEAR(a, b, eps)                                                  \
    do {                                                                        \
        double _a = (a);                                                        \
        double _b = (b);                                                        \
        double _e = (eps);                                                      \
        if (std::fabs(_a - _b) > _e) {                                          \
            std::cerr << "  FAIL " << __FILE__ << ":" << __LINE__               \
                      << " ASSERT_NEAR(" << #a << "=" << _a << ", " << #b       \
                      << "=" << _b << ", eps=" << _e << ")" << std::endl;       \
            return false;                                                       \
        }                                                                       \
    } while (0)
