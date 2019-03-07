#!/bin/sh
rm -rf build/cmake/release
cmake -DCMAKE_INSTALL_PREFIX=$HOME/ -DCMAKE_BUILD_TYPE=Release -S . -B build/cmake/release
cd build/cmake/release
make install 
