#!/bin/sh
git tag $1
cd build
echo -n $1 >version
./build_source.sh
gh release create "$1" asmotor-$1-src.* -d -n "Version $1" -p -t "Version $1"
echo -n $1.next >version
