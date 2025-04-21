#pragma once

#include <atomic>
#ifdef PBB_USE_TBB_QUEUE
#include <condition_variable>
#include <functional>
#include <mutex>
#endif
#include <thread>
#include <vector>

#include <PBB/ThreadPoolCommon.hpp>
#include <PBB/ThreadPoolTags.hpp>

#ifdef PBB_USE_TBB_QUEUE
#include <tbb/concurrent_queue.h>
#else
#include <PBB/MRMWQueue.hpp>
#endif

// Declaration in .hpp, definition in .inl or .txx

// Forward declare before using as a friend
namespace PBB::Thread
{
template <typename Tag>
struct ThreadPoolTraits;
}

namespace PBB::Thread
{

template <typename Tag, typename Derived>
class ThreadPoolBase
{
  public:
    Derived& Self() { return static_cast<Derived&>(*this); }
    const Derived& Self() const { return static_cast<const Derived&>(*this); }

    // Alternatively, we can expose functions: DoneFlag, WorkQueue, Mutex and Condition to
    // be used by @ref ThreadPoolTraits
    template <typename>
    friend struct ThreadPoolTraits;

    size_t NThreadsGet() const;

  protected:
    using TaskPtr = std::unique_ptr<IThreadTask>;
    using TaskPayload = std::pair<TaskPtr, void*>;
#ifdef PBB_USE_TBB_QUEUE
    using QueueImpl = tbb::concurrent_queue<TaskPayload>;
#else
    using QueueImpl = PBB::MRMWQueue<TaskPayload>;
#endif

    ThreadPoolBase();

    explicit ThreadPoolBase(std::size_t numThreads);

    ~ThreadPoolBase();

    void Worker()
    {
        ThreadPoolTraits<Tag>::WorkerLoop(Self());
    }

    /**
     * Invalidates the queue and joins all running threads.
     */
    void Destroy();

    std::atomic_flag m_done = ATOMIC_FLAG_INIT;
    QueueImpl m_workQueue;
    std::vector<std::thread> m_threads;

#ifdef PBB_USE_TBB_QUEUE
    // Intel TBB queue is thread-safe and non-blocking, we need synchronization
    std::mutex m_mutex;
    std::condition_variable m_condition;
#endif

    /**
     * Default worker loop using explicit conditional wait when using
     * TBB queue.
     *
     * TODO: Consider adding another trait without conditionals and yielding (busy)
     */
  public:
    void DefaultWorkerLoop()
    {
        while (!m_done.test(std::memory_order_acquire))
        {
            TaskPayload pTask{ nullptr, nullptr };

#ifdef PBB_USE_TBB_QUEUE
            {
                // Acquire the lock before waiting on the condition variable
                std::unique_lock lock(m_mutex);
                // Wait until either:
                // 1. Shutdown has been requested (m_done is set), OR
                // 2. There's work available in the queue
                //
                // Note: condition_variable::wait can return spuriously,
                m_condition.wait(lock,
                  [&] { return m_done.test(std::memory_order_acquire) || !m_workQueue.empty(); });

                // Important: we must re-check m_done after waking up
                // because:
                // - We could have been woken by notify_all during shutdown
                // - A spurious wakeup may have occurred
                // - The queue could be empty even if we were notified
                if (m_done.test(std::memory_order_acquire))
                    break;

                // At this point we assume there's work available.
                // Try to get a task from the queue — it's possible another
                // thread beat us to it, so this may still fail.
                if (!m_workQueue.try_pop(pTask) || !pTask.first)
                    continue;
            }
#else
            // The MRMWQueue handles its own wait logic
            if (!m_workQueue.Pop(pTask))
            {
                if (m_done.test(std::memory_order_acquire))
                {
                    // We are done
                    break;
                }
                else
                {
                    // Spurious wakeup
                    continue;
                }
            }
            if (!pTask.first)
            {
                // Dummy task
                continue;
            }
#endif
            pTask.first->Execute();
        }
    }

    template <typename Func, typename... Args>
    requires noexcept_invocable<Func, Args...>
    auto DefaultSubmit(Func&& func, Args&&... args, void* key = nullptr) noexcept;
};

} // namespace PBB::Thread

#include <PBB/ThreadPoolBase.inl>
