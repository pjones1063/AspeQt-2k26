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

[Files]
; The "*" tells Inno to grab the .exe AND all the Qt DLLs from windeployqt
Source: "..\release\win-temp\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\AspeQt-2K26"; Filename: "{app}\AspeQt.exe"
Name: "{autodesktop}\AspeQt-2K26"; Filename: "{app}\AspeQt.exe"
