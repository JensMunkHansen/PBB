#pragma once

#include <PBB/ThreadLocal.hpp>

namespace PBB
{
namespace detail::v17
{
template <typename T, typename U, typename SFINAE>
ThreadLocal<T, U, SFINAE>::ThreadLocal() = default;

template <typename T, typename U, typename SFINAE>
void ThreadLocal<T, U, SFINAE>::RegisterThreadLocalValue(U* pValue)
{
    thread_local bool is_registered = false;
    if (!is_registered)
    {
        {
            std::lock_guard<std::mutex> lock(_registry_mutex);
            _registry.push_back(pValue);
        }
        is_registered = true;
    }
}

template <typename T, typename U, typename SFINAE>
U& ThreadLocal<T, U, SFINAE>::Local()
{
    std::thread::id thread_id = std::this_thread::get_id();
    U* pValue = nullptr;

#if defined(PBB_USE_TBB_MAP)
    auto it = _storage.find(thread_id);
    if (it == _storage.end())
    {
        auto result = _storage.emplace(thread_id, std::make_shared<U>());
        it = result.first;
    }
    pValue = it->second.get();
#else
    {
        std::shared_lock<std::shared_mutex> read_lock(_storagemutex);
        auto it = _storage.find(thread_id);
        if (it != _storage.end() && it->second)
        {
            pValue = it->second.get();
        }
    }

    if (!pValue)
    {
        std::unique_lock<std::shared_mutex> write_lock(_storagemutex);
        auto& value_ptr = _storage[thread_id];

        if (!value_ptr)
        {
            value_ptr = std::make_shared<U>();
        }
        pValue = value_ptr.get();
    }
#endif

    RegisterThreadLocalValue(pValue);
    return *pValue;
}

template <typename T, typename U, typename SFINAE>
const std::vector<U*>& ThreadLocal<T, U, SFINAE>::GetRegistry() const
{
    return _registry;
}

template <typename T, typename U, typename SFINAE>
std::mutex& ThreadLocal<T, U, SFINAE>::GetMutex()
{
    return _registry_mutex;
}
} // namespace detail::v17
template <typename T>
using ThreadLocal = detail::v17::ThreadLocal<CacheAlignedPlacement<T>>;

} // namespace PBB
