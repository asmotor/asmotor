#!/bin/sh
MYASM="../../../build/cmake/debug/xasm/z80/motorz80 -mcg -iinclude/ -z0"
MYLIB="../../../build/cmake/debug/xlib/xlib"

$MYASM -oirq.obj irq.asm
$MYASM -outility.obj utility.asm

$MYLIB mos.lib a irq.obj utility.obj
