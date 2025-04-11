#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <chrono>
#include <sstream>
#include <thread>
#include <unordered_set>

#include <PBB/ThreadPool.hpp>
#include <PBB/ThreadPoolCustom.hpp>

using namespace PBB::Thread;
namespace
{
/**
 * Short task executes in 10 milliseconds
 *
 * @return
 */
int shortTask()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return 1;
}

/**
 * Medium task executes in 150 milliseconds
 *
 *
 * @return
 */
int mediumTask()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    return 1;
}

/**
 * Long task executes in 100 milliseconds
 *
 *
 * @return
 */
int longTask()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    return 1;
}

}

/**
 * Test that adding a task, while a long task is executing works. No starvation
 *
 */
TEST_CASE("ThreadPool_No_Starvation", "[ThreadPool]")
{
    using namespace std::chrono;
    // No tasks are executed
    int mediumTaskExecuted = 0;
    int longTaskExecuted = 0;

    // Pool with single thread
    auto& myPool = ThreadPool<Tags::DefaultPool>::InstanceGet();

    // Pointer to show if task has been executed
    int* pMediumTaskExecuted = &mediumTaskExecuted;
    int* pLongTaskExecuted = &longTaskExecuted;

    auto start = steady_clock::now();
    auto taskFuture0 =
      myPool.Submit([=]() noexcept -> void { *pLongTaskExecuted = longTask(); }, nullptr);
    auto taskFuture1 =
      myPool.Submit([=]() noexcept -> void { *pMediumTaskExecuted = mediumTask(); }, nullptr);

    taskFuture0.Get();
    taskFuture1.Get();
    auto end = steady_clock::now();

    auto duration = duration_cast<milliseconds>(end - start);
    REQUIRE(duration.count() >= 30);
    REQUIRE(duration.count() < 50);
    REQUIRE(longTaskExecuted == 1);
    REQUIRE(mediumTaskExecuted == 1);
}

/**
 * Test that adding a task, while a long task is executing works,
 * when tasks are detached (deferred). Both task must complete if
 * we wait long enough before tearing down threadpool.
 */
TEST_CASE("ThreadPool_No_Starvation_Detached", "[ThreadPool]")
{
    // No tasks are executed
    int shortTaskExecuted = 0;
    int longTaskExecuted = 0;

    // Pool with single thread
    auto& myPool = ThreadPool<Tags::DefaultPool>::InstanceGet();

    // Pointer to show if task has been executed
    int* pShortTaskExecuted = &shortTaskExecuted;
    int* pLongTaskExecuted = &longTaskExecuted;

    auto taskFuture0 =
      myPool.Submit([=]() noexcept -> void { *pLongTaskExecuted = longTask(); }, nullptr);
    taskFuture0.Detach();

    auto taskFuture1 =
      myPool.Submit([=]() noexcept -> void { *pShortTaskExecuted = shortTask(); }, nullptr);
    taskFuture1.Detach();

    // Wait enough time
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    REQUIRE(longTaskExecuted == 1);
    REQUIRE(shortTaskExecuted == 1);
}
