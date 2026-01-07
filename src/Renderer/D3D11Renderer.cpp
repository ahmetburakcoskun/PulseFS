#include "PulseFS/Renderer/D3D11Renderer.hpp"
#include <utility>

#pragma comment(lib, "d3d11.lib")

namespace PulseFS::Renderer {

D3D11Renderer::~D3D11Renderer() { Shutdown(); }

D3D11Renderer::D3D11Renderer(D3D11Renderer &&other) noexcept
    : m_device(std::move(other.m_device)),
      m_deviceContext(std::move(other.m_deviceContext)),
      m_swapChain(std::move(other.m_swapChain)),
      m_renderTargetView(std::move(other.m_renderTargetView)) {}

D3D11Renderer &D3D11Renderer::operator=(D3D11Renderer &&other) noexcept {
  if (this != &other) {
    Shutdown();
    m_device = std::move(other.m_device);
    m_deviceContext = std::move(other.m_deviceContext);
    m_swapChain = std::move(other.m_swapChain);
    m_renderTargetView = std::move(other.m_renderTargetView);
  }
  return *this;
}

PulseFS::Utils::Result<void> D3D11Renderer::Initialize(HWND hWnd) {
  if (!hWnd) {
    return PulseFS::Utils::Err<std::string>("Invalid window handle");
  }

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
  constexpr UINT numFeatureLevels =
      static_cast<UINT>(std::size(featureLevelArray));

  D3D_FEATURE_LEVEL featureLevel{};
  HRESULT hr = D3D11CreateDeviceAndSwapChain(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
      featureLevelArray, numFeatureLevels, D3D11_SDK_VERSION, &sd,
      m_swapChain.GetAddressOf(), m_device.GetAddressOf(), &featureLevel,
      m_deviceContext.GetAddressOf());

  if (hr == DXGI_ERROR_UNSUPPORTED) {
    hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
        featureLevelArray, numFeatureLevels, D3D11_SDK_VERSION, &sd,
        m_swapChain.GetAddressOf(), m_device.GetAddressOf(), &featureLevel,
        m_deviceContext.GetAddressOf());
  }

  if (FAILED(hr)) {
    return PulseFS::Utils::Err<std::string>(
        "Failed to create D3D11 device and swap chain (HRESULT: 0x" +
        std::to_string(static_cast<unsigned long>(hr)) + ")");
  }

  return CreateRenderTarget();
}

void D3D11Renderer::Shutdown() noexcept {
  CleanupRenderTarget();
  m_swapChain.Reset();
  m_deviceContext.Reset();
  m_device.Reset();
}

void D3D11Renderer::BeginFrame(const float clearColor[4]) noexcept {
  if (m_deviceContext && m_renderTargetView) {
    m_deviceContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(),
                                        nullptr);
    m_deviceContext->ClearRenderTargetView(m_renderTargetView.Get(),
                                           clearColor);
  }
}

void D3D11Renderer::EndFrame() noexcept {
  if (m_swapChain) {
    m_swapChain->Present(1, 0);
  }
}

PulseFS::Utils::Result<void> D3D11Renderer::OnResize(UINT width, UINT height) {
  if (!m_swapChain) {
    return PulseFS::Utils::Err<std::string>("Swap chain not initialized");
  }

  CleanupRenderTarget();

  const HRESULT hr =
      m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
  if (FAILED(hr)) {
    return PulseFS::Utils::Err<std::string>(
        "Failed to resize swap chain buffers (HRESULT: 0x" +
        std::to_string(static_cast<unsigned long>(hr)) + ")");
  }

  return CreateRenderTarget();
}

PulseFS::Utils::Result<void> D3D11Renderer::CreateRenderTarget() {
  Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
  HRESULT hr =
      m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));

  if (FAILED(hr)) {
    return PulseFS::Utils::Err<std::string>(
        "Failed to get back buffer from swap chain (HRESULT: 0x" +
        std::to_string(static_cast<unsigned long>(hr)) + ")");
  }

  hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr,
                                        m_renderTargetView.GetAddressOf());
  if (FAILED(hr)) {
    return PulseFS::Utils::Err<std::string>(
        "Failed to create render target view (HRESULT: 0x" +
        std::to_string(static_cast<unsigned long>(hr)) + ")");
  }

  return PulseFS::Utils::Ok<std::string>();
}

void D3D11Renderer::CleanupRenderTarget() noexcept {
  m_renderTargetView.Reset();
}

} // namespace PulseFS::Renderer
