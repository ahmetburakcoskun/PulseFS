#include "PulseFS/UI/MainWindow.hpp"
#include "PulseFS/Core/MftScanner.hpp"
#include "PulseFS/Core/UsnMonitor.hpp"
#include "PulseFS/Engine/SearchIndex.hpp"
#include "PulseFS/ImGui/ImGuiManager.hpp"
#include "PulseFS/Platform/Win32Window.hpp"
#include "PulseFS/Renderer/D3D11Renderer.hpp"
#include "PulseFS/UI/IconCache.hpp"
#include "PulseFS/UI/SearchPanel.hpp"

#include "imgui.h"
#include "imgui_impl_dx11.h"

#include <atomic>
#include <thread>

namespace PulseFS::UI {

namespace {
Engine::SearchIndex g_searchIndex;
std::atomic<bool> g_isScanning = true;
} // namespace

static void MftWorker(SearchPanel &panel) {
  const std::wstring volume = L"\\\\.\\C:";
  long long nextUsn = Core::MftScanner::Enumerate(g_searchIndex, volume);

  panel.SetIndexedCount(g_searchIndex.Count());
  panel.SetScanning(false);
  g_isScanning = false;

  static Core::UsnMonitor monitor(g_searchIndex);
  monitor.Start(volume, nextUsn);
}

void MainWindow::Run() {
  Platform::Win32Window window;
  if (!window.Create(L"PulseFS - Fast MFT Search", 1024, 768)) {
    return;
  }

  Renderer::D3D11Renderer renderer;
  auto initResult = renderer.Initialize(window.GetHandle());
  if (!initResult) {
    return;
  }

  window.Show();

  ImGuiLayer::ImGuiManager::Initialize(window.GetHandle(), renderer.GetDevice(),
                                       renderer.GetDeviceContext());

  IconCache iconCache(renderer.GetDevice());
  SearchPanel searchPanel;
  searchPanel.Initialize(g_searchIndex, &renderer, &iconCache);

  std::thread mftThread(MftWorker, std::ref(searchPanel));
  mftThread.detach();

  constexpr float clearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};

  while (window.ProcessMessages()) {
    auto [resizeW, resizeH] = window.GetPendingResize();
    if (resizeW != 0 && resizeH != 0) {
      renderer.OnResize(resizeW, resizeH);
      window.ClearPendingResize();
    }

    ImGuiLayer::ImGuiManager::BeginFrame();
    searchPanel.Render();
    ImGuiLayer::ImGuiManager::EndFrame();

    renderer.BeginFrame(clearColor);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    renderer.EndFrame();
  }

  ImGuiLayer::ImGuiManager::Shutdown();
}

} // namespace PulseFS::UI
