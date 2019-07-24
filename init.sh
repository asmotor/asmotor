#!/bin/sh
rm -rf build/cmake

mkdir -p build/cmake/release
cd build/cmake/release
cmake -DASMOTOR_VERSION=`cat ../../../build/version`.next -DCMAKE_BUILD_TYPE=Release ../../..
cd ../../..

mkdir -p build/cmake/debug
cd build/cmake/debug
cmake -DASMOTOR_VERSION=`cat ../../../build/version`.next -DCMAKE_BUILD_TYPE=Debug ../../..
cd ../../..
