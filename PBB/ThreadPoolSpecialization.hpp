// CustomPoolSpecialization.hpp

#pragma once

namespace PBB::Thread
{

template <>
class ThreadPool<Tags::CustomPool>
  : public MeyersSingleton<ThreadPool<Tags::CustomPool>>
  , public ThreadPoolBase<Tags::CustomPool>
{
  friend class MeyersSingleton<ThreadPool<Tags::CustomPool>>;

protected:
  ThreadPool() = default;
  ~ThreadPool() override = default;

public:
  template <typename Func, typename... Args>
  auto Submit(Func&& func, Args&&... args)
  {
    // You have full access to custom members here
    return this->DefaultSubmit(std::forward<Func>(func), std::forward<Args>(args)...);
  }

  void Worker()
  {
    // Custom worker that uses additional member data
    while (!m_done.test())
    {
      std::unique_ptr<IThreadTask> task;
      if (m_workQueue.try_pop(task))
      {
        ++m_customCounter;
        task->Execute();
      }
    }
  }

private:
  std::atomic<int> m_customCounter = 0;
};

} // namespace PBB::Thread
