#pragma once

#if __cplusplus < 202002L
#error "This header requires C++20"
#endif

#include <atomic>
#include <concepts>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include "ThreadPoolTags.hpp"
#include <PBB/Singleton.hpp>

namespace PBB::Thread
{

template <typename Tag>
class ThreadPool // : public Singleton<ThreadPool<Tag>>
{
  //  friend class Singleton<ThreadPool<Tag>>;

  // protected:
public:
  ThreadPool();
  ~ThreadPool();

public:
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  size_t NThreadsGet() const;

  template <typename Func, typename... Args>
  requires std::invocable<Func, Args...>
  auto submit(Func&& func, Args&&... args);

private:
  class IThreadTask
  {
  public:
    virtual ~IThreadTask() = default;
    virtual void Execute() = 0;
  };

  template <typename F>
  class ThreadTask : public IThreadTask
  {
  public:
    explicit ThreadTask(F&& func)
      : m_func(std::move(func))
    {
    }
    void Execute() override { m_func(); }

  private:
    F m_func;
  };

  template <typename T>
  class TaskFuture
  {
  public:
    explicit TaskFuture(std::future<T>&& fut)
      : m_future(std::move(fut))
      , m_detach(false)
    {
    }

    TaskFuture(TaskFuture&&) noexcept = default;
    TaskFuture& operator=(TaskFuture&&) noexcept = default;

    T Get() { return m_future.get(); }
    void Detach() { m_detach = true; }

    ~TaskFuture()
    {
      if (m_future.valid() && !m_detach)
        m_future.get();
    }

  private:
    std::future<T> m_future;
    bool m_detach;
  };

  void worker();
  void destroy();

  std::atomic_flag m_done = ATOMIC_FLAG_INIT;
  std::mutex m_mutex;
  std::condition_variable m_condition;
  std::vector<std::unique_ptr<IThreadTask>> m_queue;
  std::vector<std::thread> m_threads;
};

#ifndef PBB_HEADER_ONLY
extern template class ThreadPool<Tags::DefaultPool>; // Instantiate explicitly if needed
#endif
} // namespace sps::thread

#include "ThreadPool.inl" // Submit is inline/templated
#include "ThreadPool.txx" // Always included for header-only
