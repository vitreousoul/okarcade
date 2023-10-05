#!/usr/bin/env sh

RAYLIB_LIB="./lib/libraylibweb.a"
RAYLIB_INCLUDE="-I. -I./lib/raylib.h -L. -L$RAYLIB_LIB"
WEB_CONFIG="-s USE_GLFW=3 -s ASYNCIFY"

# --shell-file path-to/shell.html
emcc -o dist/game.html src/main.c -Os -Wall $RAYLIB_LIB $RAYLIB_INCLUDE $WEB_CONFIG -DPLATFORM_WEB
