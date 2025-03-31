#pragma once

#include <iostream>
#include <utility>

namespace PBB::Thread
{

// Default pool
template <typename Tag>
struct ThreadPoolTraits
{
  static void WorkerLoop(auto& self) { self.DefaultWorkerLoop(); }

  template <typename Func, typename... Args>
  static auto Submit(auto& self, Func&& func, Args&&... args)
  {
    return self.SubmitDefault(std::forward<Func>(func), std::forward<Args>(args)...);
  }
};

template <>
struct ThreadPoolTraits<Tags::CustomPool>
{
  static void WorkerLoop(auto& self)
  {

    while (!self.m_done.test(std::memory_order_acquire))
    {
      std::unique_ptr<IThreadTask> pTask{ nullptr };

#ifdef PBB_USE_TBB_QUEUE
      {
        // Acquire the lock before waiting on the condition variable
        std::unique_lock lock(self.m_mutex);
        // Wait until either:
        // 1. Shutdown has been requested (m_done is set), OR
        // 2. There's work available in the queue
        //
        // Note: condition_variable::wait can return spuriously,
        // so this lambda acts as a guard to re-check the condition.
        self.m_condition.wait(lock,
          [&] { return self.m_done.test(std::memory_order_acquire) || !self.m_workQueue.empty(); });

        // Important: we must re-check m_done after waking up
        // because:
        // - We could have been woken by notify_all during shutdown
        // - A spurious wakeup may have occurred
        // - The queue could be empty even if we were notified
        //
        // So this is the canonical safe way to handle shutdown.
        if (self.m_done.test(std::memory_order_acquire))
          break;

        // At this point we assume there's work available.
        // Try to get a task from the queue — it's possible another
        // thread beat us to it, so this may still fail.
        if (!self.m_workQueue.try_pop(pTask) || !pTask)
          continue;
      }
#else
      // Blocking queue handles its own wait logic
      if (!self.m_workQueue.Pop(pTask))
      {
        if (self.m_done.test(std::memory_order_acquire))
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

  template <typename Func, typename... Args>
  static auto Submit(auto& self, Func&& func, Args&&... args)
  {
    std::cout << "[CustomPool] Submitting task\n";
    return self.SubmitDefault(std::forward<Func>(func), std::forward<Args>(args)...);
  }
};

} // namespace PBB::Thread
