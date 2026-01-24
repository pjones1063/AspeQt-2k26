# Atari AspeQt-2K26

### The Modern Atari 8-bit Serial Peripheral Emulator
**Built for 2026 and beyond | Powered by Qt 6**

---

### Summary
AspeQt-2K26 is a modernized fork of the classic AspeQt/RespeQt emulator. It emulates Atari SIO peripherals (Disk Drives, Printers, etc.) when connected to an Atari 8-bit computer (400/800/XL/XE) via an SIO2PC cable.

While preserving the classic functionality, **AspeQt-2K26** migrates the codebase to **Qt 6**, fixing critical stability issues and adding support for modern large-capacity storage and current operating systems.

### What's New in AspeQt-2K26
This version includes significant architectural improvements over previous forks:

* **Qt 6 Native:** Fully ported to the Qt 6 framework for modern UI support, High-DPI scaling, and future-proofing on Windows, Linux, and macOS.
* **Large Hard Disk Support:** Fixed integer overflow logic to support **16MB+ Hard Disk Images** (up to 65,535 sectors) correctly.
* **Stability Fixes:**
    * Fixed **Autoboot crashes** (memory safety improvements).
    * Fixed **Double Density (180KB)** geometry detection (correct handling of padded vs. unpadded images).
* **Modern Desktop Integration:** Restored and fixed **Drag & Drop** functionality for Qt 6 environments.
* **Enhanced Linux/Unix Support:** Implemented proper serial port flushing (`tcdrain`) to prevent SIO timeouts and NAKs on Linux and macOS.

### Features
* **Emulation:** Emulates up to 8 disk drives (D1:-D8:) and printers.
* **File Formats:** Supports `.ATR`, 'ATX',`.XEX`, `.CAS`, and `.XFD`.
* **Client Tools:** Includes `MENU.COM` (AspeQt Client) for the Atari to set Date/Time from the PC and perform remote file management.
* **Cross-Platform:** Runs on Windows, Linux, macOS, and Raspberry Pi.

---

### Support & Community
Support and inquiries can be made on our BBS. We love talking Atari!
* **Telnet:** `telnet 13leader.net 8023`
* **Web:** [http://13leader.net](http://13leader.net)

---

### Building from Source

**Requirements:**
* CMake
* Qt 6 Development Libraries (qt6-base-dev / qt6-serialport-dev)
* C++ Compiler (GCC, Clang, or MSVC)

**Build Steps:**
```bash
mkdir build
cd build
cmake ..
make
