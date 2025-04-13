// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "PBB/SafeToolsAPI.h"
#include "PBB/spsSMP.h" // For SMP preprocessor information

#include <algorithm> // For std::toupper
#include <cstdlib>   // For std::getenv
#include <iostream>  // For std::cerr
#include <string>    // For std::string

namespace PBB
{
namespace detail
{

//------------------------------------------------------------------------------
SafeToolsAPI::SafeToolsAPI()
{
#if SPS_SMP_ENABLE_SEQUENTIAL
    this->SequentialBackend = std::make_unique<SafeTools<BackendType::Sequential>>();
#endif
#if SPS_SMP_ENABLE_STDTHREAD
    this->STDThreadBackend = std::make_unique<SafeTools<BackendType::STDThread>>();
#endif

    // Set backend from env if set
    const char* spsSMPBackendInUse = std::getenv("SPS_SMP_BACKEND_IN_USE");
    if (spsSMPBackendInUse)
    {
        this->SetBackend(spsSMPBackendInUse);
    }

    // Set max thread number from env
    this->RefreshNumberOfThread();
}

//------------------------------------------------------------------------------
// Must NOT be initialized. Default initialization to zero is necessary.
SafeToolsAPI* SafeToolsAPIInstanceAsPointer;

//------------------------------------------------------------------------------
SafeToolsAPI& SafeToolsAPI::GetInstance()
{
    return *SafeToolsAPIInstanceAsPointer;
}

//------------------------------------------------------------------------------
void SafeToolsAPI::ClassInitialize()
{
    if (!SafeToolsAPIInstanceAsPointer)
    {
        SafeToolsAPIInstanceAsPointer = new SafeToolsAPI;
    }
}

//------------------------------------------------------------------------------
void SafeToolsAPI::ClassFinalize()
{
    delete SafeToolsAPIInstanceAsPointer;
    SafeToolsAPIInstanceAsPointer = nullptr;
}

//------------------------------------------------------------------------------
BackendType SafeToolsAPI::GetBackendType()
{
    return this->ActivatedBackend;
}

//------------------------------------------------------------------------------
const char* SafeToolsAPI::GetBackend()
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
bool SafeToolsAPI::SetBackend(const char* type)
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
void SafeToolsAPI::Initialize(int numThreads)
{
    this->DesiredNumberOfThread = numThreads;
    this->RefreshNumberOfThread();
}

//------------------------------------------------------------------------------
void SafeToolsAPI::RefreshNumberOfThread()
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
int SafeToolsAPI::GetEstimatedDefaultNumberOfThreads()
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
int SafeToolsAPI::GetEstimatedNumberOfThreads()
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
void SafeToolsAPI::SetNestedParallelism(bool isNested)
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
bool SafeToolsAPI::GetNestedParallelism()
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
bool SafeToolsAPI::IsParallelScope()
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
bool SafeToolsAPI::GetSingleThread()
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
unsigned int SafeToolsAPIInitializeCount;

//------------------------------------------------------------------------------
SafeToolsAPIInitialize::SafeToolsAPIInitialize()
{
    if (++SafeToolsAPIInitializeCount == 1)
    {
        SafeToolsAPI::ClassInitialize();
    }
}

//------------------------------------------------------------------------------
SafeToolsAPIInitialize::~SafeToolsAPIInitialize()
{
    if (--SafeToolsAPIInitializeCount == 0)
    {
        SafeToolsAPI::ClassFinalize();
    }
}

} // namespace detail
} // namespace PBB
