# Atari AspeQt-2K26

![AspeQt-2k26 Badge](res/badge.png)

### The Modern Atari 8-bit Serial Peripheral Emulator
**Built for 2026 and beyond | Powered by Qt 6**

---

### Project Overview
AspeQt-2K26 is a modernized fork of the classic AspeQT/RespeQt emulator. It emulates Atari SIO peripherals (Disk Drives, Printers, etc.) when connected to an Atari 8-bit computer (400/800/XL/XE) via an SIO2PC cable.

This version migrates the codebase to **Qt 6**, fixing critical stability issues and introducing high-performance networking and hardware-level emulation for modern operating systems.

### 🚀 What's New in AspeQt-2K26

#### 1. Full Qt 6 Native Port
* **Modern Framework:** Fully ported to the Qt 6 framework for enhanced stability and long-term support.
* **High-DPI Support:** Optimized UI for modern monitors and 4K displays.

#### 2. Client TNFS Support
* **Cloud Storage:** Direct support for mounting disk images from TNFS servers (like `13leader.net`) without needing local files.
* **Dynamic Browsing:** Integrated TNFS browser with thread-safe download progress tracking.

#### 3. Native W: and Y: Drivers
* **W: (Network Device):** A native SIO implementation of the Network Streaming Device. Supports HTTP and FTP (via system `curl`) for downloading data directly to the Atari. Includes EOL translation between Unix (LF) and ATASCII (EOL).
* **Y: (Clipboard Device):** Bridges the Atari to your modern OS clipboard. Use `OPEN #1,4,0,"Y:"` to read the PC clipboard or `OPEN #1,8,0,"Y:"` to write to it.

#### 4. Modem RS232 Bridge
* **Hayes Emulation:** Bridges a secondary serial port to the Internet, allowing you to use a real Atari and 850 interface to access modern BBSes.
* **BBS Phonebook:** Integrated XML phonebook support for quick dialing.
* **Macros & Automation:** Built-in Macro support (Auto-User/Auto-Pass) to speed up BBS logins.

#### 5. Experimental 850 R: Emulation
* **Direct Hardware Emulation:** A kernel-bypass, SIO-level emulation of the **Atari 850 Interface Module**.
* **TCP/IP Integration:** Maps the 850 driver's RS-232 commands directly to TCP/IP sockets for modern "modem-less" networking.
* **Stream Mode:** Optimized for Raspberry Pi 5 native UARTs, providing FujiNet-level precision for R: device polls.

---

### 🕒 Time Synchronization (Real-Time Clock)
AspeQt-2k26 features built-in support for the **ApeTime** protocol, allowing your Atari to synchronize its clock with your PC automatically.

* **Recommended Method:** Use a standard ApeTime-compatible driver like `ATIME.SYS` on SpartaDOS X.
* **Legacy Support:** The `aspecl` client is included for backward compatibility with older setups.

---

### Support & Community
Support and inquiries can be made on our BBS or via our GitHub issues page. We love talking Atari!
* **Telnet:** `telnet 13leader.net 8023`
* **Web:** [http://13leader.net](http://13leader.net)

---

### Building from Source
**Requirements:**
* CMake 3.16+
* Qt 6.x Development Libraries
* C++17 Compiler

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel
