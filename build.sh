#!/usr/bin/env sh

WEB=0
DEBUG=1

if [ ! -d "dist" ]; then
    mkdir dist
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
    echo "Optimized build";
    TARGET="-O2 -o dist/$TARGET_NAME.exe"
elif [ $DEBUG -eq 1 ]; then
    echo "Debug build";
    TARGET="-g3 -O0 -o dist/$TARGET_NAME.out"
fi

if [ ! -d "dist" ]; then
  mkdir dist
fi

if [ $WEB -eq 1 ]; then
    RAYLIB_LIB="./lib/libraylibweb.a"
    RAYLIB_INCLUDE="-I. -I./lib/raylib.h -L. -L$RAYLIB_LIB"
    WEB_CONFIG="-s USE_GLFW=3 -s ASYNCIFY"
    SHELL_FILE="--shell-file gen/l_system.html"

    emcc -o site/l_system.html src/l_system.c -Os -Wall $RAYLIB_LIB $RAYLIB_INCLUDE $WEB_CONFIG -DPLATFORM_WEB $SHELL_FILE
else
    GRAPHICS_FRAMEWORKS="-framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL"
    GRAPHICS_LIB="lib/libraylib.a"

    SETTINGS="-std=c99 -Wall -Wextra -Wstrict-prototypes -Wold-style-definition -Wno-comment"
    SETTINGS="$SETTINGS -Wmissing-prototypes -Wmissing-declarations"
    SOURCE_FILE="./src/$TARGET_NAME.c"

    gcc $GRAPHICS_FRAMEWORKS $GRAPHICS_LIB $SETTINGS $SOURCE_FILE $TARGET
fi
