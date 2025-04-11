#include <PBB/Config.h>

#include <PBB/Common.hpp>
#include <PBB/ThreadLocal.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>

#ifdef PBB_STD_FORMAT
#include <format>
namespace fmt_shim
{
using std::format;
}
// Only add this if GCC13+ is missing support or if you want uniform behavior:
template <>
struct std::formatter<std::thread::id, char> : std::formatter<std::string, char>
{
    auto format(const std::thread::id& id, format_context& ctx) const
    {
        std::ostringstream oss;
        oss << id;
        return std::formatter<std::string, char>::format(oss.str(), ctx);
    }
};
#elif PBB_FORMAT
#include <fmt/core.h>
#include <fmt/format.h>
namespace fmt_shim
{
using fmt::format;
}
template <>
struct fmt::formatter<std::thread::id> : fmt::formatter<std::string>
{
    auto format(const std::thread::id& id, fmt::format_context& ctx) const
    {
        std::ostringstream oss;
        oss << id;
        return fmt::formatter<std::string>::format(oss.str(), ctx);
    }
};
#else
namespace fmt_shim
{
// Overload for std::thread::id
inline std::string format(const std::string& fmt, const std::thread::id& id)
{
    if (fmt.find("{}") == std::string::npos)
        return fmt; // no placeholder? just return it

    std::ostringstream oss;
    oss << id;

    std::string out = fmt;
    return out.replace(out.find("{}"), 2, oss.str());
}

// Optional: generic fallback (e.g., for string pass-through)
template <typename T>
inline std::string format(const std::string& fmt, const T& value)
{
    if constexpr (std::is_same_v<T, std::string>)
        return fmt_format(fmt, value); // fallback to above
    else
    {
        std::ostringstream oss;
        oss << value;
        std::string out = fmt;
        return out.replace(out.find("{}"), 2, oss.str());
    }
}
}
#endif

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <thread>

#include <PBB/ParallelFor.hpp>
#ifndef PBB_HEADER_ONLY
#include <PBB/ThreadPoolCommon.txx>

#endif
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
        out << "Thread " << std::this_thread::get_id() << " processing range [" << begin << ", "
            << end << ")\n";
        std::cout << out.str();
    }

    void Reduce() { std::cout << "Reducing results\n"; }
};

struct TaskThrowingUsingOperator
{
    void operator()(int begin, int end)
    {
        std::stringstream out;
        out << "Thread " << std::this_thread::get_id() << " processing range [" << begin << ", "
            << end << ")\n";
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

// Fails using clang++16, clang++18
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
#ifndef PBB_HEADER_ONLY
    PBB::Thread::ThreadPool<PBB::Thread::Tags::CustomPool>::InstanceDestroy();
#endif
}

TEST_CASE("ParallelFor_ThreadLocalVectors_ValidResult", "[ParallelFor]")
{
    VectorOutputFunctor func;
    PBB::ParallelFor(0, 100, func);
    REQUIRE(func.allValues.size() == 100);
}

TEST_CASE("ParallelFor_AllThreadsThrowingOperator_ErrorCodeReturned", "[ParallelFor]")
{
    auto func = []
    {
        struct
        {
            void Initialize() {}

            [[noreturn]] void operator()(int begin, int end)
            {
                PBB_UNREFERENCED_PARAMETER(begin);
                PBB_UNREFERENCED_PARAMETER(end);

                throw std::runtime_error(fmt_shim::format(
                  "Runtime error in operator() [thread {}]", std::this_thread::get_id()));
            }
        } f;
        return f;
    }();
    REQUIRE(PBB::ParallelFor(0, 1000, func) == 1);
}

TEST_CASE("ParallelFor_AllThreadsThrowingOnInitialize_ErrorCodeReturned", "[ParallelFor]")
{
    auto func = []
    {
        struct
        {
            [[noreturn]] void Initialize()
            {
                throw std::runtime_error("Runtime error in Initialize");
            }

            void operator()(int i, int end)
            {
                PBB_UNREFERENCED_PARAMETER(i);
                PBB_UNREFERENCED_PARAMETER(end);
            }
        } f;
        return f;
    }();
    REQUIRE(PBB::ParallelFor(0, 1000, func) == 1);
}
