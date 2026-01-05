#include <chrono>
#include <iostream>
#include <string>

#include "PulseFS/Core/MftScanner.hpp"
#include "PulseFS/Core/UsnMonitor.hpp"
#include "PulseFS/Engine/SearchIndex.hpp"
#include "PulseFS/Utils/WinHelpers.hpp"

int main() {

  if (!PulseFS::Utils::IsRunAsAdmin()) {
    std::cerr << "[ERROR] Administrator privileges required for MFT access."
              << std::endl;
    std::cin.get();
    return 1;
  }

  std::cout << "PulseFS Initializing..." << std::endl;

  PulseFS::Engine::SearchIndex searchIndex;
  const std::wstring volume = L"\\\\.\\C:";

  try {

    std::cout << "Scanning MFT..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    long long nextUsn =
        PulseFS::Core::MftScanner::Enumerate(searchIndex, volume);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> distinct = end - start;

    std::cout << "Indexed " << searchIndex.Count() << " files in "
              << distinct.count() << "s." << std::endl;

    PulseFS::Core::UsnMonitor monitor(searchIndex);
    monitor.Start(volume, nextUsn);
    std::cout << "Real-time monitoring active." << std::endl;

    bool running = true;
    while (running) {
      std::cout << "\nPulseFS> ";
      std::wstring query;
      std::wcin >> query;

      if (query == L"exit") {
        running = false;
        break;
      }

      auto s_start = std::chrono::high_resolution_clock::now();
      auto results = searchIndex.Search(query, 20);
      auto s_end = std::chrono::high_resolution_clock::now();

      std::chrono::duration<double, std::milli> s_elapsed = s_end - s_start;

      std::cout << "Found " << results.size() << " results ("
                << s_elapsed.count() << "ms):" << std::endl;
      for (auto id : results) {
        std::wcout << searchIndex.GetFullPath(id) << std::endl;
      }
    }

    monitor.Stop();

  } catch (const std::exception &e) {
    std::cerr << "[FATAL] " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
