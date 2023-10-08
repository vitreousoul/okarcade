#!/usr/bin/env sh

if [ -z "$EMSDK_PATH" ]; then
        echo "EMSDK_PATH is undefined"
        exit 1
fi

cd $EMSDK_PATH
# TODO add option to install/activate latest
# ./emsdk install latest
# ./emsdk activate latest
source ./emsdk_env.sh
