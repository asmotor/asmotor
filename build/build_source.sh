#!/bin/sh
CopyDirectory()
{
	mkdir -p source/$1
	cp ../$1/*.[ch] source/$1
}

rm -rf source
mkdir source

mkdir -p source/build/scons
cp scons/SCons* source/build/scons

CopyDirectory util
CopyDirectory xasm/common
CopyDirectory xasm/dcpu-16
CopyDirectory xasm/6502
CopyDirectory xasm/680x0
CopyDirectory xasm/gameboy
CopyDirectory xasm/mips
CopyDirectory xasm/schip
CopyDirectory xgbfix
CopyDirectory xlink
CopyDirectory xlib

TAR=tar
if [ -x /opt/local/bin/gnutar ]; then
	TAR=/opt/local/bin/gnutar
fi

DIR=asmotor-`cat version`-src
mv source $DIR
$TAR -cvjf $DIR.tar.bz2 $DIR
$TAR -cvjf $DIR.tgz $DIR
rm -rf $DIR
