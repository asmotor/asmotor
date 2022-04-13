#!/bin/sh
$DESTDIR="$home"

New-Item -Path "$DESTDIR\bin" -Force -ItemType "directory"
Remove-Item 'build\windows\release' -Recurse
New-Item -Path 'build\windows\release' -Force -ItemType "directory"

Set-Location -Path 'build\windows\release'
cmake -D ASMOTOR_VERSION=$(Get-Content ..\..\..\build\version) -D CMAKE_INSTALL_PREFIX=$DESTDIR --config Release ..\..\..
Set-Location -Path '..\..\..'

cmake --build build\windows\release --config Release
cmake --install build\windows\release --config Release
