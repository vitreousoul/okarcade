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
    /* TODO: replace pigeon with scuba art, and then change "pigeon"->"scuba" or something like that. */
    Texture2D PigeonTexture;

    Vector2 PigeonPosition;
    Vector2 PigeonAcceleration;
} game_state;

internal void HandleUserInput(game_state *GameState)
{
}

internal void UpdateAndRender(void *VoidGameState)
{
    game_state *GameState = (game_state *)VoidGameState;

    HandleUserInput(GameState);

    BeginDrawing();
    ClearBackground(BackgroundColor);
    DrawTexture(GameState->PigeonTexture, 20, 30, (Color){255,255,255,255});
    EndDrawing();
}

internal game_state InitGameState(Texture2D PigeonTexture)
{
    game_state GameState;

    GameState.PigeonTexture = PigeonTexture;

    GameState.PigeonAcceleration.x = 0.0f;
    GameState.PigeonAcceleration.y = 0.0f;

    GameState.PigeonPosition.x = 0.0f;
    GameState.PigeonPosition.y = 0.0f;

    return GameState;
}

int main(void)
{
#if defined(PLATFORM_WEB)
    InitRaylibCanvas();
#endif

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "SCUBA");

    Image PigeonImage = LoadImageFromMemory(".png", PigeonAssetData, sizeof(PigeonAssetData));
    Texture2D PigeonTexture = LoadTextureFromImage(PigeonImage);

    game_state GameState = InitGameState(PigeonTexture);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop_arg(UpdateAndRender, &GameState, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        UpdateAndRender(&GameState);
    }
#endif

    CloseWindow();
}
