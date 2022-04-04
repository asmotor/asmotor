#!/bin/bash
for command in just git cmake john; do
	if ! command -v $command >/dev/null; then
		echo "Required commands: cmake, git, just"
		if command -v apt-get >/dev/null; then
			echo -n "You have apt-get installed, would you like to install these packages (Y/n)? "
			read choice
			if [ "${choice:-Y}" == "Y" ]; then
				sudo apt-get -y install cmake git just build-essential
			fi
		fi
		exit
	fi
done

DIR=asmotor_bootstrap
git clone --recursive https://github.com/asmotor/asmotor.git $DIR
cd $DIR && ./install.sh $1 && cd ..
rm -rf $DIR
