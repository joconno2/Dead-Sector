; Dead Sector — NSIS Windows Installer
; Build with: makensis installer/dead-sector.nsi
; Requires: the dist/dead-sector-windows-<TAG>/ folder next to installer/

!define APP_NAME     "Dead Sector"
!define APP_EXE      "dead-sector.exe"
!define PUBLISHER    "Dead Sector"
!define REG_KEY      "Software\Microsoft\Windows\CurrentVersion\Uninstall\Dead Sector"

; Pull version from environment (set by CI: %TAG% = vX.Y.Z)
!ifndef APP_VERSION
  !define APP_VERSION "0.0.0"
!endif
!ifndef SRC_DIR
  !define SRC_DIR "."
!endif

Name              "${APP_NAME} ${APP_VERSION}"
OutFile           "Dead_Sector_Setup_${APP_VERSION}.exe"
InstallDir        "$PROGRAMFILES64\Dead Sector"
InstallDirRegKey  HKLM "Software\Dead Sector" ""
RequestExecutionLevel admin
SetCompressor     /SOLID lzma

; Modern UI
!include "MUI2.nsh"

!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; ---------------------------------------------------------------------------
Section "Dead Sector" SecMain
    SectionIn RO  ; required

    SetOutPath "$INSTDIR"
    File "${SRC_DIR}\${APP_EXE}"
    File "${SRC_DIR}\SDL2.dll"
    File "${SRC_DIR}\SDL2_mixer.dll"

    ; Steam DLL — present only in Steam builds
    IfFileExists "${SRC_DIR}\steam_api64.dll" 0 +2
        File "${SRC_DIR}\steam_api64.dll"

    SetOutPath "$INSTDIR\assets"
    File /r "${SRC_DIR}\assets\*.*"

    ; Start Menu shortcut
    CreateDirectory "$SMPROGRAMS\Dead Sector"
    CreateShortcut  "$SMPROGRAMS\Dead Sector\Dead Sector.lnk" "$INSTDIR\${APP_EXE}"
    CreateShortcut  "$SMPROGRAMS\Dead Sector\Uninstall.lnk"   "$INSTDIR\Uninstall.exe"

    ; Desktop shortcut
    CreateShortcut "$DESKTOP\Dead Sector.lnk" "$INSTDIR\${APP_EXE}"

    ; Registry — Add/Remove Programs entry
    WriteRegStr   HKLM "${REG_KEY}" "DisplayName"     "${APP_NAME}"
    WriteRegStr   HKLM "${REG_KEY}" "DisplayVersion"  "${APP_VERSION}"
    WriteRegStr   HKLM "${REG_KEY}" "Publisher"       "${PUBLISHER}"
    WriteRegStr   HKLM "${REG_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr   HKLM "${REG_KEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegDWORD HKLM "${REG_KEY}" "NoModify"        1
    WriteRegDWORD HKLM "${REG_KEY}" "NoRepair"        1

    WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

; ---------------------------------------------------------------------------
Section "Uninstall"
    ; Remove files
    Delete "$INSTDIR\${APP_EXE}"
    Delete "$INSTDIR\SDL2.dll"
    Delete "$INSTDIR\SDL2_mixer.dll"
    Delete "$INSTDIR\steam_api64.dll"
    Delete "$INSTDIR\Uninstall.exe"
    RMDir  /r "$INSTDIR\assets"
    RMDir  "$INSTDIR"

    ; Remove shortcuts
    Delete "$SMPROGRAMS\Dead Sector\Dead Sector.lnk"
    Delete "$SMPROGRAMS\Dead Sector\Uninstall.lnk"
    RMDir  "$SMPROGRAMS\Dead Sector"
    Delete "$DESKTOP\Dead Sector.lnk"

    ; Remove registry key
    DeleteRegKey HKLM "${REG_KEY}"
    DeleteRegKey HKLM "Software\Dead Sector"
SectionEnd
