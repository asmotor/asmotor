#!/bin/sh
rm -rf build/cmake
cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build/cmake/debug -G Ninja
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build/cmake/release -G Ninja
