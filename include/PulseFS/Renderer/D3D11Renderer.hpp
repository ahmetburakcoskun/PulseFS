#pragma once

#include "PulseFS/Utils/Result.hpp"
#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>


namespace PulseFS::Renderer {

class D3D11Renderer {
public:
  D3D11Renderer() = default;
  ~D3D11Renderer();

  D3D11Renderer(const D3D11Renderer &) = delete;
  D3D11Renderer &operator=(const D3D11Renderer &) = delete;

  D3D11Renderer(D3D11Renderer &&other) noexcept;
  D3D11Renderer &operator=(D3D11Renderer &&other) noexcept;

  [[nodiscard]] PulseFS::Utils::Result<void> Initialize(HWND hWnd);
  void Shutdown() noexcept;

  void BeginFrame(const float clearColor[4]) noexcept;
  void EndFrame() noexcept;
  [[nodiscard]] PulseFS::Utils::Result<void> OnResize(UINT width, UINT height);

  [[nodiscard]] ID3D11Device *GetDevice() const noexcept {
    return m_device.Get();
  }
  [[nodiscard]] ID3D11DeviceContext *GetDeviceContext() const noexcept {
    return m_deviceContext.Get();
  }
  [[nodiscard]] bool IsInitialized() const noexcept {
    return m_device != nullptr;
  }

private:
  [[nodiscard]] PulseFS::Utils::Result<void> CreateRenderTarget();
  void CleanupRenderTarget() noexcept;

  Microsoft::WRL::ComPtr<ID3D11Device> m_device;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_deviceContext;
  Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
};

} // namespace PulseFS::Renderer
