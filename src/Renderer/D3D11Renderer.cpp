#include "PulseFS/Renderer/D3D11Renderer.hpp"

#pragma comment(lib, "d3d11.lib")

namespace PulseFS::Renderer {

D3D11Renderer::~D3D11Renderer() { Shutdown(); }

bool D3D11Renderer::Initialize(HWND hWnd) {
  DXGI_SWAP_CHAIN_DESC sd{};
  sd.BufferCount = 2;
  sd.BufferDesc.Width = 0;
  sd.BufferDesc.Height = 0;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.RefreshRate.Numerator = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = hWnd;
  sd.SampleDesc.Count = 1;
  sd.SampleDesc.Quality = 0;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  constexpr UINT createDeviceFlags = 0;
  constexpr D3D_FEATURE_LEVEL featureLevelArray[] = {
      D3D_FEATURE_LEVEL_11_0,
      D3D_FEATURE_LEVEL_10_0,
  };

  D3D_FEATURE_LEVEL featureLevel;
  HRESULT res = D3D11CreateDeviceAndSwapChain(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
      featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &m_SwapChain, &m_Device,
      &featureLevel, &m_DeviceContext);

  if (res == DXGI_ERROR_UNSUPPORTED) {
    res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &m_SwapChain, &m_Device,
        &featureLevel, &m_DeviceContext);
  }

  if (FAILED(res)) {
    return false;
  }

  CreateRenderTarget();
  return true;
}

void D3D11Renderer::Shutdown() {
  CleanupRenderTarget();

  if (m_SwapChain) {
    m_SwapChain->Release();
    m_SwapChain = nullptr;
  }
  if (m_DeviceContext) {
    m_DeviceContext->Release();
    m_DeviceContext = nullptr;
  }
  if (m_Device) {
    m_Device->Release();
    m_Device = nullptr;
  }
}

void D3D11Renderer::BeginFrame(const float clearColor[4]) {
  m_DeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, nullptr);
  m_DeviceContext->ClearRenderTargetView(m_RenderTargetView, clearColor);
}

void D3D11Renderer::EndFrame() { m_SwapChain->Present(1, 0); }

void D3D11Renderer::OnResize(UINT width, UINT height) {
  CleanupRenderTarget();
  m_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
  CreateRenderTarget();
}

void D3D11Renderer::CreateRenderTarget() {
  ID3D11Texture2D *pBackBuffer = nullptr;
  m_SwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
  if (pBackBuffer) {
    m_Device->CreateRenderTargetView(pBackBuffer, nullptr, &m_RenderTargetView);
    pBackBuffer->Release();
  }
}

void D3D11Renderer::CleanupRenderTarget() {
  if (m_RenderTargetView) {
    m_RenderTargetView->Release();
    m_RenderTargetView = nullptr;
  }
}

} // namespace PulseFS::Renderer
