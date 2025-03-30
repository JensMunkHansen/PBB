#pragma once

#include <type_traits>

namespace PBB
{
namespace detail
{
// Generic detector
template <typename, template <typename> class, typename = std::void_t<>>
struct is_detected : std::false_type
{
};

template <typename T, template <typename> class Op>
struct is_detected<T, Op, std::void_t<Op<T>>> : std::true_type
{
};

// Helper alias
template <typename T, template <typename> class Op>
constexpr bool is_detected_v = is_detected<T, Op>::value;
} // namespace detail
} // namespace PBB
