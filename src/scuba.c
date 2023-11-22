#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "../lib/raylib.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#include "types.h"

int SCREEN_WIDTH = 1200;
int SCREEN_HEIGHT = 700;

#if defined(PLATFORM_WEB)
#include "raylib_defines.c"
#endif

#include "../gen/pigeon.h"
#include "core.c"
#include "raylib_helpers.h"
#include "math.c"
#include "ui.c"

global_variable Color BackgroundColor = (Color){58, 121, 120, 255};

typedef struct
{
    Texture2D PigeonTexture;
} app_state;


internal void UpdateAndRender(void *VoidAppState)
{
    app_state *AppState = (app_state *)VoidAppState;

    BeginDrawing();
    ClearBackground(BackgroundColor);
    DrawTexture(AppState->PigeonTexture, 20, 30, (Color){255,255,255,255});
    EndDrawing();
}

internal app_state InitAppState(Texture2D PigeonTexture)
{
    app_state AppState;

    AppState.PigeonTexture = PigeonTexture;

    return AppState;
}

int main(void)
{
#if defined(PLATFORM_WEB)
    InitRaylibCanvas();
#endif

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "SCUBA");

    Image PigeonImage = LoadImageFromMemory(".png", PigeonAssetData, sizeof(PigeonAssetData));
    Texture2D PigeonTexture = LoadTextureFromImage(PigeonImage);

    app_state AppState = InitAppState(PigeonTexture);

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
