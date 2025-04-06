#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <iostream>
#include <thread>

#include <PBB/PhoenixSingletonRef.hpp>

namespace
{

using PBB::PhoenixSingletonRef;

struct Test
{
    int value = 42;
};

template <typename T>
struct TTest
{
    T data = T();
};

TEST_CASE("PhoenixSingletonRef_Instantiation", "[PhoenixSingletonRef]")
{
    Test& test = PhoenixSingletonRef<Test>::InstanceGet();
    REQUIRE(test.value == 42);

    TTest<float>& ttest = PhoenixSingletonRef<TTest<float>>::InstanceGet();
    REQUIRE(ttest.data == 0.0f);
}

TEST_CASE("PhoenixSingletonRef_CreateDestroy_Resurrectable", "[PhoenixSingletonRef]")
{
    using RTest = PhoenixSingletonRef<Test, true>; // resurrection allowed

    Test& test1 = RTest::InstanceGet();
    REQUIRE(test1.value == 42);

    REQUIRE(RTest::InstanceDestroy() == 0);
    REQUIRE_FALSE(RTest::IsAlive());

    Test& test2 = RTest::InstanceGet(); // allowed to resurrect
    REQUIRE(test2.value == 42);
}

TEST_CASE("PhoenixSingletonRef_CreateDestroy_NonResurrectable", "[PhoenixSingletonRef]")
{
    using NRTest = PhoenixSingletonRef<Test>; // default: resurrection NOT allowed

    Test& test1 = NRTest::InstanceGet();
    REQUIRE(test1.value == 42);

    REQUIRE(NRTest::InstanceDestroy() == 0);
    REQUIRE_FALSE(NRTest::IsAlive());

    // Try to resurrect — should fail
    Test* test2 = NRTest::InstancePtrGet();
    REQUIRE(test2 == nullptr);

    // Optional: if you want to verify reference version asserts/crashes, wrap in REQUIRE_THROWS /
    // REQUIRE_DEATH (Catch2 v3) But that’s not required here — you’re already safe.
}
}
