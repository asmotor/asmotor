#!/bin/sh
DESTDIR=${1:-$HOME/} 

rm -rf build/cmake/release
cmake -DCMAKE_INSTALL_PREFIX=$DESTDIR -DCMAKE_BUILD_TYPE=Release -S . -B build/cmake/release
cmake --build build/cmake/release -j 4 --target install
