#pragma once

namespace PBB::Thread
{
template <typename Tag>
template <typename Func, typename... Args>
requires std::invocable<Func, Args...>
auto ThreadPool<Tag>::Submit(Func&& func, Args&&... args)
{
  using ResultType = std::invoke_result_t<Func, Args...>;
  using PackagedTask = std::packaged_task<ResultType()>;
  using TaskType = ThreadTask<PackagedTask>;

  auto boundLambda = [f = std::forward<Func>(func),
                       ... a = std::forward<Args>(args)]() mutable -> ResultType
  {
    if constexpr (std::is_void_v<ResultType>)
    {
      std::invoke(std::move(f), std::move(a)...);
    }
    else
    {
      return std::invoke(std::move(f), std::move(a)...);
    }
  };

  PackagedTask task{ std::move(boundLambda) };
  TaskFuture<ResultType> result{ task.get_future() };

#ifdef PBB_USE_TBB_QUEUE
  m_workQueue.push(std::make_unique<TaskType>(std::move(task)));
  {
    std::lock_guard lock(m_mutex);
    m_condition.notify_one();
  }
#else
  m_workQueue.Push(std::make_unique<TaskType>(std::move(task)));
#endif
  return result;
}

} // namespace PBB::Thread
