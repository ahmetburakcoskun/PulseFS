#include "PulseFS/Platform/Win32Window.hpp"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include <memory>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

namespace PulseFS::Platform {

Win32Window::~Win32Window() { Destroy(); }

bool Win32Window::Create(const wchar_t *title, int width, int height) {
  m_WindowClass = {
      sizeof(WNDCLASSEXW),      CS_CLASSDC, WndProc, 0L,      0L,
      GetModuleHandle(nullptr), nullptr,    nullptr, nullptr, nullptr,
      L"PulseFSWindowClass",    nullptr};

  if (!::RegisterClassExW(&m_WindowClass)) {
    return false;
  }

  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);
  int xPos = (screenWidth - width) / 2;
  int yPos = (screenHeight - height) / 2;

  m_hWnd = ::CreateWindowW(m_WindowClass.lpszClassName, title,
                           WS_OVERLAPPEDWINDOW, xPos, yPos, width, height,
                           nullptr, nullptr, m_WindowClass.hInstance, nullptr);

  if (!m_hWnd) {
    return false;
  }

  ::SetWindowLongPtr(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

  EnableDarkMode();
  return true;
}

void Win32Window::Destroy() {
  if (m_hWnd) {
    ::DestroyWindow(m_hWnd);
    m_hWnd = nullptr;
  }
  if (m_WindowClass.lpszClassName) {
    ::UnregisterClassW(m_WindowClass.lpszClassName, m_WindowClass.hInstance);
    m_WindowClass = {};
  }
}

bool Win32Window::ProcessMessages() {
  MSG msg;
  while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
    if (msg.message == WM_QUIT) {
      return false;
    }
  }
  return true;
}

void Win32Window::Show() {
  ::ShowWindow(m_hWnd, SW_SHOWDEFAULT);
  ::UpdateWindow(m_hWnd);
}

void Win32Window::EnableDarkMode() {
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

  struct LibraryDeleter {
    void operator()(HMODULE h) const {
      if (h) {
        ::FreeLibrary(h);
      }
    }
  };
  using ScopedLibrary =
      std::unique_ptr<std::remove_pointer_t<HMODULE>, LibraryDeleter>;

  ScopedLibrary hDwmapi(::LoadLibrary(TEXT("dwmapi.dll")));
  if (!hDwmapi) {
    return;
  }

  using DwmSetWindowAttributeFunc =
      HRESULT(WINAPI *)(HWND, DWORD, LPCVOID, DWORD);
  auto setWindowAttribute = reinterpret_cast<DwmSetWindowAttributeFunc>(
      ::GetProcAddress(hDwmapi.get(), "DwmSetWindowAttribute"));

  if (setWindowAttribute) {
    BOOL useDarkMode = TRUE;
    setWindowAttribute(m_hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode,
                       sizeof(useDarkMode));
  }
}

LRESULT CALLBACK Win32Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam,
                                      LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
    return true;
  }

  switch (msg) {
  case WM_SIZE:
    if (wParam != SIZE_MINIMIZED) {
      auto *window = reinterpret_cast<Win32Window *>(
          ::GetWindowLongPtr(hWnd, GWLP_USERDATA));
      if (window) {
        window->m_resizeWidth = static_cast<UINT>(LOWORD(lParam));
        window->m_resizeHeight = static_cast<UINT>(HIWORD(lParam));
      }
    }
    return 0;
  case WM_SYSCOMMAND:
    if ((wParam & 0xfff0) == SC_KEYMENU) {
      return 0;
    }
    break;
  case WM_DESTROY:
    ::PostQuitMessage(0);
    return 0;
  }
  return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

std::pair<UINT, UINT> Win32Window::GetPendingResize() const {
  return {m_resizeWidth, m_resizeHeight};
}

void Win32Window::ClearPendingResize() {
  m_resizeWidth = 0;
  m_resizeHeight = 0;
}

bool Win32Window::HasPendingResize() const {
  return m_resizeWidth != 0 && m_resizeHeight != 0;
}

} // namespace PulseFS::Platform
