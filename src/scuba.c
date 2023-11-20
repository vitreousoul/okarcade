#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "../lib/raylib.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#include "types.h"
#include "core.c"
#include "raylib_helpers.h"
#include "math.c"
#include "ui.c"

int SCREEN_WIDTH = 1200;
int SCREEN_HEIGHT = 700;
#define TARGET_FPS 30

global_variable Color BackgroundColor = (Color){58, 141, 230, 255};
typedef struct
{
} app_state;


#if defined(PLATFORM_WEB)
EM_JS(void, UpdateCanvasDimensions, (), {
    var canvas = document.getElementById("canvas");
    var body = document.querySelector("body");

    if (canvas && body) {
        var rect = body.getBoundingClientRect();
        canvas.width = rect.width;
        canvas.height = rect.height;
    }
});

EM_JS(s32, GetCanvasWidth, (), {
    var canvas = document.getElementById("canvas");
    if (canvas) {
        var rect = canvas.getBoundingClientRect();
        return canvas.width;
    } else {
        return -1.0;
    }
});

EM_JS(s32, GetCanvasHeight, (), {
    var canvas = document.getElementById("canvas");
    if (canvas) {
        var rect = canvas.getBoundingClientRect();
        return canvas.height;
    } else {
        return -1.0;
    }
});
#endif

internal void UpdateAndRender(void *VoidAppState)
{
    app_state *AppState = (app_state *)VoidAppState;

    BeginDrawing();
    ClearBackground(BackgroundColor);
    EndDrawing();
}

internal app_state InitAppState(void)
{
    app_state AppState;

    return AppState;
}

int main(void)
{
#if defined(PLATFORM_WEB)
    UpdateCanvasDimensions();

    s32 CanvasWidth = GetCanvasWidth();
    s32 CanvasHeight = GetCanvasHeight();
    if (CanvasWidth > 0.0f && CanvasHeight > 0.0f)
    {
        SCREEN_WIDTH = CanvasWidth;
        SCREEN_HEIGHT = CanvasHeight;
    }
#endif

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "SCUBA");

    app_state AppState = InitAppState();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop_arg(UpdateAndRender, &AppState, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        UpdateAndRender(&AppState);
    }
#endif

    CloseWindow();
}
