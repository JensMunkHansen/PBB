#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <thread>

#include <PBB/MRMWQueue.hpp>

namespace
{
void ThreadPush(void* arg)
{
  auto pQueue = static_cast<PBB::MRMWQueue<float>*>(arg);
  for (size_t i = 0; i < 100; i++)
  {
    float value = static_cast<float>(i);
    pQueue->Push(value);
  }
}

void ThreadTryPop(void* arg)
{
  auto pQueue = static_cast<PBB::MRMWQueue<float>*>(arg);
  float value = 0.0f;
  for (size_t i = 0; i < 100; i++)
  {
    pQueue->TryPop(value);
  }
}
}
TEST_CASE("MRMWQueue_PushPopCopyable_NoErrors", "[MRMWQueue]")
{
  PBB::MRMWQueue<float> queue;
  for (size_t i = 0; i < 10; i++)
  {
    queue.Push(static_cast<float>(i));
  }
  for (size_t i = 0; i < 10; i++)
  {
    float val = 0.0f;
    queue.TryPop(val);
  }
  REQUIRE(true); // placeholder
}

TEST_CASE("MRMWQueue_PushPop_NoErrors", "[MRMWQueue]")
{
  PBB::MRMWQueue<float> queue;
  std::thread first(ThreadPush, static_cast<void*>(&queue));
  std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });
  first.join();

  std::thread second(ThreadTryPop, static_cast<void*>(&queue));
  second.join();
  REQUIRE(true); // placeholder
}
