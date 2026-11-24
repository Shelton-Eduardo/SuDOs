# SuDOs
**System-level utilities for Directories & Operating storage**

SuDOs is a lightweight, zero-dependency, low-level C++ library built from scratch for direct unbuffered file I/O and system-level storage control. Designed without templates or heavy standard library stream abstractions, SuDOs gives developers deterministic control over user-space page buffers, sector-aligned disk transfers, durability guarantees, and binary/text stream operations across **Windows** and **Linux**.

---

## Versioning Scheme (`proud.core_feature.shame_bugs`)

SuDOs follows a custom three-part versioning convention:
* **`proud` (Major):** Major architectural milestones (`1` marks full cross-platform unbuffered stream parity between Windows and Linux).
* **`core_feature` (Minor):** Completed subsystems and API capabilities.
* **`shame_bugs` (Patch):** Known rough edges, bug fixes, and polish items.

---

## Roadmap

* **v1.0.0 (Current):** Cross-platform unbuffered file streams (`os::Ifstream` & `os::Ofstream`), custom 4096-byte page-aligned user-space buffering, random-access positioning (`seek`, `tell`), raw binary serialization (`<<` / `>>`), and OS-aware text mode conversions across Windows and Linux.
* **v2.0.0 (Planned):** File and folder manipulation (`rename`, `move`) alongside working directory control (`getcwd`, `chdir`).
* **v3.0.0 (Planned):** Directory enumeration (`listdir`) and recursive directory tree traversal (`walk`).

---

## Platform Architecture & OS Parity (`v1.0.0`)

SuDOs unifies Windows and Linux under a single `os::` interface backed by native OS system calls:

| Feature | Windows (`src/windows.cpp`) | Linux (`src/Linux.cpp`) | Notes |
| :--- | :--- | :--- | :--- |
| **Native File API** | Win32 API (`CreateFileW`, `ReadFile`, `WriteFile`, `SetFilePointerEx`) | POSIX (`open`, `read`, `write`, `lseek`) | 64-bit file offsets (`_FILE_OFFSET_BITS 64` on Linux) |
| **OS Cache Bypass** | `FILE_FLAG_NO_BUFFERING` | `O_DIRECT` | Bypasses kernel page cache via direct DMA |
| **Durability / Write-Through** | `FILE_FLAG_WRITE_THROUGH` | `O_DSYNC` / `fsync` | Operational on both platforms |
| **Page Buffer Allocator** | `VirtualAlloc` / `VirtualFree` | `mmap` / `munmap` | Enforced multiple of `4096` bytes (`PAGE_BUFFER_SIZE`) |
| **Exact EOF Truncation** | `SetFileInformationByHandle` / `SetEndOfFile` | `ftruncate` | Trims padded page writes to exact byte length |
| **File Sharing Flags** | `share_read`, `share_write`, `share_delete` | *No-op* | Advisory/mandatory Win32 sharing semantics; ignored on Linux |
| **Wide Path (`wchar_t*`)** | Native UTF-16 (`CreateFileW`) | Converted via `wide_to_ascii` | Cross-platform path support |

---

## Key Features

### 1. Deterministic Unbuffered File Streams (`os::Ifstream` & `os::Ofstream`)
* **Page-Aligned User-Space Buffering:** `StreamBuffer` manages an explicit memory buffer aligned to 4096-byte boundaries (`PAGE_BUFFER_SIZE`, configurable at compile time as any multiple of `4096`), eliminating double-buffering in kernel RAM.
* **Transparent Unaligned Reads & Writes:** Callers can read, write, append, and seek arbitrary byte counts without manually padding buffers or aligning file offsets to sector boundaries.
* **Read-Modify-Overwrite Append & Exact Truncation:** Appending to existing files cleanly patches trailing partial pages and truncates the final file size to the exact logical byte count on flush/close.
* **Random-Access Positioning:** Full `seek(os::Cursor)` and `tell()` support with `os::Cursor` arithmetic (`+`, `-`, `*`, `/`, `%`) and sentinel constants (`os::Cursor::start`, `os::Cursor::npos`).

### 2. Zero-Overhead Raw Binary Serialization & Text Mode
* **Binary Stream Operators (`<<` & `>>`):** Overloaded for fixed-size primitives (`char`, `short`, `int`, `long`, `long long`, `float`, `double`, `long double`, and unsigned variants), reading and writing exact `sizeof(T)` raw bytes directly rather than performing ASCII formatting.
* **Bounded C-String Writes:** `operator<<(const char*)` enforces a compile-time safety ceiling (`WRITE_MAX_SIZE`, default `2048` bytes) to guard against non-null-terminated buffers.
* **OS-Aware Text Mode:** Supports text mode (`OpenMode::NONE` / omitting `OpenMode::binary`) with platform-specific ASCII line-ending conversions.

### 3. Hybrid Error Handling Model
* **Typed Setup & Positioning Exceptions (`os::exceptions`):** Constructors, `open()`, `seek()`, and `get()` throw descriptive exceptions (`FileNotFound`, `FileAlreadyExists`, `FileAccessDenied`, `FileSeekError`, `BufferFailure`, `SystemFailureError`, etc.).
* **`noexcept` Hot-Path I/O:** High-frequency operations (`read()`, `write()`, `refil()`, `close()`) are marked `noexcept` and expose stream validity through `is_open()` and `explicit operator bool()`.

---

## Build & Compilation

SuDOs has **zero external dependencies** and no template instantiation overhead. It is tested with **C++20** and compatible with **C++17 and above**.

### Compile-Time Configuration Macros
Override any of these before including `os.hpp` (or via `-D` compiler flags):
* `PAGE_BUFFER_SIZE` (default: `4096`) — Internal page buffer size (automatically enforced to a multiple of `4096`).
* `WRITE_MAX_SIZE` (default: `2048`) — Maximum byte scan length for null-terminated `const char*` stream insertion.
* `PATH_MAX_SIZE` (default: `1024`) — Maximum supported path buffer length.

### Building from Source

**(GCC / Clang):**
```bash
g++ -std=c++20 -O2 -Iinclude src/General.cpp src/Linux.cpp main.cpp -o app
