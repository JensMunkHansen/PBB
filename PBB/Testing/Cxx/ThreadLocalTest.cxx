#include <sps/threadlocal.hpp>

#include <iostream>
#include <sstream>
#include <string>

class ExampleFunctor
{
public:
  mutable int globalSum;
  explicit ExampleFunctor()
    : threadLocalStorage()
  {
  }

  // Initialize thread-local variables
  void Initialize()
  {
    T& localSum = threadLocalStorage.GetThreadLocalValue();
    localSum = 0; // Initialize to zero
    // threadLocalStorage.RegisterThreadLocalValue(); // Register the thread-local variable
  }

  // Process data within the given range
  void operator()(int iStart, int iEnd, std::ostringstream& threadOutput)
  {
    T& localSum = threadLocalStorage.GetThreadLocalValue();
    for (int i = iStart; i < iEnd; ++i)
    {
      localSum += i; // Sum the range of numbers
    }
    threadOutput << "Thread [" << std::this_thread::get_id() << "] sum: " << localSum
                 << " for range [" << iStart << ", " << iEnd << ")"
                 << std::endl; // Collecting output locally
  }

  // Reduce the results across all threads
  void Reduce() const
  {
    globalSum = 0;
    std::lock_guard<std::mutex> lock(threadLocalStorage.GetMutex()); // Lock during reduction
    const auto& registry = threadLocalStorage.GetRegistry();
    for (const auto& entry : registry)
    {
      if (entry)
      {
        globalSum += *entry;
      }
    }
    std::cout << "Global result: " << globalSum << std::endl; // Final output
  }

private:
  using T = int;
  sps::ThreadLocal<T> threadLocalStorage; // Thread-local storage for this functor
};

class ThreadedDispatcher
{
public:
  template <typename Functor>
  int Dispatch(Functor& func, int dataSize, int numThreads)
  {
    std::vector<std::thread> threads(numThreads);              // Vector to hold threads
    std::vector<std::ostringstream> threadOutputs(numThreads); // Collect thread outputs

    // Step 1 and 2: Initialize thread-local data and process the data in chunks
    int chunkSize = dataSize / numThreads;
    int remaining = dataSize % numThreads;

    for (int t = 0; t < numThreads; ++t)
    {
      threads[t] = std::thread(
        [&func, &threadOutputs, t, chunkSize, dataSize, remaining, numThreads]
        {
          func.Initialize();

          int iStart = t * chunkSize;
          int iEnd = (t + 1) * chunkSize;

          if (t == numThreads - 1)
          {
            iEnd += remaining; // Last thread takes the remaining elements
          }

          func(iStart, iEnd, threadOutputs[t]); // Process data and collect output locally
        });
    }

    // Join all threads after processing
    for (auto& thread : threads)
    {
      thread.join();
    }

    // Output all collected thread outputs in the main thread
    for (const auto& output : threadOutputs)
    {
      std::cout << output.str();
    }

    // Step 3: Perform the reduction by the main thread
    func.Reduce();
    return func.globalSum;
  }
};

int main()
{
  ExampleFunctor func;
  ThreadedDispatcher dispatcher;
  int dataSize = 100;
  int numThreads = 35; // Start with 16 threads to demonstrate scaling

  int globalSum = dispatcher.Dispatch(func, dataSize, numThreads);

  return globalSum == 4950 ? 0 : 1;
}
