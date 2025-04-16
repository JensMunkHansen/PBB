#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include <PBB/Common.hpp>

namespace PBB
{
namespace detail::v17
{
template <typename T, typename U>
class IThreadLocal
{
  public:
    IThreadLocal() = default;
    virtual ~IThreadLocal() = default;
    virtual U& Local() = 0;

    class ItImpl
    {
      public:
        ItImpl() = default;
        virtual ~ItImpl() = default;
        ItImpl(const ItImpl&) = default;
        ItImpl(ItImpl&&) noexcept = default;
        ItImpl& operator=(const ItImpl&) = default;
        ItImpl& operator=(ItImpl&&) noexcept = default;

        virtual void Increment() = 0;

        virtual bool Compare(ItImpl* other) = 0;

        virtual T& GetContent() = 0;

        virtual T* GetContentPtr() = 0;

        std::unique_ptr<ItImpl> Clone() const { return std::unique_ptr<ItImpl>(CloneImpl()); }

      protected:
        virtual ItImpl* CloneImpl() const = 0;
    };
    //  virtual std::unique_ptr<ItImpl> begin() = 0;

    //    virtual std::unique_ptr<ItImpl> end() = 0;

  private:
    //    PBB_DELETE_CTORS(IThreadLocal);
};
} // namespace detail::v17
} // namespace PBB
