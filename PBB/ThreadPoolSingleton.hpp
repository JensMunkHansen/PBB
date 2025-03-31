#pragma once

#include <PBB/Config.h>
#include <PBB/ThreadPool.hpp>

namespace PBB
{
#ifndef PBB_HEADER_ONLY
// In compiled mode, just declare it
Thread::ThreadPool<Thread::Tags::DefaultPool>& GetThreadPoolInstance();
#endif
}
