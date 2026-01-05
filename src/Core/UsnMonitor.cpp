#include "PulseFS/Core/UsnMonitor.hpp"
#include "PulseFS/Utils/WinHelpers.hpp"
#include <iostream>
#include <vector>
#include <windows.h>
#include <winioctl.h>

namespace PulseFS::Core {

UsnMonitor::UsnMonitor(Engine::SearchIndex &index)
    : m_index(index), m_running(false) {}

UsnMonitor::~UsnMonitor() { Stop(); }

void UsnMonitor::Start(const std::wstring &volumePath, long long startUsn) {
  Stop();
  m_running = true;
  m_thread = std::thread(&UsnMonitor::MonitorLoop, this, volumePath, startUsn);
}

void UsnMonitor::Stop() {
  m_running = false;
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void UsnMonitor::MonitorLoop(std::wstring volumePath, long long startUsn) {
  auto hVol = Utils::OpenVolume(volumePath);
  if (!hVol)
    return;

  USN_JOURNAL_DATA_V0 journalData = {0};
  DWORD bytesReturned;
  if (!::DeviceIoControl(hVol.get(), FSCTL_QUERY_USN_JOURNAL, NULL, 0,
                         &journalData, sizeof(journalData), &bytesReturned,
                         NULL)) {
    return;
  }

  READ_USN_JOURNAL_DATA readData = {0};
  readData.StartUsn = startUsn;
  readData.ReasonMask = 0xFFFFFFFF;
  readData.ReturnOnlyOnClose = FALSE;
  readData.Timeout = 1;
  readData.BytesToWaitFor = 0;
  readData.UsnJournalID = journalData.UsnJournalID;
  readData.MinMajorVersion = 2;
  readData.MaxMajorVersion = 2;

  const int BUF_LEN = 65536;
  std::vector<char> buffer(BUF_LEN);

  while (m_running) {
    DWORD bytesRead = 0;
    BOOL success = ::DeviceIoControl(hVol.get(), FSCTL_READ_USN_JOURNAL,
                                     &readData, sizeof(readData), &buffer[0],
                                     BUF_LEN, &bytesRead, NULL);

    if (success) {
      if (bytesRead > sizeof(USN)) {

        readData.StartUsn = *((USN *)&buffer[0]);

        DWORD offset = sizeof(USN);
        while (offset < bytesRead) {
          PUSN_RECORD_V2 pRecord = (PUSN_RECORD_V2)(&buffer[offset]);

          std::wstring name(pRecord->FileName,
                            pRecord->FileNameLength / sizeof(WCHAR));

          if (pRecord->Reason & USN_REASON_FILE_CREATE) {
            m_index.Insert({name, pRecord->FileReferenceNumber,
                            pRecord->ParentFileReferenceNumber, true});

          } else if (pRecord->Reason & USN_REASON_FILE_DELETE) {
            m_index.Remove(pRecord->FileReferenceNumber);
          } else if (pRecord->Reason & USN_REASON_RENAME_NEW_NAME) {
            m_index.Rename(pRecord->FileReferenceNumber, name,
                           pRecord->ParentFileReferenceNumber);
          }

          offset += pRecord->RecordLength;
        }

      } else {

        readData.StartUsn = *((USN *)&buffer[0]);
      }
    } else {

      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }
}
} // namespace PulseFS::Core
