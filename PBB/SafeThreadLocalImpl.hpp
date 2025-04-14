#pragma once

#include "PBB/SafeThreadLocal.hpp"

#include "PBB/SafeFunctors.txx"
#include "PBB/SafeThreadLocalBackend.hpp"
// #include "PBB/SafeFunctorsImpl.hpp"

//#include "PBB/STDThread/spsSMPThreadLocalBackend.h"
//#include "PBB/STDThread/spsSMPToolsImpl.txx"

#include <iterator>
#include <utility> // For std::move

namespace PBB
{
namespace detail
{
template <typename T>
class SafeThreadLocal<BackendType::STDThread, T> : public ISafeThreadLocal<T>
{
    typedef typename SafeThreadLocal<T>::ItImpl ItImplAbstract;

  public:
    SafeThreadLocal()
      : Backend(GetNumberOfThreadsSTDThread())
    {
    }

    explicit SafeThreadLocal(const T& exemplar)
      : Backend(GetNumberOfThreadsSTDThread())
      , Exemplar(exemplar)
    {
    }

    ~SafeThreadLocal() override
    {
        PBB::detail::ThreadSpecificStorageIterator it;
        it.SetThreadSpecificStorage(this->Backend);
        for (it.SetToBegin(); !it.GetAtEnd(); it.Forward())
        {
            delete reinterpret_cast<T*>(it.GetStorage());
        }
    }

    T& Local() override
    {
        PBB::detail::STDThread::StoragePointerType& ptr = this->Backend.GetStorage();
        T* local = reinterpret_cast<T*>(ptr);
        if (!ptr)
        {
            ptr = local = new T(this->Exemplar);
        }
        return *local;
    }

    size_t size() const override { return this->Backend.GetSize(); }

    class ItImpl : public SafeThreadLocalAbstract<T>::ItImpl
    {
      public:
        void Increment() override { this->Impl.Forward(); }

        bool Compare(ItImplAbstract* other) override
        {
            return this->Impl == static_cast<ItImpl*>(other)->Impl;
        }

        T& GetContent() override { return *reinterpret_cast<T*>(this->Impl.GetStorage()); }

        T* GetContentPtr() override { return reinterpret_cast<T*>(this->Impl.GetStorage()); }

      protected:
        ItImpl* CloneImpl() const override { return new ItImpl(*this); }

      private:
        PBB::detail::STDThread::ThreadSpecificStorageIterator Impl;

        friend class SafeThreadLocal<BackendType::STDThread, T>;
    };

    std::unique_ptr<ItImplAbstract> begin() override
    {
        auto it = std::make_unique<ItImpl>();
        it->Impl.SetThreadSpecificStorage(this->Backend);
        it->Impl.SetToBegin();
        return std::move(it);
    }

    std::unique_ptr<ItImplAbstract> end() override
    {
        // XXX(c++14): use std::make_unique
        auto it = std::make_unique<ItImpl>();
        it->Impl.SetThreadSpecificStorage(this->Backend);
        it->Impl.SetToEnd();
        return std::move(it);
    }

  private:
    PBB::detail::STDThread::ThreadSpecific Backend;
    T Exemplar;

    // disable copying
    SafeThreadLocal(const SafeThreadLocal&) = delete;
    void operator=(const SafeThreadLocal&) = delete;
};

} // namespace detail
} // namespace sps
