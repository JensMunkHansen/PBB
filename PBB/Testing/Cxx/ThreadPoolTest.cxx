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
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    return 1;
}

struct FunctorWithInitialize
{
    void Initialize()
    {

        {
            std::lock_guard lock(m_initMutex);
            if (m_initializedThreads.insert(std::this_thread::get_id()).second)
            {
                ++nInitializeCalls;
            }
        }

        std::stringstream out;
        out << "Thread " << std::this_thread::get_id() << " calling Initialize()\n";
        std::cout << out.str();
    }

    void operator()(int begin, int end)
    {
        {
            std::lock_guard lock(m_opMutex);
            if (m_calledThreads.insert(std::this_thread::get_id()).second)
            {
                ++nOperatorCalls;
            }
        }

        std::stringstream out;
        out << "Thread " << std::this_thread::get_id() << " processing range [" << begin << ", "
            << end << ")\n";
        std::cout << out.str();
    }

    void Reduce() { std::cout << "Reducing results\n"; }
    // Number of unique threads calling initialize
    size_t UniqueInitializeCount() const { return nInitializeCalls.load(); }
    // Number of unique threads calling operator()
    size_t UniqueOperatorCallCount() const { return nOperatorCalls.load(); }

    std::unordered_set<std::thread::id> m_initializedThreads;
    std::unordered_set<std::thread::id> m_calledThreads;
    std::mutex m_initMutex;
    std::mutex m_opMutex;

    std::atomic<size_t> nInitializeCalls{ 0 };
    std::atomic<size_t> nOperatorCalls{ 0 };
};

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
    auto taskFuture0 = myPool.Submit([=]() -> void { *pLongTaskExecuted = longTask(); }, nullptr);
    auto taskFuture1 =
      myPool.Submit([=]() -> void { *pMediumTaskExecuted = mediumTask(); }, nullptr);

    taskFuture0.Get();
    taskFuture1.Get();
    auto end = steady_clock::now();

    auto duration = duration_cast<milliseconds>(end - start);
    REQUIRE(duration.count() >= 150);
    REQUIRE(duration.count() < 200);
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

    auto taskFuture0 = myPool.Submit([=]() -> void { *pLongTaskExecuted = longTask(); }, nullptr);
    taskFuture0.Detach();

    auto taskFuture1 = myPool.Submit([=]() -> void { *pShortTaskExecuted = shortTask(); }, nullptr);
    taskFuture1.Detach();

    // Wait enough time
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    REQUIRE(longTaskExecuted == 1);
    REQUIRE(shortTaskExecuted == 1);
}

TEST_CASE("ThreadPool_With_Initialize", "[ThreadPool]")
{
    FunctorWithInitialize func;
    // Just a unique-ish key for initialization
    void* call_key = static_cast<void*>(&func);

    auto& pool = ThreadPool<Tags::CustomPool>::InstanceGet();
    const size_t nThreads = pool.NThreadsGet();

    // Register an Initialize function using a unique-ish key
    pool.RegisterInitialize(call_key, [&func] { func.Initialize(); });

    std::vector<PBB::Thread::TaskFuture<void>> futures;

    // Should be enough to all threads execute
    const size_t nTasks = 2 * nThreads;

    // Start many tasks
    for (size_t iTask = 0; iTask < nTasks; iTask++)
    {
        int chunk_begin = iTask * 5;
        int chunk_end = (iTask + 1) * 5;
        auto fut = pool.Submit(
          [chunk_begin, chunk_end, &func]() -> void { func(chunk_begin, chunk_end); }, call_key);
        futures.emplace_back(std::move(fut));
    }

    // Get the results
    for (auto& fut : futures)
    {
        fut.Get();
    }

    REQUIRE(func.UniqueInitializeCount() == func.UniqueOperatorCallCount());
    pool.RemoveInitialize(call_key);
}

TEST_CASE("Promise_Throws_When_SetException_Called", "[SanityCheck]")
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    promise.set_exception(std::make_exception_ptr(std::runtime_error("test fail")));

    bool caught = false;
    try
    {
        future.get();
    }
    catch (const std::exception& e)
    {
        caught = true;
        REQUIRE(std::string(e.what()) == "test fail");
    }

    REQUIRE(caught);
}

TEST_CASE("ThreadPool_InitializeExceptionIsPropagated", "[ThreadPool]")
{
    struct Dummy
    {
    };
    Dummy dummy;
    void* call_key = static_cast<void*>(&dummy);

    auto& pool = ThreadPool<Tags::CustomPool>::InstanceGet();

    pool.RegisterInitialize(call_key,
      []
      {
          std::cerr << "[Test] Throwing from Initialize()\n";
          throw std::runtime_error("Initialization failed!");
      });

    auto future = pool.Submit([] { FAIL("Task should not run when init fails"); }, call_key);

    future.Detach();

    bool got_expected_exception = false;
    bool correct_type = false;

    try
    {
        future.Get();
        FAIL("Expected exception from Get(), but none was thrown");
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Test] Caught exception: " << typeid(e).name() << ", what(): " << e.what()
                  << "\n";

        // Check that it is a std::runtime_error exactly
        correct_type = dynamic_cast<const std::runtime_error*>(&e) != nullptr;
        got_expected_exception = std::string(e.what()) == "Initialization failed!";
    }
    catch (...)
    {
        FAIL("Caught unexpected exception type");
    }

    REQUIRE(got_expected_exception);
    REQUIRE(correct_type);
    pool.RemoveInitialize(call_key);
}
