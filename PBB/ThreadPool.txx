#include "ThreadPool.hpp"

namespace PBB::Thread
{

template <typename Tag>
ThreadPool<Tag>::ThreadPool()
{
  const std::size_t n = std::max(2u, std::thread::hardware_concurrency() - 1u);
  m_done.clear();
  for (std::size_t i = 0; i < n; ++i)
    m_threads.emplace_back(&ThreadPool::worker, this);
}

template <typename Tag>
ThreadPool<Tag>::~ThreadPool()
{
  destroy();
}

template <typename Tag>
void ThreadPool<Tag>::destroy()
{
  m_done.test_and_set();
  m_condition.notify_all();

  for (auto& t : m_threads)
    if (t.joinable())
      t.join();

  m_threads.clear();
  m_queue.clear();
}

template <typename Tag>
void ThreadPool<Tag>::worker()
{
  while (!m_done.test())
  {
    std::unique_ptr<IThreadTask> task;

    {
      std::unique_lock lock(m_mutex);
      m_condition.wait(lock, [&] { return m_done.test() || !m_queue.empty(); });

      if (m_done.test())
        break;

      if (!m_queue.empty())
      {
        task = std::move(m_queue.back());
        m_queue.pop_back();
      }
    }

    if (task)
      task->Execute();
  }
}

template <typename Tag>
size_t ThreadPool<Tag>::NThreadsGet() const
{
  return m_threads.size();
}

} // namespace PBB::Thread

#if 0

#if defined(_MSC_VER)
#define PBB_USED __declspec(selectany)
#else
#define PBB_USED __attribute__((used))
#endif


// /NODEFAULTLIB (must not be used)
// Use std::axit if singleton resides in DLL
// If used across use DllMain

#include "YourSingletonHeader.hpp"
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_DETACH:
        // This is called when the DLL is unloaded
        PBB::Singleton<YourClass>::InstanceDestroy();
        break;
    }
    return TRUE;
}

#endif
