#pragma once

#include <PBB/ThreadPool.hpp>

namespace PBB
{
#if defined(PBB_HEADER_ONLY)

inline Thread::ThreadPool<Thread::Tags::DefaultPool>& GetThreadPoolInstance()
{
  static Thread::ThreadPool<Thread::Tags::DefaultPool> instance;
  return instance;
}

#else

// In compiled mode, just declare it
PBB::Thread::ThreadPool<Thread::Tags::DefaultPool>& GetThreadPoolInstance();

#endif
}
