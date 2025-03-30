#pragma once

#include "ThreadPoolBase.hpp"
#include "ThreadPoolTraits.hpp"

namespace PBB::Thread
{

template <typename Tag>
class ThreadPool
  : public MeyersSingleton<ThreadPool<Tag>>
  , public ThreadPoolBase<Tag>
{
  friend class MeyersSingleton<ThreadPool<Tag>>;

protected:
  ThreadPool() = default;
  ~ThreadPool() override = default;

public:
  using IThreadTask = typename ThreadPool<Tag>::IThreadTask;

  template <typename Func, typename... Args>
  requires std::invocable<Func, Args...>
  auto Submit(Func&& func, Args&&... args)
  {
    return ThreadPoolTraits<Tag>::Submit(
      *this, std::forward<Func>(func), std::forward<Args>(args)...);
  }

  void Worker() { ThreadPoolTraits<Tag>::WorkerLoop(*this); }

  using ThreadPoolBase<Tag>::DefaultWorkerLoop;
  using ThreadPoolBase<Tag>::DefaultSubmit;

  // TaskFuture and inner ThreadTask stay the same
};

} // namespace PBB::Thread
