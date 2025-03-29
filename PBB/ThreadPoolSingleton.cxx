#include "ThreadPoolSingleton.hpp"

namespace PBB::Thread
{
template <typename Tag>
ThreadPool<Tag>& GetThreadPoolInstance()
{
  static ThreadPool<Tag> instance;
  return instance;
}

ThreadPool<Tags::DefaultPool>& GetDefaultThreadPool()
{
  return GetThreadPoolInstance<Tags::DefaultPool>();
}
}
