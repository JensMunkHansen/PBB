#pragma once

#include <future>
#include <memory>
#include <utility>

namespace PBB::Thread
{

// Base class for all thread pool tasks
class IThreadTask
{
public:
  virtual ~IThreadTask() = default;
  virtual void Execute() = 0;

protected:
  IThreadTask() = default;

private:
  IThreadTask(const IThreadTask&) = delete;
  IThreadTask& operator=(const IThreadTask&) = delete;
};

// Template wrapper for move-constructible callables
template <typename Func>
requires std::is_move_constructible_v<Func>
class ThreadTask : public IThreadTask
{
public:
  explicit ThreadTask(Func&& func)
    : m_func(std::move(func))
  {
  }

  void Execute() override final { m_func(); }

private:
  Func m_func;
};

enum class FuturePolicy
{
  Wait,
  Detach
};

// TaskFuture wrapper for fire-and-forget or normal usage
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

  T Get() { return m_future.get(); }
  void Detach() { m_policy = FuturePolicy::Detach; }

  ~TaskFuture()
  {
    if (m_future.valid() && m_policy == FuturePolicy::Wait)
    {
      m_future.get();
    }
  }

private:
  std::future<T> m_future;
  FuturePolicy m_policy;
};

} // namespace PBB::Thread
