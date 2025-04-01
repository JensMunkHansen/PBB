#include <PBB/ThreadPoolSingleton.hpp>

namespace PBB::Thread
{
template <typename Tag>
ThreadPool<Tag>& GetThreadPoolInstance()
{
    return ThreadPool<Tag>::InstanceGet();
}

ThreadPool<Tags::DefaultPool>& GetDefaultThreadPool()
{
    return GetThreadPoolInstance<Tags::DefaultPool>();
}
} // namespace PBB::Thread
