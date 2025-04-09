#pragma once

#if __cplusplus < 202002L
#error "This header requires C++20"
#endif

#include <atomic>
#include <concepts>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include <PBB/Config.h>
#ifdef PBB_HEADER_ONLY
#include <PBB/MeyersSingleton.hpp>
#else
#include <PBB/PhoenixSingleton.hpp>
#endif
#ifdef PBB_USE_TBB_QUEUE
#include <tbb/concurrent_queue.h>
#else
#include <PBB/MRMWQueue.hpp>
#endif

#include <PBB/MeyersSingleton.hpp>
#include <PBB/ResettableSingleton.hpp>
#include <PBB/ThreadPoolBase.hpp>
#include <PBB/ThreadPoolTags.hpp>
#include <PBB/ThreadPoolTraits.hpp>

namespace PBB::Thread
{

#ifdef PBB_HEADER_ONLY
template <typename Tag>
class ThreadPool
  : public MeyersSingleton<ThreadPool<Tag>>
  , public ThreadPoolBase<Tag>
{
    friend class MeyersSingleton<ThreadPool<Tag>>;
#else
template <typename Tag>
class ThreadPool
  : public PhoenixSingleton<ThreadPool<Tag>>
  , public ThreadPoolBase<Tag>
{
    friend class PhoenixSingleton<ThreadPool<Tag>>;
#endif
  public:
    template <typename Func, typename... Args>
    requires std::invocable<Func, Args...>
    auto Submit(Func&& func, Args&&... args, void* key)
    {
        return ThreadPoolTraits<Tag>::Submit(
          *this, std::forward<Func>(func), std::forward<Args>(args)..., key);
    }
    template <typename Func, typename... Args>
    auto SubmitDefault(Func&& func, Args&&... args, void* key);

  protected:
    ThreadPool() = default;
    ~ThreadPool() = default;

    using ThreadPoolBase<Tag>::DefaultSubmit;
    using ThreadPoolBase<Tag>::DefaultWorkerLoop;
};

} // namespace PBB::Thread

#include <PBB/ThreadPool.inl>
#include <PBB/ThreadPool.txx>
