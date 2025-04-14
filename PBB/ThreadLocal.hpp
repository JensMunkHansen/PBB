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

namespace PBB
{

// ----------------- C++17 Version (no support for std::atomic<std::shared_ptr<T>> ----------
namespace detail::classic
{
template <typename T>
requires std::default_initializable<UnderlyingTypeT<T>>
class ThreadLocal
{
    using U = UnderlyingTypeT<T>;

    std::unordered_map<std::thread::id, std::shared_ptr<U>> _map;
    std::mutex _mutex;

    std::vector<U*> _registry;
    std::mutex _registry_mutex;

  public:
    U& Local()
    {
        std::thread::id tid = std::this_thread::get_id();
        std::shared_ptr<U> ptr;

        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto& ref = _map[tid];
            if (!ref)
            {
                ref = std::make_shared<U>();
                std::lock_guard<std::mutex> reg_lock(_registry_mutex);
                _registry.push_back(ref.get());
            }
            ptr = ref;
        }

        return *ptr;
    }

    const std::vector<U*>& GetRegistry() const { return _registry; }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _map.clear();

        std::lock_guard<std::mutex> reg_lock(_registry_mutex);
        _registry.clear();
    }
    std::mutex& GetMutex() { return _registry_mutex; }
};
} // namespace detail::classic

// ----------------- Expose the Most Recent Implementation in `PBB::detail` -----------------
namespace detail
{
using namespace detail::classic;
}
template <typename T>
using ThreadLocal = detail::ThreadLocal<CacheAlignedPlacement<T>>;
} // namespace PBB
