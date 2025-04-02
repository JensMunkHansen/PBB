#include <PBB/Config.h>

#ifndef PBB_HEADER_ONLY
// We need export declarations - only for shared libs
#include <PBB/ThreadPoolSingleton.hpp>
#endif

#include <PBB/MRMWQueue.hpp>

#include <iostream>

int main(int argc, char* argv[])
{
    PBB::MRMWQueue<int> queue;
    std::cout << "Header-only usage\n";

#ifndef PBB_HEADER_ONLY
    // Default pool exposed on API
    auto& Instance = PBB::GetDefaultThreadPool();
    auto future = Instance.Submit([=]() -> void { std::cout << "Hello World\n"; }, nullptr);
    future.Get();
    std::cout << "Library usage\n";
#endif
    return 0;
}
