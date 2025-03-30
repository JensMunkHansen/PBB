#pragma once

namespace PBB::Thread
{

template <typename Tag>
class ThreadPoolBase
{
protected:
  using IThreadTask = typename ThreadPool<Tag>::IThreadTask;

#ifdef PBB_USE_TBB_QUEUE
  using QueueImpl = tbb::concurrent_queue<std::unique_ptr<IThreadTask>>;
#else
  using QueueImpl = PBB::MRMWQueue<std::unique_ptr<IThreadTask>>;
#endif

  ThreadPoolBase() = default;
  ~ThreadPoolBase() = default;

  std::atomic_flag m_done = ATOMIC_FLAG_INIT;
  QueueImpl m_workQueue;
  std::vector<std::thread> m_threads;

#ifdef PBB_USE_TBB_QUEUE
  std::mutex m_mutex;
  std::condition_variable m_condition;
#endif

  void DefaultWorkerLoop()
  {
    while (!m_done.test())
    {
      std::unique_ptr<IThreadTask> task;
      if (m_workQueue.try_pop(task))
      {
        task->Execute();
      }
#ifdef PBB_USE_TBB_QUEUE
      else
      {
        std::unique_lock lock(m_mutex);
        m_condition.wait(lock);
      }
#endif
    }
  }

  template <typename Func, typename... Args>
  auto DefaultSubmit(Func&& func, Args&&... args);
};

} // namespace PBB::Thread
