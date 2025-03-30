#include "ThreadPool.hpp"

namespace PBB::Thread
{

template <typename Tag>
ThreadPool<Tag>::ThreadPool()
  : ThreadPool(std::max(2u, std::thread::hardware_concurrency() - 1u))
{
}

template <typename Tag>
ThreadPool<Tag>::ThreadPool(std::size_t numThreads)
{
  m_done.clear();
  for (std::size_t i = 0; i < numThreads; ++i)
  {
    m_threads.emplace_back(&ThreadPool::Worker, this);
  }
}

template <typename Tag>
ThreadPool<Tag>::~ThreadPool()
{
  Destroy();
}

template <typename Tag>
void ThreadPool<Tag>::Destroy()
{
  m_done.test_and_set(std::memory_order_release);

#ifdef PBB_USE_TBB_QUEUE
  {
    std::lock_guard lock(m_mutex);
    m_condition.notify_all();
  }
#else
  m_workQueue.Invalidate();
#endif

  for (auto& thread : m_threads)
  {
    if (thread.joinable())
    {
      thread.join();
    }
  }
}

template <typename Tag>
void ThreadPool<Tag>::Worker()
{
  while (!m_done.test(std::memory_order_acquire))
  {
    std::unique_ptr<IThreadTask> pTask{ nullptr };

#ifdef PBB_USE_TBB_QUEUE
    {
      // Acquire the lock before waiting on the condition variable
      std::unique_lock lock(m_mutex);
      // Wait until either:
      // 1. Shutdown has been requested (m_done is set), OR
      // 2. There's work available in the queue
      //
      // Note: condition_variable::wait can return spuriously,
      // so this lambda acts as a guard to re-check the condition.
      m_condition.wait(
        lock, [&] { return m_done.test(std::memory_order_acquire) || !m_workQueue.empty(); });

      // Important: we must re-check m_done after waking up
      // because:
      // - We could have been woken by notify_all during shutdown
      // - A spurious wakeup may have occurred
      // - The queue could be empty even if we were notified
      //
      // So this is the canonical safe way to handle shutdown.
      if (m_done.test(std::memory_order_acquire))
        break;

      // At this point we assume there's work available.
      // Try to get a task from the queue — it's possible another
      // thread beat us to it, so this may still fail.
      if (!m_workQueue.try_pop(pTask) || !pTask)
        continue;
    }
#else
    // Blocking queue handles its own wait logic
    if (!m_workQueue.Pop(pTask))
    {
      if (m_done.test(std::memory_order_acquire))
      {
        break;
      }
      else
      {
        continue;
      }
    }
    if (!pTask)
    {
      continue;
    }
#endif
    pTask->Execute();
  }
}

template <typename Tag>
size_t ThreadPool<Tag>::NThreadsGet() const
{
  return m_threads.size();
}

} // namespace PBB::Thread
