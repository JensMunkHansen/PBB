// In ThreadPoolCustomSpecializations.hpp
#pragma once

#include <PBB/MeyersSingleton.hpp>
#include <PBB/ThreadPool.hpp>
#include <PBB/ThreadPoolBase.hpp>
#include <PBB/ThreadPoolTraits.hpp>

namespace PBB::Thread
{

template <>
class ThreadPool<Tags::CustomPool>
  : public MeyersSingleton<ThreadPool<Tags::CustomPool>>
  , public ThreadPoolBase<Tags::CustomPool>
{
  friend class MeyersSingleton<ThreadPool<Tags::CustomPool>>;

protected:
  ThreadPool() {}
  ~ThreadPool() override = default;

  void Destroy();

public:
  size_t NThreadsGet() const;

  void Worker() { ThreadPoolTraits<Tags::CustomPool>::WorkerLoop(*this); }

  template <typename Func, typename... Args>
  requires std::invocable<Func, Args...>
  auto Submit(Func&& func, Args&&... args)
  {
    return ThreadPoolTraits<Tags::CustomPool>::Submit(
      *this, std::forward<Func>(func), std::forward<Args>(args)...);
  }

  template <typename Func, typename... Args>
  auto SubmitDefault(Func&& func, Args&&... args)
  {
    return this->template DefaultSubmit(std::forward<Func>(func), std::forward<Args>(args)...);
  }

  // Expose new custom members
  void EnableLogging(bool enable) { m_loggingEnabled = enable; }
  bool LoggingEnabled() const { return m_loggingEnabled; }

private:
  std::atomic<bool> m_loggingEnabled{ true };
};

} // namespace PBB::Thread
