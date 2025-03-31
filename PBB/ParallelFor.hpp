#include <PBB/DetectionTraits.hpp>
#include <PBB/ThreadPool.hpp>

#include <iostream>
#include <limits>
#include <sstream>

namespace PBB
{
namespace detail
{
using namespace PBB::Thread;
template <typename T>
using detect_initialize = decltype(std::declval<T>().Initialize());
template <typename T>
constexpr bool has_initialize_v = is_detected_v<T, detect_initialize>;

template <typename T>
using detect_reduce = decltype(std::declval<T>().Reduce());
template <typename T>
constexpr bool has_reduce_v = is_detected_v<T, detect_reduce>;

inline void PrintNestedException(const std::exception& e, int level = 0)
{
  std::ostringstream err;
  err << std::string(level * 2, ' ') << "- " << e.what() << "\n";
  std::cerr << err.str();
  try
  {
    std::rethrow_if_nested(e);
  }
  catch (const std::exception& nested)
  {
    PrintNestedException(nested, level + 1);
  }
  catch (...)
  {
    err.str("");
    err.clear();
    err << std::string((level + 1) * 2, ' ') << "- Unknown nested exception\n";
    std::cerr << err.str();
  }
}

// This will not work safely if:
// You return from ParallelFor before all threads finish (not your case)
// You ever detach threads or store the lambdas outside this scope (also not your case)

/*
 * Requires Initialize() function to be guarded by a thread_local
 * bool, since same thread may execute multiple batches.
 *
 * TODO: Add map of initialization function to threadpool using
 * address of functor and inside Worker, check if Initialize() has
 * been called and it now call it.
 */
template <typename PoolTag = Tags::DefaultPool, typename Functor>
int ParallelFor(
  int begin, int end, Functor& func, size_t nChunks = std::numeric_limits<std::size_t>::max())
{
  int err = 0;
  auto& pool = ThreadPool<PoolTag>::InstanceGet();
  const size_t num_threads = std::min(pool.NThreadsGet(), nChunks);
  std::cout << "Number of threads: " << num_threads << std::endl;
  std::vector<ThreadPool<Tags::DefaultPool>::TaskFuture<void>> futures;

  int total = end - begin;
  int chunk_size = (total + num_threads - 1) / num_threads;

  for (int i = 0; i < num_threads; ++i)
  {
    int chunk_begin = begin + i * chunk_size;
    int chunk_end = std::min(chunk_begin + chunk_size, end);

    if (chunk_begin < chunk_end)
    {
      // Compose the function outside the thread pool
      auto composed = [chunk_begin, chunk_end, &func]()
      {
        try
        {
          if constexpr (has_initialize_v<Functor>)
          {
            func.Initialize(); // per-thread init
          }
          func(chunk_begin, chunk_end);
        }
        catch (...)
        {
          std::ostringstream oss;
          oss << "Exception in worker thread " << std::this_thread::get_id()
              << " while processing range [" << chunk_begin << ", " << chunk_end << ")";
          std::throw_with_nested(std::runtime_error(oss.str()));
        }
      };

      // Submit the composed callable
      futures.emplace_back(std::move(pool.Submit(std::move(composed))));
    }
  }

  std::vector<std::exception_ptr> errors;

  std::atomic_thread_fence(std::memory_order_seq_cst);

  for (auto& fut : futures)
  {
    try
    {
      fut.Get();
    }
    catch (...)
    {
      errors.emplace_back(std::current_exception());
    }
  }
  if (!errors.empty())
  {
    err = 1;
    for (const auto& e : errors)
    {
      try
      {
        if (e)
          std::rethrow_exception(e);
      }
      catch (const std::exception& ex)
      {
        PrintNestedException(ex);
      }
    }
  }

  if (!err)
  {
    if constexpr (has_reduce_v<Functor>)
    {
      func.Reduce();
    }
  }
  return err;
}
} // namespace detail

// Expose with default pool
template <typename Functor>
int ParallelFor(
  int begin, int end, Functor&& func, size_t nThreads = std::numeric_limits<std::size_t>::max())
{
  return detail::ParallelFor<Thread::Tags::DefaultPool>(
    begin, end, std::forward<Functor>(func), nThreads);
}

} // namespace PBB
