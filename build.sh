#!/usr/bin/env sh

COMPILER="gcc"
WEB=0
DEBUG=1

if [ ! -d "dist" ]; then
    mkdir dist
fi

if [ ! -d "save" ]; then
    mkdir save
fi

if [ -z "$1" ]; then
    echo "Please give an argument... I wish I could tell you more about your options...";
    exit 1
fi

if [ "$1" == "web" ]; then
    WEB=1;

    if [ -z "$2" ]; then
        echo "No target name specified for web build";
        exit 1;
    fi

    TARGET_NAME="$2";
else
    TARGET_NAME="$1";
fi

if [ $DEBUG -eq 0 ]; then
    echo "Optimized build : $TARGET_NAME";
    TARGET="-O2"
    EXECUTABLE_FILE="-o dist/$TARGET_NAME.exe"
elif [ $DEBUG -eq 1 ]; then
    echo "Debug build : $TARGET_NAME";
    TARGET="-g3 -O0"
    EXECUTABLE_FILE="-o dist/$TARGET_NAME.out"
fi

SETTINGS="-std=c99 -Wall -Wextra -Wstrict-prototypes -Wold-style-definition -Wno-comment"
SETTINGS="$SETTINGS -Wno-unused-function"
SETTINGS="$SETTINGS -Wno-unused-parameter"
# SETTINGS="$SETTINGS -Wmissing-prototypes -Wmissing-declarations"

SOURCE_FILE="./src/$TARGET_NAME.c"

# Save expanded macros
# MACRO_FILE="-E -o gen/$TARGET_NAME.i"
# $COMPILER $GRAPHICS_FRAMEWORKS $GRAPHICS_LIB $SETTINGS $SOURCE_FILE $TARGET $MACRO_FILE

if [ $WEB -eq 1 ]; then
    RAYLIB_LIB="./lib/libraylibweb.a"
    RAYLIB_INCLUDE="-I. -I./lib/raylib.h -L. -L$RAYLIB_LIB"
    WEB_CONFIG="-s USE_GLFW=3 -s ASYNCIFY"
    SHELL_FILE="--shell-file gen/l_system.html"

    emcc -o site/l_system.html src/l_system.c -Os -Wall $SETTINGS $RAYLIB_LIB $RAYLIB_INCLUDE $WEB_CONFIG -DPLATFORM_WEB $SHELL_FILE
else
    GRAPHICS_FRAMEWORKS="-framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL"
    GRAPHICS_LIB="lib/libraylib.a"

    $COMPILER $GRAPHICS_FRAMEWORKS $GRAPHICS_LIB $SETTINGS $SOURCE_FILE $TARGET $EXECUTABLE_FILE
fi
