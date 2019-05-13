#!/bin/sh
DIR=asmotor_bootstrap
git clone --recursive https://github.com/asmotor/asmotor.git $DIR
cd $DIR && ./install.sh $1 && cd ..
rm -rf $DIR
