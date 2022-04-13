#!/bin/bash
for command in just git cmake; do
	if ! command -v $command >/dev/null; then
		packages="$command $packages"
	fi
done

if [ "$packages" != "" ]; then
	echo "Required packages: $packages"
	if command -v apt-get >/dev/null; then
		echo -n "You have apt-get installed, would you like to install these packages [Y/n]? "
		read choice
		if [ "${choice:-Y}" == "Y" ]; then
			sudo apt-get -y install $packages build-essential || exit 1
		else
			exit 1
		fi
	else
		echo "Please install commands $packages."
	fi
fi

DIR=_asmotor_bootstrap
git clone --recursive https://github.com/asmotor/asmotor.git $DIR
cd $DIR && just install $1 && cd ..
rm -rf $DIR
