; ============================================
; Archivista - Inno Setup Installer Script
; ============================================
; This script is used by GitHub Actions to build the installer.
; The AppVersion is injected via the /D command line option.

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif

[Setup]
AppName=Archivista
AppVersion={#AppVersion}
AppPublisher=WildanMZaki
AppPublisherURL=https://github.com/WildanMZaki/Archivista
AppSupportURL=https://github.com/WildanMZaki/Archivista/issues
AppUpdatesURL=https://github.com/WildanMZaki/Archivista/releases
DefaultDirName={autopf}\Archivista
DefaultGroupName=Archivista
AllowNoIcons=yes
OutputDir=Output
OutputBaseFilename=Archivista-{#AppVersion}-Setup
SetupIconFile=..\assets\archivista.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
LicenseFile=
PrivilegesRequired=lowest

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "contextmenu"; Description: "Add ""Open with Archivista"" to right-click context menu for .txt files"; GroupDescription: "Context Menu:"; Flags: unchecked
Name: "openwith"; Description: "Register Archivista in ""Open With"" program list"; GroupDescription: "Context Menu:"; Flags: unchecked

[Registry]
; Direct context menu for .txt files (right-click → "Open with Archivista")
Root: HKCU; Subkey: "Software\Classes\.txt\shell\Archivista"; ValueType: string; ValueName: ""; ValueData: "Open with Archivista"; Tasks: contextmenu; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\.txt\shell\Archivista"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\Archivista.exe"",0"; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\.txt\shell\Archivista\command"; ValueType: string; ValueName: ""; ValueData: """{app}\Archivista.exe"" ""%1"""; Tasks: contextmenu; Flags: uninsdeletekey

; Register in Windows "Open With" program list (right-click → Open with → Choose another app)
Root: HKCU; Subkey: "Software\Classes\Applications\Archivista.exe"; ValueType: string; ValueName: "FriendlyAppName"; ValueData: "Archivista"; Tasks: openwith; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\Applications\Archivista.exe\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\Archivista.exe"" ""%1"""; Tasks: openwith; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\Applications\Archivista.exe\SupportedTypes"; ValueType: string; ValueName: ".txt"; ValueData: ""; Tasks: openwith; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\Applications\Archivista.exe\SupportedTypes"; ValueType: string; ValueName: ".log"; ValueData: ""; Tasks: openwith; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\Applications\Archivista.exe\SupportedTypes"; ValueType: string; ValueName: ".md"; ValueData: ""; Tasks: openwith; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\Applications\Archivista.exe\SupportedTypes"; ValueType: string; ValueName: ".ini"; ValueData: ""; Tasks: openwith; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\Applications\Archivista.exe\SupportedTypes"; ValueType: string; ValueName: ".cfg"; ValueData: ""; Tasks: openwith; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\Applications\Archivista.exe\SupportedTypes"; ValueType: string; ValueName: ".csv"; ValueData: ""; Tasks: openwith; Flags: uninsdeletekey

[Files]
; Main executable - built by CMake in the workflow
Source: "..\build\release\Archivista.exe"; DestDir: "{app}"; Flags: ignoreversion

; Uncomment below if you have additional assets to bundle
; Source: "..\assets\*"; DestDir: "{app}\assets"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Archivista"; Filename: "{app}\Archivista.exe"
Name: "{group}\{cm:UninstallProgram,Archivista}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Archivista"; Filename: "{app}\Archivista.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\Archivista.exe"; Description: "{cm:LaunchProgram,Archivista}"; Flags: nowait postinstall skipifsilent
