$DESTDIR="$home"
$REPO_URL="https://github.com/asmotor/asmotor.git"

$null = New-Item -Path "$DESTDIR\bin" -Force -ItemType "directory"
git clone $REPO_URL --recurse-submodules
Set-Location -Path "asmotor"
# git checkout installwin
# git submodule update

$null = New-Item -Path "build\windows\release" -Force -ItemType "directory"

Set-Location -Path 'build\windows\release'
cmake -D ASMOTOR_VERSION=$(Get-Content ..\..\..\build\version) -D CMAKE_INSTALL_PREFIX=$DESTDIR ..\..\..
Set-Location -Path '..\..\..'

cmake --build build\windows\release --config Release
cmake --install build\windows\release --config Release

Set-Location -Path '..'
Remove-Item -Path "asmotor" -Recurse -Force
