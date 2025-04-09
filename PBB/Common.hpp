#pragma once

#include <exception>

#ifdef PBB_TESTING
#define PBB_ASSERT(x)                                                                              \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
            throw std::logic_error("Assertion failed: " #x);                                       \
    } while (0)
#else
#include <cassert>
#define PBB_ASSERT(x) assert(x)
#endif

#define PBB_DELETE_CTORS(Name)                                                                     \
    Name() = delete;                                                                               \
    Name(const Name&) = delete;                                                                    \
    Name(Name&&) = delete;                                                                         \
    Name& operator=(const Name&) = delete;                                                         \
    Name& operator=(Name&&) = delete;
