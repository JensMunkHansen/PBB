#include <catch2/catch_test_macros.hpp>

#include "PBB/ThreadPool.hpp"
#include "PBB/ThreadPoolTags.hpp"
#include <PBB/ThreadLocal.hpp>

#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace
{

} // namespace

TEST_CASE("PlaceHolder", "[ThreadLocal]")
{
    REQUIRE(true);
}
