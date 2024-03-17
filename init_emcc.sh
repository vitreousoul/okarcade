#!/usr/bin/env sh

#
# NOTE: This script assumes that the emsdk source is a sibling directory to okarcade.
# NOTE: Run using "source" so that the shell session can access environment-variables
# that are set when calling "emsdk_env.sh".
#       Ex.
#           source init_emcc.sh
#

cd ../emsdk
./emsdk activate latest
source ./emsdk_env.sh
cd ../okarcade
