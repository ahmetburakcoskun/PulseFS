#include "PulseFS/Core/MftScanner.hpp"
#include "PulseFS/Utils/WinHelpers.hpp"
#include <iostream>
#include <vector>
#include <windows.h>
#include <winioctl.h>

namespace PulseFS::Core {

long long MftScanner::Enumerate(Engine::SearchIndex &index,
                                const std::wstring &volumePath) {

  auto hVol = Utils::OpenVolume(volumePath);
  if (!hVol) {
    throw std::runtime_error("Failed to open volume for MFT scanning.");
  }

  USN_JOURNAL_DATA_V0 journalData = {0};
  DWORD bytesReturned = 0;
  if (!::DeviceIoControl(hVol.get(), FSCTL_QUERY_USN_JOURNAL, NULL, 0,
                         &journalData, sizeof(journalData), &bytesReturned,
                         NULL)) {
    throw std::runtime_error("Failed to query USN Journal.");
  }

  USN nextUsn = journalData.NextUsn;

  MFT_ENUM_DATA med = {0};
  med.StartFileReferenceNumber = 0;
  med.LowUsn = 0;
  med.HighUsn = nextUsn;
  med.MinMajorVersion = 2;
  med.MaxMajorVersion = 2;

  const int BUF_LEN = 65536;
  std::vector<char> buffer(BUF_LEN);

  index.Reserve(2000000);

  bool finished = false;
  size_t fileCount = 0;

  while (!finished) {
    if (::DeviceIoControl(hVol.get(), FSCTL_ENUM_USN_DATA, &med, sizeof(med),
                          &buffer[0], BUF_LEN, &bytesReturned, NULL)) {

      DWORDLONG *pNextFileRef = (DWORDLONG *)&buffer[0];
      med.StartFileReferenceNumber = *pNextFileRef;

      DWORD offset = sizeof(DWORDLONG);
      while (offset < bytesReturned) {
        PUSN_RECORD_V2 pRecord = (PUSN_RECORD_V2)(&buffer[offset]);

        std::wstring fileName(pRecord->FileName,
                              pRecord->FileNameLength / sizeof(WCHAR));
        if (!fileName.empty()) {
          index.Insert({std::move(fileName), pRecord->FileReferenceNumber,
                        pRecord->ParentFileReferenceNumber, true});
          fileCount++;
        }

        offset += pRecord->RecordLength;
      }
    } else {

      finished = true;
    }
  }

  return nextUsn;
}
} // namespace PulseFS::Core
