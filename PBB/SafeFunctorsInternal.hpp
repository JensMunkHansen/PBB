#pragma once

#include <PBB/Common.hpp>

#include <iterator> // For std::advance

namespace PBB
{
namespace detail
{

template <typename InputIt, typename OutputIt, typename Functor>
class UnaryTransformCall
{
  protected:
    InputIt In;
    OutputIt Out;
    Functor& Transform;

  public:
    UnaryTransformCall(InputIt _in, OutputIt _out, Functor& _transform)
      : In(_in)
      , Out(_out)
      , Transform(_transform)
    {
    }

    void Execute(size_t begin, size_t end)
    {
        InputIt itIn(In);
        OutputIt itOut(Out);
        std::advance(itIn, begin);
        std::advance(itOut, begin);
        for (size_t it = begin; it < end; it++)
        {
            *itOut = Transform(*itIn);
            ++itIn;
            ++itOut;
        }
    }
};

template <typename InputIt1, typename InputIt2, typename OutputIt, typename Functor>
class BinaryTransformCall : public UnaryTransformCall<InputIt1, OutputIt, Functor>
{
    InputIt2 In2;

  public:
    BinaryTransformCall(InputIt1 _in1, InputIt2 _in2, OutputIt _out, Functor& _transform)
      : UnaryTransformCall<InputIt1, OutputIt, Functor>(_in1, _out, _transform)
      , In2(_in2)
    {
    }

    void Execute(size_t begin, size_t end)
    {
        InputIt1 itIn1(this->In);
        InputIt2 itIn2(In2);
        OutputIt itOut(this->Out);
        std::advance(itIn1, begin);
        std::advance(itIn2, begin);
        std::advance(itOut, begin);
        for (size_t it = begin; it < end; it++)
        {
            *itOut = this->Transform(*itIn1, *itIn2);
            ++itIn1;
            ++itIn2;
            ++itOut;
        }
    }
};

template <typename T>
struct FillFunctor
{
    const T& Value;

  public:
    FillFunctor(const T& _value)
      : Value(_value)
    {
    }

    T operator()(T PBB_NOT_USED(inValue)) { return Value; }
};

} // namespace detail
} // namespace sps
