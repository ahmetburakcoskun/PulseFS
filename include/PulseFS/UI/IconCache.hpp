#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>

namespace PulseFS::UI {

class IconCache {
public:
  IconCache(ID3D11Device* device);
  ~IconCache();

  IconCache(const IconCache&) = delete;
  IconCache& operator=(const IconCache&) = delete;

  ID3D11ShaderResourceView* GetIcon(const std::wstring& path, bool isDirectory);
  ID3D11ShaderResourceView* GetImageThumbnail(const std::wstring& path);

  void Clear();

private:
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateIconTexture(HICON hIcon);

  HICON GetShellIcon(const std::wstring& path, bool isDirectory);

  ID3D11Device* m_device;
  ID3D11DeviceContext* m_deviceContext;

  std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_iconCache;
  std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_thumbnailCache;

  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_folderIcon;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_defaultFileIcon;
};

} // namespace PulseFS::UI
