# "just" scripts

initialized_marker := ".initialized"
version_file := "build/version"
current_version := `cat build/version`
source_base_name := ("asmotor-" + current_version + "-src")
source_pkg_dir := join("build", source_base_name)
initialized := path_exists(initialized_marker)

tar := if path_exists("/opt/local/bin/gnutar") == "true" { "/opt/local/bin/gnutar" } else { "tar" }

# Show all recipes
default:
	@just --list


# Clean directory forcing a new build
clean:
	@rm -rf build/cmake {{initialized_marker}}


# Initialize repository for use
init:
	#!/usr/bin/env bash
	set -euo pipefail
	if ! {{initialized}}; then
		mkdir -p build/cmake/debug
		cd build/cmake/debug
		cmake -DASMOTOR_VERSION={{current_version}}.next -DCMAKE_BUILD_TYPE=Debug ../../..
		cd ../../..
		touch {{initialized_marker}}
	fi


# Build the project in Debug mode
build: init
	@cmake --build build/cmake/debug -- -j $(nproc)


# Build and install the project in Release mode, defaulting to $HOME/.local
install directory="$HOME/.local":
	#!/usr/bin/env bash
	set -euo pipefail
	rm -rf build/cmake/release
	mkdir -p build/cmake/release
	cd build/cmake/release
	cmake -DASMOTOR_VERSION=`cat ../../../build/version` -DCMAKE_INSTALL_PREFIX={{directory}} -DCMAKE_BUILD_TYPE=Release ../../..
	cd ../../..
	cmake --build build/cmake/release --target install -- -j $(nproc)


# Set the ASMotor version number to use when building
set-version version:
	@echo -n {{version}} >{{version_file}}
	@rm -f {{initialized_marker}}


# Build source package
source version: (set-version version) _clean_src_dir (_copy_dir_to_src "util" "xasm/6502" "xasm/680x0" "xasm/common" "xasm/dcpu-16" "xasm/mips" "xasm/rc8" "xasm/schip" "xasm/z80" "xlink" "xlib")
	#!/usr/bin/env bash
	set -euo pipefail

	cp xasm/CMakeLists.txt {{source_pkg_dir}}/xasm
	cp CMakeLists.txt CHANGELOG.md LICENSE.md README.md ucm.cmake *.sh *.ps1 {{source_pkg_dir}}

	mkdir -p {{source_pkg_dir}}/build
	cp -rf build/*.cmake build/version build/Modules {{source_pkg_dir}}/build

	{{tar}} -C build -cvjf {{source_base_name}}.tar.bz2 {{source_base_name}}
	{{tar}} -C build -cvzf {{source_base_name}}.tgz {{source_base_name}}
	rm -rf {{source_pkg_dir}}


# Tag, build and release a source package to github
publish version: (source version) && (set-version (version + "-next"))
	#!/usr/bin/env bash
	set -euo pipefail
	git tag -f {{version}} -m "Tagged {{version}}"
	git push
	git push --tags
	gh release create "{{version}}" {{source_base_name}}.* -d -n "Version {{version}}" -p -t "Version {{version}}"


_copy_dir_to_src +DIRS:
	#!/usr/bin/env bash
	set -euo pipefail

	for dir in {{DIRS}}; do
		mkdir -p {{source_pkg_dir}}/$dir
		cp $dir/*.[ch] {{source_pkg_dir}}/$dir
		cp $dir/CMake* {{source_pkg_dir}}/$dir
	done


_clean_src_dir:
	#!/usr/bin/env bash
	set -euo pipefail
	rm -f build/*-src.tar.bz2
	rm -f build/*-src.tgz
	rm -rf {{source_pkg_dir}}
	mkdir {{source_pkg_dir}}
