#pragma once

#include <Windows.h>
#include <utility>

namespace PulseFS::Platform {

class Win32Window {
public:
  Win32Window() = default;
  ~Win32Window();

  Win32Window(const Win32Window &) = delete;
  Win32Window &operator=(const Win32Window &) = delete;

  bool Create(const wchar_t *title, int width, int height);
  void Destroy();
  bool ProcessMessages();
  void Show();

  [[nodiscard]] HWND GetHandle() const { return m_hWnd; }
  [[nodiscard]] std::pair<UINT, UINT> GetPendingResize() const;
  void ClearPendingResize();
  [[nodiscard]] bool HasPendingResize() const;

private:
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam,
                                  LPARAM lParam);
  void EnableDarkMode();

  HWND m_hWnd = nullptr;
  WNDCLASSEXW m_WindowClass{};

  UINT m_resizeWidth = 0;
  UINT m_resizeHeight = 0;
};

} // namespace PulseFS::Platform
