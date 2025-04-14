#pragma once

#include "PBB/pbb_export.h" // For export macro

#include <atomic>
#include <cstdint> // For uint_fast32_t
#include <mutex>   // std::mutex, std::lock_guard
#include <thread>

namespace PBB
{
namespace detail
{
namespace STDThread
{

typedef size_t ThreadIdType;
typedef uint_fast32_t HashType;
typedef void* StoragePointerType;

struct Slot
{
    std::atomic<ThreadIdType> ThreadId;
    std::mutex Mutex;
    StoragePointerType Storage;

    Slot();
    ~Slot() = default;

  private:
    // not copyable
    Slot(const Slot&);
    void operator=(const Slot&);
};

struct HashTableArray
{
    size_t Size, SizeLg;
    std::atomic<size_t> NumberOfEntries;
    Slot* Slots;
    HashTableArray* Prev;

    explicit HashTableArray(size_t sizeLg);
    ~HashTableArray();

  private:
    // disallow copying
    HashTableArray(const HashTableArray&);
    void operator=(const HashTableArray&);
};

class PBB_EXPORT ThreadSpecific final
{
  public:
    explicit ThreadSpecific(unsigned numThreads);
    ~ThreadSpecific();

    StoragePointerType& GetStorage();
    size_t GetSize() const;

  private:
    std::atomic<HashTableArray*> Root;
    std::atomic<size_t> Size;
    std::mutex Mutex;

    friend class ThreadSpecificStorageIterator;
};

class ThreadSpecificStorageIterator
{
  public:
    ThreadSpecificStorageIterator()
      : ThreadSpecificStorage(nullptr)
      , CurrentArray(nullptr)
      , CurrentSlot(0)
    {
    }

    void SetThreadSpecificStorage(ThreadSpecific& threadSpecifc)
    {
        this->ThreadSpecificStorage = &threadSpecifc;
    }

    void SetToBegin()
    {
        this->CurrentArray = this->ThreadSpecificStorage->Root;
        this->CurrentSlot = 0;
        if (!this->CurrentArray->Slots->Storage)
        {
            this->Forward();
        }
    }

    void SetToEnd()
    {
        this->CurrentArray = nullptr;
        this->CurrentSlot = 0;
    }

    bool GetInitialized() const { return this->ThreadSpecificStorage != nullptr; }

    bool GetAtEnd() const { return this->CurrentArray == nullptr; }

    void Forward()
    {
        while (true)
        {
            if (++this->CurrentSlot >= this->CurrentArray->Size)
            {
                this->CurrentArray = this->CurrentArray->Prev;
                this->CurrentSlot = 0;
                if (!this->CurrentArray)
                {
                    break;
                }
            }
            Slot* slot = this->CurrentArray->Slots + this->CurrentSlot;
            if (slot->Storage)
            {
                break;
            }
        }
    }

    StoragePointerType& GetStorage() const
    {
        Slot* slot = this->CurrentArray->Slots + this->CurrentSlot;
        return slot->Storage;
    }

    bool operator==(const ThreadSpecificStorageIterator& it) const
    {
        return (this->ThreadSpecificStorage == it.ThreadSpecificStorage) &&
          (this->CurrentArray == it.CurrentArray) && (this->CurrentSlot == it.CurrentSlot);
    }

  private:
    ThreadSpecific* ThreadSpecificStorage;
    HashTableArray* CurrentArray;
    size_t CurrentSlot;
};

} // STDThread;
} // namespace detail
} // namespace PBB
