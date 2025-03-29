#pragma once

namespace PBB::Thread
{

template <typename Tag>
template <typename Func, typename... Args>
requires std::invocable<Func, Args...>
auto ThreadPool<Tag>::submit(Func&& func, Args&&... args)
{
    using ResultType = std::invoke_result_t<Func, Args...>;
    using Task = std::packaged_task<ResultType()>;
    using TaskPtr = std::unique_ptr<IThreadTask>;

    auto bound = [f = std::forward<Func>(func), ... a = std::forward<Args>(args)]() mutable -> ResultType {
        if constexpr (std::is_void_v<ResultType>)
            std::invoke(std::move(f), std::move(a)...);
        else
            return std::invoke(std::move(f), std::move(a)...);
    };

    Task task(std::move(bound));
    TaskFuture<ResultType> result(task.get_future());

    {
        std::lock_guard lock(m_mutex);
        m_queue.emplace_back(std::make_unique<ThreadTask<Task>>(std::move(task)));
    }

    m_condition.notify_one();
    return result;
}

} // namespace PBB::Thread

#if PBB_HEADER_ONLY
#include "ThreadPool.inl"
#endif