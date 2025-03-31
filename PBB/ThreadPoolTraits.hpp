#pragma once

#include <any>
#include <iostream>
#include <shared_mutex>
#include <utility>

namespace PBB::Thread
{

// Default pool
template <typename Tag>
struct ThreadPoolTraits
{
  static void WorkerLoop(auto& self) { self.DefaultWorkerLoop(); }

  template <typename Func, typename... Args>
  static auto Submit(auto& self, Func&& func, Args&&... args, void* key)
  {
    return self.SubmitDefault(std::forward<Func>(func), std::forward<Args>(args)..., key);
  }
};

template <>
struct ThreadPoolTraits<Tags::CustomPool>
{
  static void WorkerLoop(auto& self)
  {
    thread_local bool initialized = false;
    thread_local void* init_key = nullptr;
    thread_local std::any init_result; // Store return value of `Initialize()` if any.

    while (!self.m_done.test(std::memory_order_acquire))
    {
      std::pair<std::unique_ptr<IThreadTask>, void*> pTask{ nullptr, nullptr };
      if (self.m_workQueue.Pop(pTask))
      {
        if (init_key != pTask.second)
        {
          initialized = false;
          init_key = pTask.second;
          init_result.reset();
        }

        if (!initialized)
        {
          std::function<void()> initTask = nullptr;
          {
            // Acquire lock only while accessing `m_initTasks`
            std::shared_lock lock(self.m_initTasksMutex);
            auto it = self.m_initTasks.find(pTask.second);
            if (it != self.m_initTasks.end())
            {
              // Copy function reference (copy or move)
              initTask = it->second;
            }
          }
          if (initTask)
          {
            try
            {
              initTask();
              initialized = true;
            }
            catch (const std::exception& e)
            {
              std::cerr << "Thread " << std::this_thread::get_id()
                        << " failed initialization: " << e.what() << std::endl;
            }
          }
        }

        pTask.first->Execute();
      }
    }
  }

  template <typename Func, typename... Args>
  static auto Submit(auto& self, Func&& func, Args&&... args, void* key)
  {
    std::cout << "[CustomPool] Submitting task\n";
    return self.DefaultSubmit(std::forward<Func>(func), std::forward<Args>(args)..., key);
  }
};

} // namespace PBB::Thread
