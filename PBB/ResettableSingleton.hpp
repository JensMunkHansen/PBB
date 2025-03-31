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
    if (!instance_)
      instance_ = std::unique_ptr<T>(SingletonAccess::Construct<T>());
    return *instance_;
  }

#ifdef PBB_ENABLE_TEST_SINGLETON
  static void Reset()
  {
    instance_.reset();
  }
#else
  static void Reset() = delete;
#endif

protected:
  ResettableSingleton() = default;

private:
  inline static std::unique_ptr<T> instance_;
};

} // namespace PBB
