#pragma once

#if __cplusplus < 201103L
#error "This header requires at least C++11"
#endif

#if defined(__clang__) && !defined(__clang_analyzer__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
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
    static T* InstancePtrGet() { return &InstanceGet(); }

  protected:
    MeyersSingleton() = default;
    virtual ~MeyersSingleton() = default;

    MeyersSingleton(const MeyersSingleton&) = delete;
    MeyersSingleton& operator=(const MeyersSingleton&) = delete;
};

} // namespace PBB
#if defined(__clang__) && !defined(__clang_analyzer__)
#pragma clang diagnostic pop
#endif
