#!/bin/sh
rm -rf build/amiga
mkdir -p build/amiga/release
cd build/amiga/release
cmake -DASMOTOR_VERSION=`cat ../../../build/version`.next -DCMAKE_BUILD_TYPE=Release -DTOOLCHAIN_PATH=~/.local -DCMAKE_TOOLCHAIN_FILE=../../m68k-amigaos.cmake ../../..
cd ../../..
