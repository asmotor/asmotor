#!/bin/sh
DESTDIR=${1:-$HOME/} 

rm -rf build/cmake/release
cmake -DCMAKE_INSTALL_PREFIX=$DESTDIR -DCMAKE_BUILD_TYPE=Release -S . -B build/cmake/release
cd build/cmake/release
make install 
