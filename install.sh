#!/bin/sh
DESTDIR=${1:-$HOME/} 

rm -rf build/cmake/release
mkdir -p build/cmake/release
cd build/cmake/release
cmake -DCMAKE_INSTALL_PREFIX=$DESTDIR -DCMAKE_BUILD_TYPE=Release ../../..
cd ../../..
cmake --build build/cmake/release --target install
