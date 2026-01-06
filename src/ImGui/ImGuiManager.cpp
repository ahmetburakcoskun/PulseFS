#include "PulseFS/ImGui/ImGuiManager.hpp"
#include "PulseFS/ImGui/ImGuiTheme.hpp"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

namespace PulseFS::ImGuiLayer {

void ImGuiManager::Initialize(HWND hWnd, ID3D11Device *device,
                              ID3D11DeviceContext *context) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGuiTheme::ApplyPulseFSTheme();

  ImGui_ImplWin32_Init(hWnd);
  ImGui_ImplDX11_Init(device, context);
}

void ImGuiManager::Shutdown() {
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiManager::BeginFrame() {
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
}

void ImGuiManager::EndFrame() { ImGui::Render(); }

} // namespace PulseFS::ImGuiLayer
