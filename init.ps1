Remove-Item -Recurse build/cmake
cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build/cmake/debug
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build/cmake/release
