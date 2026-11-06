; Inno Setup script: the Windows installer for the tester beta (VST3, 64-bit).
; Built by the "Windows beta" GitHub workflow:  ISCC /DMKVER=84 /DSRC=<path to the .vst3 folder> MishkalBeta.iss
#ifndef MKVER
  #define MKVER "0"
#endif
#ifndef SRC
  #define SRC "..\..\build\MishkalBeta_artefacts\Release\VST3\Mishkal Goliath Beta.vst3"
#endif

[Setup]
AppName=Mishkal Goliath Beta
AppVersion=0.{#MKVER}
AppPublisher=Mishkal Audio
AppId={{6B1E2B7A-4C1D-4F3E-9A60-4D49534B4C42}
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
Uninstallable=yes
UninstallDisplayName=Mishkal Goliath Beta
OutputBaseFilename=Install Mishkal Goliath Beta V{#MKVER}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Messages]
WelcomeLabel2=This installs the Mishkal Goliath Beta (V{#MKVER}) VST3 plug-in, a mixing and mastering processor by Lovy Elias.%n%nAfter installing, restart your DAW and rescan plug-ins. Thank you for testing!

[InstallDelete]
; the beta was called "Mishkal Goliath Strip Beta" before V78: remove it so the host doesn't see the same plug-in twice
Type: filesandordirs; Name: "{commoncf64}\VST3\Mishkal Goliath Strip Beta.vst3"

[Files]
Source: "{#SRC}\*"; DestDir: "{commoncf64}\VST3\Mishkal Goliath Beta.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Mishkal Goliath Beta.vst3"
