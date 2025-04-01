#pragma once

#include <PBB/ThreadPool.hpp>
#include <PBB/ThreadPoolBase.hpp>

namespace PBB::Thread
{

/**
 * Destryoy
 *
 * Invalidates the queue and joins all running threads.
 */
template <typename Tag>
void ThreadPoolBase<Tag>::Destroy()
{
  this->m_done.test_and_set(std::memory_order_release);

#ifdef PBB_USE_TBB_QUEUE
  {
    std::lock_guard lock(m_mutex);
    m_condition.notify_all();
  }
#else
  this->m_workQueue.Invalidate();
#endif

  for (auto& thread : this->m_threads)
  {
    if (thread.joinable())
    {
      thread.join();
    }
  }
}

/**
 * Constructor
 *
 * Always create at least one thread. If hardware_concurrency() returns 0,
 * subtracting one would turn it to UINT_MAX, so get the maximum of
 * hardware_concurrency() and 2 before subtracting 1.
 */
template <typename Tag>
ThreadPoolBase<Tag>::ThreadPoolBase()
  : ThreadPoolBase(std::max(2u, std::thread::hardware_concurrency()) - 1u)
{
}

/**
 * Constructor. No implicit conversion allowed
 */
template <typename Tag>
ThreadPoolBase<Tag>::ThreadPoolBase(std::size_t numThreads)
{
  this->m_done.clear();
  for (std::size_t i = 0; i < numThreads; ++i)
  {
    this->m_threads.emplace_back([this] { static_cast<ThreadPool<Tag>*>(this)->Worker(); });
  }
}

template <typename Tag>
ThreadPoolBase<Tag>::~ThreadPoolBase()
{
  this->Destroy();
}

template <typename Tag>
size_t ThreadPoolBase<Tag>::NThreadsGet() const
{
  return this->m_threads.size();
}

} // namespace PBB::Thread
