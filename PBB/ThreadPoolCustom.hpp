// In ThreadPoolCustomSpecializations.hpp
#pragma once

#include <PBB/MeyersSingleton.hpp>
#include <PBB/ThreadPool.hpp>
#include <PBB/ThreadPoolBase.hpp>
#include <PBB/ThreadPoolTraits.hpp>

#include <any>
#include <shared_mutex>

namespace PBB::Thread
{

template <>
class ThreadPool<Tags::CustomPool>
  : public MeyersSingleton<ThreadPool<Tags::CustomPool>>
  , public ThreadPoolBase<Tags::CustomPool>
{
  friend class MeyersSingleton<ThreadPool<Tags::CustomPool>>;

  template <typename>
  friend struct ThreadPoolTraits;

protected:
  ThreadPool() {}
  ~ThreadPool() override = default;

  void Destroy();

public:
  void Worker() { ThreadPoolTraits<Tags::CustomPool>::WorkerLoop(*this); }

  template <typename Func, typename... Args>
  requires std::invocable<Func, Args...>
  auto Submit(Func&& func, Args&&... args, void* key)
  {
    return ThreadPoolTraits<Tags::CustomPool>::Submit(
      *this, std::forward<Func>(func), std::forward<Args>(args)..., key);
  }

  template <typename Func, typename... Args>
  auto SubmitDefault(Func&& func, Args&&... args)
  {
    return this->template DefaultSubmit(
      std::forward<Func>(func), std::forward<Args>(args)..., nullptr);
  }

  template <typename Func, typename... Args>
  void RegisterInitialize(void* key, Func&& func, Args&&... args)
  {
    using ResultType = std::invoke_result_t<Func, Args...>;
    auto boundTask = [f = std::forward<Func>(func),
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
    {
      std::unique_lock lock(m_initTasksMutex);
      m_initTasks[key] = std::move(
        [boundTask = std::move(boundTask)]() mutable -> std::any
        {
          if constexpr (std::is_void_v<std::invoke_result_t<decltype(boundTask)>>)
          {
            boundTask();
            return std::any();
          }
          else
          {
            return boundTask();
          }
        });
    }
  }
  void RemoveInitialize(void* key)
  {
    std::unique_lock lock(m_initTasksMutex);
    m_initTasks.erase(key);
  }

private:
  std::unordered_map<void*, std::function<std::any()>> m_initTasks;
  std::shared_mutex m_initTasksMutex;
};

} // namespace PBB::Thread
