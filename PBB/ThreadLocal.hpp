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
    tbb::concurrent_unordered_map<std::thread::id, StorageSharedPtr>
      _storage; // Thread-local storage
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
    U& Local()
    {
        // Pre-emptying or rescheduling task does not modify thread_id. It
        // is not a stack variable that another can use.
        std::thread::id thread_id = std::this_thread::get_id();

        U* pValue = nullptr;
#if defined(PBB_USE_TBB_MAP)
        // TBB version - Corrected for C++17
        auto it = _storage.find(thread_id);
        if (it == _storage.end())
        {
            std::pair<
              typename tbb::concurrent_unordered_map<std::thread::id, StorageSharedPtr>::iterator,
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

// ----------------- Expose the Most Recent Implementation in `PBB::detail` -----------------
namespace detail
{
using namespace detail::v17;
}
template <typename T>
using ThreadLocal = detail::ThreadLocal<CacheAlignedPlacement<T>>;
} // namespace PBB
