#!/bin/sh
test() {
	echo Testing $i
	../../build/cmake/debug/xasm/6502/qasm6502 $1 >$1.out 2>$1.err
	cat $1.r $1.out $1.err >$1.output 2>/dev/null
	rm $1.r $1.out $1.err 2>/dev/null
	diff -Z $1.output $1.answer
	if [ $? -eq 0 ]; then
		rm $1.output
	fi
}

for i in *.asm; do
	test $i
done

