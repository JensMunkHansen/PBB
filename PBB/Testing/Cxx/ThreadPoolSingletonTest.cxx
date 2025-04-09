#include <catch2/catch_test_macros.hpp>

#include <PBB/ThreadPoolSingleton.h>

#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace
{

} // namespace

TEST_CASE("InstantiateAndUse", "[ThreadPoolSingleton]")
{
    auto& Instance = PBB::GetDefaultThreadPool();
    auto future = Instance.Submit([=]() noexcept -> void { /* Do nothing */ }, nullptr);
    future.Get();
    REQUIRE(true);
}
