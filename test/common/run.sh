#!/bin/sh
test() {
	echo Testing $1
	../../build/cmake/debug/xasm/$2 -fv -o$1.r $1 >$1.out 2>$1.err
	cat $1.r $1.out $1.err >$1.output 2>/dev/null
	rm $1.r $1.out $1.err 2>/dev/null
	diff -Z $1.output $1.answer
	if [ $? -eq 0 ]; then
		rm $1.output
	fi
}

for asm in dcpu-16/motordcpu16 680x0/motor68k 6502/motor6502 z80/motorz80 mips/motormips schip/motorschip; do
	echo Using assembler $asm
	for i in *.asm; do
		test $i $asm
	done
done
