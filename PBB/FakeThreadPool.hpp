#pragma once
#include <PBB/ResettableSingleton.hpp>
#include <string>
#include <vector>

namespace PBB
{

class FakeThreadPool : public ResettableSingleton<FakeThreadPool>
{
  // ✅ Give only SingletonAccess permission to call the constructor
  friend struct SingletonAccess;

private:
  FakeThreadPool() = default;

public:
  void Submit(const std::string& task) { m_tasks.push_back(task); }
  const std::vector<std::string>& Tasks() const { return m_tasks; }
  void Clear() { m_tasks.clear(); }

private:
  std::vector<std::string> m_tasks;
};

} // namespace PBB
