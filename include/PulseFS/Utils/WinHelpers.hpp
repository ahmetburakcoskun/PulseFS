#pragma once

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <windows.h>

namespace PulseFS::Utils {

struct HandleDeleter {
  void operator()(HANDLE h) const {
    if (h && h != INVALID_HANDLE_VALUE) {
      ::CloseHandle(h);
    }
  }
};

using ScopedHandle = std::unique_ptr<void, HandleDeleter>;

inline bool IsRunAsAdmin() {
  BOOL fIsRunAsAdmin = FALSE;
  SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
  PSID pAdminSID = NULL;

  if (::AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                 &pAdminSID)) {
    if (!::CheckTokenMembership(NULL, pAdminSID, &fIsRunAsAdmin)) {
      fIsRunAsAdmin = FALSE;
    }
    ::FreeSid(pAdminSID);
  }
  return fIsRunAsAdmin == TRUE;
}

inline ScopedHandle OpenVolume(const std::wstring &volumePath) {
  HANDLE h = ::CreateFileW(volumePath.c_str(), GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_EXISTING, 0, NULL);

  if (h == INVALID_HANDLE_VALUE) {

    return nullptr;
  }
  return ScopedHandle(h);
}

} // namespace PulseFS::Utils
