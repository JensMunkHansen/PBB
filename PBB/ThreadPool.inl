#pragma once

namespace PBB::Thread {

template <typename Tag>
template <typename Func, typename... Args>
auto ThreadPool<Tag>::SubmitDefault(Func&& func, Args&&... args, void* key)
{
  return this->DefaultSubmit(std::forward<Func>(func), std::forward<Args>(args)...);
}

template <typename Tag>
template <typename Func, typename... Args>
auto ThreadPoolBase<Tag>::DefaultSubmit(Func&& func, Args&&... args, void* key)
{
  static_assert(std::invocable<Func, Args...>, "Submitted task must be invocable with given args");

  using ResultType = std::invoke_result_t<Func, Args...>;
  using PackagedTask = std::packaged_task<ResultType()>;
  using TaskType = ThreadTask<PackagedTask>;

  // Lambda are faster than using std::bind (but code more ugly)
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
  m_workQueue.push(std::make_pair(std::make_unique<TaskType>(std::move(task)), key));
  {
    std::lock_guard lock(m_mutex);
    m_condition.notify_one();
  }
#else
  m_workQueue.Push(std::make_pair(std::make_unique<TaskType>(std::move(task)), key));
#endif
  return result;
}

} // namespace PBB::Thread

