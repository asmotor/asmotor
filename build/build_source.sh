#!/bin/sh
CopyDirectory()
{
	mkdir -p source/$1
	cp ../$1/*.[ch] source/$1
	cp ../$1/CMake* source/$1
}

rm -rf source
mkdir source

CopyDirectory util
CopyDirectory xasm/6502
CopyDirectory xasm/680x0
CopyDirectory xasm/common
CopyDirectory xasm/dcpu-16
CopyDirectory xasm/mips
CopyDirectory xasm/rc8
CopyDirectory xasm/schip
CopyDirectory xasm/z80
CopyDirectory xlink
CopyDirectory xlib

cp ../CMakeLists.txt source
cp ../xasm/CMakeLists.txt source/xasm
cp ../README.md source
cp ../ucm.cmake source
cp ../*.sh source
cp ../*.ps1 source

mkdir -p source/build
cp *.cmake source/build
cp version source/build
cp -rf Modules source/build

TAR=tar
if [ -x /opt/local/bin/gnutar ]; then
	TAR=/opt/local/bin/gnutar
fi

DIR=asmotor-`cat version`-src
mv source $DIR
$TAR -cvjf $DIR.tar.bz2 $DIR
$TAR -cvzf $DIR.tgz $DIR
rm -rf $DIR
