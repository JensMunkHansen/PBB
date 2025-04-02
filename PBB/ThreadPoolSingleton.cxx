#include <PBB/pbb_export.h>

#include <PBB/ThreadPoolSingleton.h>

namespace PBB::Thread
{
template <typename Tag>
ThreadPool<Tag>& GetThreadPoolInstance()
{
    return ThreadPool<Tag>::InstanceGet();
}
} // namespace PBB::Thread

namespace PBB
{
PBB_EXPORT Thread::ThreadPool<Thread::Tags::DefaultPool>& GetDefaultThreadPool()
{
    return Thread::GetThreadPoolInstance<Thread::Tags::DefaultPool>();
}
}

template class PBB_EXPORT PBB::Thread::ThreadPool<PBB::Thread::Tags::DefaultPool>;
