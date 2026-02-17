[Setup]
AppName=AspeQt-2K26
AppVersion=1.0.0
DefaultDirName={autopf}\AspeQt-2K26
DefaultGroupName=AspeQt-2K26
UninstallDisplayIcon={app}\AspeQt.exe
Compression=lzma2
SolidCompression=yes
OutputDir=..\release
OutputBaseFilename=AspeQt-2K26-Setup

; --- NEW: Icon for the Installer Executable itself ---
SetupIconFile=..\res\Atari.ico

; --- NEW: Explicitly allow user to change Install Path & Start Menu Folder ---
DisableDirPage=no
DisableProgramGroupPage=no

[Files]
; The "*" tells Inno to grab the .exe AND all the Qt DLLs from windeployqt
Source: "..\release\win-temp\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

; --- NEW: Copy the icon to the install folder so shortcuts can use it ---
Source: "..\res\Atari.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
; --- UPDATED: Added IconFilename to force shortcuts to use Atari.ico ---
Name: "{group}\AspeQt-2K26"; Filename: "{app}\AspeQt.exe"; IconFilename: "{app}\Atari.ico"
Name: "{autodesktop}\AspeQt-2K26"; Filename: "{app}\AspeQt.exe"; IconFilename: "{app}\Atari.ico"
