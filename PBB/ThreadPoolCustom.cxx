#include <PBB/ThreadPoolCommon.hpp>
#include <PBB/ThreadPoolCommon.txx>

#include <PBB/ThreadPool.txx>
#include <PBB/ThreadPoolCustom.hpp>

static_assert(
  std::is_class_v<PBB::Thread::ThreadPool<PBB::Thread::Tags::CustomPool>>, "Not a class!");

namespace PBB::Thread
{

ThreadPool<Tags::CustomPool>::~ThreadPool() = default;
template class PBB_EXPORT ThreadPool<Tags::CustomPool>;
class PBB_EXPORT ThreadPoolCustom;
} // namespace PBB::Thread

/*
#include <PBB/ThreadPool.hpp>

namespace PBB::Thread {

template class ThreadPool<Tags::CustomPool>;

} // namespace PBB::Thread

// Pull in the template method definitions
#include <PBB/ThreadPool.txx>
 */
