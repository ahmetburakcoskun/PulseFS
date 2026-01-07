#pragma once

#include "PulseFS/Engine/SearchIndex.hpp"
#include "PulseFS/Renderer/D3D11Renderer.hpp"
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

namespace PulseFS::UI {

class IconCache;
class SearchPanel {
public:
  void Initialize(Engine::SearchIndex &index, Renderer::D3D11Renderer* renderer, IconCache* iconCache);
  void Render();

  void SetScanning(bool scanning) { m_IsScanning = scanning; }
  void SetIndexedCount(size_t count) { m_IndexedCount = count; }

private:
  void RenderSearchBar();
  void RenderStatusBar();
  void RenderResultsTable();
  void OpenFile(const std::wstring &path);

  static std::string WideToUtf8(const std::wstring &wstr);
  static std::wstring Utf8ToWide(const std::string &str);
  static const char* GetFileIcon(const std::wstring &fileName, unsigned long fileAttributes);
  static bool IsImageFile(const std::wstring &fileName);

  Engine::SearchIndex *m_SearchIndex = nullptr;
  Renderer::D3D11Renderer *m_Renderer = nullptr;
  IconCache *m_IconCache = nullptr;
  char m_SearchQueryBuf[256] = "";
  std::wstring m_CurrentQuery;
  std::vector<unsigned long long> m_SearchResults;
  std::mutex m_ResultsMutex;

  std::atomic<bool> m_IsScanning = true;
  std::atomic<size_t> m_IndexedCount = 0;
  std::atomic<uint64_t> m_SearchTimeMs = 0;
  std::atomic<bool> m_SearchPending = false;
};

} // namespace PulseFS::UI
