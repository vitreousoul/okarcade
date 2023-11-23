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

#include "../gen/scuba.h"
#include "core.c"
#include "raylib_helpers.h"
#include "math.c"
#include "ui.c"

global_variable Color BackgroundColor = (Color){176, 176, 168, 255};

typedef struct
{
    Texture2D ScubaTexture;

    Vector2 PlayerPosition;
    Vector2 PlayerAcceleration;
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

    { /* draw player */
        Vector2 SourceSize = (Vector2){12,9};
        Rectangle SourceRectangle = (Rectangle){5,3,SourceSize.x,SourceSize.y};
        Vector2 Position = (Vector2){20,30};
        f32 Scale = 8.0f;
        Rectangle DestRectangle = (Rectangle){Position.x,Position.y,Scale*SourceSize.x,Scale*SourceSize.y};
        Color Tint = (Color){255,255,255,255};
        Vector2 Origin = (Vector2){0,0};

        DrawTexturePro(GameState->ScubaTexture, SourceRectangle, DestRectangle, Origin, 0.0f, Tint);
    }

    EndDrawing();
}

internal game_state InitGameState(Texture2D ScubaTexture)
{
    game_state GameState;

    GameState.ScubaTexture = ScubaTexture;

    GameState.PlayerAcceleration.x = 0.0f;
    GameState.PlayerAcceleration.y = 0.0f;
    GameState.PlayerPosition.x = 0.0f;
    GameState.PlayerPosition.y = 0.0f;

    return GameState;
}

int main(void)
{
#if defined(PLATFORM_WEB)
    InitRaylibCanvas();
#endif

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "SCUBA");

    Image ScubaImage = LoadImageFromMemory(".png", ScubaAssetData, sizeof(ScubaAssetData));
    Texture2D ScubaTexture = LoadTextureFromImage(ScubaImage);

    game_state GameState = InitGameState(ScubaTexture);

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
