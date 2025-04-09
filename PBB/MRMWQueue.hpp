/**
 * @file   mimo20.hpp
 * @author Jens Munk Hansen <jens.munk.hansen@gmail.com>
 * @date   Thu Oct 19 01:34:56 2017
 *
 * @brief  Multi-reader-multi-writer (MRMW) thread-safe queue
 *         and circular buffers.
 *
 * Copyright 2025 Jens Munk Hansen
 *
 */
/*
 *  This file is part of SOFUS.
 *
 *  SOFUS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SOFUS is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with SOFUS.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  TODO: make version using std::shared_lock - one writer, multiple readers
 */

#pragma once

#if __cplusplus < 202002L
#error "This header requires at least C++20"
#endif

#include <atomic>
#include <concepts>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>

namespace PBB
{
namespace detail::v20
{
template <typename T>
concept CopyConstructibleType = std::is_copy_constructible_v<T>;

template <typename T>
concept NoThrowMoveConstructible = std::is_nothrow_move_constructible_v<T>;

template <typename T>
concept NoThrowMoveAssignable = std::is_nothrow_move_assignable_v<T>;

template <typename T>
concept NoThrowCopyConstructible = std::is_nothrow_copy_constructible_v<T>;

template <typename T>
class IMRMWQueue
{
  public:
    virtual ~IMRMWQueue() noexcept = default;

    IMRMWQueue(IMRMWQueue&& other) noexcept = default;
    IMRMWQueue& operator=(IMRMWQueue&& other) noexcept = default;

    virtual bool TryPop(T& destination) = 0;
    virtual bool Pop(T& destination) = 0;
    virtual bool Push(T&& source) = 0;
    virtual void Invalidate() = 0;
    virtual bool Valid() const = 0;
    virtual void Clear() = 0;
    virtual bool Empty() const = 0;

    bool Push(
      const T& souce) noexcept requires CopyConstructibleType<T> && NoThrowCopyConstructible<T>;

  protected:
    IMRMWQueue() noexcept = default;

  private:
    IMRMWQueue(const IMRMWQueue&) = delete;
    IMRMWQueue& operator=(const IMRMWQueue&) = delete;
};

template <typename T>
class MRMWQueue : public IMRMWQueue<T>
{
  public:
    MRMWQueue() noexcept = default;

    ~MRMWQueue() noexcept override
    {
        Invalidate();
        Clear();
    }

    std::mutex& GetMutex() noexcept { return m_mutex; }

    bool TryPop(T& destination) noexcept override
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        if (m_queue.empty())
        {
            return false;
        }
        destination = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    bool Pop(T& destination) override
    {
        if (!m_valid.load(std::memory_order_acquire))
            return false;

        std::unique_lock<std::mutex> lock{ m_mutex };
        m_condition.wait(
          lock, [this]() { return !m_queue.empty() || !m_valid.load(std::memory_order_acquire); });

        if (!m_valid.load(std::memory_order_acquire))
            return false;

        if (m_queue.empty())
        {
            return false;
        }

        destination = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    bool Push(T&& source) noexcept override
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_queue.push(std::move(source));
        m_condition.notify_one();
        return true;
    }

    bool Push(
      const T& source) noexcept requires CopyConstructibleType<T> && NoThrowCopyConstructible<T>
    {
        std::lock_guard<std::mutex> guard(this->m_mutex);
        this->m_queue.push(source);
        this->m_condition.notify_one();
        return true;
    }

    void Invalidate() noexcept override
    {
        m_valid.store(false, std::memory_order_release);
        std::lock_guard<std::mutex> guard{ m_mutex };
        m_condition.notify_all();
    }

    bool Valid() const noexcept override { return m_valid.load(std::memory_order_acquire); }

    void Clear() noexcept override
    {
        std::lock_guard<std::mutex> guard{ m_mutex };
        while (!m_queue.empty())
        {
            m_queue.pop();
        }
        m_condition.notify_all();
    }

    bool Empty() const noexcept override
    {
        std::lock_guard<std::mutex> guard{ m_mutex };
        return m_queue.empty();
    }

  protected:
    std::queue<T> m_queue;      ///< Queue containing e.g. callables
    mutable std::mutex m_mutex; ///< Mutex for locking

    std::atomic<bool> m_valid{ true };   ///< State for invalidation
    std::condition_variable m_condition; ///< Condition for signal not empty
};

template <typename T>
class LockGuard
{
  public:
    explicit LockGuard(MRMWQueue<T>& queue) noexcept
      : m_queue(queue)
      , m_lock(queue.get_mutex())
    {
    }
    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;

  private:
    MRMWQueue<T>& m_queue;
    std::lock_guard<std::mutex> m_lock;
};

} // namespace detail::v20

template <typename T>
using IMRMWQueue = detail::v20::IMRMWQueue<T>;
template <typename T>
using MRMWQueue = detail::v20::MRMWQueue<T>;
template <typename T>
using LockGuard = detail::v20::LockGuard<T>;
} // namespace PBB

/* Local variables: */
/* indent-tabs-mode: nil */
/* tab-width: 2 */
/* c-basic-offset: 2 */
/* End: */
