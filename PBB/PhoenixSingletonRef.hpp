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
#include <cstdlib>
#include <mutex>

#include <PBB/Platform.hpp>

namespace PBB::detail
{

template <bool Resurrectable>
struct ResurrectionState;

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

template <>
struct ResurrectionState<true>
{
    static constexpr bool IsDestroyed() { return false; }
    static void MarkDestroyed() {}
};

} // namespace PBB::detail
namespace PBB
{
template <class T, bool Resurrectable = false>
class PhoenixSingletonRef
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
                if (detail::ResurrectionState<Resurrectable>::IsDestroyed())
                    return nullptr;

                pInstance = new T;
                g_instance.store(pInstance, std::memory_order_release);
            }
        }
        return pInstance;
    }

    static T& InstanceGet()
    {
        T* ptr = InstancePtrGet();
        assert(ptr && "PhoenixSingleton: Access after destruction when resurrection is disallowed");
        return *ptr;
    }

    static int InstanceDestroy() PBB_ATTR_DESTRUCTOR
    {
        T* pInstance = g_instance.exchange(nullptr, std::memory_order_acq_rel);
        if (pInstance)
        {
            delete pInstance;
            detail::ResurrectionState<Resurrectable>::MarkDestroyed();
            return 0;
        }
        return -1;
    }

    static bool IsAlive() { return g_instance.load(std::memory_order_acquire) != nullptr; }

  private:
    static std::atomic<T*> g_instance;
    static std::recursive_mutex g_mutex;
};

template <class T, bool R>
std::atomic<T*> PhoenixSingletonRef<T, R>::g_instance{ nullptr };

template <class T, bool R>
std::recursive_mutex PhoenixSingletonRef<T, R>::g_mutex;

}

#define PBB_REGISTER_SINGLETON_DESTRUCTOR(Type, Resurrectable)                                     \
    namespace                                                                                      \
    {                                                                                              \
    [[maybe_unused]] static auto _ForceSingletonRefDestroy_##Type =                                \
      &PBB::PhoenixSingletonRef<Type, Resurrectable>::InstanceDestroy;                             \
    __attribute__((destructor)) static void _AutoDestroySingleton_##Type()                         \
    {                                                                                              \
        (void)PBB::PhoenixSingletonRef<Type, Resurrectable>::InstanceDestroy();                    \
    }                                                                                              \
    }

// PBB_REGISTER_SINGLETON_DESTRUCTOR(MySingletonType, false)

// .cxx
// template class PBB::PhoenixSingleton<MySingletonType, false>;
// const int force_destroy_MySingletonType = PBB::PhoenixSingleton<MySingletonType,
// false>::InstanceDestroy();

#if defined(_MSC_VER)

// ✅ MSVC: Use static object whose destructor calls InstanceDestroy()
#define PBB_REGISTER_SINGLETON_DESTRUCTOR(Type, Resurrectable)                                     \
    namespace                                                                                      \
    {                                                                                              \
    struct PBB_SingletonAutoDestroy_##Type                                                         \
    {                                                                                              \
        ~PBB_SingletonAutoDestroy_##Type()                                                         \
        {                                                                                          \
            (void)PBB::PhoenixSingleton<Type, Resurrectable>::InstanceDestroy();                   \
        }                                                                                          \
    };                                                                                             \
    [[maybe_unused]] static PBB_SingletonAutoDestroy_##Type _autoDestroy_##Type;                   \
    [[maybe_unused]] static auto _forceInstanceDestroy_##Type =                                    \
      &PBB::PhoenixSingleton<Type, Resurrectable>::InstanceDestroy;                                \
    }

#elif defined(__GNUC__) || defined(__clang__)

// ✅ GCC/Clang: Use `__attribute__((destructor))`
#define PBB_REGISTER_SINGLETON_DESTRUCTOR(Type, Resurrectable)                                     \
    namespace                                                                                      \
    {                                                                                              \
    [[maybe_unused]] static auto _forceInstanceDestroy_##Type =                                    \
      &PBB::PhoenixSingleton<Type, Resurrectable>::InstanceDestroy;                                \
    __attribute__((destructor)) static void PBB_AutoDestroy_##Type()                               \
    {                                                                                              \
        (void)PBB::PhoenixSingleton<Type, Resurrectable>::InstanceDestroy();                       \
    }                                                                                              \
    }

#else

// ✅ Fallback: Use `std::atexit()`
#define PBB_REGISTER_SINGLETON_DESTRUCTOR(Type, Resurrectable)                                     \
    namespace                                                                                      \
    {                                                                                              \
    struct PBB_SingletonAutoDestroy_##Type                                                         \
    {                                                                                              \
        PBB_SingletonAutoDestroy_##Type()                                                          \
        {                                                                                          \
            std::atexit(                                                                           \
              [] { (void)PBB::PhoenixSingleton<Type, Resurrectable>::InstanceDestroy(); });        \
        }                                                                                          \
    };                                                                                             \
    [[maybe_unused]] static PBB_SingletonAutoDestroy_##Type _autoDestroy_##Type;                   \
    [[maybe_unused]] static auto _forceInstanceDestroy_##Type =                                    \
      &PBB::PhoenixSingleton<Type, Resurrectable>::InstanceDestroy;                                \
    }

#endif
