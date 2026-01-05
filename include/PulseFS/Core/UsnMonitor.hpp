#pragma once

#include "PulseFS/Engine/SearchIndex.hpp"
#include <atomic>
#include <string>
#include <thread>

namespace PulseFS::Core {

class UsnMonitor {
public:
  explicit UsnMonitor(Engine::SearchIndex &index);
  ~UsnMonitor();

  void Start(const std::wstring &volumePath, long long startUsn);
  void Stop();

private:
  void MonitorLoop(std::wstring volumePath, long long startUsn);

  Engine::SearchIndex &m_index;
  std::atomic<bool> m_running;
  std::thread m_thread;
};

} // namespace PulseFS::Core
