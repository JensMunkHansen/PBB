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
#ifdef PBB_USE_TBB_QUEUE
#include <tbb/concurrent_queue.h>
#else
#include <PBB/MRMWQueue.hpp>
#endif
#include <thread>
#include <type_traits>
#include <vector>

#include <PBB/MeyersSingleton.hpp>
#include <PBB/ResettableSingleton.hpp>
#include <PBB/ThreadPoolBase.hpp>
#include <PBB/ThreadPoolTags.hpp>
#include <PBB/ThreadPoolTraits.hpp>

namespace PBB::Thread
{

template <typename Tag>
class ThreadPool
  : public MeyersSingleton<ThreadPool<Tag>>
  //  : public ResettableSingleton<ThreadPool<Tag>>
  , public ThreadPoolBase<Tag>
{
  friend class MeyersSingleton<ThreadPool<Tag>>;

protected:
public:
  ThreadPool() = default;
  ~ThreadPool() = default;

  template <typename Func, typename... Args>
  auto SubmitDefault(Func&& func, Args&&... args);

  template <typename Func, typename... Args>
  requires std::invocable<Func, Args...>
  auto Submit(Func&& func, Args&&... args)
  {
    return ThreadPoolTraits<Tag>::Submit(
      *this, std::forward<Func>(func), std::forward<Args>(args)...);
  }

  void Worker() { ThreadPoolTraits<Tag>::WorkerLoop(*this); }

  using ThreadPoolBase<Tag>::DefaultSubmit;
  using ThreadPoolBase<Tag>::DefaultWorkerLoop;
};

} // namespace PBB::Thread

#include <PBB/ThreadPool.inl>
#include <PBB/ThreadPool.txx>
