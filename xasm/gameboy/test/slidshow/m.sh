#!/bin/sh
MYASM="../../../../build/scons/build_gameboy/motorgb -i../minios/include/"
MYLINK="../../../../build/scons/build_xlink/xlink"
MYFIX="../../../../build/scons/build_xgbfix/xgbfix"

$MYASM -omain.obj main.asm
$MYASM -ouser.obj user.asm
$MYASM -opics.obj pics.asm

$MYLINK -mmap.map -tg -ogbfade.gb main.obj user.obj pics.obj ../minios/mos.lib
$MYFIX -tGBFADE -p -v gbfade.gb
