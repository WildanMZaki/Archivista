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
; Uncomment the line below after adding your icon file at assets/archivista.ico
; SetupIconFile=..\assets\archivista.ico
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
