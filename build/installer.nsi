Name "ASMotor"
OutFile "asmotor-setup.exe"

!define UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_KEY "${UNINSTKEY}"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME "CurrentUser"
!define MULTIUSER_INSTALLMODE_INSTDIR "$(^Name)"
!define MULTIUSER_INSTALLMODE_COMMANDLINE
!define MULTIUSER_EXECUTIONLEVEL Highest
!define MULTIUSER_MUI

!include "LogicLib.nsh"
!include "MultiUser.nsh"
!include "MUI2.nsh"
!include "x64.nsh"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MULTIUSER_PAGE_INSTALLMODE
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

;--------------------------------

!include "LogicLib.nsh"
!include "WinMessages.nsh"

;--------------------------------
; Check for max string length build

!macro SetReqStrLen Req_STRLEN
!define "Check_${NSIS_MAX_STRLEN}"
!ifndef "Check_${Req_STRLEN}"
    !error "You're not using the ${Req_STRLEN} string length special build! ${NSIS_MAX_STRLEN} is no good!"
!else
  !undef "Check_${NSIS_MAX_STRLEN}"
  !undef "SetReqStrLen"
!endif
!macroend
!define SetReqStrLen "!insertmacro SetReqStrLen"

${SetReqStrLen} 8192

;--------------------------------

; Registry Entry for environment
; All users (Warning: length often exceeds 1024 if many packages are installed):
;!define Environ 'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"'
; Current user only:
!define Environ 'HKCU "Environment"'
 
; AddToPath - Appends dir to PATH
;   (does not work on Win9x/ME)
;
; Usage:
;   Push "dir"
;   Call AddToPath
 
Function AddToPath
  Exch $0
  Push $1
  Push $2
  Push $3
  Push $4
 
  ; NSIS ReadRegStr returns empty string on string overflow
  ; Native calls are used here to check actual length of PATH
 
  ; $4 = RegOpenKey(HKEY_CURRENT_USER, "Environment", &$3)
  System::Call "advapi32::RegOpenKey(i 0x80000001, t'Environment', *i.r3) i.r4"
  IntCmp $4 0 0 done done
  ; $4 = RegQueryValueEx($3, "PATH", (DWORD*)0, (DWORD*)0, &$1, ($2=NSIS_MAX_STRLEN, &$2))
  ; RegCloseKey($3)
  System::Call "advapi32::RegQueryValueEx(i $3, t'PATH', i 0, i 0, t.r1, *i ${NSIS_MAX_STRLEN} r2) i.r4"
  System::Call "advapi32::RegCloseKey(i $3)"
 
  ${If} $4 = 234 ; ERROR_MORE_DATA
    DetailPrint "AddToPath: original length $2 > ${NSIS_MAX_STRLEN}"
    MessageBox MB_OK "PATH not updated, original length $2 > ${NSIS_MAX_STRLEN}" /SD IDOK
    Goto done
  ${EndIf}
 
  ${If} $4 <> 0 ; NO_ERROR
    ${If} $4 <> 2 ; ERROR_FILE_NOT_FOUND
      DetailPrint "AddToPath: unexpected error code $4"
      Goto done
    ${EndIf}
    StrCpy $1 ""
  ${EndIf}
 
  ; Check if already in PATH
  Push "$1;"
  Push "$0;"
  Call StrStr
  Pop $2
  StrCmp $2 "" 0 done
  Push "$1;"
  Push "$0\;"
  Call StrStr
  Pop $2
  StrCmp $2 "" 0 done
 
  ; Prevent NSIS string overflow
  StrLen $2 $0
  StrLen $3 $1
  IntOp $2 $2 + $3
  IntOp $2 $2 + 2 ; $2 = strlen(dir) + strlen(PATH) + sizeof(";")
  ${If} $2 > ${NSIS_MAX_STRLEN}
    DetailPrint "AddToPath: new length $2 > ${NSIS_MAX_STRLEN}"
    MessageBox MB_OK "PATH not updated, new length $2 > ${NSIS_MAX_STRLEN}." /SD IDOK
    Goto done
  ${EndIf}
 
  ; Append dir to PATH
  DetailPrint "Add to PATH: $0"
  StrCpy $2 $1 1 -1
  ${If} $2 == ";"
    StrCpy $1 $1 -1 ; remove trailing ';'
  ${EndIf}
  ${If} $1 != "" ; no leading ';'
    StrCpy $0 "$1;$0"
  ${EndIf}
  WriteRegExpandStr ${Environ} "PATH" $0
  SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
 
done:
  Pop $4
  Pop $3
  Pop $2
  Pop $1
  Pop $0
FunctionEnd

;--------------------------------

Function .onInit
  !insertmacro MULTIUSER_INIT
  ${If} ${RunningX64}
    StrCpy $INSTDIR $PROGRAMFILES64\ASMotor
  ${Else}
    StrCpy $INSTDIR $PROGRAMFILES\ASMotor
  ${EndIf}
FunctionEnd

Function un.onInit
  !insertmacro MULTIUSER_UNINIT
FunctionEnd

;--------------------------------

; The stuff to install
Section
  ; Set output path to the installation directory.
  ${If} ${RunningX64}
    SetOutPath $INSTDIR
    File "../_bin_w64/*.exe"
  ${Else}
    SetOutPath $INSTDIR
    File "../_bin_w32/*.exe"
  ${EndIf}

  Push "$INSTDIR"
  Call AddToPath  
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\ASMotor "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr ShCtx "${UNINSTKEY}" DisplayName "$(^Name)"
  WriteRegStr ShCtx "${UNINSTKEY}" UninstallString '"$INSTDIR\uninstall.exe"'
  WriteRegStr ShCtx "${UNINSTKEY}" $MultiUser.InstallMode 1 ; Write MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME so the correct context can be detected in the uninstaller.
  WriteUninstaller "uninstall.exe"
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  Delete "$INSTDIR\*.exe"
  DeleteRegKey ShCtx "${UNINSTKEY}"
  RMDir $INSTDIR
SectionEnd
