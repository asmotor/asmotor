#!/bin/sh
MYASM="../../../../build/scons/build_gameboy/motorgb -iinclude/ -z0"
MYLIB="../../../../build/scons/build_xlib/xlib"

$MYASM -oirq.obj irq.asm
$MYASM -outility.obj utility.asm

$MYLIB mos.lib a irq.obj utility.obj
