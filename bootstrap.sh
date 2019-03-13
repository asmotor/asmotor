#!/bin/sh
git clone --recursive https://github.com/asmotor/asmotor.git && cd asmotor && ./install.sh && cd .. && rm -rf asmotor
