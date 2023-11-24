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

#define TEXTURE_MAP_SCALE 8

global_variable Color BackgroundColor = (Color){176, 176, 168, 255};

#define MAX_ENTITY_COUNT 256

typedef s32 error_code;

typedef struct
{
    Vector2 Position;
    Vector2 Velocity;
    Vector2 Acceleration;
} entity;

typedef struct
{
    s32 EntityCount;
    entity Entities[MAX_ENTITY_COUNT];

    entity *PlayerEntity;

    Texture2D ScubaTexture;
} game_state;

typedef enum
{
    sprite_type_NONE,
    sprite_type_Fish,
    sprite_type_Count,
} sprite_type;

typedef struct
{
    Rectangle SourceRectangle;
} sprite;

static sprite Sprites[sprite_type_Count] = {0};

internal entity *AddEntity(game_state *GameState)
{
    entity *Entity = 0;

    if (GameState->EntityCount < MAX_ENTITY_COUNT)
    {
        Entity = GameState->Entities + GameState->EntityCount;
        GameState->EntityCount += 1;
    }
    else
    {
        LogError("adding entity");
    }

    return Entity;
}

internal void HandleUserInput(game_state *GameState)
{
}

internal void DrawSprite(game_state *GameState, sprite_type Type, Vector2 Position)
{
    Rectangle SourceRectangle = Sprites[Type].SourceRectangle;
    Rectangle DestRectangle = R2(Position.x,
                                 Position.y,
                                 TEXTURE_MAP_SCALE*SourceRectangle.width,
                                 TEXTURE_MAP_SCALE*SourceRectangle.height);
    Color Tint = (Color){255,255,255,255};
    Vector2 Origin = (Vector2){0,0};

    DrawTexturePro(GameState->ScubaTexture, SourceRectangle, DestRectangle, Origin, 0.0f, Tint);
}

internal void UpdateAndRender(void *VoidGameState)
{
    game_state *GameState = (game_state *)VoidGameState;

    HandleUserInput(GameState);

    BeginDrawing();
    ClearBackground(BackgroundColor);

    { /* draw player */
        DrawSprite(GameState, sprite_type_Fish, GameState->PlayerEntity->Position);
    }

    EndDrawing();
}

internal game_state InitGameState(Texture2D ScubaTexture)
{
    game_state GameState = {0};

    GameState.ScubaTexture = ScubaTexture;

    Sprites[sprite_type_Fish].SourceRectangle = R2(5,3,12,9);

    GameState.PlayerEntity = AddEntity(&GameState);

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
