#!/bin/sh
rm -rf build/cmake
cmake -DASMOTOR_VERSION=`cat build/version`.next -DCMAKE_BUILD_TYPE=Debug -S . -B build/cmake/debug
cmake -DASMOTOR_VERSION=`cat build/version`.next -DCMAKE_BUILD_TYPE=Release -S . -B build/cmake/release
