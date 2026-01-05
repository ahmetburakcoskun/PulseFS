#pragma once

#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace PulseFS::Engine {

struct FileEntry {
  std::wstring name;
  unsigned long long id;
  unsigned long long parentId;
  bool active = true;
};

class SearchIndex {
public:
  SearchIndex();
  ~SearchIndex();

  SearchIndex(const SearchIndex &) = delete;
  SearchIndex &operator=(const SearchIndex &) = delete;

  void Reserve(size_t capacity);

  void Insert(const FileEntry &entry);

  void Remove(unsigned long long id);

  void Rename(unsigned long long id, const std::wstring &newName,
              unsigned long long newParentId);

  std::vector<unsigned long long> Search(std::wstring_view query,
                                         size_t maxResults = 20) const;

  std::wstring GetFullPath(unsigned long long id) const;

  size_t Count() const;

private:
  std::wstring ResolvePathInternal(unsigned long long id) const;

  std::vector<FileEntry> m_files;
  std::unordered_map<unsigned long long, size_t> m_idToIndex;
  mutable std::shared_mutex m_mutex;
};

} // namespace PulseFS::Engine
