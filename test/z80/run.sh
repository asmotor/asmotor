#!/bin/sh
function test {
	echo Testing $i
	../../build/scons/build_gameboy/motorgb -mcz -fv -o$1.output $1
	diff $1.output $1.answer
	if [ $? -eq 0 ]; then
		rm $1.output
	fi
}

for i in *.asm; do
	test $i
done

