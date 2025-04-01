/**
 * @file   threadlocal.hpp
 * @author Jens Munk Hansen <jens.munk.hansen@gmail.com>
 * @date   Tue Mar 11 07:17:29 PM CET 2025
 *
 * @brief
 *
 * Copyright 2025 Jens Munk Hansen
 *
 */
#pragma once

#if __cplusplus < 201703L
#error "This header requires at least C++17"
#endif

#include <PBB/Config.h>
#include <PBB/Memory.hpp>

#include <atomic>
#include <concepts>
#include <memory>
#include <mutex>
#include <shared_mutex> // Used to minimize contention (C++17)
#include <thread>
#include <vector>

#ifdef PBB_USE_TBB_MAP
#include <tbb/concurrent_unordered_map.h>
#else
#include <unordered_map>
#endif

namespace PBB
{

// ----------------- C++17 Version (no support for std::atomic<std::shared_ptr<T>> ----------
namespace detail::v17
{
//! ThreadLocal
/*!  ThreadLocal types with dynamic storage.
 */
template <typename T, typename U = UnderlyingTypeT<T>,
  typename = std::enable_if_t<std::is_default_constructible_v<U>>>
class ThreadLocal
{
private:
  using StorageSharedPtr = std::shared_ptr<U>;
#ifdef PBB_USE_TBB_MAP
  tbb::concurrent_unordered_map<std::thread::id, StorageSharedPtr> _storage; // Thread-local storage
#else
  std::unordered_map<std::thread::id, StorageSharedPtr> _storage;
  std::shared_mutex _storagemutex;
#endif

  std::vector<U*> _registry;          // Stores thread-local variable references
  mutable std::mutex _registry_mutex; // Protects the registry

  /**
   * Register thread-local value, only once per thread
   *
   */
  void RegisterThreadLocalValue(U* pValue)
  {
    thread_local bool is_registered = false;
    if (!is_registered)
    {
      { // Lock the registry separately
        std::lock_guard<std::mutex> lock(_registry_mutex);
        _registry.push_back(pValue);
      }

      is_registered = true;
    }
  }

public:
  ThreadLocal() = default;

  /**
   * Access thread-local value
   *
   * @return Reference to be modified by a worker thread
   */
  U& GetThreadLocalValue()
  {
    thread_local bool is_registered = false;

    // Pre-emptying or rescheduling task does not modify thread_id. It
    // is not a stack variable that another can use.
    std::thread::id thread_id = std::this_thread::get_id();

    U* pValue = nullptr;
#if defined(PBB_USE_TBB_MAP)
    // TBB version - Corrected for C++17
    auto it = _storage.find(thread_id);
    if (it == _storage.end())
    {
      std::pair<typename tbb::concurrent_unordered_map<std::thread::id, StorageSharedPtr>::iterator,
        bool>
        result;
      result = _storage.emplace(thread_id, std::make_shared<U>());

      it = result.first;
    }
    pValue = it->second.get();
#else
    {
      std::shared_lock<std::shared_mutex> read_lock(_storagemutex);
      auto it = _storage.find(thread_id);
      if (it != _storage.end() && it->second)
      {
        pValue = it->second.get();
      }
    }

    if (!pValue)
    {
      // If not found, take an exclusive lock and insert
      std::unique_lock<std::shared_mutex> write_lock(_storagemutex);
      auto& value_ptr = _storage[thread_id];

      // Double-check to avoid race conditions
      if (!value_ptr)
      {
        value_ptr = std::make_shared<U>();
      }
      pValue = value_ptr.get();
    }
#endif

    // Auto-registration
    RegisterThreadLocalValue(pValue);
    return *pValue;
  }

public:
  /**
   * Access registry (should only be done by a single thread)
   *
   * @return Reference to registry
   */
  const std::vector<U*>& GetRegistry() const
  {
    // TODO: Check main thread - the thread used for registering sps::resource
    return _registry;
  }

  std::mutex& GetMutex() const
  {
    return _registry_mutex;
  }
};
} // namespace detail::v17

// ----------------- C++20 Version (support for std::atomic<std::shared_ptr<T>> ------------

// Not working unless I use TBB
#if __cplusplus >= 202002L
namespace detail::v20
{
// template <typename T, typename U = T>
// requires std::default_initializable<T>
template <typename T>
requires std::default_initializable<UnderlyingTypeT<T>>
class ThreadLocal
{
private:
  using U = UnderlyingTypeT<T>;
  // Must be copyable (or moveable), so we cannot use std::unique_ptr
  // using StorageSharedPtr = std::atomic<std::shared_ptr<T>>;
  using StorageSharedPtr = std::atomic<std::shared_ptr<U>>;
#ifdef PBB_USE_TBB_MAP
  tbb::concurrent_unordered_map<std::thread::id, StorageSharedPtr> _storage;
#else
  std::unordered_map<std::thread::id, StorageSharedPtr> _storage;
  // We need a mutex since std::unordered_map is not thread-safe
  std::mutex _storage_mutex;
#endif

  std::vector<U*> _registry;          // Stores thread-local variable references
  mutable std::mutex _registry_mutex; // Protects the registry

  /**
   * Register thread-local value, only once per thread. TODO: Remove this
   *
   */
  void RegisterThreadLocalValue(U* pValue)
  {
    thread_local bool is_registered = false;
    if (!is_registered)
    {
      {
        // Lock the registry separately (not thread-safe by design)
        std::lock_guard<std::mutex> lock(_registry_mutex);
        _registry.push_back(pValue);
      }
      is_registered = true;
    }
  }

public:
  ThreadLocal() = default;

  /**
   * Access thread-local value
   *
   * @return Reference to be modified by a worker thread
   */
  U& GetThreadLocalValue()
  {
    thread_local bool is_registered = false;

    // Pre-emptying or rescheduling task does not modify this and
    // it is not a stack variable that another can use.
    std::thread::id thread_id = std::this_thread::get_id();

    { // Scoped lock to protect storage_ from race conditions
#ifndef PBB_USE_TBB_MAP
      std::lock_guard<std::mutex> lock(
        _storage_mutex); // Can be skipped if we use tbb::concurrent_unordered_map
#endif
      // For std::unordered_map, we could operate without atomics (we are behind a lock)
      auto& value_ptr = _storage[thread_id];
      std::shared_ptr<U> current_value = value_ptr.load();
      if (!current_value)
      {
        // Not initialized, initialize is value
        std::shared_ptr<U> new_value = std::make_shared<U>();
        /*
          Attempts to atomically store new_value using
          compare_exchange_strong. If another thread already
          initialized it in the meantime, compare_exchange_strong
          fails, and current_value is updated with the latest value
          from value_ptr.load().
        */
        if (!value_ptr.compare_exchange_strong(current_value, new_value))
        {
          current_value = value_ptr.load();
        }
      }
    }
    auto p = _storage[thread_id].load(std::memory_order_acquire);

    // Auto-registration
    RegisterThreadLocalValue(p.get());
    return *p;
  }

public:
  /**
   * Access registry (should only be done by a single thread)
   *
   * @return Reference to registry
   */
  const std::vector<U*>& GetRegistry() const
  {
    return _registry;
  }

  /**
   * Access for locking registry from outside
   *
   * @return Reference to registry mutex
   */
  std::mutex& GetMutex() const
  {
    return _registry_mutex;
  }
};
} // namespace detail::v20
#endif

// ----------------- Expose the Most Recent Implementation in `PBB::detail` -----------------
namespace detail
{
//#if __cplusplus >= 202002L && (!defined(__GNUC__) || (__GNUC__ > 11))
#if __cplusplus >= 202002L && defined(PBB_ATOMIC_SHARED_PTR)
using namespace detail::v20; // Default to latest implementation in C++20+
#else
using namespace detail::v17; // Default to C++17 implementation otherwise
#endif
}
template <typename T>
using ThreadLocal = detail::ThreadLocal<CacheAlignedPlacement<T>>;
} // namespace PBB
