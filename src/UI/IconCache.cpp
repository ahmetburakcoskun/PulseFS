#include "PulseFS/UI/IconCache.hpp"
#include <algorithm>
#include <shellapi.h>
#include <wincodec.h>
#include <wrl/client.h>

#pragma comment(lib, "windowscodecs.lib")

namespace PulseFS::UI {

IconCache::IconCache(ID3D11Device* device) : m_device(device) {
  if (m_device) {
    m_device->GetImmediateContext(&m_deviceContext);
  }
}

IconCache::~IconCache() {
  Clear();
}

void IconCache::Clear() {
  m_iconCache.clear();
  m_folderIcon.Reset();
  m_defaultFileIcon.Reset();
}

HICON IconCache::GetShellIcon(const std::wstring& path, bool isDirectory) {
  SHFILEINFOW sfi = {0};
  DWORD flags = SHGFI_ICON | SHGFI_SMALLICON;

  if (isDirectory) {
    SHGetFileInfoW(path.c_str(), 0, &sfi, sizeof(sfi), flags);
  } else {
    SHGetFileInfoW(path.c_str(), 0, &sfi, sizeof(sfi), flags);
  }

  return sfi.hIcon;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> IconCache::CreateIconTexture(HICON hIcon) {
  if (!hIcon || !m_device) {
    return nullptr;
  }

  ICONINFO iconInfo;
  if (!GetIconInfo(hIcon, &iconInfo)) {
    return nullptr;
  }

  BITMAP bmp;
  GetObject(iconInfo.hbmColor ? iconInfo.hbmColor : iconInfo.hbmMask, sizeof(BITMAP), &bmp);

  int width = bmp.bmWidth;
  int height = bmp.bmHeight;

  HDC hdcScreen = GetDC(NULL);
  HDC hdcMem = CreateCompatibleDC(hdcScreen);

  BITMAPINFO bmi = {0};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = width;
  bmi.bmiHeader.biHeight = -height;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  void* bits = nullptr;
  HBITMAP hBitmap = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
  
  if (!hBitmap || !bits) {
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);
    return nullptr;
  }

  HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
  DrawIconEx(hdcMem, 0, 0, hIcon, width, height, 0, NULL, DI_NORMAL);
  SelectObject(hdcMem, hOldBitmap);

  D3D11_TEXTURE2D_DESC texDesc = {};
  texDesc.Width = width;
  texDesc.Height = height;
  texDesc.MipLevels = 1;
  texDesc.ArraySize = 1;
  texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  texDesc.SampleDesc.Count = 1;
  texDesc.Usage = D3D11_USAGE_DEFAULT;
  texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

  D3D11_SUBRESOURCE_DATA initData = {};
  initData.pSysMem = bits;
  initData.SysMemPitch = width * 4;

  Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
  HRESULT hr = m_device->CreateTexture2D(&texDesc, &initData, &texture);

  DeleteObject(hBitmap);
  DeleteDC(hdcMem);
  ReleaseDC(NULL, hdcScreen);
  DeleteObject(iconInfo.hbmColor);
  DeleteObject(iconInfo.hbmMask);

  if (FAILED(hr)) {
    return nullptr;
  }

  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Format = texDesc.Format;
  srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels = 1;

  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
  hr = m_device->CreateShaderResourceView(texture.Get(), &srvDesc, &srv);

  if (FAILED(hr)) {
    return nullptr;
  }

  return srv;
}

ID3D11ShaderResourceView* IconCache::GetIcon(const std::wstring& path, bool isDirectory) {
  if (!m_device) {
    return nullptr;
  }

  if (isDirectory) {
    auto it = m_iconCache.find(path);
    if (it != m_iconCache.end()) {
      return it->second.Get();
    }
    
    HICON hIcon = GetShellIcon(path, true);
    if (!hIcon) {
      if (!m_folderIcon) {
        hIcon = GetShellIcon(L"C:\\", true);
        m_folderIcon = CreateIconTexture(hIcon);
        if (hIcon) DestroyIcon(hIcon);
      }
      return m_folderIcon.Get();
    }
    
    auto texture = CreateIconTexture(hIcon);
    DestroyIcon(hIcon);
    if (texture) {
      m_iconCache[path] = texture;
      return texture.Get();
    }
    return m_folderIcon.Get();
  }

  size_t dotPos = path.find_last_of(L'.');
  std::wstring ext;
  
  if (dotPos != std::wstring::npos && dotPos < path.length() - 1) {
    ext = path.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
  } else {
    ext = L".unknown";
  }

  bool hasCustomIcon = (ext == L".exe" || ext == L".lnk" || ext == L".ico" || 
                        ext == L".dll" || ext == L".scr" || ext == L".cpl");

  if (hasCustomIcon) {
    auto it = m_iconCache.find(path);
    if (it != m_iconCache.end()) {
      return it->second.Get();
    }
    
    HICON hIcon = GetShellIcon(path, false);
    if (hIcon) {
      auto texture = CreateIconTexture(hIcon);
      DestroyIcon(hIcon);
      if (texture) {
        m_iconCache[path] = texture;
        return texture.Get();
      }
    }
    if (!m_defaultFileIcon) {
      hIcon = GetShellIcon(L"C:\\file.txt", false);
      m_defaultFileIcon = CreateIconTexture(hIcon);
      if (hIcon) DestroyIcon(hIcon);
    }
    return m_defaultFileIcon.Get();
  }

  auto it = m_iconCache.find(ext);
  if (it != m_iconCache.end()) {
    return it->second.Get();
  }

  HICON hIcon = GetShellIcon(path, false);
  
  if (!hIcon) {
    if (!m_defaultFileIcon) {
      hIcon = GetShellIcon(L"C:\\file.txt", false);
      m_defaultFileIcon = CreateIconTexture(hIcon);
      if (hIcon) DestroyIcon(hIcon);
    }
    return m_defaultFileIcon.Get();
  }

  auto texture = CreateIconTexture(hIcon);
  DestroyIcon(hIcon);

  if (texture) {
    m_iconCache[ext] = texture;
    return texture.Get();
  }

  return nullptr;
}

ID3D11ShaderResourceView* IconCache::GetImageThumbnail(const std::wstring& path) {
  if (!m_device) {
    return nullptr;
  }

  auto it = m_thumbnailCache.find(path);
  if (it != m_thumbnailCache.end()) {
    return it->second.Get();
  }

  CoInitialize(nullptr);

  Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory;
  HRESULT hr = CoCreateInstance(
    CLSID_WICImagingFactory,
    nullptr,
    CLSCTX_INPROC_SERVER,
    IID_PPV_ARGS(&wicFactory)
  );

  if (FAILED(hr)) {
    return nullptr;
  }

  Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
  hr = wicFactory->CreateDecoderFromFilename(
    path.c_str(),
    nullptr,
    GENERIC_READ,
    WICDecodeMetadataCacheOnDemand,
    &decoder
  );

  if (FAILED(hr)) {
    return nullptr;
  }

  Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
  hr = decoder->GetFrame(0, &frame);
  if (FAILED(hr)) {
    return nullptr;
  }

  UINT width = 0, height = 0;
  frame->GetSize(&width, &height);

  const UINT maxSize = 256;
  UINT thumbWidth = width;
  UINT thumbHeight = height;
  
  if (width > maxSize || height > maxSize) {
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    if (width > height) {
      thumbWidth = maxSize;
      thumbHeight = static_cast<UINT>(maxSize / aspect);
    } else {
      thumbHeight = maxSize;
      thumbWidth = static_cast<UINT>(maxSize * aspect);
    }
  }

  Microsoft::WRL::ComPtr<IWICBitmapScaler> scaler;
  hr = wicFactory->CreateBitmapScaler(&scaler);
  if (FAILED(hr)) {
    return nullptr;
  }

  hr = scaler->Initialize(frame.Get(), thumbWidth, thumbHeight, WICBitmapInterpolationModeFant);
  if (FAILED(hr)) {
    return nullptr;
  }

  Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
  hr = wicFactory->CreateFormatConverter(&converter);
  if (FAILED(hr)) {
    return nullptr;
  }

  hr = converter->Initialize(
    scaler.Get(),
    GUID_WICPixelFormat32bppBGRA,
    WICBitmapDitherTypeNone,
    nullptr,
    0.0f,
    WICBitmapPaletteTypeCustom
  );

  if (FAILED(hr)) {
    return nullptr;
  }

  UINT stride = thumbWidth * 4;
  UINT bufferSize = stride * thumbHeight;
  std::vector<BYTE> buffer(bufferSize);

  hr = converter->CopyPixels(nullptr, stride, bufferSize, buffer.data());
  if (FAILED(hr)) {
    return nullptr;
  }

  D3D11_TEXTURE2D_DESC texDesc = {};
  texDesc.Width = thumbWidth;
  texDesc.Height = thumbHeight;
  texDesc.MipLevels = 1;
  texDesc.ArraySize = 1;
  texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  texDesc.SampleDesc.Count = 1;
  texDesc.Usage = D3D11_USAGE_DEFAULT;
  texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

  D3D11_SUBRESOURCE_DATA initData = {};
  initData.pSysMem = buffer.data();
  initData.SysMemPitch = stride;

  Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
  hr = m_device->CreateTexture2D(&texDesc, &initData, &texture);
  if (FAILED(hr)) {
    return nullptr;
  }

  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Format = texDesc.Format;
  srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels = 1;

  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
  hr = m_device->CreateShaderResourceView(texture.Get(), &srvDesc, &srv);
  if (FAILED(hr)) {
    return nullptr;
  }

  m_thumbnailCache[path] = srv;
  return srv.Get();
}

} // namespace PulseFS::UI
