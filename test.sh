#!/bin/sh
./b.sh
cd test

for i in *; do
	cd $i
    ./run.sh
    cd ..
done

cd ..
