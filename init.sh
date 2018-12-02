#!/bin/sh
mkdir -p build/cmake/release
cd build/cmake/release
cmake -DCMAKE_BUILD_TYPE=Release ../../..
cd ../../..

mkdir -p build/cmake/debug
cd build/cmake/debug
cmake -DCMAKE_BUILD_TYPE=Debug ../../..
cd ../../..

