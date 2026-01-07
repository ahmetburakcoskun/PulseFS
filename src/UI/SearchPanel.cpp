#include "PulseFS/UI/SearchPanel.hpp"
#include "PulseFS/UI/IconCache.hpp"
#include "imgui.h"
#include <Windows.h>
#include <algorithm>
#include <chrono>
#include <shellapi.h>
#include <thread>


namespace PulseFS::UI {

void SearchPanel::Initialize(Engine::SearchIndex &index, Renderer::D3D11Renderer* renderer, IconCache* iconCache) {
  m_SearchIndex = &index;
  m_Renderer = renderer;
  m_IconCache = iconCache;

  std::thread([this]() {
    std::wstring lastQuery;
    auto lastRefreshTime = std::chrono::steady_clock::now();
    const auto refreshInterval = std::chrono::milliseconds(500);

    while (true) {
      std::wstring query;
      {
        std::lock_guard<std::mutex> lock(m_ResultsMutex);
        query = m_CurrentQuery;
      }

      auto now = std::chrono::steady_clock::now();
      bool shouldRefresh = (now - lastRefreshTime) >= refreshInterval;

      if (m_SearchPending || (shouldRefresh && !query.empty())) {
        if (!query.empty()) {
          auto start = std::chrono::high_resolution_clock::now();
          auto results = m_SearchIndex->Search(query, 100);
          auto end = std::chrono::high_resolution_clock::now();

          {
            std::lock_guard<std::mutex> lock(m_ResultsMutex);
            m_SearchResults = std::move(results);
            m_SearchTimeMs =
                std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                      start)
                    .count();
          }
          lastQuery = query;
          lastRefreshTime = now;
        } else {
          std::lock_guard<std::mutex> lock(m_ResultsMutex);
          m_SearchResults.clear();
          lastQuery.clear();
        }
        m_SearchPending = false;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }).detach();
}

void SearchPanel::Render() {
  if (!m_IsScanning) {
    m_IndexedCount = m_SearchIndex->Count();
  }

  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
      ImGuiWindowFlags_NoDocking;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

  ImGui::Begin("MainDockSpace", nullptr, window_flags);
  ImGui::PopStyleVar(3);

  constexpr float margin = 10.0f;
  ImGui::SetCursorPos(ImVec2(margin, margin));

  if (ImGui::BeginChild("ContentArea", ImVec2(-margin, -margin), false,
                        ImGuiWindowFlags_None)) {
    RenderSearchBar();
    RenderStatusBar();
    ImGui::Separator();
    RenderResultsTable();
  }
  ImGui::EndChild();
  ImGui::End();
}

void SearchPanel::RenderSearchBar() {
  ImGui::SetNextItemWidth(-1.0f);
  if (ImGui::InputText("##Search", m_SearchQueryBuf,
                       IM_ARRAYSIZE(m_SearchQueryBuf))) {
    std::lock_guard<std::mutex> lock(m_ResultsMutex);
    m_CurrentQuery = Utf8ToWide(m_SearchQueryBuf);
    m_SearchPending = true;
  }

  if (!ImGui::IsItemFocused() && ImGui::IsWindowFocused()) {
    ImGui::SetKeyboardFocusHere(-1);
  }
}

void SearchPanel::RenderStatusBar() {
  if (m_IsScanning) {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
                       "Scanning MFT... Please wait.");
  } else {
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                       "Index Ready. %zu files.", m_IndexedCount.load());
    ImGui::SameLine();

    size_t resultCount = 0;
    {
      std::lock_guard<std::mutex> lock(m_ResultsMutex);
      resultCount = m_SearchResults.size();
    }
    ImGui::TextDisabled("| Found %zu results | Search Time: %llu ms",
                        resultCount, m_SearchTimeMs.load());
  }
}

void SearchPanel::RenderResultsTable() {
  if (ImGui::BeginTable("Results", 2,
                        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                            ImGuiTableFlags_BordersOuter |
                            ImGuiTableFlags_SizingStretchProp)) {

    ImGui::TableSetupColumn("File Name", ImGuiTableColumnFlags_WidthStretch,
                            0.3f);
    ImGui::TableSetupColumn("Full Path", ImGuiTableColumnFlags_WidthStretch,
                            0.7f);
    ImGui::TableHeadersRow();

    std::vector<unsigned long long> currentResultsCopy;
    {
      std::lock_guard<std::mutex> lock(m_ResultsMutex);
      currentResultsCopy = m_SearchResults;
    }

    std::sort(currentResultsCopy.begin(), currentResultsCopy.end(),
              [this](unsigned long long idA, unsigned long long idB) {
                unsigned long attrsA = m_SearchIndex->GetAttributes(idA);
                unsigned long attrsB = m_SearchIndex->GetAttributes(idB);
                bool isDirA = (attrsA & FILE_ATTRIBUTE_DIRECTORY) != 0;
                bool isDirB = (attrsB & FILE_ATTRIBUTE_DIRECTORY) != 0;
                
                if (isDirA != isDirB) {
                  return isDirA;
                }
                return false;
              });

    for (auto id : currentResultsCopy) {
      std::wstring fullPath = m_SearchIndex->GetFullPath(id);
      if (fullPath.empty())
        continue;

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);

      std::string fullPathUtf8 = WideToUtf8(fullPath);
      size_t lastSlash = fullPathUtf8.find_last_of('\\');
      std::string fileName = (lastSlash != std::string::npos)
                                 ? fullPathUtf8.substr(lastSlash + 1)
                                 : fullPathUtf8;

      unsigned long fileAttrs = m_SearchIndex->GetAttributes(id);
      bool isDir = (fileAttrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
      
      if (m_IconCache) {
        ID3D11ShaderResourceView* iconTexture = m_IconCache->GetIcon(fullPath, isDir);
        if (iconTexture) {
          ImGui::Image((void*)iconTexture, ImVec2(16, 16));
          ImGui::SameLine();
        }
      }
      
      ImGui::Text("%s", fileName.c_str());

      if (IsImageFile(fullPath) && ImGui::IsItemHovered() && m_IconCache) {
        auto* thumbnail = m_IconCache->GetImageThumbnail(fullPath);
        if (thumbnail) {
          ImGui::BeginTooltip();
          ImGui::Image((void*)thumbnail, ImVec2(256, 256));
          ImGui::EndTooltip();
        }
      }
      
      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        OpenFile(fullPath);
      }

      ImGui::TableSetColumnIndex(1);
      ImGui::TextDisabled("%s", fullPathUtf8.c_str());
    }
    ImGui::EndTable();
  }
}

void SearchPanel::OpenFile(const std::wstring &path) {
  if (path.empty())
    return;
  std::wstring cmd = L"/select,\"" + path + L"\"";
  ShellExecuteW(NULL, L"open", L"explorer.exe", cmd.c_str(), NULL, SW_SHOW);
}

std::string SearchPanel::WideToUtf8(const std::wstring &wstr) {
  if (wstr.empty())
    return std::string();
  int size_needed = WideCharToMultiByte(
      CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
  std::string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()),
                      &strTo[0], size_needed, NULL, NULL);
  return strTo;
}

std::wstring SearchPanel::Utf8ToWide(const std::string &str) {
  if (str.empty())
    return std::wstring();
  int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0],
                                        static_cast<int>(str.size()), NULL, 0);
  std::wstring wstrTo(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()),
                      &wstrTo[0], size_needed);
  return wstrTo;
}

const char* SearchPanel::GetFileIcon(const std::wstring &fileName, unsigned long fileAttributes) {
  if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
    return "[DIR]";
  }
  
  size_t lastDot = fileName.find_last_of(L'.');
  if (lastDot == std::wstring::npos) {
    return "[FILE]";
  }
  
  std::wstring ext = fileName.substr(lastDot + 1);
  std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
  
  if (ext == L"jpg" || ext == L"jpeg" || ext == L"png" || 
      ext == L"bmp" || ext == L"gif" || ext == L"ico" || 
      ext == L"tiff" || ext == L"webp") {
    return "[IMG]";
  }
  else if (ext == L"mp4" || ext == L"avi" || ext == L"mkv" || 
           ext == L"mov" || ext == L"wmv" || ext == L"flv") {
    return "[VID]";
  }
  else if (ext == L"mp3" || ext == L"wav" || ext == L"flac" || 
           ext == L"m4a" || ext == L"wma" || ext == L"ogg") {
    return "[AUD]";
  }
  else if (ext == L"zip" || ext == L"rar" || ext == L"7z" || 
           ext == L"tar" || ext == L"gz") {
    return "[ZIP]";
  }
  else if (ext == L"exe" || ext == L"msi" || ext == L"bat") {
    return "[EXE]";
  }
  else if (ext == L"txt" || ext == L"log") {
    return "[TXT]";
  }
  else if (ext == L"pdf") {
    return "[PDF]";
  }
  else if (ext == L"doc" || ext == L"docx") {
    return "[DOC]";
  }
  else if (ext == L"cpp" || ext == L"h" || ext == L"hpp" || 
           ext == L"c" || ext == L"cs" || ext == L"java" || 
           ext == L"py" || ext == L"js" || ext == L"html" || 
           ext == L"css" || ext == L"json") {
    return "[CODE]";
  }
  
  return "[FILE]";
}

bool SearchPanel::IsImageFile(const std::wstring &fileName) {
  size_t lastDot = fileName.find_last_of(L'.');
  if (lastDot == std::wstring::npos)
    return false;
  
  std::wstring ext = fileName.substr(lastDot + 1);
  std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
  
  return ext == L"jpg" || ext == L"jpeg" || ext == L"png" || 
         ext == L"bmp" || ext == L"gif" || ext == L"ico" || 
         ext == L"tiff" || ext == L"tif" || ext == L"webp";
}

} // namespace PulseFS::UI
