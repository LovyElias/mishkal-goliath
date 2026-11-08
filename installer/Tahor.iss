; Inno Setup script: the Windows installer for the Mishkal Tahor tester beta (VST3, 64-bit).
; Built by the "Tahor Windows beta" GitHub workflow:  ISCC /DMKVER=11 /DSRC=<path to the .vst3 folder> Tahor.iss
#ifndef MKVER
  #define MKVER "0"
#endif
#ifndef SRC
  #define SRC "..\build\Tahor_artefacts\Release\VST3\Mishkal Tahor.vst3"
#endif

[Setup]
AppName=Mishkal Tahor Beta
AppVersion=0.{#MKVER}
AppPublisher=Mishkal Audio
AppId={{3C9A7E21-5B6D-4A8F-9E14-54414F4F5231}
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
Uninstallable=yes
UninstallDisplayName=Mishkal Tahor Beta
OutputBaseFilename=Install Mishkal Tahor Beta V{#MKVER}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Messages]
WelcomeLabel2=This installs the Mishkal Tahor Beta (V{#MKVER}) VST3 plug-in - Reso, Tahor and Character in one EQ, by Lovy Elias.%n%nAfter installing, restart your DAW and rescan plug-ins. Thank you for testing!

[Files]
Source: "{#SRC}\*"; DestDir: "{commoncf64}\VST3\Mishkal Tahor.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Mishkal Tahor.vst3"
