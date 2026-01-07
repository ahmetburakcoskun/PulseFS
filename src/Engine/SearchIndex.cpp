#include "PulseFS/Engine/SearchIndex.hpp"
#include <algorithm>
#include <cwctype>
#include <iostream>

namespace PulseFS::Engine {

SearchIndex::SearchIndex() {

  m_files.reserve(100000);
  m_idToIndex.reserve(100000);
}

SearchIndex::~SearchIndex() = default;

void SearchIndex::Reserve(size_t capacity) {
  std::unique_lock lock(m_mutex);
  m_files.reserve(capacity);
  m_idToIndex.reserve(capacity);
}

void SearchIndex::Insert(const FileEntry &entry) {
  std::unique_lock lock(m_mutex);

  if (auto it = m_idToIndex.find(entry.id); it != m_idToIndex.end()) {
    size_t idx = it->second;
    m_files[idx] = entry;
    m_files[idx].active = true;
  } else {
    m_files.push_back(entry);
    m_idToIndex[entry.id] = m_files.size() - 1;
  }
}

void SearchIndex::Remove(unsigned long long id) {
  std::unique_lock lock(m_mutex);
  if (auto it = m_idToIndex.find(id); it != m_idToIndex.end()) {
    size_t idx = it->second;
    m_files[idx].active = false;
    m_idToIndex.erase(it);
  }
}

void SearchIndex::Rename(unsigned long long id, const std::wstring &newName,
                         unsigned long long newParentId) {
  std::unique_lock lock(m_mutex);
  if (auto it = m_idToIndex.find(id); it != m_idToIndex.end()) {
    size_t idx = it->second;
    m_files[idx].name = newName;
    m_files[idx].parentId = newParentId;
    m_files[idx].active = true;
  } else {

    Insert({newName, id, newParentId, true});
  }
}

std::vector<unsigned long long> SearchIndex::Search(std::wstring_view query,
                                                    size_t maxResults) const {
  std::shared_lock lock(m_mutex);
  std::vector<unsigned long long> results;
  results.reserve(maxResults);

  int count = 0;

  for (const auto &file : m_files) {
    if (!file.active)
      continue;

    auto it = std::search(file.name.begin(), file.name.end(), query.begin(),
                          query.end(), [](wchar_t a, wchar_t b) {
                            return std::towlower(a) == std::towlower(b);
                          });

    if (it != file.name.end()) {
      results.push_back(file.id);
      if (++count >= static_cast<int>(maxResults))
        break;
    }
  }
  return results;
}

std::wstring SearchIndex::ResolvePathInternal(unsigned long long id) const {
  std::wstring path;
  unsigned long long currentId = id;
  int safety = 0;

  auto it = m_idToIndex.find(currentId);
  if (it == m_idToIndex.end())
    return L"";

  path = m_files[it->second].name;
  currentId = m_files[it->second].parentId;

  while (safety++ < 256) {
    auto parentIt = m_idToIndex.find(currentId);
    if (parentIt == m_idToIndex.end())
      break;

    const auto &file = m_files[parentIt->second];

    if (file.id == file.parentId) {

      break;
    }

    path = file.name + L"\\" + path;
    currentId = file.parentId;
  }
  return L"C:\\" + path;
}

std::wstring SearchIndex::GetFullPath(unsigned long long id) const {
  std::shared_lock lock(m_mutex);
  return ResolvePathInternal(id);
}

unsigned long SearchIndex::GetAttributes(unsigned long long id) const {
  std::shared_lock lock(m_mutex);
  if (auto it = m_idToIndex.find(id); it != m_idToIndex.end()) {
    size_t idx = it->second;
    return m_files[idx].fileAttributes;
  }
  return 0;
}

size_t SearchIndex::Count() const {
  std::shared_lock lock(m_mutex);
  return m_idToIndex.size();
}
} // namespace PulseFS::Engine
