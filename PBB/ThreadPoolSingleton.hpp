#pragma once

#include <PBB/pbb_export.h>

#include <PBB/Config.h>
#include <PBB/ThreadPool.hpp>

namespace PBB
{
#ifndef PBB_HEADER_ONLY
// In compiled mode, just declare it
PBB_EXPORT Thread::ThreadPool<Thread::Tags::DefaultPool>& GetDefaultThreadPool();
#endif
} // namespace PBB
