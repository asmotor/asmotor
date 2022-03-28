#!/bin/sh
test() {
	echo Testing $i
	../../build/cmake/debug/xasm/rc8/motorrc8 -fv -o$1.r $1 >$1.out 2>$1.err
	cat $1.r $1.out $1.err >$1.output 2>/dev/null
	rm $1.r $1.out $1.err 2>/dev/null
	diff -Z $1.answer $1.output
	if [ $? -eq 0 ]; then
		rm $1.output
	fi
}

for i in *.rc8; do
	test $i
done

