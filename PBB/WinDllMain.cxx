#include <PBB/ThreadPool.hpp>
#include <windows.h>

using namespace PBB::Thread;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
  switch (ul_reason_for_call)
  {
    case DLL_PROCESS_DETACH:
      // This is called when the DLL is unloaded
      // ThreadPool<Tags::DefaultPool>::InstanceDestroy();
      break;
  }
  return TRUE;
}
