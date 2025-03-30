#include "PBB/ThreadPoolTags.hpp"
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <thread>

#include <PBB/ThreadPool.hpp>

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
 * Long task executes in 100 milliseconds
 *
 *
 * @return
 */
int longTask()
{
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  return 1;
}
}

/**
 * Test that adding a task, while a long task is executing works. No starvation
 *
 */
TEST_CASE("ThreadPool_No_Starvation", "[ThreadPool]")
{
  // No tasks are executed
  int shortTaskExecuted = 0;
  int longTaskExecuted = 0;

  // Pool with single thread
  auto& myPool = ThreadPool<Tags::DefaultPool>::InstanceGet();

  // Pointer to show if task has been executed
  int* pShortTaskExecuted = &shortTaskExecuted;
  int* pLongTaskExecuted = &longTaskExecuted;

  auto taskFuture0 = myPool.Submit([=]() -> void { *pLongTaskExecuted = longTask(); });
  auto taskFuture1 = myPool.Submit([=]() -> void { *pShortTaskExecuted = shortTask(); });

  taskFuture0.Get();
  taskFuture1.Get();

  REQUIRE(longTaskExecuted == 1);
  REQUIRE(shortTaskExecuted == 1);
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

  auto taskFuture0 = myPool.Submit([=]() -> void { *pLongTaskExecuted = longTask(); });
  taskFuture0.Detach();

  auto taskFuture1 = myPool.Submit([=]() -> void { *pShortTaskExecuted = shortTask(); });
  taskFuture1.Detach();

  // Wait enough time
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  REQUIRE(longTaskExecuted == 1);
  REQUIRE(shortTaskExecuted == 1);
}
