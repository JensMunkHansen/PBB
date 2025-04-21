#include <PBB/ThreadPool.hpp>
#include <PBB/pbb_export.h>

namespace PBB::Thread
{
template <typename Tag>
ThreadPool<Tag>::~ThreadPool() = default;

template class PBB_EXPORT ThreadPool<Tags::DefaultPool>;
} // namespace PBB::Thread
