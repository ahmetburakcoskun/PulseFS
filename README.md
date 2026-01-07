# PulseFS

PulseFS is a high-performance Windows search utility designed to bypass the latency of the standard Windows Indexing Service. It interacts directly with the NTFS Master File Table (MFT) for initial indexing and monitors the Update Sequence Number (USN) Change Journal for real-time file system synchronization.

The primary goal of this project is to provide a transparent, open-source alternative to closed-source NTFS search tools, demonstrating low-level file system interactions using modern C++.

## Performance Metrics

- **Indexing Speed:** Approximately 836,000 files in under 2 seconds.
- **Search Latency:** Near-instant results (~100ms) via optimized in-memory metadata storage.
- **Memory Footprint:** Roughly 200MB of RAM for a standard 1TB system volume.

## Technical Architecture

- **MFT Parsing:** Opens raw volume handles (e.g., `\\.\C:`) to parse the Master File Table directly, avoiding the overhead of recursive Win32 directory crawling.
- **Live Synchronization:** Hooks into the NTFS USN Journal to detect file creation, deletion, and renaming events without re-scanning the disk.
- **Modular Design:** Architected into distinct modules for scanning, monitoring, and search indexing to ensure maintainability and performance.
- **Zero-Copy Search:** Utilizes `std::wstring_view` and efficient data structures to minimize memory allocations during query execution.

## Project Structure

- `include/PulseFS/Core`: Headers for MFT scanning and USN monitoring.
- `include/PulseFS/Engine`: Headers for search logic and memory indexing.
- `src/`: Implementation files and application entry point.
- `CMakeLists.txt`: Build configuration for Windows C++ environments.

## Build Instructions

PulseFS requires **CMake**. Note that administrative privileges are mandatory to acquire raw disk handles.

Bash
**Prerequisites:**

- CMake 3.15+
- C++20 compatible compiler (MSVC 19.29+, GCC 10+, Clang 10+)
- Windows 10/11 (NTFS Volume)
```
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Roadmap

- [x] Transitioning from CLI to a graphical user interface using Dear ImGui.
- [ ] Implementing multi-threaded indexing for high-capacity storage arrays.
- [ ] Adding advanced query support (Regex, file size filters, and extension flags).
- [ ] Multi-volume support for concurrent indexing of multiple NTFS drives.

## License

This project is licensed under the MIT License.
