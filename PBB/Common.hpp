#pragma once

#include <concepts> // for std::invocable
#include <exception>
#include <functional> // for std::invoke
#include <utility>    // for std::declval

template <typename F, typename... Args>
concept noexcept_invocable = std::invocable<F, Args...> &&
  noexcept(std::invoke(std::declval<F>(), std::declval<Args>()...));

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

#define PBB_DELETE_COPY_CTORS(Name)                                                                \
    Name(const Name&) = delete;                                                                    \
    Name& operator=(const Name&) = delete;

#define PBB_UNREFERENCED_PARAMETER(x) ((void)(x))
