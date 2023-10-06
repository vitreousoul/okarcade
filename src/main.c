#include <stdlib.h>
#include <stdio.h>

#include "../lib/raylib.h"

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
