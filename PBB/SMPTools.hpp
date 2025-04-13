#pragma once

#include "PBB/pbb_export.h"

#include "PBB/Common.hpp"
#include "PBB/spsSMP.h"

#include <atomic>

#define SPS_SMP_MAX_BACKENDS_NB 2

#define SPS_SMP_BACKEND_SEQUENTIAL 0
#define SPS_SMP_BACKEND_STDTHREAD 1

namespace PBB
{
namespace detail
{
enum class BackendType
{
    Sequential = SPS_SMP_BACKEND_SEQUENTIAL,
    STDThread = SPS_SMP_BACKEND_STDTHREAD,
};

#if SPS_SMP_DEFAULT_IMPLEMENTATION_SEQUENTIAL
const BackendType DefaultBackend = BackendType::Sequential;
#elif SPS_SMP_DEFAULT_IMPLEMENTATION_STDTHREAD
const BackendType DefaultBackend = BackendType::STDThread;
#endif

template <BackendType Backend>
class SMPTools
{
  public:
    //--------------------------------------------------------------------------------
    void Initialize(int numThreads = 0);

    //--------------------------------------------------------------------------------
    int GetEstimatedNumberOfThreads();

    //--------------------------------------------------------------------------------
    int GetEstimatedDefaultNumberOfThreads();

    //--------------------------------------------------------------------------------
    void SetNestedParallelism(bool isNested);

    //--------------------------------------------------------------------------------
    bool GetNestedParallelism();

    //--------------------------------------------------------------------------------
    bool IsParallelScope();

    //--------------------------------------------------------------------------------
    bool GetSingleThread();

    //--------------------------------------------------------------------------------
    template <typename FunctorInternal>
    void For(size_t first, size_t last, size_t grain, FunctorInternal& fi);

    //--------------------------------------------------------------------------------
    template <typename InputIt, typename OutputIt, typename Functor>
    void Transform(InputIt inBegin, InputIt inEnd, OutputIt outBegin, Functor transform);

    //--------------------------------------------------------------------------------
    template <typename InputIt1, typename InputIt2, typename OutputIt, typename Functor>
    void Transform(
      InputIt1 inBegin1, InputIt1 inEnd, InputIt2 inBegin2, OutputIt outBegin, Functor transform);

    //--------------------------------------------------------------------------------
    template <typename Iterator, typename T>
    void Fill(Iterator begin, Iterator end, const T& value);

    //--------------------------------------------------------------------------------
    template <typename RandomAccessIterator>
    void Sort(RandomAccessIterator begin, RandomAccessIterator end);

    //--------------------------------------------------------------------------------
    template <typename RandomAccessIterator, typename Compare>
    void Sort(RandomAccessIterator begin, RandomAccessIterator end, Compare comp);

    //--------------------------------------------------------------------------------
    SMPTools();

    //--------------------------------------------------------------------------------
    SMPTools(const SMPTools& other);

    //--------------------------------------------------------------------------------
    void operator=(const SMPTools& other);

  private:
    bool NestedActivated = false;
    std::atomic<bool> IsParallel{ false };
};

template <BackendType Backend>
void SMPTools<Backend>::SetNestedParallelism(bool isNested)
{
    this->NestedActivated = isNested;
}

template <BackendType Backend>
bool SMPTools<Backend>::GetNestedParallelism()
{
    return this->NestedActivated;
}

template <BackendType Backend>
bool SMPTools<Backend>::IsParallelScope()
{
    return this->IsParallel;
}

template <BackendType Backend>
SMPTools<Backend>::SMPTools()
  : NestedActivated(true)
  , IsParallel(false)
{
}

template <BackendType Backend>
SMPTools<Backend>::SMPTools(const SMPTools& other)
  : NestedActivated(other.NestedActivated)
  , IsParallel(other.IsParallel.load())
{
}

template <BackendType Backend>
void SMPTools<Backend>::operator=(const SMPTools& other)
{
    this->NestedActivated = other.NestedActivated;
    this->IsParallel = other.IsParallel.load();
}

using ExecuteFunctorPtrType = void (*)(void*, size_t, size_t, size_t);

} // namespace detail
} // namespace PBB
