#include <PBB/ThreadPool.hpp>

namespace PBB::Thread
{

// Explicit instantiation for DefaultPool
template class ThreadPool<Tags::DefaultPool>;

} // namespace PBB::Thread
