// In a header like PhoenixSingleton.hpp

#pragma once

#if defined(_MSC_VER)

// MSVC-specific destructor registration
#include <cstdlib>
#define PBB_REGISTER_SINGLETON_DESTRUCTOR(Type)                                                    \
    namespace                                                                                      \
    {                                                                                              \
    struct AutoDestroy_##Type                                                                      \
    {                                                                                              \
        ~AutoDestroy_##Type() { (void)PBB::PhoenixSingleton<Type>::InstanceDestroy(); }            \
    };                                                                                             \
    static AutoDestroy_##Type autoDestroyInstance_##Type;                                          \
    }

#elif defined(__GNUC__) || defined(__clang__)

// GCC/Clang-style destructor attribute
#define PBB_REGISTER_SINGLETON_DESTRUCTOR(Type)                                                    \
    __attribute__((destructor(101))) static void DestroyPhoenixSingleton_##Type()                  \
    {                                                                                              \
        (void)PBB::PhoenixSingleton<Type>::InstanceDestroy();                                      \
    }

#else

// Fallback: use atexit
#include <cstdlib>
#define PBB_REGISTER_SINGLETON_DESTRUCTOR(Type)                                                    \
    namespace                                                                                      \
    {                                                                                              \
    struct AutoDestroy_##Type                                                                      \
    {                                                                                              \
        AutoDestroy_##Type()                                                                       \
        {                                                                                          \
            std::atexit([]() { (void)PBB::PhoenixSingleton<Type>::InstanceDestroy(); });           \
        }                                                                                          \
    };                                                                                             \
    static AutoDestroy_##Type autoDestroyInstance_##Type;                                          \
    }

#endif

private:
static std::atomic<T*> g_instance;
static std::atomic<bool> g_destroyed;

static T* InstanceGet()
{
    T* pInstance = g_instance.load(std::memory_order_acquire);
    if (!pInstance)
    {
        std::lock_guard<std::recursive_mutex> guard(g_mutex);
        pInstance = g_instance.load(std::memory_order_relaxed);
        if (!pInstance && !g_destroyed.load(std::memory_order_acquire))
        {
            pInstance = new T;
            g_instance.store(pInstance, std::memory_order_release);
        }
    }
    return pInstance;
}
