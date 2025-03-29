#pragma once

#if __cplusplus < 201103L
#error "This header requires at least C++11"
#endif

// Macro for registering destructor - in case compiler doesn't support attributes on templates
#define REGISTER_SINGLETON_DESTRUCTOR(Type, Priority)                                              \
  __attribute__((destructor(Priority))) static void DestroyPhoenixSingleton_##Type()               \
  {                                                                                                \
    (void)PBB::PhoenixSingleton<Type>::InstanceDestroy();                                          \
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
  static T* InstanceGet()
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

template <class T>
const int PhoenixSingleton<T>::s_atexit = PhoenixSingleton<T>::InstanceDestroy();

} // namespace PBB
