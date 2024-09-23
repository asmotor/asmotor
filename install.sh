#!/bin/bash

# create TTY file descriptor #3 for "read"
exec 3<>/dev/tty

for command in just git cmake; do
	if ! command -v $command >/dev/null; then
		PACKAGES="$command $PACKAGES"
	fi
done

install_packages() {
	if command -v $1 >/dev/null; then
        read -u 3 -p "You have the package manager \"$1\" installed, would you like to install these packages [Y/n]? " choice
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

if [ "$PACKAGES" != "" ]; then
	echo "Required packages: $PACKAGES"
	install_packages "brew" "brew install $PACKAGES" || \
	install_packages "port" "sudo port -N install $PACKAGES" || \
	install_packages "apt-get" "sudo apt-get -y install $PACKAGES build-essential" || \
	install_packages "dnf" "sudo dnf -y install $PACKAGES" || \
	install_packages "pacman" "sudo pacman -Syu --noconfirm --needed $PACKAGES base-devel" || \
	exit_error "Please install missing commands before proceeding"
fi

JUST_VERSION=`just -V | echo -e 1.0.1\\\\n$(grep -o -P "(\\d\.)+\\d") | sort -t "." -k1,1n -k2,2n -k3,3n | head -1`
if [ "$JUST_VERSION" != "1.0.1" ]; then
	echo "\"just\" $JUST_VERSION installed but version 1.0.1 or later required."
	echo "For manual installation please follow the directions at https://github.com/casey/just"
	exit 1
fi

DIR=/tmp/_asmotor_bootstrap
rm -rf $DIR
git clone --recursive https://github.com/asmotor/asmotor.git $DIR
cd $DIR && just -f .justfile append-git-version install $1 && cd ..
rm -rf $DIR
