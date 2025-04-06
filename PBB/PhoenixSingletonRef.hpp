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

namespace PBB::detail
{

// Resurrection tracking trait
template <bool Resurrectable>
struct ResurrectionState;

// Non-resurrectable case — has real tracking
template <>
struct ResurrectionState<false>
{
    static std::atomic<bool>& Get()
    {
        static std::atomic<bool> destroyed{ false };
        return destroyed;
    }

    static bool IsDestroyed() { return Get().load(std::memory_order_acquire); }

    static void MarkDestroyed() { Get().store(true, std::memory_order_release); }
};

// Resurrectable case — no tracking at all
template <>
struct ResurrectionState<true>
{
    static constexpr bool IsDestroyed() { return false; }
    static void MarkDestroyed() {}
};
}

template <class T, bool Resurrectable = false>
class PhoenixSingleton
{
  public:
    static T& InstanceGet()
    {
        T* pInstance = g_instance.load(std::memory_order_acquire);
        if (!pInstance)
        {
            std::lock_guard<std::recursive_mutex> guard(g_mutex);
            pInstance = g_instance.load(std::memory_order_relaxed);
            if (!pInstance)
            {
                if constexpr (!Resurrectable)
                {
                    if (PBB::detail::ResurrectionState<Resurrectable>::IsDestroyed())
                        return nullptr;
                }
                (void)s_atexit;
                pInstance = new T;
                g_instance.store(pInstance, std::memory_order_release);
            }
        }
        return *pInstance;
    }

    static int InstanceDestroy() PBB_ATTR_DESTRUCTOR
    {
        T* pInstance = g_instance.exchange(nullptr, std::memory_order_acq_rel);
        if (pInstance)
        {
            delete pInstance;
            if constexpr (!Resurrectable)
            {
                PBB::detail::ResurrectionState<Resurrectable>::markDestroyed();
            }
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
    // Only needed if !Resurrectable
    static std::atomic<bool> g_destroyed;
    static const int s_atexit;
};

template <class T, bool Resurrectable>
std::atomic<T*> PhoenixSingleton<T, Resurrectable>::g_instance{ nullptr };

template <class T, bool Resurrectable>
std::recursive_mutex PhoenixSingleton<T, Resurrectable>::g_mutex;

template <class T, bool Resurrectable>
const int PhoenixSingleton<T, Resurrectable>::s_atexit =
  PhoenixSingleton<T, Resurrectable>::InstanceDestroy();

} // namespace PBB
