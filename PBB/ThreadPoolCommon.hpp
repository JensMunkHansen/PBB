#pragma once

#include <future>
#include <memory>
#include <utility>

namespace PBB::Thread
{

//! Thread task interface
/*!
  Abstract interface for thread task.
 */
class IThreadTask
{
  public:
    virtual ~IThreadTask() = default;
    virtual void Execute() = 0;
    virtual void OnInitializeFailure(std::exception_ptr eptr) {}

  protected:
    IThreadTask() = default;

  private:
    /**
       Ensure that derived classes must be non-copyable or assignable.
     */
    IThreadTask(const IThreadTask&) = delete;
    IThreadTask& operator=(const IThreadTask&) = delete;
};

//! Thread task
/*!
 * Thread task implementation.
 *
 * Template wrapper for move-constructible callables
 */
template <typename Func>
requires std::is_move_constructible_v<Func>
class ThreadTask : public IThreadTask
{
  public:
    /**
     * Construct thread task (we don't allow implicit conversion of func)
     *
     * @param func
     *
     */
    explicit ThreadTask(Func&& func)
      : m_func(std::move(func))
    {
    }

    /**
     * Function executed by thread
     *
     */
    void Execute() final { m_func(); }

  private:
    Func m_func;
};

enum class FuturePolicy
{
    Wait,
    Detach
};

//! Task future
/*! Simple wrapper around std::future. The behavoir of futures
 * returned from std::async is added. The object will block and
 * wait for completion unless, detach is called.
 */
template <typename T>
class TaskFuture
{
  public:
    explicit TaskFuture(std::future<T>&& future, FuturePolicy policy = FuturePolicy::Wait)
      : m_future(std::move(future))
      , m_policy(policy)
    {
    }

    TaskFuture(TaskFuture&&) noexcept = default;
    TaskFuture& operator=(TaskFuture&&) noexcept = default;

    TaskFuture(const TaskFuture&) = delete;
    TaskFuture& operator=(const TaskFuture&) = delete;

    /**
     * Get the std::future to wait for. Deduced return types
     * available using c++14
     *
     * @return
     */
    T Get() { return m_future.get(); }
    void Detach() { m_policy = FuturePolicy::Detach; }

    /**
     * Destructor
     *
     */
    ~TaskFuture()
    {
        if (m_future.valid() && m_policy == FuturePolicy::Wait)
        {
            // Block if not detached
            m_future.get();
        }
    }

  private:
    std::future<T> m_future;
    FuturePolicy m_policy;
};

//! InitAwareTask
/*! Extension of @ref ThreadTask with a shared promise to track
 * potential exceptions thrown during initialization.
 */
template <typename Func, typename Promise>
class InitAwareTask : public ThreadTask<Func>
{
  public:
    using Base = ThreadTask<Func>;

    InitAwareTask(Func&& func, std::shared_ptr<Promise> promise)
      : Base(std::move(func))
      , m_promise(std::move(promise))
    {
    }
#if 0
  // Store last exception inside the promise
  void OnInitializeFailure(const std::exception& /*unused*/) override
  {
    m_promise->set_exception(std::current_exception());
  }
#endif
    // Store last exception inside the promise
    void OnInitializeFailure(std::exception_ptr eptr) override
    {
        m_promise->set_exception(std::move(eptr));
    }

  private:
    std::shared_ptr<Promise> m_promise;
};

} // namespace PBB::Thread
