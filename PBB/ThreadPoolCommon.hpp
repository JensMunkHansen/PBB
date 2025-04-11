#pragma once

#include <any>
#include <functional>
#include <future>
#include <memory>
#include <utility>

#include <PBB/Common.hpp>
#include <PBB/Config.h>

namespace PBB::Thread
{

// InitKey now owns the functor to keep it alive
struct InitKey
{
    std::shared_ptr<void> shared;
    std::size_t version;

    bool operator==(const InitKey& rhs) const noexcept
    {
        return shared == rhs.shared && version == rhs.version;
    }

    struct Hash
    {
        std::size_t operator()(const InitKey& k) const noexcept
        {
            return std::hash<std::shared_ptr<void>>{}(k.shared) ^
              std::hash<std::size_t>{}(k.version);
        }
    };
};
}

namespace PBB::Thread
{
class IThreadTask
{
  public:
    virtual ~IThreadTask();
    virtual void Execute() = 0;
    virtual void OnInitializeFailure(std::exception_ptr eptr) noexcept
    {
        PBB_UNREFERENCED_PARAMETER(eptr);
    }

  protected:
    IThreadTask() = default;

  private:
    IThreadTask(const IThreadTask&) = delete;
    IThreadTask& operator=(const IThreadTask&) = delete;
};

template <typename Func>
requires std::is_move_constructible_v<Func>
class ThreadTask : public IThreadTask
{
  public:
    explicit ThreadTask(Func&& func);
    void Execute() final;

  private:
    Func m_func;
};

enum class FuturePolicy
{
    Wait,
    Detach
};

template <typename T>
class TaskFuture
{
  public:
    explicit TaskFuture(std::future<T>&& future, FuturePolicy policy = FuturePolicy::Wait);
    TaskFuture(TaskFuture&&) noexcept = default;
    TaskFuture& operator=(TaskFuture&&) noexcept = default;
    PBB_DELETE_COPY_CTORS(TaskFuture);

    T Get();
    void Detach();
    ~TaskFuture();

  private:
    std::future<T> m_future;
    FuturePolicy m_policy;
};

template <typename Func, typename Promise>
class InitAwareTask : public ThreadTask<Func>
{
  public:
    using Base = ThreadTask<Func>;
    PBB_DELETE_CTORS(InitAwareTask);

    explicit InitAwareTask(Func&& func, std::shared_ptr<Promise> promise);
    void OnInitializeFailure(std::exception_ptr eptr) noexcept override;

  private:
    std::shared_ptr<Promise> m_promise;
};

} // namespace PBB::Thread
#ifdef PBB_HEADER_ONLY
#include <PBB/ThreadPoolCommon.txx>
namespace PBB::Thread
{
IThreadTask::~IThreadTask() = default;
}
#endif
