#!/bin/sh
$DESTDIR="$home"

mkdir "$DESTDIR\bin"

Remove-Item 'build/windows/release' -Recurse
mkdir 'build/windows/release'

Set-Location 'build/windows/release'
cmake -D ASMOTOR_VERSION=$(Get-Content ..\..\..\build\version) -D CMAKE_INSTALL_PREFIX=$DESTDIR --config Release ..\..\..
Set-Location '..\..\..'

cmake --build build\windows\release --config Release
cmake --install build\windows\release --config Release
