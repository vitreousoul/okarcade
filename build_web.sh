#!/usr/bin/env sh

cd dist
echo "Preprocessing..."
./main.out preprocess
cd ..

RAYLIB_LIB="./lib/libraylibweb.a"
RAYLIB_INCLUDE="-I. -I./lib/raylib.h -L. -L$RAYLIB_LIB"
WEB_CONFIG="-s USE_GLFW=3 -s ASYNCIFY"
SHELL_FILE="--shell-file gen/l_system.html"

emcc -o dist/l_system.html src/l_system.c -Os -Wall $RAYLIB_LIB $RAYLIB_INCLUDE $WEB_CONFIG -DPLATFORM_WEB $SHELL_FILE
