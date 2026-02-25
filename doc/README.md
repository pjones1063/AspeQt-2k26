AspeQt-2k26

High-Performance Peripheral Emulator for Atari 8-Bit Computers

Introduction High-Speed SIO Disk & Folder Images TNFS Client Modem
Bridge Wiring Diagram Printer (P:) Web Pipe (W:)

> Introduction
>
> AspeQt-2k26 is a modernized version of the classic AspeQt emulator,
> updated to use the Qt6 framework. It transforms your modern PC into a
> powerful peripheral server for Atari 8-bit computers (400/800/XL/XE).
>
> Using a standard SIO2PC cable (USB or Serial), AspeQt can emulate:
>
> Disk Drives (D1:-D8:) - Supports ATR, XFD, PRO images, and direct
> Folder Mounting.
>
> TNFS Client - Mount and stream disk images directly from the internet
> (FujiNet protocol).
>
> Modem Bridge - Connect physical Atari modems to Telnet BBSs via the
> PC.
>
> Printers (P:) - Captures printed output to text or graphical view.
>
> Clipboard Bridge (Y:) - Bi-directional copy/paste between PC and
> Atari.
>
> Web Pipe (W:) - Multi-channel gateway for Python scripts and AI.
>
> High-Speed SIO
>
> AspeQt supports "divisor 0" high-speed SIO (approx. 126kbps) with
> custom OS ROMs (like QMEG or Hiassoft) and standard high-speed
> protocols (UltraSpeed, Happy, etc.).
>
> Note: Reliable high-speed operation depends on your SIO2PC hardware.
> FTDI-based USB cables generally offer the best performance.
>
> Disk & Folder Usage
>
> Mounting Disk Images
>
> Drag and drop .atr , .xfd , or .pro files directly into the drive
> slots (D1-D8). Double-clicking a slot opens the file browser.
>
> Virtual DOS (Folder Images)
>
> You can mount a local PC directory as a writable disk. To boot from a
> folder, right-click the slot and select Mount Folder.
>
> TNFS Client
>
> AspeQt-2k26 includes native support for TNFS (The Network File
> System), allowing you to mount and stream disk images directly from
> the internet using the FujiNet protocol. Access this via the Tools
> menu.
>
> Modem Bridge (Internet Adapter)
>
> The Modem Bridge allows your Atari 8-bit computer to connect to modern
> Telnet BBSs using your PC's internet connection. It acts as a gateway
> between your physical Atari hardware and the TCP/IP network.
>
> 1\. Required Hardware
>
> This feature requires a physical serial interface. It does not work
> with a standard SIO2PC cable alone.
>
> Atari Interface: Atari 850, ICD R-Verter, or P:R: Connection.
>
> Cables: A Null Modem cable connecting the Atari Interface to your PC.
>
> PC Hardware: A USB-to-Serial adapter (FTDI recommended).

||
||
||
||

> 2\. Configuration Reference
>
> Configure the bridge settings via Tools \> Options \> Modem Bridge.
> Correct settings are vital for stable communication.

||
||
||
||
||
||
||
||
||

> 3\. The Phonebook
>
> Access via Tools \> Phonebook. Use this to store your favorite BBS
> addresses and credentials.
>
> Dialing: Select an entry and click Dial. AspeQt will look up the
> credentials and connect.
>
> Auto-Login: If you save a Username and Password in the phonebook, you
> can use the Macros listed below to auto-type them.
>
> Getting a Phonebook: You can download a compatible XML or SyncTerm
> phonebook list from the Telnet BBS Guide:
> <u>https://telnetbbsguide.com/lists/new-bbs-s</u>y<u>stems/</u>
>
> 4\. Supported Commands & Macros
>
> The bridge emulates a standard Hayes modem. It supports the following
> commands from your Atari terminal software (BobTerm, Ice-T, etc.):

||
||
||
||
||
||
||
||
||
||

> 5\. ATASCII Translation
>
> Modern internet servers expect standard ASCII input. AspeQt
> automatically performs the following translations:
>
> Backspace: Atari Delete (Code 126/127) is automatically converted to
> ASCII Backspace (Code 8).
>
> Newlines: Atari EOL (155) is converted to standard Network Newlines
> (CR/LF).
>
> Appendix: Wiring the Atari 850 Null Modem Cable
>
> Unlike modern PCs, the Atari 850 Interface uses a non-standard DB9
> pinout. You cannot use a standard off-the-shelf "Serial Cable" or
> "Null Modem Cable" without an adapter, as the wire mappings will be
> incorrect.
>
> To connect your Atari 850 (Port 1) to a PC USB-Serial Adapter, you
> must build a custom cable or use a breakout box with the following
> wiring scheme.
>
> The "Magic" Signal: DTR
>
> The Atari 850 requires the DSR (Data Set Ready) and CD (Carrier
> Detect) pins to be HIGH to detect that a modem is present. AspeQt
> emulates this by asserting its DTR (Data Terminal Ready) signal. Your
> cable MUST connect the PC's DTR pin to the Atari's DSR and CD pins,
>
> or the bridge will remain "Dead."
>
> ATARI 850 (PORT 1)
>
> \[ DB9 MALE Connector \]
>
> (Plugs into Atari 850)

PC USB ADAPTER

\[ DB9 FEMALE Connector \]

(Plugs into PC)

> PIN FUNCTION FUNCTION PIN
>
> +---+------------+ +------------+---+ \| 1 \| DTR (Out)
> \|-----------------------------\>\| DSR / DCD \|1+6\|
> +---+------------+ (Cross) +------------+---+ \| 6 \| DSR (In)
> \|\<-----------------------------\| DTR (Out) \| 4 \|
> +---+------------+ +------------+---+
>
> +---+------------+ +------------+---+ \| 3 \| TX (Out)
> \|-----------------------------\>\| RX (In) \| 2 \| +---+------------+
> (Cross) +------------+---+ \| 4 \| RX (In)
> \|\<-----------------------------\| TX (Out) \| 3 \|
> +---+------------+ +------------+---+
>
> +---+------------+ +------------+---+ \| 7 \| RTS (Out)
> \|-----------------------------\>\| CTS (In) \| 8 \|
> +---+------------+ (Cross) +------------+---+ \| 8 \| CTS (In)
> \|\<-----------------------------\| RTS (Out) \| 7 \|
> +---+------------+ +------------+---+
>
> +---+------------+ +------------+---+ \| 5 \| GROUND
> \|------------------------------\| GROUND \| 5 \| +---+------------+
> (Direct) +------------+---+
>
> Connector Gender Guide
>
> Atari 850 Side: The 850 ports are *Female*. Your cable needs a DB9
> Male connector.
>
> PC Side: Most USB-Serial dongles are *Male*. Your cable needs a DB9
> Female connector.
>
> Troubleshooting: If you are using a commercially available "Atari 850
> Modem Cable" (which is usually DB9-to-DB25), you will need a standard
> DB25-to-DB9 Null Modem Adapter attached to the PC side to perform the
> necessary signal crossing.
>
> Time Synchronization (Real-Time Clock)
>
> AspeQt-2k26 allows your Atari 8-bit computer to read the current date
> and time from your PC. This is essential for file timestamps in DOS.
>
> Recommended Method: ApeTime Protocol
>
> AspeQt natively emulates the ApeTime device. This is the modern
> standard supported by most DOS variants.
>
> SpartaDOS X: Enable the \`DATE\` and \`TIME\` drivers, or load
> \`ATIME.SYS\`.
>
> RealDOS / BW-DOS: Load the \`ATIME.SYS\` driver in your
> \`CONFIG.SYS\`.
>
> MyDOS: Requires a specialized autorun utility (check the \`indus\` or
> \`ape\` folders on your utility disks).
>
> Note: You do not need to run any special client software on the PC
> side. AspeQt handles the protocol automatically in the background.
>
> Legacy Method: AspeCl (Deprecated)
>
> The older \`aspecl\` client (Device \$46) is included for backward
> compatibility with very old setups but is not recommended for new
> users. Please use the ApeTime protocol instead.
>
> Printer Emulation (P:)
>
> Output sent to P: is captured by AspeQt. View the output via Tools \>
> View Printer Output.
>
> The Y: Device (Clipboard Bridge)
>
> The Y: Device bridges your Atari 8-bit and your modern PC's system
> clipboard. It allows you to "Paste" code from your PC directly into
> Atari BASIC, or "Copy" listings from the Atari back to your PC.
>
> Import Code (PC → Atari): Copy text on PC, then type ENTER "Y:" on
> Atari.
>
> Export Code (Atari → PC): Type LIST "Y:" on Atari, then Paste on PC.
>
> The W: Device (Web Pipe)
>
> The W: device transforms the Atari 8-bit into a "Thin Client" capable
> of interacting with modern web services. It acts as a transparent
> proxy, forwarding standard BASIC text commands to a backend script
> running on your PC via HTTP.
>
> 1\. Multi-Channel Support (W1: - W4:)
>
> AspeQt-2k26 supports up to 4 simultaneous connections. This allows for
> complex "Full Duplex" applications where one channel reads data while
> another writes data, or background threads poll for status updates.
>
> W: or W1: - Primary Channel (Device \$57)
>
> W2: - Secondary Channel (Device \$56)
>
> W3: - Tertiary Channel (Device \$55)
>
> W4: - Quaternary Channel (Device \$54)
>
> 10 REM -- QUAD CHANNEL EXAMPLE --
>
> 20 OPEN \#1,8,0,"W1:http://192.168.1.5:5000/cmd" :REM Command Channel
> 30 OPEN \#2,4,0,"W2:http://192.168.1.5:5000/data" :REM Data Stream
>
> 40 OPEN \#3,4,0,"W3:http://192.168.1.5:5000/status":REM Background
> Poll 50 PRINT \#1;"GET STATUS"
>
> 60 INPUT \#3,STATUS\$
>
> 70 IF STATUS\$="READY" THEN INPUT \#2,DATA\$ 80 CLOSE \#1 : CLOSE \#2
> : CLOSE \#3
>
> 2\. Configuration: Text vs. Binary Mode
>
> The W: device can be configured globally via Options \> Emulation, or
> overridden per-connection using the command parameters.
>
> Text Mode: Best for BASIC and Web APIs.
>
> *Write:* Converts Atari EOL ( \$9B ) → Unix Newline ( \n ).
>
> *Read:* Converts Unix Newline ( \n ) → Atari EOL ( \$9B ).
>
> Binary Mode: Best for Game Loaders and Asset Streaming. Data is passed
> raw, byte-for-byte. No translation occurs.
>
> On-the-Fly Mode Switching (Advanced)
>
> You can override the global setting for a specific connection by using
> the AUX2 parameter (the 3rd number in the OPEN or XIO command).
>
> 0: Use Global AspeQt Settings (Default)
>
> 1: Force TEXT Mode (Translate EOLs)
>
> 2: Force BINARY Mode (Raw Data)
>
> 10 REM -- MODE SWITCHING EXAMPLES --20
> U\$="W:http://192.168.1.5:5000/api"
>
> 30 REM Force TEXT Mode (AUX2=1) for API call 40 OPEN \#1, 8, 1, U\$
>
> 50 PRINT \#1;"HELLO" :REM Sends "HELLO\n" 60 CLOSE \#1
>
> 70 REM Force BINARY Mode (AUX2=2) for Game Asset 80 OPEN \#1, 8, 2,
> "W:http://server.com/sprite.dat" 90 REM Data read here will NOT have
> \$9B converted 100 CLOSE \#1
>
> 3\. Advanced: XIO Commands (One-Shot)
>
> The XIO command allows you to send a "Fire and Forget" packet without
> the overhead of OPEN/PRINT/CLOSE. This is ideal for telemetry,
> logging, or high-speed triggers.
>
> Command 80 (\$50): Fast POST
>
> Sends a payload to a URL in a single operation. The format is
> "W#:URL,DATA" . It supports all channels (W1-W4) and the AUX2 mode
> flag.
>
> 10 DIM PKT\$(120)
>
> 20 URL\$="http://192.168.1.5:5000/score" 30 SCORE=15000
>
> 40 REM -- Build Packet --50 PKT\$="W1:":PKT\$(4)=URL\$
>
> 60 PKT\$(LEN(PKT\$)+1)=",":PKT\$(LEN(PKT\$)+1)=STR\$(SCORE)
>
> 70 REM -- Execute XIO 80 with FORCE BINARY (AUX2=2) --80 XIO
> 80,#1,0,2,PKT\$
>
> 4\. Application Examples Library
>
> This library contains a collection of "Hybrid Computing" examples that
> demonstrate the power of the W: Device.
>
> 1\. Gemini AI Chatbot
>
> Turns the Atari into a client for Google's Gemini Large Language
> Model. You can type natural language questions ("Who is Captain
> Kirk?") and receive intelligent, summarized answers on the Atari
> screen.
>
> BASIC File: GEMINI_AI.LST
>
> Python Script: gemini_ai.py
>
> Dependencies: pip install google-generativeai
>
> 2\. Basic Graphical Weather Station
>
> Fetches live weather data from the Open-Meteo API for a specific
> location and displays it on the Atari using GRAPHICS 5 (4-color mode).
>
> BASIC File: WEATHER.LST
>
> Python Script: weather.py
>
> Dependencies: pip install requests
>
> 3\. Cloud Notepad
>
> Demonstrates Reading and Writing to the PC's hard drive. It acts as a
> "Cloud Drive" for text.
>
> BASIC File: CLOUD_NOTE.LST
>
> Python Script: cloud_note.py
>
> 4\. Text-to-Speech Synthesizer
>
> The "Talking Atari." Any text typed on the Atari is sent to the PC,
> where it is read aloud using the Linux espeak engine.
>
> BASIC File: TTS_SERVER.LST
>
> Python Script: tts_server.py
>
> Dependencies: sudo apt install espeak (Linux)
>
> 5\. Web News Scraper
>
> Connects to a modern news website, scrapes the HTML, strips out
> ads/javascript, and delivers a plain-text headline summary to the
> Atari.
>
> BASIC File: SCRAPER.LST
>
> Python Script: scraper.py
>
> Dependencies: pip install requests beautifulsoup4
>
> 6\. Remote Execution (Linux & Windows)
>
> Allows the Atari to execute shell commands or launch GUI applications
> on the Host PC.
>
> BASIC Files: EXEC_LINUX.LST / EXEC_WIN.LST
>
> Python Scripts: exec_linux.py / exec_win.py
>
> 7\. File Upload / Telemetry
>
> A utility to upload raw data or text files from the Atari to the PC.
>
> BASIC File: FILE_UPLOAD.LST
>
> Python Script: file_upload.py
>
> Quick Dependency Install
>
> \# Linux/Mac
>
> sudo apt install espeak
>
> pip install flask requests beautifulsoup4 google-generativeai psutil
>
> \# Windows
>
> pip install flask requests beautifulsoup4 google-generativeai psutil
>
> © 2026 AspeQt-2k26 Development Team.
>
> Based on the original AspeQt by FJC and Ray Lidstrom.
