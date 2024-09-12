# "just" scripts

initialized_marker := ".initialized"
version_file := join(invocation_directory(), "build/version")
version := `cat build/version`
package_base_name := ("asmotor-" + version)
source_name := (package_base_name + "-src.tar.gz")
binary_name := (package_base_name + "-bin-" + os() + ".tar.gz")
binary_windows_32_name := (package_base_name + "-bin-windows-32bit.zip")
binary_windows_64_name := (package_base_name + "-bin-windows-64bit.zip")
binary_macos_intel_name := (package_base_name + "-bin-macos-intel.zip")
binary_macos_arm_name := (package_base_name + "-bin-macos-arm.zip")
binary_amiga_name := (package_base_name + "-bin-amiga-")
source_pkg_dir := join("build", package_base_name)
initialized := path_exists(initialized_marker)

tar := if path_exists("/opt/local/bin/gnutar") == "true" { "/opt/local/bin/gnutar" } else { "tar" }

# Show all recipes
@default:
	just --list


# Clean directory forcing a new build
@clean: _clean_src_dir
	rm -rf build/cmake {{initialized_marker}} makedeb build/amiga


# Initialize repository for use
@init toolchain="":
	if ! {{initialized}}; then \
		mkdir -p build/cmake/debug; \
		cd build/cmake/debug; \
		cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_TOOLCHAIN_FILE={{toolchain}} -DASMOTOR_VERSION={{version}}.next -DCMAKE_BUILD_TYPE=Debug ../../..; \
		cd ../../..; \
		touch {{initialized_marker}}; \
	fi


# Build the project in Debug mode
@build toolchain="": init
	cmake --build build/cmake/debug


# Build and install the project in Release mode, defaulting to $HOME/.local
@install directory="$HOME/.local" sudo="" toolchain="" extra_params="":
	rm -rf build/cmake/release
	mkdir -p build/cmake/release
	cd build/cmake/release; cmake {{extra_params}} -DCMAKE_TOOLCHAIN_FILE={{toolchain}} -DASMOTOR_VERSION={{version}} -DCMAKE_INSTALL_PREFIX={{directory}} -DCMAKE_BUILD_TYPE=Release ../../..; cd ../../..
	cmake --build build/cmake/release -- -j
	{{sudo}} cmake --install build/cmake/release


# Set the ASMotor version number to use when building
@set-version new_version:
	echo -n {{new_version}} >{{version_file}}
	rm -f {{initialized_marker}}


# Build release archive with native C compiler
@binary_native: (install join(justfile_directory(), "_binary"))
	cd _binary/bin; {{tar}} -cvzf "../../{{binary_name}}" *
	rm -rf _binary

# Build release archive with MinGW i686 compiler
@binary_windows_32: (install join(justfile_directory(), "_binary_windows_32") "" "build/mingw64-i686.cmake")
	cd _binary_windows_32/bin; zip "../../{{binary_windows_32_name}}" *
	rm -rf _binary_windows_32

# Build release archive with MinGW x86_64 compiler
@binary_windows_64: (install join(justfile_directory(), "_binary_windows_64") "" "build/mingw64-x86_64.cmake")
	cd _binary_windows_64/bin; zip "../../{{binary_windows_64_name}}" *
	rm -rf _binary_windows_64

# Build release archive with osxcross x86_64 compiler
@binary_macos_intel: (install join(justfile_directory(), "_binary_macos_intel") "" "build/macos-intel.cmake")
	cd _binary_macos_intel/bin; zip "../../{{binary_macos_intel_name}}" *
	rm -rf _binary_macos_intel

# Build release archive with osxcross arm compiler
@binary_macos_arm: (install join(justfile_directory(), "_binary_macos_arm") "" "build/macos-arm.cmake")
	cd _binary_macos_arm/bin; zip "../../{{binary_macos_arm_name}}" *
	rm -rf _binary_macos_arm

@binary_macos: binary_macos_intel binary_macos_arm

@windows_installer: binary_windows_32 binary_windows_64
	rm -rf _bin_w64 _bin_w32
	mkdir _bin_w64 _bin_w32
	cd _bin_w64; unzip ../{{binary_windows_64_name}}
	cd _bin_w32; unzip ../{{binary_windows_32_name}}
	makensis ./build/installer.nsi
	mv ./build/setup-asmotor.exe .
	rm -rf _bin_w64 _bin_w32


# Build release archive with Amiga compiler, optimized for CPU (000, 020_881, 060)
@binary_amiga cpu="000" toolchain_path="/opt/amiga" extra_params="":
	#!/bin/sh
	JUSTFILE_DIR={{justfile_directory()}}
	BIN_DIR=$JUSTFILE_DIR/_binary_amiga_{{cpu}}
	just install $BIN_DIR "" "build/m68k-amigaos.cmake" "{{extra_params}} -DTOOLCHAIN_PATH={{toolchain_path}} -DM68K_CRT=nix20"
	cd _binary_amiga_{{cpu}}/bin
	zip "../../{{binary_amiga_name}}{{cpu}}.zip" *
	cd ..
	rm -rf _binary_amiga_{{cpu}}

# Build release archive with Amiga compiler, optimized for 68000
@binary_amiga_000 toolchain_path="/opt/amiga":
	just binary_amiga "000" {{toolchain_path}}

# Build release archive with Amiga compiler, optimized for 68020
@binary_amiga_020 toolchain_path="/opt/amiga":
	just binary_amiga "020" {{toolchain_path}} "-DM68K_CPU=68020"

# Build release archive with Amiga compiler, optimized for 68020 with FPU
@binary_amiga_020_fpu toolchain_path="/opt/amiga":
	just binary_amiga "020-fpu" {{toolchain_path}} "-DM68K_CPU=68020 -DM68K_FPU=hard"

# Build release archive with Amiga compiler, optimized for 68060 with FPU
@binary_amiga_060 toolchain_path="/opt/amiga":
	just binary_amiga "060-fpu" {{toolchain_path}} "-DM68K_CPU=68060 -DM68K_FPU=hard"


# Build source package
@source: _clean_src_dir (_copy_dir_to_src "util" "xasm/6502" "xasm/6809" "xasm/680x0" "xasm/motor" "xasm/dcpu-16" "xasm/mips" "xasm/rc8" "xasm/schip" "xasm/z80" "xlink" "xlib")
	cp xasm/CMakeLists.txt {{source_pkg_dir}}/xasm
	cp .justfile CMakeLists.txt CHANGELOG.md LICENSE.md README.md ucm.cmake *.sh *.ps1 {{source_pkg_dir}}

	mkdir -p {{source_pkg_dir}}/build
	cp -rf build/*.cmake build/version build/Modules {{source_pkg_dir}}/build

	{{tar}} -C build -cvzf {{source_name}} {{package_base_name}}
	rm -rf {{source_pkg_dir}}


# Tag, build and release a source package to github
@publish: source binary_windows_32 binary_windows_64 deb
	git tag -f {{version}} -m "Tagged {{version}}"
	git push
	git push --force --tags
	gh release create "{{version}}" {{binary_name}} {{source_name}} *.deb -d -n "Version {{version}}" -p -t "Version {{version}}"


# Build a .deb distribution package
deb: source
	#!/bin/sh
	set -eu

	rm -f *.deb
	rm -rf _makedeb
	mkdir -p _makedeb
	cp {{source_name}} _makedeb
	cat >_makedeb/PKGBUILD <<EOF
	# Maintainer: Carsten Elton Sorensen <cso@rift.dk>
	pkgname=asmotor
	pkgver={{version}}
	pkgrel=1
	pkgdesc="Cross assembler package for several CPUs"
	arch=("{{arch()}}")
	url="https://github.com/asmotor/asmotor"
	license=("GPL-3")
	makedepends=("cmake" "build-essential")
	minimum_libc=2.14
	depends=("libc6>=\${minimum_libc}")
	source=("{{source_name}}")
	md5sums=("SKIP")
	prepare() {
		echo "Checking if \"just\" installed"
		command -v just >/dev/null
	}
	package() {
		cd "\${pkgname}-\${pkgver}"
		just install "\${pkgdir}/"
		cd "\${pkgdir}/bin"
		_used_libc=\$(ldd -v * 2>/dev/null | grep GLIBC | sed 's/.*GLIBC_//' | sed 's/).*//' | sort -t "." -k1,1n -k2,2n -k3,3n | tail -n 1)
		if [ \${minimum_libc} != \${_used_libc} ]; then
			echo "Compiled tools will require libc6>=\${_used_libc} which is not equal to \${minimum_libc}"
			exit 1
		fi
	}
	EOF
	cd _makedeb
	makedeb
	mv *.deb ..
	cd ..
	rm -rf _makedeb
	ls -1 *.deb


test: build
	#!/bin/sh
	cd test
	for i in *; do
		cd $i
		./run.sh
		cd ..
	done


@_copy_dir_to_src +DIRS:
	for dir in {{DIRS}}; do \
		mkdir -p {{source_pkg_dir}}/$dir; \
		cp $dir/*.[ch] $dir/CMake* {{source_pkg_dir}}/$dir; \
	done


@_clean_src_dir:
	rm -rf asmotor-*.tar.bz2 asmotor-*.tgz {{source_pkg_dir}}
	mkdir {{source_pkg_dir}}

