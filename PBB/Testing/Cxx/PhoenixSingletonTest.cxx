#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <iostream>
#include <thread>

#include <PBB/PhoenixSingleton.hpp>

namespace
{

class Test : public PBB::PhoenixSingleton<Test>
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

    friend class PhoenixSingleton<Test>;
};

template <class T>
class TTest : public PBB::PhoenixSingleton<TTest<T>>
{
  public:
    T m_value;
    TTest() { m_value = T(1.0); }
    ~TTest() { std::cout << "Destroyed\n"; }

  private:
    friend class PBB::PhoenixSingleton<TTest<T>>;
};
}

TEST_CASE("PhoenixSingleton_Instantiation", "[PhoenixSingleton]")
{
    Test* test = Test::InstancePtrGet();
    TTest<float>* ttest = TTest<float>::InstancePtrGet();

    REQUIRE(true); // placeholder
}

TEST_CASE("PhoenixSingleton_CreateAndDestroy", "[PhoenixSingleton]")
{
    Test* test = Test::InstancePtrGet();
    test->InstanceDestroy();

    REQUIRE(true); // placeholder
}
