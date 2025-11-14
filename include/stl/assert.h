#pragma once

#include "stl/traceback.h"

#include <sstream>

#undef assert

template <typename... Msg> void assert_impl(bool condition, Msg&&... message) {
    if (condition) [[likely]]
        return;

    std::ostringstream oss;
    oss << "Assertion failed at " << __FILE__ << ":" << __LINE__
        << " in function " << __func__ << "\n";

    (oss << ... << std::forward<Msg>(message)) << "\n";
    std::cerr << oss.str();

    stl::stacktrace();
    exit(EXIT_FAILURE);
}

#ifdef NDEBUG
    #define assert(...) ((void)0)
#else
    #define assert(...) assert_impl(__VA_ARGS__)
#endif
