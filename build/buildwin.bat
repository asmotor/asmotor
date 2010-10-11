@echo off

set VS10="%VS100COMNTOOLS%..\IDE\devenv.exe"
set NSIS=%PROGRAMFILES(x86)%\NSIS\nsis.exe
set SLN="Visual Studio 2010\Windows.sln"

echo Building Release Win32
%VS10% %SLN% /Rebuild "Release|Win32"
IF ERRORLEVEL 1 GOTO :Error
del "Visual Studio 2010\Win32\Release\*.pdb"

echo Building Release x64
%VS10% %SLN% /Rebuild "Release|x64"
IF ERRORLEVEL 1 GOTO :Error
del "Visual Studio 2010\x64\Release\*.pdb"

GOTO Done

:Error
ECHO Failed
pause

:Done
