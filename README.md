# Atari AspeQt-2K26

### The Modern Atari 8-bit Serial Peripheral Emulator
**Built for 2026 and beyond | Powered by Qt 6**

---

### Summary
AspeQt-2K26 is a modernized fork of the classic AspeQT/RespeQt emulator. It emulates Atari SIO peripherals (Disk Drives, Printers, etc.) when connected to an Atari 8-bit computer (400/800/XL/XE) via an SIO2PC cable.

While preserving the classic functionality, **AspeQt-2K26** migrates the codebase to **Qt 6**, fixing critical stability issues and adding support for modern large-capacity storage and current operating systems.

### What's New in AspeQt-2K26
* **Qt 6 Native:** Fully ported to the Qt 6 framework for modern UI support and High-DPI scaling.
* **Self-Contained Firmware:** All necessary boot firmware (MyDOS, Atari DOS, etc.) is now embedded within the executable.
* **Large Hard Disk Support:** Fixed integer overflow logic to support **16MB+ Hard Disk Images** (up to 65,535 sectors) correctly.
* **"Happy Mode" Improvements:** Enhanced handshake logic and proper Windows timing via `winmm`.
* **Stability Fixes:** Crash-proof UI actions on empty slots and resolved memory safety issues in the autoboot loader.
* **Modern Desktop Integration:** Restored and fixed **Drag & Drop** functionality for Qt 6 environments.
* **Enhanced Linux/Unix Support:** Implemented proper serial port flushing (`tcdrain`) to prevent SIO timeouts and NAKs.

---

### Automated Deployment
This project includes automated scripts to generate production-ready installers for all major platforms.

#### **Windows (QEMU/Native)**
The Windows script builds the project using MinGW and generates both a portable ZIP and a professional installer.
* **Requirements:** Qt 6.10.2, MinGW 13.1.0, and Inno Setup 6.
* **Command:** Run `powershell ./release/deploy_win.ps1`.

#### **Linux (Debian/Ubuntu)**
The Linux script generates a `.deb` package and a portable `.tar.gz`.
* **Requirements:** CMake, CPack, and Qt 6 development libraries.
* **Command:** Run `bash ./release/deploy_linux.sh`.

#### **macOS**
Standard build process for modern Apple Silicon (M3) environments.
* **Requirements:** Xcode or Clang, and Qt 6.
* **Command:** Use `macdeployqt` to wrap the `.app` bundle into a `.dmg`.

---

### Troubleshooting (Windows/MinGW)
If the Windows build fails, check the following common roadblocks:

* **Stale Cache:** If moving the project from Linux to Windows, you MUST delete the `build/` folder entirely to clear Linux-specific paths from `CMakeCache.txt`.
* **Missing windres:** If you see `'windres' is not recognized`, ensure `CMAKE_RC_COMPILER` is explicitly set in the deployment script to point to the MinGW `bin` folder.
* **sh.exe Conflict:** If the build fails with `sh.exe was found in your PATH`, remove Git Bash from your system PATH temporarily.
* **-j Invalid Number:** Ensure the multi-core flag is passed as a sub-expression: `-j ($env:NUMBER_OF_PROCESSORS)`.

---

### Support & Community
Support and inquiries can be made on our BBS. We love talking Atari!
* **Telnet:** `telnet 13leader.net 8023`
* **Web:** [http://13leader.net](http://13leader.net)

---

### Building from Source (Manual)
**Requirements:**
* CMake 3.16+
* Qt 6.x Development Libraries
* C++17 Compiler

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel

