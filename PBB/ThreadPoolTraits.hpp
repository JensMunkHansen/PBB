#pragma once

namespace PBB::Thread
{

template <typename Tag>
struct ThreadPoolTraits
{
  static void WorkerLoop(auto& self) { self.DefaultWorkerLoop(); }

  template <typename Func, typename... Args>
  static auto Submit(auto& self, Func&& func, Args&&... args)
  {
    return self.DefaultSubmit(std::forward<Func>(func), std::forward<Args>(args)...);
  }
};

} // namespace PBB::Thread
