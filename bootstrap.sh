#!/bin/sh
DIR=asmotor_bootstrap
git clone --recursive https://github.com/asmotor/asmotor.git $DIR && cd $DIR && . install.sh && cd .. && rm -rf $DIR
