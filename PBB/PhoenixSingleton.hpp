#pragma once

#if __cplusplus < 201103L
#error "This header requires at least C++11"
#endif

// Double-checked locking with std::atomic<T*>

// Manual destruction with atexit or __attribute__((destructor))

// Fully reentrant, lazy, and leak-safe

// Compatible with types that might be resurrected after destruction (i.e. phoenix behavior)

// Macro for registering destructor - in case compiler doesn't support attributes on templates
#define REGISTER_SINGLETON_DESTRUCTOR(Type, Priority)                                              \
    __attribute__((destructor(Priority))) static void DestroyPhoenixSingleton_##Type()             \
    {                                                                                              \
        (void)PBB::PhoenixSingleton<Type>::InstanceDestroy();                                      \
    }

#include <atomic>
#include <mutex>

#include <PBB/Platform.hpp>

namespace PBB
{
template <class T>
class PhoenixSingleton
{
  public:
    static T* InstancePtrGet()
    {
        T* pInstance = g_instance.load(std::memory_order_acquire);
        if (!pInstance)
        {
            std::lock_guard<std::recursive_mutex> guard(g_mutex);
            pInstance = g_instance.load(std::memory_order_relaxed);
            if (!pInstance)
            {
                (void)s_atexit;
                pInstance = new T;
                g_instance.store(pInstance, std::memory_order_release);
            }
        }
        return pInstance;
    }
    static T& InstanceGet()
    {
        T* ptr = InstancePtrGet();
        return *ptr;
    }
    static int InstanceDestroy() PBB_ATTR_DESTRUCTOR
    {
        T* pInstance = g_instance.exchange(nullptr, std::memory_order_acq_rel);
        if (pInstance)
        {
            delete pInstance;
            return 0;
        }
        return -1;
    }

    static bool IsAlive() { return g_instance.load(std::memory_order_acquire) != nullptr; }

#ifdef PBB_ENABLE_TEST_SINGLETON
    static void ResetForTest()
    {
        InstanceDestroy();
    }
#else
    static void ResetForTest() = delete; // or static_assert(false)
#endif

  protected:
    PhoenixSingleton() = default;
    virtual ~PhoenixSingleton() = default;

    PhoenixSingleton(const PhoenixSingleton&) = delete;
    PhoenixSingleton& operator=(const PhoenixSingleton&) = delete;

  private:
    static std::atomic<T*> g_instance;
    static std::recursive_mutex g_mutex;
    static const int s_atexit;
};

template <class T>
std::atomic<T*> PhoenixSingleton<T>::g_instance{ nullptr };

template <class T>
std::recursive_mutex PhoenixSingleton<T>::g_mutex;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
template <class T>
const int PhoenixSingleton<T>::s_atexit = PhoenixSingleton<T>::InstanceDestroy();

#pragma clang diagnostic pop

} // namespace PBB
