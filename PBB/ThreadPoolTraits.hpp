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
            std::shared_lock lock(self.m_initTasksMutex);
            auto it = self.m_initTasks.find(pTask.second);
            if (it != self.m_initTasks.end())
            {
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
            catch (...)
            {
              if (pTask.first)
              {
                try
                {
                  std::rethrow_exception(std::current_exception());
                }
                catch (const std::exception& e)
                {
                  pTask.first->OnInitializeFailure(e);
                }
              }
              continue; // ✅ Skip Execute
            }
          }
          else
          {
            // ✅ No initTask found — do NOT execute!
            continue;
          }
        }

        // ✅ Only run if successfully initialized
        pTask.first->Execute();
      }
    }
  }

#if 1

  /*
Great question — let's unpack this clearly and with intent. The use of
std::shared_ptr<std::promise<T>> and std::weak_ptr<std::promise<T>> is a subtle but powerful pattern
that gives us:

✅ Safe and correct control of the promise's lifetime
✅ Protection from double .set_value() / .set_exception()
✅ Exception-safe and memory-safe Submit() + Initialize() behavior

We store the promise in two places:

Inside the lambda that runs the actual task.

Inside the task object (like InitAwareTask) that handles OnInitializeFailure().

So we need shared ownership of the promise — hence shared_ptr.

This way:

Both the execution path and the error path can safely access and set the promise.

The promise lives as long as either the task body or the failure handler needs it.


Why std::weak_ptr<Promise> in the lambda?
We capture a weak_ptr inside the lambda to avoid keeping the promise alive unnecessarily if:

Initialization fails (and OnInitializeFailure() already sets the exception)

The lambda is never executed (e.g., skipped due to continue;)

The task is canceled or dropped

Then in the lambda:

cpp
Copy
Edit
if (auto p = weak_promise.lock()) {
  p->set_value(...);
}
This ensures that:

The lambda only sets the promise if it's still valid

We avoid double-setting, which would cause a crash (std::future_error)

The lambda is totally safe, even if skipped or run late

shared_ptr	Allows Submit() and InitAwareTask to own and set the promise
weak_ptr	Lets the lambda access the promise if still valid without extending lifetime
lock()	Prevents set_value() or set_exception() on an already-completed/destroyed promise
This combo	Makes your code memory-safe, exception-safe, and crash-free ✅

   */

  template <typename Func, typename... Args>
  static auto Submit(auto& self, Func&& func, Args&&... args, void* key)
  {
    using ResultType = std::invoke_result_t<Func, Args...>;
    using Promise = std::promise<ResultType>;
    using Future = PBB::Thread::TaskFuture<ResultType>;

    auto promise = std::make_shared<Promise>();
    auto future = Future{ promise->get_future() };
    auto weak_promise = std::weak_ptr<Promise>(promise);

    auto wrapped =
      [func = std::forward<Func>(func), ... args = std::forward<Args>(args), weak_promise]() mutable
    {
      try
      {
        if constexpr (std::is_void_v<ResultType>)
        {
          std::invoke(std::move(func), std::move(args)...);
          if (auto p = weak_promise.lock())
            p->set_value();
        }
        else
        {
          if (auto p = weak_promise.lock())
            p->set_value(std::invoke(std::move(func), std::move(args)...));
        }
      }
      catch (...)
      {
        if (auto p = weak_promise.lock())
          p->set_exception(std::current_exception());
      }
    };

    using Task = PBB::Thread::InitAwareTask<decltype(wrapped), Promise>;
    auto task = std::make_unique<Task>(std::move(wrapped), std::move(promise));
    self.m_workQueue.Push(std::make_pair(std::move(task), key));

    return future;
  }
#else
  template <typename Func, typename... Args>
  static auto Submit(auto& self, Func&& func, Args&&... args, void* key)
  {
    using ResultType = std::invoke_result_t<Func, Args...>;
    using Promise = std::promise<ResultType>;
    using Future = PBB::Thread::TaskFuture<ResultType>;

    auto promise = std::make_shared<Promise>();
    auto future = Future{ promise->get_future() };
    auto weak_promise = std::weak_ptr<Promise>(promise);

    // Use direct lambda capture instead of bind
    auto wrapped =
      [func = std::forward<Func>(func), ... args = std::forward<Args>(args), weak_promise]() mutable
    {
      try
      {
        if constexpr (std::is_void_v<ResultType>)
        {
          std::invoke(std::move(func), std::move(args)...);
          if (auto p = weak_promise.lock())
            p->set_value();
        }
        else
        {
          if (auto p = weak_promise.lock())
            p->set_value(std::invoke(std::move(func), std::move(args)...));
        }
      }
      catch (...)
      {
        if (auto p = weak_promise.lock())
          p->set_exception(std::current_exception());
      }
    };

    // Exception-aware task
    struct InitAwareTask : PBB::Thread::ThreadTask<decltype(wrapped)>
    {
      std::shared_ptr<Promise> promise;

      InitAwareTask(decltype(wrapped)&& f, std::shared_ptr<Promise> p)
        : PBB::Thread::ThreadTask<decltype(wrapped)>(std::move(f))
        , promise(std::move(p))
      {
      }

      void OnInitializeFailure(const std::exception& /*unused*/) override
      {
        promise->set_exception(std::current_exception());
      }
    };

    auto task = std::make_unique<InitAwareTask>(std::move(wrapped), std::move(promise));
    self.m_workQueue.Push(std::make_pair(std::move(task), key));

    return future;
  }
#endif
};

} // namespace PBB::Thread

// ASAN_OPTIONS=detect_leaks=1 ./your_app
// -fsanitize=address -g
