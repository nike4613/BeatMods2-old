#ifndef BEATMODS2_UTIL_MACRO_H
#define BEATMODS2_UTIL_MACRO_H

#include <stdexcept>

#define EXEC(...) __VA_ARGS__

#define _STR(s) #s
#define STR(s) _STR(s)

#define sassert(...) { if (!(__VA_ARGS__)) throw std::runtime_error("Assertion failed: " STR((__VA_ARGS__))); }

#endif