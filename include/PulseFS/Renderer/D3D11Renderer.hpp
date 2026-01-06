#pragma once

#include <Windows.h>
#include <d3d11.h>


namespace PulseFS::Renderer {

class D3D11Renderer {
public:
  D3D11Renderer() = default;
  ~D3D11Renderer();

  D3D11Renderer(const D3D11Renderer &) = delete;
  D3D11Renderer &operator=(const D3D11Renderer &) = delete;

  bool Initialize(HWND hWnd);
  void Shutdown();

  void BeginFrame(const float clearColor[4]);
  void EndFrame();
  void OnResize(UINT width, UINT height);

  [[nodiscard]] ID3D11Device *GetDevice() const { return m_Device; }
  [[nodiscard]] ID3D11DeviceContext *GetDeviceContext() const {
    return m_DeviceContext;
  }

private:
  void CreateRenderTarget();
  void CleanupRenderTarget();

  ID3D11Device *m_Device = nullptr;
  ID3D11DeviceContext *m_DeviceContext = nullptr;
  IDXGISwapChain *m_SwapChain = nullptr;
  ID3D11RenderTargetView *m_RenderTargetView = nullptr;
};

} // namespace PulseFS::Renderer
