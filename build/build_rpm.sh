#!/bin/sh

# Build source package
./build_source.sh

VERSION=`cat version`
PACKAGE=asmotor-$VERSION
SOURCE=$PACKAGE-src.tgz
PWD=`pwd`

# Make RPM build directory
rm -rf rpmtemp
mkdir -p rpmtemp/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# Copy sources
cp $SOURCE rpmtemp/SOURCES/$VERSION.tar.gz

# Create spec file
cat <<EOF >rpmtemp/SPECS/$PACKAGE.spec
Summary: A collection of cross assemblers for several architectures
Name: asmotor
Version: $VERSION
Release: 1
License: GPL3
Group: Applications/Development
Source: https://github.com/csoren/asmotor/archive/$VERSION.tar.gz
URL: https://github.com/csoren/asmotor
Vendor: Carsten Elton Sorensen
Packager: Carsten Elton Sorensen <csoren@gmail.com>

%description
ASMotor is a portable and generic assembler engine and development system written in ANSI C and licensed under the GNU Public License v3. The package consists of the assembler, the librarian and the linker. It can be used as either a cross or native development system.

The assembler syntax is based on the A68k style macro language.

Currently supported CPUs are the 680x0 family, 6809, 6502, MIPS32, Gameboy, Z80, 0x10c, Chip-8/SuperChip and the RC800 CPU core.

ASMotor is the spiritual successor to RGBDS, which was a fairly popular development package for the Gameboy. ASMotor is written by the original RGBDS author.

%prep
%setup -n $PACKAGE-src/build/scons

%build
scons

%install
scons install --prefix=\$RPM_BUILD_ROOT/usr

%files
/usr/bin/motor6502
/usr/bin/motor68k
/usr/bin/motordcpu16
/usr/bin/motormips
/usr/bin/motorrc8
/usr/bin/motorschip
/usr/bin/motorz80
/usr/bin/xlib
/usr/bin/xlink

EOF

rpmbuild --define "_topdir $PWD/rpmtemp" -ba rpmtemp/SPECS/$PACKAGE.spec
