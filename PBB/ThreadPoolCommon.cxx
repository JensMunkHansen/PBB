#include <PBB/ThreadPoolCommon.hpp>
#include <PBB/ThreadPoolCommon.txx>

namespace PBB::Thread
{
IThreadTask::~IThreadTask() = default;
}
namespace PBB::Thread
{
template class TaskFuture<void>;
template class ThreadTask<std::packaged_task<void()>>;
}
