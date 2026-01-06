#pragma once

#include <Windows.h>
#include <d3d11.h>


namespace PulseFS::ImGuiLayer {

class ImGuiManager {
public:
  static void Initialize(HWND hWnd, ID3D11Device *device,
                         ID3D11DeviceContext *context);
  static void Shutdown();
  static void BeginFrame();
  static void EndFrame();
};

} // namespace PulseFS::ImGuiLayer
