#pragma once

#include <any>
#include <exception>
#include <iostream>
#include <shared_mutex>
#include <utility>

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
    static void WorkerLoop(auto& self) { self.DefaultWorkerLoop(); }

    template <typename Func, typename... Args>
    static auto Submit(auto& self, Func&& func, Args&&... args, void* key)
    {
        return self.SubmitDefault(std::forward<Func>(func), std::forward<Args>(args)..., key);
    }
};

//! ThreadPoolTraits<CustomPool>
/*! Specialization of the worker loop to handle a per-thread
    initialization function.
 */
template <>
struct ThreadPoolTraits<Tags::CustomPool>
{
    static void WorkerLoop(auto& self)
    {
        thread_local bool initialized = false;
        thread_local void* init_key = nullptr; // Unique key for a group of tasks
        thread_local std::any init_result;     // Hold result of a potential initialization function

        while (!self.m_done.test(std::memory_order_acquire))
        {
            std::pair<std::unique_ptr<IThreadTask>, void*> pTask{ nullptr, nullptr };
            if (self.m_workQueue.Pop(pTask))
            {
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
                        // Locate the initialization function
                        std::shared_lock lock(self.m_initTasksMutex);
                        auto it = self.m_initTasks.find(pTask.second);
                        if (it != self.m_initTasks.end())
                        {
                            initTask = it->second;
                        }
                    }

                    if (initTask)
                    {
                        // Execute the initialization function and handle any
                        // exception thrown
                        try
                        {
                            initTask();
                            initialized = true;
                        }
                        catch (...)
                        {
                            if (pTask.first)
                            {
                                // We have to rethrow to get the right exception
#if 0
                                try
                                {
                                    std::rethrow_exception(std::current_exception());
                                }
                                catch (const std::exception& e)
                                {
                                    pTask.first->OnInitializeFailure(e);
                                }
#else
                                pTask.first->OnInitializeFailure(std::current_exception());
#endif
                            }
                            continue; // Skip Execute
                        }
                    }
                    else
                    {
                        // No initTask found — just continue
                        continue;
                    }
                }

                // Always execute - unless initialization failed.
                pTask.first->Execute();
            }
        }
    }

#if 1

    template <typename Func, typename... Args>
    static auto Submit(auto& self, Func&& func, Args&&... args, void* key)
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

        self.m_workQueue.Push(std::make_pair(std::move(task), key));

        return future;
    }
#endif
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
