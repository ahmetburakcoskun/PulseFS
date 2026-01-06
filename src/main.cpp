#include "PulseFS/UI/MainWindow.hpp"
#include "PulseFS/Utils/WinHelpers.hpp"
#include <iostream>
#include <windows.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PWSTR pCmdLine, int nCmdShow) {
  if (!PulseFS::Utils::IsRunAsAdmin()) {
    MessageBoxA(NULL, "Administrator privileges required for MFT access.",
                "PulseFS Error", MB_ICONERROR);
    return 1;
  }

  PulseFS::UI::MainWindow::Run();

  return 0;
}
