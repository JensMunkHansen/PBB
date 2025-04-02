#include <PBB/Config.h>

#ifndef PBB_HEADER_ONLY
// We need export declarations - only for shared libs
#include <PBB/pbb_export.h>
#endif

#include <PBB/MRMWQueue.hpp>

int main(int argc, char* argv[])
{
    PBB::MRMWQueue<int> queue;
    return 0;
}
