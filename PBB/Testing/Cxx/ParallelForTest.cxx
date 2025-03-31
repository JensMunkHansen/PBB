#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <iostream>
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

struct TaskThrowing
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
} // namespace

TEST_CASE("ParallelFor_Two_Threads_Throwing", "[ParallelFor]")
{
  std::thread t1(ParallelForInThread);
  std::thread t2(ParallelForInThread);

  t1.join();
  t2.join();
}

TEST_CASE("ParallelFor_Thread_Throwing", "[ParallelFor]")
{
  TaskThrowing task;
  PBB::ParallelFor(0, 100, task);
}
