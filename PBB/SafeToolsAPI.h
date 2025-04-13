#pragma onceo

#include "PBB/pbb_export.h" // For export macro
#include "PBB/spsSMP.h"     // For default backend

#include <memory>

#include "PBB/SafeFunctors.hpp"

// Sequential and STDThread inline implementation
#include "PBB/SafeFunctors.txx"

namespace PBB
{
namespace detail
{

using SafeFunctorsDefaultImpl = SafeFunctors<DefaultBackend>;

class PBB_EXPORT SafeToolsAPI
{
  public:
    //--------------------------------------------------------------------------------
    static SafeToolsAPI& GetInstance();

    //--------------------------------------------------------------------------------
    BackendType GetBackendType();

    //--------------------------------------------------------------------------------
    const char* GetBackend();

    //--------------------------------------------------------------------------------
    bool SetBackend(const char* type);

    //--------------------------------------------------------------------------------
    void Initialize(int numThreads = 0);

    //--------------------------------------------------------------------------------
    int GetEstimatedNumberOfThreads();

    //--------------------------------------------------------------------------------
    int GetEstimatedDefaultNumberOfThreads();

    //------------------------------------------------------------------------------
    void SetNestedParallelism(bool isNested);

    //--------------------------------------------------------------------------------
    bool GetNestedParallelism();

    //--------------------------------------------------------------------------------
    bool IsParallelScope();

    //--------------------------------------------------------------------------------
    bool GetSingleThread();

    //--------------------------------------------------------------------------------
    int GetInternalDesiredNumberOfThread() { return this->DesiredNumberOfThread; }

    //------------------------------------------------------------------------------
    template <typename Config, typename T>
    void LocalScope(Config const& config, T&& lambda)
    {
        const Config oldConfig(*this);
        *this << config;
        try
        {
            lambda();
        }
        catch (...)
        {
            *this << oldConfig;
            throw;
        }
        *this << oldConfig;
    }

    //--------------------------------------------------------------------------------
    template <typename FunctorInternal>
    void For(spsIdType first, spsIdType last, spsIdType grain, FunctorInternal& fi)
    {
        switch (this->ActivatedBackend)
        {
            case BackendType::Sequential:
                this->SequentialBackend->For(first, last, grain, fi);
                break;
            case BackendType::STDThread:
                this->STDThreadBackend->For(first, last, grain, fi);
                break;
        }
    }

    //--------------------------------------------------------------------------------
    template <typename InputIt, typename OutputIt, typename Functor>
    void Transform(InputIt inBegin, InputIt inEnd, OutputIt outBegin, Functor& transform)
    {
        switch (this->ActivatedBackend)
        {
            case BackendType::Sequential:
                this->SequentialBackend->Transform(inBegin, inEnd, outBegin, transform);
                break;
            case BackendType::STDThread:
                this->STDThreadBackend->Transform(inBegin, inEnd, outBegin, transform);
                break;
        }
    }

    //--------------------------------------------------------------------------------
    template <typename InputIt1, typename InputIt2, typename OutputIt, typename Functor>
    void Transform(
      InputIt1 inBegin1, InputIt1 inEnd, InputIt2 inBegin2, OutputIt outBegin, Functor& transform)
    {
        switch (this->ActivatedBackend)
        {
            case BackendType::Sequential:
                this->SequentialBackend->Transform(inBegin1, inEnd, inBegin2, outBegin, transform);
                break;
            case BackendType::STDThread:
                this->STDThreadBackend->Transform(inBegin1, inEnd, inBegin2, outBegin, transform);
                break;
        }
    }

    //--------------------------------------------------------------------------------
    template <typename Iterator, typename T>
    void Fill(Iterator begin, Iterator end, const T& value)
    {
        switch (this->ActivatedBackend)
        {
            case BackendType::Sequential:
                this->SequentialBackend->Fill(begin, end, value);
                break;
            case BackendType::STDThread:
                this->STDThreadBackend->Fill(begin, end, value);
                break;
        }
    }

    //--------------------------------------------------------------------------------
    template <typename RandomAccessIterator>
    void Sort(RandomAccessIterator begin, RandomAccessIterator end)
    {
        switch (this->ActivatedBackend)
        {
            case BackendType::Sequential:
                this->SequentialBackend->Sort(begin, end);
                break;
            case BackendType::STDThread:
                this->STDThreadBackend->Sort(begin, end);
                break;
        }
    }

    //--------------------------------------------------------------------------------
    template <typename RandomAccessIterator, typename Compare>
    void Sort(RandomAccessIterator begin, RandomAccessIterator end, Compare comp)
    {
        switch (this->ActivatedBackend)
        {
            case BackendType::Sequential:
                this->SequentialBackend->Sort(begin, end, comp);
                break;
            case BackendType::STDThread:
                this->STDThreadBackend->Sort(begin, end, comp);
                break;
        }
    }

    // disable copying
    SafeToolsAPI(SafeToolsAPI const&) = delete;
    void operator=(SafeToolsAPI const&) = delete;

  protected:
    //--------------------------------------------------------------------------------
    // Address the static initialization order 'fiasco' by implementing
    // the schwarz counter idiom.
    static void ClassInitialize();
    static void ClassFinalize();
    friend class SafeToolsAPIInitialize;

  private:
    //--------------------------------------------------------------------------------
    SafeToolsAPI();

    //--------------------------------------------------------------------------------
    void RefreshNumberOfThread();

    //--------------------------------------------------------------------------------
    // This operator overload is used to unpack Config parameters and set them
    // in SafeToolsAPI (e.g `*this << config;`)
    template <typename Config>
    SafeToolsAPI& operator<<(Config const& config)
    {
        this->Initialize(config.MaxNumberOfThreads);
        this->SetBackend(config.Backend.c_str());
        this->SetNestedParallelism(config.NestedParallelism);
        return *this;
    }

    /**
     * Indicate which backend to use.
     */
    BackendType ActivatedBackend = DefaultBackend;

    /**
     * Max threads number
     */
    int DesiredNumberOfThread = 0;

    /**
     * Sequential backend
     */
#if SPS_SMP_ENABLE_SEQUENTIAL
    std::unique_ptr<SafeFunctors<BackendType::Sequential>> SequentialBackend;
#else
    std::unique_ptr<SafeFunctorsDefaultImpl> SequentialBackend;
#endif

    /**
     * STDThread backend
     */
#if SPS_SMP_ENABLE_STDTHREAD
    std::unique_ptr<SafeFunctors<BackendType::STDThread>> STDThreadBackend;
#else
    std::unique_ptr<SafeFunctorsDefaultImpl> STDThreadBackend;
#endif
};

//--------------------------------------------------------------------------------
class PBB_EXPORT SafeToolsAPIInitialize
{
  public:
    SafeToolsAPIInitialize();
    ~SafeToolsAPIInitialize();
};

//--------------------------------------------------------------------------------
// This instance will show up in any translation unit that uses SafeToolsAPI singleton.
// It will make sure SafeToolsAPI is initialized before it is used and finalized when it
// is done being used.
static SafeToolsAPIInitialize SafeToolsAPIInitializer;

} // namespace detail
} // namespace PBB
