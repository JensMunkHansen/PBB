#include <PBB/ThreadPoolCustom.hpp>
#include <PBB/pbb_export.h>

namespace PBB::Thread
{
ThreadPool<Tags::CustomPool>::~ThreadPool() = default;
template class PBB_EXPORT ThreadPool<Tags::CustomPool>;
class PBB_EXPORT ThreadPoolCustom;

}

/*
#include <PBB/ThreadPool.hpp>

namespace PBB::Thread {

template class ThreadPool<Tags::CustomPool>;

} // namespace PBB::Thread

// Pull in the template method definitions
#include <PBB/ThreadPool.txx>
 */
