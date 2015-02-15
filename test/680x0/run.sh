#!/bin/sh
function test {
	echo Test assembling $1
	../../build/scons/build_680x0/motor68k -f$2 -o$1.bin $1 >$1.out 2>&1
    od -t x1 $1.bin | sed 's/  */ /g' | sed -e '$a\' >$1.r
	cat $1.r $1.out >$1.output 2>/dev/null
	rm $1.bin $1.r $1.out 2>/dev/null
	diff $1.output $1.answer
	if [ $? -eq 0 ]; then
		rm $1.output
	fi
}

function testlink {
	echo Test linking $1
	../../build/scons/build_680x0/motor68k -o$1.obj $1 >$1.out 2>&1
	../../build/scons/build_xlink/xlink -ta -o$1.bin $1.obj >>$1.out 2>&1
	od -t x1 $1.bin | sed 's/  */ /g' | sed -e '$a\' >$1.r
	cat $1.r $1.out >$1.output 2>/dev/null
	rm $.bin $1.r $1.out 2>/dev/null
	diff $1.output $1.answer
	if [ $? -eq 0 ]; then
		rm $1.output
	fi
}

test amigaexe.68k g
test amigaobj.68k h
test test.68k b

testlink amigaexe.68k

