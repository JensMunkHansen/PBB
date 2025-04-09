#pragma once
#include <memory>

namespace PBB
{

struct SingletonAccess
{
    template <typename T>
    static T* Construct()
    {
        return new T();
    }
};

template <typename T>
class ResettableSingleton
{
  public:
    static T& InstanceGet()
    {
#if 0
      // Can only be constructed once.
std::call_once(init_flag_, [] {
            instance_ = std::unique_ptr<T>(SingletonAccess::Construct<T>());
        });
        return *instance_;
#else
        std::lock_guard lock(instance_mutex_);
        if (!instance_)
            instance_ = std::unique_ptr<T>(SingletonAccess::Construct<T>());
        return *instance_;
#endif
    }

#ifdef PBB_ENABLE_TEST_SINGLETON
    static void Reset()
    {
        std::lock_guard lock(instance_mutex_);
        instance_.reset();
    }
#else
    static void Reset() = delete;
#endif

  protected:
    ResettableSingleton() = default;

  private:
    // One instance per class
    inline static std::unique_ptr<T> instance_;
    inline static std::mutex instance_mutex_;
};

} // namespace PBB
