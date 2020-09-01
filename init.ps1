Remove-Item -Force -Recurse build/cmake
$version = Get-Content .\build\version
cmake -DASMOTOR_VERSION=$($version).next -DCMAKE_BUILD_TYPE=Debug -S . -B build/cmake/debug
cmake -DASMOTOR_VERSION=$($version).next -DCMAKE_INSTALL_PREFIX=bin -DCMAKE_BUILD_TYPE=Release -S . -B build/cmake/release
