; PC Migrate Portable - Windows installer (NSIS)
; Build: makensis -DVERSION=1.1.0 installer/PCMigrate.nsi
Unicode true
!ifndef VERSION
  !define VERSION "1.1.0"
!endif
!define APPNAME "PC Migrate Portable"
!define EXENAME "PCMigrate.exe"
!define UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\PCMigratePortable"

!include "MUI2.nsh"
!include "x64.nsh"
!include "LogicLib.nsh"

Name "${APPNAME} ${VERSION}"
OutFile "..\dist\PCMigrate-Setup.exe"
InstallDir "$PROGRAMFILES64\PCMigrate"
InstallDirRegKey HKLM "Software\PCMigratePortable" "InstallDir"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
BrandingText "${APPNAME} ${VERSION}"

VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "${APPNAME}"
VIAddVersionKey "FileDescription" "${APPNAME} Setup"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "LegalCopyright" "Yacine Boulyali - MIT License"

!define MUI_ICON "..\src\app.ico"
!define MUI_UNICON "..\src\app.ico"
!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TEXT "This will install ${APPNAME} ${VERSION}.$\r$\n$\r$\nInterface: English, Français, العربية, Español (change it any time in Settings).$\r$\n$\r$\nMove user files, browsers, Windows settings and installed programs (with their data) from an old PC to a new one.$\r$\n$\r$\nTip: the program is portable. You can also copy PCMigrate.exe to a USB stick and run it from Hiren's BootCD PE."
!define MUI_FINISHPAGE_RUN "$INSTDIR\${EXENAME}"
!define MUI_FINISHPAGE_RUN_TEXT "Launch ${APPNAME}"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README_AR.md"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Open the guide (Arabic)"
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "Arabic"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_RESERVEFILE_LANGDLL

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_ICONSTOP "${APPNAME} requires 64-bit Windows."
    Abort
  ${EndIf}
  SetRegView 64
  !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

Section "!${APPNAME} (required)" SecMain
  SectionIn RO
  SetOutPath "$INSTDIR"
  File "..\build\${EXENAME}"
  File "..\launcher\Run-PCMigrate.bat"
  File "..\launcher\Copy-To-USB.bat"
  File "..\docs\README_AR.md"
  File /oname=LICENSE.txt "..\LICENSE"
  ; the language chosen in the installer becomes the program's language (kept on upgrade)
  ${IfNot} ${FileExists} "$INSTDIR\PCMigrate.ini"
    StrCpy $0 "en"
    ${If} $LANGUAGE == ${LANG_FRENCH}
      StrCpy $0 "fr"
    ${ElseIf} $LANGUAGE == ${LANG_ARABIC}
      StrCpy $0 "ar"
    ${ElseIf} $LANGUAGE == ${LANG_SPANISH}
      StrCpy $0 "es"
    ${EndIf}
    WriteINIStr "$INSTDIR\PCMigrate.ini" "Settings" "Language" $0
  ${EndIf}
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\PCMigratePortable" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayName" "${APPNAME}"
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM "${UNINSTKEY}" "Publisher" "Yacine Boulyali"
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayIcon" "$INSTDIR\${EXENAME}"
  WriteRegStr HKLM "${UNINSTKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${UNINSTKEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKLM "${UNINSTKEY}" "URLInfoAbout" "https://github.com/yacineboulyali/pcmigrate-portable"
  WriteRegDWORD HKLM "${UNINSTKEY}" "NoModify" 1
  WriteRegDWORD HKLM "${UNINSTKEY}" "NoRepair" 1
  WriteRegDWORD HKLM "${UNINSTKEY}" "EstimatedSize" 600
SectionEnd

Section "Start menu shortcut" SecStart
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\${APPNAME}"
  CreateShortcut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\${EXENAME}"
  CreateShortcut "$SMPROGRAMS\${APPNAME}\Copy to USB.lnk" "$INSTDIR\Copy-To-USB.bat"
  CreateShortcut "$SMPROGRAMS\${APPNAME}\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Desktop shortcut" SecDesktop
  SetShellVarContext all
  CreateShortcut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\${EXENAME}"
SectionEnd

Section "Uninstall"
  SetRegView 64
  SetShellVarContext all
  Delete "$INSTDIR\${EXENAME}"
  Delete "$INSTDIR\Run-PCMigrate.bat"
  Delete "$INSTDIR\Copy-To-USB.bat"
  Delete "$INSTDIR\README_AR.md"
  Delete "$INSTDIR\LICENSE.txt"
  Delete "$INSTDIR\PCMigrate.ini"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
  Delete "$SMPROGRAMS\${APPNAME}\*.lnk"
  RMDir "$SMPROGRAMS\${APPNAME}"
  Delete "$DESKTOP\${APPNAME}.lnk"
  DeleteRegKey HKLM "${UNINSTKEY}"
  DeleteRegKey HKLM "Software\PCMigratePortable"
SectionEnd

Function un.onInit
  SetRegView 64
FunctionEnd
