#!/bin/sh
MYASM="../../../build/cmake/debug/xasm/z80/motorz80 -i../minios/include/ -mcg"
MYLINK="../../../build/cmake/debug/xlink/xlink"
MYFIX="../../../build/cmake/debug/xgbfix/xgbfix"

$MYASM -omain.obj main.asm
$MYASM -ouser.obj user.asm
$MYASM -opics.obj pics.asm

$MYLINK -mmap.map -tg -ogbfade.gb main.obj user.obj pics.obj ../minios/mos.lib
$MYFIX -tGBFADE -p -v gbfade.gb
