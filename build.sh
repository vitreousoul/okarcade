#!/usr/bin/env sh

GRAPHICS_FRAMEWORKS="-framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL"
GRAPHICS_LIB="lib/libraylib.a"

DEBUG=1
SETTINGS="-std=c99 -Wall -Wextra -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Wno-comment"
SOURCE_FILE="./src/main.c"

if [ ! -d "dist" ]; then
    mkdir dist
fi

if [ $DEBUG -eq 0 ]; then
    echo "Optimized build";
    TARGET="-O2 -o dist/main.exe"
elif [ $DEBUG -eq 1 ]; then
    echo "Debug build";
    TARGET="-g3 -O0 -o dist/main.out"
fi

if [ ! -d "dist" ]; then
  mkdir dist
fi

gcc $GRAPHICS_FRAMEWORKS $GRAPHICS_LIB $SETTINGS $SOURCE_FILE $TARGET
