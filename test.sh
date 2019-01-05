#!/bin/sh
./b.sh
cd test

cd gameboy
./run.sh
cd ..

cd z80
./run.sh
cd ..

cd common
./run.sh
cd ..

cd 680x0
./run.sh
cd ..

cd ..
