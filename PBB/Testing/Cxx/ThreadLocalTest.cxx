#include "PBB/ThreadPool.hpp"
#include "PBB/ThreadPoolTags.hpp"
#include <catch2/catch_test_macros.hpp>

#include <PBB/ParallelFor.hpp>
#include <PBB/ThreadLocal.hpp>

#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace
{
class ExampleFunctor
{
public:
  int globalSum;
  explicit ExampleFunctor()
  {
    globalSum = 0;
    threadLocalStorage = std::make_unique<PBB::ThreadLocal<int>>();
  }

  // Initialize thread-local variables
  void Initialize()
  {
    thread_local bool initialized = false;
    T& localSum = threadLocalStorage->GetThreadLocalValue();
    if (!initialized)
    {
      localSum = 0;
      initialized = true;
    }
  }

  // Process data within the given range
  void operator()(int iStart, int iEnd)
  {
    int& localSum = threadLocalStorage->GetThreadLocalValue();
    for (int i = iStart; i < iEnd; ++i)
    {
      localSum += i; // Sum the range of numbers
    }
  }

  // Reduce the results across all threads
  void Reduce()
  {
    globalSum = 0;
    std::lock_guard<std::mutex> lock(threadLocalStorage->GetMutex()); // Lock during reduction
    const auto& registry = threadLocalStorage->GetRegistry();
    for (const auto& entry : registry)
    {
      if (entry)
      {
        globalSum += *entry;
      }
    }
  }

public:
  using T = int;
  std::unique_ptr<PBB::ThreadLocal<T>> threadLocalStorage;

  // But then ExampleFunctor should be a shared ptr.
  // std::unique_ptr<PBB::ThreadLocal<T>> threadLocalStorage;
  // But this is tricky
};

struct VectorOutputFunctor
{
  using T = std::vector<int>;
  std::unique_ptr<PBB::ThreadLocal<T>> threadLocalStorage;

  std::vector<int> allValues;
  explicit VectorOutputFunctor()
  {
    threadLocalStorage = std::make_unique<PBB::ThreadLocal<std::vector<int>>>();
  }

  // Initialize thread-local variables
  void Initialize()
  {
    thread_local bool initialized = false;
    if (!initialized)
    {
      auto& localValues = threadLocalStorage->GetThreadLocalValue();
      localValues = std::vector<int>();
    }
  }

  // Process data within the given range
  void operator()(int iStart, int iEnd)
  {
    auto& localValues = threadLocalStorage->GetThreadLocalValue();
    for (int i = iStart; i < iEnd; ++i)
    {
      localValues.push_back(i);
    }
  }

  // Reduce the results across all threads
  void Reduce()
  {
    std::lock_guard<std::mutex> lock(threadLocalStorage->GetMutex()); // Lock during reduction
    const auto& registry = threadLocalStorage->GetRegistry();
    for (const auto& entry : registry)
    {
      if (entry)
      {
        allValues.insert(allValues.begin(), entry->begin(), entry->end());
      }
    }
  }

  // But then ExampleFunctor should be a shared ptr.
  // std::unique_ptr<PBB::ThreadLocal<T>> threadLocalStorage;
  // But this is tricky
};

} // namespace

TEST_CASE("ThreadLocal_Dispatch_Using_ThreadLocal", "[ThreadLocal]")
{
  ExampleFunctor func;
  PBB::ParallelFor(0, 100, func);
  REQUIRE(func.globalSum == 4950);
}

TEST_CASE("ThreadLocal_Dispatch_Using_ThreadLocalVector", "[ThreadLocal]")
{
  VectorOutputFunctor func;
  PBB::ParallelFor(0, 100, func);
  REQUIRE(func.allValues.size() == 100);
}
