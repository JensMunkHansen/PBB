#pragma once

#include <any>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <PBB/Config.h>
#include <PBB/pbb_export.h>

#ifdef PBB_HEADER_ONLY
#include <PBB/MeyersSingleton.hpp>
#else
#include <PBB/PhoenixSingleton.hpp>
#endif

#include <PBB/ThreadPool.hpp>
#include <PBB/ThreadPoolBase.hpp>
#include <PBB/ThreadPoolTraits.hpp>

namespace PBB::Thread
{

#ifdef PBB_HEADER_ONLY
template <>
class ThreadPool<Tags::CustomPool>
  : public MeyersSingleton<ThreadPool<Tags::CustomPool>>
  , public ThreadPoolBase<Tags::CustomPool, ThreadPool<Tags::CustomPool>>
{
    friend class MeyersSingleton<ThreadPool<Tags::CustomPool>>;
#else
template <>
class PBB_EXPORT ThreadPool<Tags::CustomPool>
  : public PhoenixSingleton<ThreadPool<Tags::CustomPool>>
  , public ThreadPoolBase<Tags::CustomPool, ThreadPool<Tags::CustomPool>>
{
    friend class PhoenixSingleton<ThreadPool<Tags::CustomPool>>;
#endif
    template <typename>
    friend struct ThreadPoolTraits;

  protected:
    ThreadPool() = default;
    ~ThreadPool() override;

    void Destroy();

  public:
    template <typename Func, typename... Args>
    requires std::invocable<Func, Args...>
    auto Submit(Func&& func, Args&&... args, void* key)
    {
        // To please Microsoft, who cannot resolve this
        return ThreadPoolTraits<Tags::CustomPool>::Submit(
          Self(), std::forward<Func>(func), std::forward<Args>(args)..., key);
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
    // To please Microsoft, who cannot fully resolve this
    ThreadPool<Tags::CustomPool>& Self()
    {
        return *static_cast<ThreadPool<Tags::CustomPool>*>(this);
    }

    const ThreadPool<Tags::CustomPool>& Self() const
    {
        return *static_cast<const ThreadPool<Tags::CustomPool>*>(this);
    }
    std::unordered_map<void*, std::function<std::any()>> m_initTasks;
    std::shared_mutex m_initTasksMutex;
};

} // namespace PBB::Thread

#ifdef PBB_HEADER_ONLY
namespace PBB::Thread
{
inline ThreadPool<Tags::CustomPool>::~ThreadPool() = default;
}
#endif
