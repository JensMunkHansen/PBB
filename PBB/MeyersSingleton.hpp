#pragma once

#if __cplusplus < 201103L
#error "This header requires at least C++11"
#endif

namespace PBB
{
template <class T>
class MeyersSingleton
{
public:
  static T& InstanceGet()
  {
    static T instance;
    return instance;
  }

protected:
  MeyersSingleton() = default;
  virtual ~MeyersSingleton() = default;

  MeyersSingleton(const MeyersSingleton&) = delete;
  MeyersSingleton& operator=(const MeyersSingleton&) = delete;
};

} // namespace PBB
