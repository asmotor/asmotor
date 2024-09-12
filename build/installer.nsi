!include "x64.nsh"

Name "ASMotor"
OutFile "setup-asmotor.exe"

; Registry key to check for directory (so if you install again, it will overwrite the old one automatically)
InstallDirRegKey HKLM "Software\ASMotor" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

Function .onInit
  ${If} ${RunningX64}
    StrCpy $INSTDIR $PROGRAMFILES64\ASMotor
  ${Else}
    StrCpy $INSTDIR $PROGRAMFILES\ASMotor
  ${EndIf}
FunctionEnd

;--------------------------------

; Pages

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "Executables (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  ${If} ${RunningX64}
    SetOutPath $INSTDIR
    File "../_bin_w64/*.exe"
  ${Else}
    SetOutPath $INSTDIR
    File "../_bin_w32/*.exe"
  ${EndIf}
  
  ; Put file there
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\ASMotor "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Example2" "DisplayName" "ASMotor"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Example2" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Example2" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Example2" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\ASMotor"
  CreateShortCut "$SMPROGRAMS\ASMotor\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
;  CreateShortCut "$SMPROGRAMS\ASMotor\Documentation.lnk" "$INSTDIR\ASMotor.pdf" "" "$INSTDIR\ASMotor.pdf" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ASMotor"
  DeleteRegKey HKLM SOFTWARE\ASMotor

  ; Remove files and uninstaller
  Delete $INSTDIR\*.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\ASMotor\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\ASMotor"
  RMDir "$INSTDIR"

SectionEnd
