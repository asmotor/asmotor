#!/bin/sh
DIR=asmotor-`cat version`-src

CopyDirectory()
{
	mkdir -p $DIR/$1
	cp ../$1/*.[ch] $DIR/$1
	cp ../$1/CMake* $DIR/$1
}

rm *-src.tar.bz2
rm *-src.tgz
rm -rf $DIR
mkdir $DIR

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

cp ../CMakeLists.txt $DIR
cp ../xasm/CMakeLists.txt $DIR/xasm
cp ../CHANGELOG.md $DIR
cp ../LICENSE.md $DIR
cp ../README.md $DIR
cp ../ucm.cmake $DIR
cp ../*.sh $DIR
cp ../*.ps1 $DIR

mkdir -p $DIR/build
cp *.cmake $DIR/build
cp version $DIR/build
cp -rf Modules $DIR/build

TAR=tar
if [ -x /opt/local/bin/gnutar ]; then
	TAR=/opt/local/bin/gnutar
fi

$TAR -cvjf $DIR.tar.bz2 $DIR
$TAR -cvzf $DIR.tgz $DIR
rm -rf $DIR
