# "just" scripts

initialized_marker := ".initialized"
version_file := "build/version"
current_version := `cat build/version`
source_base_name := "asmotor-`cat build/version`"
source_pkg_dir := join("build", source_base_name)
initialized := path_exists(initialized_marker)

tar := if path_exists("/opt/local/bin/gnutar") == "true" { "/opt/local/bin/gnutar" } else { "tar" }

# Show all recipes
@default:
	just --list


# Clean directory forcing a new build
@clean: _clean_src_dir
	rm -rf build/cmake {{initialized_marker}} makedeb


# Initialize repository for use
@init:
	if ! {{initialized}}; then \
		mkdir -p build/cmake/debug; \
		cd build/cmake/debug; \
		cmake -DASMOTOR_VERSION={{current_version}}.next -DCMAKE_BUILD_TYPE=Debug ../../..; \
		cd ../../..; \
		touch {{initialized_marker}}; \
	fi


# Build the project in Debug mode
@build: init
	cmake --build build/cmake/debug


# Build and install the project in Release mode, defaulting to $HOME/.local
@install directory="$HOME/.local":
	rm -rf build/cmake/release
	mkdir -p build/cmake/release
	cd build/cmake/release; cmake -DASMOTOR_VERSION={{current_version}} -DCMAKE_INSTALL_PREFIX={{directory}} -DCMAKE_BUILD_TYPE=Release ../../..; cd ../../..
	cmake --build build/cmake/release --target install -- -j $(nproc)


# Set the ASMotor version number to use when building
@set-version version:
	echo -n {{version}} >{{version_file}}
	rm -f {{initialized_marker}}


# Build source package
@source version: (set-version version) _clean_src_dir (_copy_dir_to_src "util" "xasm/6502" "xasm/680x0" "xasm/common" "xasm/dcpu-16" "xasm/mips" "xasm/rc8" "xasm/schip" "xasm/z80" "xlink" "xlib")
	cp xasm/CMakeLists.txt {{source_pkg_dir}}/xasm
	cp .justfile CMakeLists.txt CHANGELOG.md LICENSE.md README.md ucm.cmake *.sh *.ps1 {{source_pkg_dir}}

	mkdir -p {{source_pkg_dir}}/build
	cp -rf build/*.cmake build/version build/Modules {{source_pkg_dir}}/build

	{{tar}} -C build -cvjf {{source_base_name}}.tar.bz2 {{source_base_name}}
	{{tar}} -C build -cvzf {{source_base_name}}.tgz {{source_base_name}}
	rm -rf {{source_pkg_dir}}


# Tag, build and release a source package to github
@publish version: (source version) && (set-version (version + ".next"))
	git tag -f {{version}} -m "Tagged {{version}}"
	git push
	git push --tags
	gh release create "{{version}}" {{source_base_name}}.* -d -n "Version {{version}}" -p -t "Version {{version}}"


# Build a .deb distribution package
deb: (source current_version)
	#!/bin/sh
	set -eu

	mkdir -p makedeb
	cp "{{source_base_name}}.tar.bz2" makedeb
	cat >makedeb/PKGBUILD <<EOF
	# Maintainer: Carsten Elton Sorensen <cso@rift.dk>
	pkgname=asmotor
	pkgver={{current_version}}
	pkgrel=1
	pkgdesc="Cross assembler package for several CPU's"
	arch=("{{arch()}}")
	url="https://github.com/asmotor/asmotor"
	license=("GPL-3")
	makedepends=("cmake" "build-essential")
	minimum_libc=2.14
	depends=("libc6>=\${minimum_libc}")
	source=("{{source_base_name}}.tar.bz2")
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
	cd makedeb
	makedeb
	mv *.deb ..
	cd ..
	rm -rf makedeb
	ls -1 *.deb


@_copy_dir_to_src +DIRS:
	for dir in {{DIRS}}; do \
		mkdir -p {{source_pkg_dir}}/$dir; \
		cp $dir/*.[ch] $dir/CMake* {{source_pkg_dir}}/$dir; \
	done


@_clean_src_dir:
	rm -rf asmotor-*.tar.bz2 asmotor-*.tgz {{source_pkg_dir}}
	mkdir {{source_pkg_dir}}
