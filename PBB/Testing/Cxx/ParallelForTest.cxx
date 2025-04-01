#include "PBB/ThreadLocal.hpp"
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>

#include <PBB/ParallelFor.hpp>

namespace
{
struct TaskReporting
{
  void Initialize()
  {
    std::stringstream out;
    out << "Thread " << std::this_thread::get_id() << " initializing\n";
    std::cout << out.str();
  }

  void operator()(int begin, int end)
  {
    std::stringstream out;
    out << "Thread " << std::this_thread::get_id() << " processing range [" << begin << ", " << end
        << ")\n";
    std::cout << out.str();
  }

  void Reduce() { std::cout << "Reducing results\n"; }
};

struct TaskThrowingUsingOperator
{
  void operator()(int begin, int end)
  {
    std::stringstream out;
    out << "Thread " << std::this_thread::get_id() << " processing range [" << begin << ", " << end
        << ")\n";
    std::cout << out.str();
    if ((begin <= 50) && (50 < end))
    {
      throw std::runtime_error("Invalid index");
    }
  }

  void Reduce() { std::cout << "Reducing results\n"; }
};

void ParallelForInThread()
{
  TaskReporting task;
  PBB::ParallelFor(0, 100, task);
  std::cout << "ParallelFor in thread completed.\n";
}

class PartialSummationFunctor
{
public:
  int globalSum;
  std::unique_ptr<PBB::ThreadLocal<int>> threadLocalStorage;

  explicit PartialSummationFunctor()
  {
    globalSum = 0;
    threadLocalStorage = std::make_unique<PBB::ThreadLocal<int>>();
  }

  // Initialize thread-local variables
  void Initialize()
  {
    int& localSum = threadLocalStorage->GetThreadLocalValue();
    localSum = 0;
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

  // But then PartialSummationFunctor should be a shared ptr.
  // std::unique_ptr<PBB::ThreadLocal<T>> threadLocalStorage;
  // But this is tricky
};

} // namespace

TEST_CASE("ParallelFor_TwoInvocations_ResultsCorrect", "[ParallelFor]")
{
  std::thread t1(ParallelForInThread);
  std::thread t2(ParallelForInThread);

  t1.join();
  t2.join();
}

TEST_CASE("ParallelFor_ThreadThrowingOperator_ErrorCodeReturned", "[ParallelFor]")
{
  TaskThrowingUsingOperator task;
  REQUIRE(PBB::ParallelFor(0, 100, task) == 1);
}

TEST_CASE("ParallelFor_PartialSummationUsingThreadLocal_ValidResult", "[ParallelFor]")
{
  PartialSummationFunctor func;
  PBB::ParallelFor(0, 100, func);
  REQUIRE(func.globalSum == 4950);
}

TEST_CASE("ParallelFor_ThreadLocalVectors_ValidResult", "[ParallelFor]")
{
  VectorOutputFunctor func;
  PBB::ParallelFor(0, 100, func);
  REQUIRE(func.allValues.size() == 100);
}
