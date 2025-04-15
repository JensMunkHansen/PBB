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

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <PBB/Config.h>
#include <PBB/Memory.hpp>

#ifdef PBB_USE_TBB_MAP
#include <tbb/concurrent_unordered_map.h>
#endif

namespace PBB
{
namespace detail::v17
{
template <typename T, typename U>
class IThreadLocal
{
  public:
    virtual ~IThreadLocal() = default;
    virtual U& Local() = 0;
    const std::vector<U*>& GetRegistry() const;
    std::mutex& GetMutex();
};
// requires std::is_default_constructible_v<U>

template <typename T, typename U = UnderlyingTypeT<T>,
  typename = std::enable_if_t<std::is_default_constructible_v<U>>>
class ThreadLocal : public IThreadLocal<T, U>
{
  private:
    using StorageSharedPtr = std::shared_ptr<U>;

#ifdef PBB_USE_TBB_MAP
    tbb::concurrent_unordered_map<std::thread::id, StorageSharedPtr> _storage;
#else
    std::unordered_map<std::thread::id, StorageSharedPtr> _storage;
    std::shared_mutex _storagemutex;
#endif

    std::vector<U*> _registry;
    std::mutex _registry_mutex;

    void RegisterThreadLocalValue(U* pValue);

  public:
    ThreadLocal();
    U& Local() override;
    const std::vector<U*>& GetRegistry() const;
    std::mutex& GetMutex();
};

} // namespace detail::v17

} // namespace PBB

#include <PBB/ThreadLocal.txx>
