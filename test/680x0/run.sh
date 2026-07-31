#!/bin/bash

FAILURES=0

test() {
	echo Test assembling $1
	echo EMPTY >$1.bin
	../../build/cmake/debug/xasm/680x0/motor68k -mga -f$2 -o$1.bin $1 >$1.out 2>$1.err
	od -t x1 $1.bin | sed 's/  */ /g' | sed -e '$a\' >$1.r
	cat $1.r $1.out $1.err >$1.obj.output 2>/dev/null
	rm $1.bin $1.r $1.out $1.err 2>/dev/null
	if ! diff -b $1.obj.output $1.obj.answer >/dev/null 2>&1; then
		echo "FAIL: $1"
		FAILURES=$((FAILURES + 1))
	else
		rm -f $1.obj.output
	fi
}

testlink() {
	echo Test linking $1
	echo EMPTY >$1.bin
	../../build/cmake/debug/xasm/680x0/motor68k -mga -o$1.obj $1 >$1.out 2>$1.err
	../../build/cmake/debug/xlink/xlink -t$2 -o$1.bin $1.obj >>$1.out 2>>$1.err
	od -t x1 $1.bin | sed 's/  */ /g' | sed -e '$a\' >$1.r
	cat $1.r $1.out $1.err >$1.bin.output 2>/dev/null
	rm -f $1.bin $1.r $1.out $1.err 2>/dev/null
	if ! diff -b $1.bin.output $1.bin.answer >/dev/null 2>&1; then
		echo "FAIL: $1"
		FAILURES=$((FAILURES + 1))
	else
		rm -f $1.bin.output
	fi
}

test amigaexe.68k g
test amigaobj.68k h
test test.68k b
test fpu.68k b
test 68080.68k b
test odd-instr.68k b
test issue15.68k b
test issue17.68k b
test 68040.68k b

testlink amigaexe.68k a
testlink amigaobj.68k b

exit $FAILURES

