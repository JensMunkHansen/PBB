#include "PBB/spsSMP.h" // For SMP preprocessor information

#include "PBB/SMPTools.h" // The API

#include <algorithm> // For std::toupper
#include <cstdlib>   // For std::getenv
#include <iostream>  // For std::cerr
#include <string>    // For std::string

namespace PBB
{
namespace detail
{

//------------------------------------------------------------------------------
SMPToolsAPI::SMPToolsAPI()
{
    this->SequentialBackend = std::make_unique<SMPToolsImpl<BackendType::Sequential>>();
    this->STDThreadBackend = std::make_unique<SMPToolsImpl<BackendType::STDThread>>();

    // Set backend from env if set
    const char* SMPBackendInUse = std::getenv("SPS_SMP_BACKEND_IN_USE");
    if (SMPBackendInUse)
    {
        this->SetBackend(SMPBackendInUse);
    }

    // Set max thread number from env
    this->RefreshNumberOfThread();
}

//------------------------------------------------------------------------------
// Must NOT be initialized. Default initialization to zero is necessary.
SMPToolsAPI* SMPToolsAPIInstanceAsPointer;

//------------------------------------------------------------------------------
SMPToolsAPI& SMPToolsAPI::GetInstance()
{
    return *SMPToolsAPIInstanceAsPointer;
}

//------------------------------------------------------------------------------
void SMPToolsAPI::ClassInitialize()
{
    if (!SMPToolsAPIInstanceAsPointer)
    {
        SMPToolsAPIInstanceAsPointer = new SMPToolsAPI;
    }
}

//------------------------------------------------------------------------------
void SMPToolsAPI::ClassFinalize()
{
    delete SMPToolsAPIInstanceAsPointer;
    SMPToolsAPIInstanceAsPointer = nullptr;
}

//------------------------------------------------------------------------------
BackendType SMPToolsAPI::GetBackendType()
{
    return this->ActivatedBackend;
}

//------------------------------------------------------------------------------
const char* SMPToolsAPI::GetBackend()
{
    switch (this->ActivatedBackend)
    {
        case BackendType::Sequential:
            return "Sequential";
        case BackendType::STDThread:
            return "STDThread";
    }
    return nullptr;
}

//------------------------------------------------------------------------------
bool SMPToolsAPI::SetBackend(const char* type)
{
    std::string backend(type);
    std::transform(backend.cbegin(), backend.cend(), backend.begin(), ::toupper);
    if (backend == "SEQUENTIAL" && this->SequentialBackend)
    {
        this->ActivatedBackend = BackendType::Sequential;
    }
    else if (backend == "STDTHREAD" && this->STDThreadBackend)
    {
        this->ActivatedBackend = BackendType::STDThread;
    }
    else
    {
        std::cerr << "WARNING: tried to use a non implemented SMPTools backend \"" << type
                  << "\"!\n";
        std::cerr << "The available backends are:"
                  << (this->SequentialBackend ? " \"Sequential\"" : "")
                  << (this->STDThreadBackend ? " \"STDThread\"" : "") << "\n";
        std::cerr << "Using " << this->GetBackend() << " instead." << std::endl;
        return false;
    }
    this->RefreshNumberOfThread();
    return true;
}

//------------------------------------------------------------------------------
void SMPToolsAPI::Initialize(int numThreads)
{
    this->DesiredNumberOfThread = numThreads;
    this->RefreshNumberOfThread();
}

//------------------------------------------------------------------------------
void SMPToolsAPI::RefreshNumberOfThread()
{
    const int numThreads = this->DesiredNumberOfThread;
    switch (this->ActivatedBackend)
    {
        case BackendType::Sequential:
            this->SequentialBackend->Initialize(numThreads);
            break;
        case BackendType::STDThread:
            this->STDThreadBackend->Initialize(numThreads);
            break;
    }
}

//------------------------------------------------------------------------------
int SMPToolsAPI::GetEstimatedDefaultNumberOfThreads()
{
    switch (this->ActivatedBackend)
    {
        case BackendType::Sequential:
            return this->SequentialBackend->GetEstimatedDefaultNumberOfThreads();
        case BackendType::STDThread:
            return this->STDThreadBackend->GetEstimatedDefaultNumberOfThreads();
    }
    return 0;
}

//------------------------------------------------------------------------------
int SMPToolsAPI::GetEstimatedNumberOfThreads()
{
    switch (this->ActivatedBackend)
    {
        case BackendType::Sequential:
            return this->SequentialBackend->GetEstimatedNumberOfThreads();
        case BackendType::STDThread:
            return this->STDThreadBackend->GetEstimatedNumberOfThreads();
    }
    return 0;
}

//------------------------------------------------------------------------------
void SMPToolsAPI::SetNestedParallelism(bool isNested)
{
    switch (this->ActivatedBackend)
    {
        case BackendType::Sequential:
            this->SequentialBackend->SetNestedParallelism(isNested);
            break;
        case BackendType::STDThread:
            this->STDThreadBackend->SetNestedParallelism(isNested);
            break;
    }
}

//------------------------------------------------------------------------------
bool SMPToolsAPI::GetNestedParallelism()
{
    switch (this->ActivatedBackend)
    {
        case BackendType::Sequential:
            return this->SequentialBackend->GetNestedParallelism();
        case BackendType::STDThread:
            return this->STDThreadBackend->GetNestedParallelism();
    }
    return false;
}

//------------------------------------------------------------------------------
bool SMPToolsAPI::IsParallelScope()
{
    switch (this->ActivatedBackend)
    {
        case BackendType::Sequential:
            return this->SequentialBackend->IsParallelScope();
        case BackendType::STDThread:
            return this->STDThreadBackend->IsParallelScope();
    }
    return false;
}

//------------------------------------------------------------------------------
bool SMPToolsAPI::GetSingleThread()
{
    // Currently, this will work as expected for one parallel area and or nested
    // parallel areas. If there are two or more parallel areas that are not nested,
    // this function will not work properly.
    switch (this->ActivatedBackend)
    {
        case BackendType::Sequential:
            return this->SequentialBackend->GetSingleThread();
        case BackendType::STDThread:
            return this->STDThreadBackend->GetSingleThread();
        default:
            return false;
    }
}

//------------------------------------------------------------------------------
// Must NOT be initialized. Default initialization to zero is necessary.
unsigned int SMPToolsAPIInitializeCount;

//------------------------------------------------------------------------------
SMPToolsAPIInitialize::SMPToolsAPIInitialize()
{
    if (++SMPToolsAPIInitializeCount == 1)
    {
        SMPToolsAPI::ClassInitialize();
    }
}

//------------------------------------------------------------------------------
SMPToolsAPIInitialize::~SMPToolsAPIInitialize()
{
    if (--SMPToolsAPIInitializeCount == 0)
    {
        SMPToolsAPI::ClassFinalize();
    }
}

} // namespace detail
} // namespace PBB
