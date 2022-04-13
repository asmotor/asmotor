#!/bin/bash

for command in just git cmake; do
	if ! command -v $command >/dev/null; then
		packages="$command $packages"
	fi
done

install_packages() {
	if command -v $1 >/dev/null; then
		echo -n "You have the package manager \"$1\" installed, would you like to install these packages [Y/n]? "
		read choice
		if [[ "${choice:-Y}" =~ ^[Yy]$ ]]; then
			$2 || exit 1
			echo "Packages installed successfully"
			return 0
		fi
	fi
	return 1
}

exit_error() {
	echo $1
	exit 1
}

if [ "$packages" != "" ]; then
	echo "Required packages: $packages"
	install_packages "brew" "brew install $packages" || \
	install_packages "port" "sudo port -N install $packages" || \
	install_packages "apt-get" "sudo apt-get -y install $packages build-essential" || \
	install_packages "dnf" "sudo dnf -y install $packages" || \
	exit_error "Please install missing commands before proceeding"
fi

just_version=`just -V | echo -e 1.0.1\\\\n$(grep -o -P "(\\d\.)+\\d") | sort -t "." -k1,1n -k2,2n -k3,3n | head -1`
if [ "$just_version" != "1.0.1" ]; then
	echo "\"just\" version 1.0.1 or later required, installed version is $just_version"
	exit 1
fi

DIR=/tmp/_asmotor_bootstrap
rm -rf $DIR
git clone --recursive https://github.com/asmotor/asmotor.git $DIR
cd $DIR && just -f .justfile install $1 && cd ..
rm -rf $DIR
