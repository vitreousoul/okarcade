#!/usr/bin/env sh

cd dist
echo "Preprocessing..."
./main.out preprocess
cd ..

APP_NAME="$1"

SETTINGS="-std=c99 -Wall -Wextra -Wstrict-prototypes -Wold-style-definition -Wno-comment"
SETTINGS="$SETTINGS -Wno-unused-function"
SETTINGS="$SETTINGS -Wno-unused-parameter"

RAYLIB_LIB="./lib/libraylibweb.a"
RAYLIB_INCLUDE="-I. -I./lib/raylib.h -L. -L$RAYLIB_LIB"
WEB_CONFIG="-s USE_GLFW=3 -s ASYNCIFY"
SHELL_FILE="--shell-file gen/$APP_NAME.html"

emcc -o site/$APP_NAME.html src/$APP_NAME.c -Os -Wall $SETTINGS $RAYLIB_LIB $RAYLIB_INCLUDE $WEB_CONFIG -DPLATFORM_WEB $SHELL_FILE
