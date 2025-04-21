#pragma once

#include <concepts>
#include <future>
#include <utility>

#include <PBB/ThreadPoolCommon.hpp>
namespace PBB::Thread
{

// ThreadTask implementation

template <typename Func>
requires std::is_move_constructible_v<Func> ThreadTask<Func>::ThreadTask(Func&& func)
  : m_func(std::move(func))
{
}

template <typename Func>
requires std::is_move_constructible_v<Func>
void ThreadTask<Func>::Execute()
{
    m_func();
}

// TaskFuture implementation

template <typename T>
TaskFuture<T>::TaskFuture(std::future<T>&& future, FuturePolicy policy)
  : m_future(std::move(future))
  , m_policy(policy)
{
}

template <typename T>
T TaskFuture<T>::Get()
{
    return m_future.get();
}

template <typename T>
void TaskFuture<T>::Detach()
{
    m_policy = FuturePolicy::Detach;
}

template <typename T>
TaskFuture<T>::~TaskFuture()
{
    if (m_future.valid() && m_policy == FuturePolicy::Wait)
    {
        m_future.get();
    }
}

// InitAwareTask implementation

template <typename Func, typename Promise>
InitAwareTask<Func, Promise>::InitAwareTask(Func&& func, std::shared_ptr<Promise> promise)
  : ThreadTask<Func>(std::move(func))
  , m_promise(std::move(promise))
{
}

template <typename Func, typename Promise>
void InitAwareTask<Func, Promise>::OnInitializeFailure(std::exception_ptr eptr) noexcept
{
    try
    {
        m_promise->set_exception(std::move(eptr));
    }
    catch (const std::future_error&)
    {
        // Promise already satisfied — ignore
    }
}

} // namespace PBB::Thread
