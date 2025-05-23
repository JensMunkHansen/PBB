#pragma once

#if __cplusplus < 202002L
#error "This header requires at least C++20"
#endif

#include <any>
#include <exception>
#include <iostream>
#include <shared_mutex>
#include <utility>

#include <PBB/Common.hpp>
#include <PBB/ThreadPoolBase.hpp>
#include <PBB/ThreadPoolTags.hpp>
namespace PBB::Thread
{

//! ThreadPoolTraits
/*! Traits for specializing functionality for submitting tasks and the
    worker loop. This is the default behavior
 */
template <typename Tag>
struct ThreadPoolTraits
{
    PBB_DELETE_CTORS(ThreadPoolTraits);
    static void WorkerLoop(auto& self)
    {
        //        self.DefaultWorkerLoop();
        self.InvokeDefaultWorkerLoop();
    }

    template <typename Pool, typename Func, typename... Args>
    static auto Submit(Pool& self, Func&& func, Args&&... args, void* key)
    {
        // Just forward to DefaultSubmit, which requires noexcept.
        return self.SubmitDefault(std::forward<Func>(func), std::forward<Args>(args)..., key);
    }
};

//! ThreadPoolTraits<CustomPool>
/*! Specialization of the worker loop and submit functions to
    handle a per-thread initialization function and exception handling
 */
template <>
struct ThreadPoolTraits<Tags::CustomPool>
{
    // To keep clangd silent
    PBB_DELETE_CTORS(ThreadPoolTraits);
    /**
     * WorkerLoop
     *
     * @brief Workerloop supporting an initialization function and exception handling
     *
     * @param ThreadPool instance
     */
    static void WorkerLoop(auto& self)
    {

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif
        thread_local bool initialized = false;
        thread_local void* init_key = nullptr; // Unique key for a group of tasks
        [[maybe_unused]] thread_local std::any
          init_result; // Hold result of a potential initialization function
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
        while (!self.m_done.test(std::memory_order_acquire))
        {
            typename std::remove_reference_t<decltype(self)>::TaskPayload pTask{ nullptr, nullptr };
#ifdef PBB_USE_TBB_QUEUE
            {
                // Acquire the lock before waiting on the condition variable
                std::unique_lock lock(self.m_mutex);
                // Wait until either:
                // 1. Shutdown has been requested (m_done is set), OR
                // 2. There's work available in the queue
                //
                // Note: condition_variable::wait can return spuriously,
                self.m_condition.wait(lock,
                  [&] {
                      return self.m_done.test(std::memory_order_acquire) ||
                        !self.m_workQueue.empty();
                  });

                // Important: we must re-check m_done after waking up
                // because:
                // - We could have been woken by notify_all during shutdown
                // - A spurious wakeup may have occurred
                // - The queue could be empty even if we were notified
                if (self.m_done.test(std::memory_order_acquire))
                    break;

                // At this point we assume there's work available.
                // Try to get a task from the queue — it's possible another
                // thread beat us to it, so this may still fail.
                if (!self.m_workQueue.try_pop(pTask) || !pTask.first)
                    continue;
            }
#else
            if (!self.m_workQueue.Pop(pTask))
            {
                if (self.m_done.test(std::memory_order_acquire))
                {
                    // Facilitate that we can destroy pool
                    break;
                }
                else
                {
                    continue;
                }
            }
            if (!pTask.first)
            {
                continue;
            }
#endif
            if (init_key != pTask.second)
            {
                // Reset an earlier initialization result
                initialized = false;
                init_key = pTask.second;
                init_result.reset();
            }

            if (!initialized)
            {
                std::function<void()> initTask = nullptr;
                {
#ifdef _MSC_VER
                    // Locate the initialization function
                    std::shared_lock<std::shared_mutex> lock(self.m_initTasksMutex);
                    // Microsoft bug
                    typename decltype(self.m_initTasks)::const_iterator it{};
                    it = self.m_initTasks.find(pTask.second);
                    if (it != self.m_initTasks.end())
                    {
                        initTask = it->second;
                    }
#else
                    std::shared_lock lock(self.m_initTasksMutex);
                    if (auto it = self.m_initTasks.find(pTask.second); it != self.m_initTasks.end())
                    {
                        initTask = it->second;
                    }
#endif
                }

                if (initTask)
                {
                    // Execute the initialization function and
                    // handle any exception thrown
                    try
                    {
                        initTask();
                        initialized = true;
                    }
                    catch (...)
                    {
                        if (pTask.first)
                        {
                            pTask.first->OnInitializeFailure(std::current_exception());
                        }
                        continue; // Skip Execute
                    }
                }
            }

            // Always execute - unless initialization failed.
            pTask.first->Execute();
        }
    }

    /**
     * @brief Submit function supporting invocable throwing and a registered per-thread
     * initialization function
     *
     * @param parameter description
     * @return description
     */
    template <typename Pool, typename Func, typename... Args>
    static auto Submit(Pool& self, Func&& func, Args&&... args, void* key)
    {
        using ResultType = std::invoke_result_t<Func, Args...>;
        using Promise = std::promise<ResultType>;
        using Future = TaskFuture<ResultType>;

        auto promise = std::make_shared<Promise>();
        auto future = Future{ promise->get_future() };
        auto weak_promise = std::weak_ptr<Promise>(promise);

        auto wrapped = [func = std::forward<Func>(func), ... args = std::forward<Args>(args),
                         weak_promise]() mutable
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

        using Task = InitAwareTask<decltype(wrapped), Promise>;
        auto task = std::make_unique<Task>(std::move(wrapped), std::move(promise));
        // auto task = std::make_unique<Task>(wrapped, promise);
#ifdef PBB_USE_TBB_QUEUE
        self.m_workQueue.push({ std::move(task), key });
        {
            std::lock_guard lock(self.m_mutex);
            self.m_condition.notify_one();
        }
#else
        self.m_workQueue.Push({ std::move(task), key });
#endif

        return future;
    }
};

} // namespace PBB::Thread

/*
Great question — let's unpack this clearly and with intent. The use of
std::shared_ptr<std::promise<T>> and std::weak_ptr<std::promise<T>> is a subtle but powerful
pattern that gives us:

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

// ASAN_OPTIONS=detect_leaks=1 ./your_app
// -fsanitize=address -g
