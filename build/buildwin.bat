@echo off

set VS10="%VS100COMNTOOLS%..\IDE\devenv.exe"
set NSIS=%PROGRAMFILES(x86)%\NSIS\nsis.exe

echo Building Release Win32
%VS10% ..\asmotor.sln /Rebuild "Release|Win32"
IF ERRORLEVEL 1 GOTO :Error
del ..\Win32Release\*.pdb

echo Building Release x64
%VS10% ..\asmotor.sln /Rebuild "Release|x64"
IF ERRORLEVEL 1 GOTO :Error
del ..\x64Release\*.pdb

GOTO Done

:Error
ECHO Failed
pause

:Done
