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

#include <PBB/Config.h>
#include <PBB/MeyersSingleton.hpp>
#include <PBB/PhoenixSingleton.hpp>
#include <PBB/ThreadPoolTags.hpp>

namespace PBB
{
namespace Thread
{
template <typename Tag>
class ThreadPool : public MeyersSingleton<ThreadPool<Tag>>
{
  friend class MeyersSingleton<ThreadPool<Tag>>;

protected:
  ThreadPool();

  explicit ThreadPool(std::size_t numThreads);

  ~ThreadPool() override;

public:
  size_t NThreadsGet() const;

  template <typename Func, typename... Args>
  requires std::invocable<Func, Args...>
  auto Submit(Func&& func, Args&&... args);

  template <typename T>
  class TaskFuture
  {
  public:
    explicit TaskFuture(std::future<T>&& future)
      : m_future{ std::move(future) }
      , m_detach{ false }
    {
    }

    TaskFuture(TaskFuture&& other) noexcept = default;
    TaskFuture& operator=(TaskFuture&& other) noexcept = default;

    TaskFuture(const TaskFuture&) = delete;
    TaskFuture& operator=(const TaskFuture&) = delete;

    auto Get() { return m_future.get(); }
    void Detach() { m_detach = true; }

    ~TaskFuture()
    {
      if (m_future.valid() && !m_detach)
      {
        m_future.get();
      }
    }

  private:
    std::future<T> m_future;
    bool m_detach;
  };

private:
  class IThreadTask;
#ifdef PBB_USE_TBB_QUEUE
  using QueueImpl = tbb::concurrent_queue<std::unique_ptr<IThreadTask>>;
#else
  using QueueImpl = PBB::MRMWQueue<std::unique_ptr<IThreadTask>>;
#endif

  class IThreadTask
  {
  public:
    virtual ~IThreadTask() = default;
    virtual void Execute() = 0;

  protected:
    IThreadTask() = default;

  private:
    IThreadTask(const IThreadTask&) = delete;
    IThreadTask& operator=(const IThreadTask&) = delete;
  };

  template <typename Func>
  requires std::is_move_constructible_v<Func>
  class ThreadTask : public IThreadTask
  {
  public:
    explicit ThreadTask(Func&& func)
      : m_func{ std::move(func) }
    {
    }

    void Execute() override { m_func(); }

  private:
    ThreadTask(const ThreadTask&) = delete;
    ThreadTask& operator=(const ThreadTask&) = delete;
    Func m_func;
  };

  void Worker();
  void Destroy();

  std::atomic_flag m_done = ATOMIC_FLAG_INIT;
#ifdef PBB_USE_TBB_QUEUE
  std::mutex m_mutex;
  std::condition_variable m_condition;
#endif
  QueueImpl m_workQueue;
  std::vector<std::thread> m_threads;
};

#ifndef PBB_HEADER_ONLY
extern template class ThreadPool<Tags::DefaultPool>; // Instantiate explicitly if needed
#endif

// Expose default Submit function
namespace DefaultPool
{
template <typename F>
auto Submit(F&& f)
{
  return ThreadPool<Tags::DefaultPool>::InstanceGet().Submit(std::forward<F>(f));
}
} // namespace DefaultPool
} // namespace Thread
} // namespace PBB

#include "ThreadPool.inl" // Submit is inline/templated
#include "ThreadPool.txx" // Always included for header-only
