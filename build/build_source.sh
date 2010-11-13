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
CopyDirectory xasm/6502
CopyDirectory xasm/680x0
CopyDirectory xasm/gameboy
CopyDirectory xasm/mips
CopyDirectory xgbfix
CopyDirectory xlink
CopyDirectory xlib

DIR=asmotor-`cat version`-src
mv source $DIR
gnutar -cvjf $DIR.tar.bz2 $DIR
rm -rf $DIR
