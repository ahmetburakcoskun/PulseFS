#pragma once

#include "PulseFS/Engine/SearchIndex.hpp"
#include <string>

namespace PulseFS::Core {

class MftScanner {
public:
  static long long Enumerate(Engine::SearchIndex &index,
                             const std::wstring &volumePath);
};

} // namespace PulseFS::Core
