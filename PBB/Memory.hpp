/**
 * @file   Memory.hpp
 * @author Jens Munk Hansen <jens.munk.hansen@gmail.com>
 * @date   Tue Mar 11 07:17:29 PM CET 2025
 *
 * @brief
 *
 * Copyright 2025 Jens Munk Hansen
 *
 */
#pragma once

#include <bit>
#include <cstddef>     // std::byte
#include <memory>      // std::destroy_at, std::launder
#include <new>         // Placement new
#include <type_traits> // std::aligned_storage_t
#include <utility>     // std::forward

#if __cplusplus < 201703L
#error "This header requires at least C++17"
#endif

namespace PBB
{
// Cache for cache friendly storage
constexpr size_t CACHE_LINE_SIZE = 64;

// Explicit padding approach - expensive due to explicit padding
template <typename T>
struct alignas(CACHE_LINE_SIZE) CacheAlignedStorage
{
  T value;
  // Ensure full padding to avoid warnings
  char padding[CACHE_LINE_SIZE - (sizeof(T) % CACHE_LINE_SIZE)] = {};
};

// ----------------- C++17 Version (Uses std::aligned_storage_t) -----------------
namespace detail::v17
{
template <typename T>
struct alignas(CACHE_LINE_SIZE) CacheAlignedPlacement
{
  static constexpr size_t TotalSize =
    ((sizeof(T) + CACHE_LINE_SIZE - 1) / CACHE_LINE_SIZE) * CACHE_LINE_SIZE;

  alignas(CACHE_LINE_SIZE) std::aligned_storage_t<TotalSize, CACHE_LINE_SIZE> storage;

  template <typename... Args>
  explicit CacheAlignedPlacement(Args&&... args)
  {
    new (&storage) T(std::forward<Args>(args)...);
  }

  ~CacheAlignedPlacement() { get().~T(); }

  // Ensure that the compiler does not incorrectly optimize accesses
  // to the object due to strict aliasing rules.
  T& get() { return *std::launder(reinterpret_cast<T*>(&storage)); }
  const T& get() const { return *std::launder(reinterpret_cast<const T*>(&storage)); }
};
} // namespace detail::v17

// --C++20 Version (Uses std::byte for allocation, std::aligned_storage_t deprecated in C++23) --
#if __cplusplus >= 202002L
namespace detail::v20
{
template <typename T, bool IsNoThrow = std::is_nothrow_constructible_v<T>>
struct alignas(CACHE_LINE_SIZE) CacheAlignedPlacement;

template <typename T>
struct alignas(CACHE_LINE_SIZE) CacheAlignedPlacement<T, true>
{
private:
  static constexpr size_t TotalSize =
    ((sizeof(T) + CACHE_LINE_SIZE - 1) / CACHE_LINE_SIZE) * CACHE_LINE_SIZE;

  struct Data
  {
    std::byte storage[TotalSize];
  };

  static_assert(sizeof(Data) == TotalSize, "Padding issue detected!");

  Data data;

public:
  CacheAlignedPlacement() { construct(); }

  template <typename... Args>
  explicit CacheAlignedPlacement(Args&&... args)
  {
    construct(std::forward<Args>(args)...);
  }

  ~CacheAlignedPlacement() { destroy(); }

  template <typename... Args>
  void construct(Args&&... args)
  {
    new (data.storage) T(std::forward<Args>(args)...);
  }

  void destroy() { std::destroy_at(std::launder(reinterpret_cast<T*>(data.storage))); }

  T& get() { return *std::launder(reinterpret_cast<T*>(data.storage)); }
  const T& get() const { return *std::launder(reinterpret_cast<const T*>(data.storage)); }
};

// Specialization when `T` **is NOT** `nothrow_constructible` (Includes `std::byte initialized`)
template <typename T>
struct alignas(CACHE_LINE_SIZE) CacheAlignedPlacement<T, false>
{
private:
  static constexpr size_t TotalSize =
    ((sizeof(T) + 1 + CACHE_LINE_SIZE - 1) / CACHE_LINE_SIZE) * CACHE_LINE_SIZE;

  // We have to use std::byte to not infer an extra CACHE_LINE_SIZE alignment
  struct Data
  {
    std::byte storage[TotalSize - 1]; // Use all but the last byte
    std::byte initialized;
  };

  static_assert(sizeof(Data) == TotalSize, "Padding issue detected!");

  Data data;

public:
  CacheAlignedPlacement() { construct(); }

  template <typename... Args>
  explicit CacheAlignedPlacement(Args&&... args)
  {
    construct(std::forward<Args>(args)...);
  }

  ~CacheAlignedPlacement() { destroy(); }

  template <typename... Args>
  void construct(Args&&... args)
  {
    if (data.initialized == std::byte{ 0 })
    {
      new (data.storage) T(std::forward<Args>(args)...);
      data.initialized = std::byte{ 1 };
    }
  }

  void destroy()
  {
    // if (__builtin_expect(data.initialized == std::byte{1}, 1)) {
    bool is_initialized = std::bit_cast<bool>(data.initialized);
    if (is_initialized)
    {
      std::destroy_at(std::launder(reinterpret_cast<T*>(data.storage)));
      data.initialized = std::byte{ 0 };
    }
  }

  T& get() { return *std::launder(reinterpret_cast<T*>(data.storage)); }
  const T& get() const { return *std::launder(reinterpret_cast<const T*>(data.storage)); }
};

} // namespace detail::v20
#endif

// ----------------- Expose the Most Recent Implementation in `PBB::detail` -----------------
namespace detail
{
#if __cplusplus >= 202002L
template <typename T>
using CacheAlignedPlacement = detail::v20::CacheAlignedPlacement<T>;
#else
template <typename T>
using CacheAlignedPlacement = detail::v17::CacheAlignedPlacement<T>;
#endif
}

template <typename T>
using CacheAlignedPlacement = detail::CacheAlignedPlacement<T>;

// Type trait to determine the underlying type
template <typename T>
struct UnderlyingType
{
  using type = T;
};

template <typename T>
struct UnderlyingType<CacheAlignedStorage<T>>
{
  using type = T;
};

template <typename T>
struct UnderlyingType<CacheAlignedPlacement<T>>
{
  using type = T;
};

// Helper alias
template <typename T>
using UnderlyingTypeT = typename UnderlyingType<T>::type;

} // namespace sps
