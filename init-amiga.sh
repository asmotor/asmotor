#!/bin/sh
rm -rf build/amiga
mkdir -p build/amiga/release
cd build/amiga/release
cmake -DASMOTOR_VERSION=`cat ../../../build/version`.next -DM68K_CRT=nix20 -DCMAKE_BUILD_TYPE=Release -DTOOLCHAIN_PATH=~/.local -DCMAKE_TOOLCHAIN_FILE=../../m68k-amigaos.cmake ../../..
cd ../../..

mkdir -p build/amiga/release_020_881
cd build/amiga/release_020_881
cmake -DASMOTOR_VERSION=`cat ../../../build/version`.next -DM68K_CPU=68020 -DM68K_FPU=hard -DM68K_CRT=nix20 -DCMAKE_BUILD_TYPE=Release -DTOOLCHAIN_PATH=~/.local -DCMAKE_TOOLCHAIN_FILE=../../m68k-amigaos.cmake ../../..
cd ../../..
