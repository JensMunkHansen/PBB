#pragma once

#include <PBB/Config.h>
#include <PBB/ThreadPool.hpp>
#include <PBB/pbb_export.h>

namespace PBB
{
#ifndef PBB_HEADER_ONLY
// In compiled mode, just declare it
PBB_EXPORT Thread::ThreadPool<Thread::Tags::DefaultPool>& GetThreadPoolInstance();
#endif
} // namespace PBB
