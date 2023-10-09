#include <stdlib.h>
#include <stdio.h>

#include "../lib/raylib.h"

#define internal static
#define global_variable static

typedef uint8_t u8;
typedef uint32_t u32;

typedef int32_t s32;

typedef uint32_t b32;

typedef float f32;

#include "platform.h"
#include "ui.c"

int SCREEN_WIDTH = 600;
int SCREEN_HEIGHT = 400;
#define TARGET_FPS 30

#include "./l_system.c"

int main(void)
{
    int Result = 0;

    RUN_LSystem();

    return Result;
}
