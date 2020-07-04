#!/bin/sh
ASM=../../../build/cmake/debug/xasm/rc8/motorrc8
EMU=../../../../rc8-emu/target/debug/rc8-emu

$ASM -fb -oload.bin load.rc8
#$EMU -d load.bin >load.output
$EMU load.bin

$ASM -fb -omandelbrot.bin mandelbrot.rc8
#$EMU -d mandelbrot.bin >mandelbrot.output
$EMU mandelbrot.bin 
