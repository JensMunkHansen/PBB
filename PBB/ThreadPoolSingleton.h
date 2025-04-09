#pragma once

#include <PBB/Config.h>

#ifndef PBB_HEADER_ONLY
#include <PBB/pbb_export.h>
#endif

#include <PBB/ThreadPool.hpp>

namespace PBB
{
#ifndef PBB_HEADER_ONLY
// In compiled mode, just declare it
PBB_EXPORT Thread::ThreadPool<Thread::Tags::DefaultPool>& GetDefaultThreadPool();
#endif
} // namespace PBB
