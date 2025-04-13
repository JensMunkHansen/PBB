#pragma once

#include <algorithm>  // For std::sort
#include <functional> // For std::bind

#include "PBB/pbb_export.h" // For export maccros

#include "PBB/SafeThreadPool.h" // For threadpool

#include "PBB/SMPTools.h"
#include "PBB/SMPTools.hpp"
#include "PBB/SMPToolsInternal.hpp"

namespace PBB
{
namespace detail
{

int PBB_EXPORT GetNumberOfThreadsSTDThread();

//--------------------------------------------------------------------------------
template <>
template <typename FunctorInternal>
void SMPTools<BackendType::Sequential>::For(
  spsIdType first, spsIdType last, spsIdType grain, FunctorInternal& fi)
{
    spsIdType n = last - first;
    if (!n)
    {
        return;
    }

    if (grain == 0 || grain >= n)
    {
        fi.Execute(first, last);
    }
    else
    {
        spsIdType b = first;
        while (b < last)
        {
            spsIdType e = b + grain;
            if (e > last)
            {
                e = last;
            }
            fi.Execute(b, e);
            b = e;
        }
    }
}

//--------------------------------------------------------------------------------
template <>
template <typename InputIt, typename OutputIt, typename Functor>
void SMPTools<BackendType::Sequential>::Transform(
  InputIt inBegin, InputIt inEnd, OutputIt outBegin, Functor transform)
{
    std::transform(inBegin, inEnd, outBegin, transform);
}

//--------------------------------------------------------------------------------
template <>
template <typename InputIt1, typename InputIt2, typename OutputIt, typename Functor>
void SMPTools<BackendType::Sequential>::Transform(
  InputIt1 inBegin1, InputIt1 inEnd, InputIt2 inBegin2, OutputIt outBegin, Functor transform)
{
    std::transform(inBegin1, inEnd, inBegin2, outBegin, transform);
}

//--------------------------------------------------------------------------------
template <>
template <typename Iterator, typename T>
void SMPTools<BackendType::Sequential>::Fill(Iterator begin, Iterator end, const T& value)
{
    std::fill(begin, end, value);
}

//--------------------------------------------------------------------------------
template <>
template <typename RandomAccessIterator>
void SMPTools<BackendType::Sequential>::Sort(RandomAccessIterator begin, RandomAccessIterator end)
{
    std::sort(begin, end);
}

//--------------------------------------------------------------------------------
template <>
template <typename RandomAccessIterator, typename Compare>
void SMPTools<BackendType::Sequential>::Sort(
  RandomAccessIterator begin, RandomAccessIterator end, Compare comp)
{
    std::sort(begin, end, comp);
}

//--------------------------------------------------------------------------------
template <>
PBB_EXPORT void SMPTools<BackendType::Sequential>::Initialize(int);

//--------------------------------------------------------------------------------
template <>
PBB_EXPORT int SMPTools<BackendType::Sequential>::GetEstimatedNumberOfThreads();

//--------------------------------------------------------------------------------
template <>
PBB_EXPORT bool SMPTools<BackendType::Sequential>::GetSingleThread();

//--------------------------------------------------------------------------------
template <>
template <typename FunctorInternal>
void SMPTools<BackendType::STDThread>::For(
  size_t first, size_t last, size_t grain, FunctorInternal& fi)
{
    size_t n = last - first;
    if (n <= 0)
    {
        return;
    }

    if (grain >= n || (!this->NestedActivated && SafeThreadPool::GetInstance().IsParallelScope()))
    {
        // We are in parallel scope and nesting is not allowed
        fi.Execute(first, last);
    }
    else
    {
        int threadNumber = GetNumberOfThreadsSTDThread();

        if (grain <= 0)
        {
            size_t estimateGrain = (last - first) / (threadNumber * 4);
            grain = (estimateGrain > 0) ? estimateGrain : 1;
        }

        auto proxy = SafeThreadPool::GetInstance().AllocateThreads(threadNumber);

        for (size_t from = first; from < last; from += grain)
        {
            const auto to = (std::min)(from + grain, last);
            proxy.DoJob([&fi, from, to] { fi.Execute(from, to); });
        }

        proxy.Join();
    }
}

//--------------------------------------------------------------------------------
template <>
template <typename InputIt, typename OutputIt, typename Functor>
void SMPTools<BackendType::STDThread>::Transform(
  InputIt inBegin, InputIt inEnd, OutputIt outBegin, Functor transform)
{
    auto size = std::distance(inBegin, inEnd);

    UnaryTransformCall<InputIt, OutputIt, Functor> exec(inBegin, outBegin, transform);
    this->For(0, size, 0, exec);
}

//--------------------------------------------------------------------------------
template <>
template <typename InputIt1, typename InputIt2, typename OutputIt, typename Functor>
void SMPTools<BackendType::STDThread>::Transform(
  InputIt1 inBegin1, InputIt1 inEnd, InputIt2 inBegin2, OutputIt outBegin, Functor transform)
{
    auto size = std::distance(inBegin1, inEnd);

    BinaryTransformCall<InputIt1, InputIt2, OutputIt, Functor> exec(
      inBegin1, inBegin2, outBegin, transform);
    this->For(0, size, 0, exec);
}

//--------------------------------------------------------------------------------
template <>
template <typename Iterator, typename T>
void SMPTools<BackendType::STDThread>::Fill(Iterator begin, Iterator end, const T& value)
{
    auto size = std::distance(begin, end);

    FillFunctor<T> fill(value);
    UnaryTransformCall<Iterator, Iterator, FillFunctor<T>> exec(begin, begin, fill);
    this->For(0, size, 0, exec);
}

//--------------------------------------------------------------------------------
template <>
template <typename RandomAccessIterator>
void SMPTools<BackendType::STDThread>::Sort(RandomAccessIterator begin, RandomAccessIterator end)
{
    std::sort(begin, end);
}

//--------------------------------------------------------------------------------
template <>
template <typename RandomAccessIterator, typename Compare>
void SMPTools<BackendType::STDThread>::Sort(
  RandomAccessIterator begin, RandomAccessIterator end, Compare comp)
{
    std::sort(begin, end, comp);
}

//--------------------------------------------------------------------------------
template <>
PBB_EXPORT void SMPTools<BackendType::STDThread>::Initialize(int);

//--------------------------------------------------------------------------------
template <>
PBB_EXPORT int SMPTools<BackendType::STDThread>::GetEstimatedNumberOfThreads();

//--------------------------------------------------------------------------------
template <>
PBB_EXPORT int SMPTools<BackendType::STDThread>::GetEstimatedDefaultNumberOfThreads();

//--------------------------------------------------------------------------------
template <>
PBB_EXPORT bool SMPTools<BackendType::STDThread>::GetSingleThread();

//--------------------------------------------------------------------------------
template <>
PBB_EXPORT bool SMPTools<BackendType::STDThread>::IsParallelScope();

} // namespace detail
} // namespace sps
