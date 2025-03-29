#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <iostream>
#include <thread>

#include <PBB/MeyersSingleton.hpp>

namespace
{

class Test : public PBB::MeyersSingleton<Test>
{
public:
private:
  float m_float;
  float* m_heapFloat;
  Test()
    : m_heapFloat(nullptr)
  {
    m_float = 2.0f;
    m_heapFloat = new float;
  }
  ~Test()
  {
    std::cout << "Destroyed\n";
    delete m_heapFloat;
    m_heapFloat = nullptr;
  }
  Test& operator=(const Test& rhs) = delete;

  friend class MeyersSingleton<Test>;
};

template <class T>
class TTest : public PBB::MeyersSingleton<TTest<T>>
{
public:
  T m_value;
  TTest() { m_value = T(1.0); }
  ~TTest() { std::cout << "Destroyed\n"; }

private:
  friend class PBB::MeyersSingleton<TTest<T>>;
};
}

TEST_CASE("MeyersSingleton_instantiation", "[MeyersSingleton]")
{
  Test& test = Test::InstanceGet();
  TTest<float>& ttest = TTest<float>::InstanceGet();

  REQUIRE(true); // placeholder
}

TEST_CASE("MeyersSingleton_thread_safety", "[MeyersSingleton][thread]")
{
  constexpr int numThreads = 2;
  std::vector<std::thread> threads;
  std::atomic<bool> success{ true };

  const void* testInstancePtr = nullptr;
  const void* ttestInstancePtr = nullptr;

  std::mutex ptrMutex;

  for (int i = 0; i < numThreads; ++i)
  {
    threads.emplace_back(
      [&]()
      {
        try
        {
          auto& test = PBB::MeyersSingleton<Test>::InstanceGet();
          auto& ttest = PBB::MeyersSingleton<TTest<float>>::InstanceGet();

          std::lock_guard<std::mutex> lock(ptrMutex);
          if (!testInstancePtr)
            testInstancePtr = &test;
          else if (testInstancePtr != &test)
            success = false;

          if (!ttestInstancePtr)
            ttestInstancePtr = &ttest;
          else if (ttestInstancePtr != &ttest)
            success = false;
        }
        catch (...)
        {
          success = false;
        }
      });
  }

  for (auto& t : threads)
    t.join();

  REQUIRE(success);
}
