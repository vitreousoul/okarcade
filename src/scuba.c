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
#include "math.c"
#include "raylib_helpers.h"
#include "ui.c"

#define TEXTURE_MAP_SCALE 7
#define MAX_ENTITY_COUNT 256
#define MAX_DELTA_TIME (1.0f/50.0f)

global_variable Color BackgroundColor = (Color){20, 116, 92, 255};


typedef enum
{
    sprite_type_NONE,
    sprite_type_Fish,
    sprite_type_Eel,
    sprite_type_Coral,
    sprite_type_Count,
} sprite_type;

typedef struct
{
    Vector2 Position;
    Vector2 Velocity;
    Vector2 Acceleration;
    sprite_type SpriteType;
} entity;

typedef struct
{
    f32 DeltaTime;
    f32 LastTime;

    s32 EntityCount;
    entity Entities[MAX_ENTITY_COUNT];

    entity *PlayerEntity;
    entity *EelEntity;
    entity *CoralEntity;

    Texture2D ScubaTexture;
} game_state;

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
    GameState->PlayerEntity->Acceleration.x = 0;
    GameState->PlayerEntity->Acceleration.y = 0;

    if (IsKeyDown(KEY_D))
    {
        GameState->PlayerEntity->Acceleration.x += 1.0f;
    }

    if (IsKeyDown(KEY_S))
    {
        GameState->PlayerEntity->Acceleration.y += 1.0f;
    }

    if (IsKeyDown(KEY_A))
    {
        GameState->PlayerEntity->Acceleration.x -= 1.0f;
    }

    if (IsKeyDown(KEY_W))
    {
        GameState->PlayerEntity->Acceleration.y -= 1.0f;
    }
}

internal void DrawSprite(game_state *GameState, entity *Entity)
{
    Rectangle SourceRectangle = Sprites[Entity->SpriteType].SourceRectangle;
    Rectangle DestRectangle = R2(Entity->Position.x,
                                 Entity->Position.y,
                                 TEXTURE_MAP_SCALE*SourceRectangle.width,
                                 TEXTURE_MAP_SCALE*SourceRectangle.height);
    Color Tint = (Color){255,255,255,255};
    Vector2 Origin = (Vector2){0,0};

    DrawTexturePro(GameState->ScubaTexture, SourceRectangle, DestRectangle, Origin, 0.0f, Tint);
}

internal void UpdateEntity(game_state *GameState, entity *Entity)
{
    f32 AccelerationScale = 800.0f;
    f32 DT = GameState->DeltaTime;

    Vector2 P = Entity->Position;
    Vector2 V = Entity->Velocity;
    Vector2 A = MultiplyV2S(Entity->Acceleration, AccelerationScale);
    A = AddV2(A, MultiplyV2S(V, -2.0f)); /* friction */

    Vector2 NewVelocity = AddV2(MultiplyV2S(A, DT), V);
    Vector2 NewPosition = AddV2(AddV2(MultiplyV2S(A, 0.5f * DT * DT),
                                      MultiplyV2S(V, DT)),
                                P);

    Entity->Velocity = NewVelocity;
    Entity->Position = NewPosition;
}

internal Rectangle GetSpriteRectangle(entity *Entity)
{
    Rectangle SourceRectangle = Sprites[Entity->SpriteType].SourceRectangle;
    Rectangle SpriteRectangle = (Rectangle){
        Entity->Position.x,
        Entity->Position.y,
        TEXTURE_MAP_SCALE * SourceRectangle.width,
        TEXTURE_MAP_SCALE * SourceRectangle.height
    };

    return SpriteRectangle;
}

internal b32 CollideRanges(f32 ValueA, f32 LengthA, f32 ValueB, f32 LengthB)
{
    f32 MaxA = ValueA + LengthA;
    f32 MaxB = ValueB + LengthB;

    b32 ALessThanB = MaxA < ValueB;
    b32 BLessThanA = MaxB < ValueA;

    b32 Collides = !(ALessThanB || BLessThanA);

    return Collides;
}

internal b32 CollideRectangles(entity *EntityA, entity *EntityB)
{
    if (EntityA->SpriteType == sprite_type_Coral || EntityB->SpriteType == sprite_type_Coral)
    {
        return 0;
    }

    Rectangle A = GetSpriteRectangle(EntityA);
    Rectangle B = GetSpriteRectangle(EntityB);

    b32 XCollides = CollideRanges(A.x, A.width, B.x, B.width);
    b32 YCollides = CollideRanges(A.y, A.height, B.y, B.height);
    b32 Collides = XCollides && YCollides;

    return Collides;
}

internal void DebugDrawCollisions(game_state *GameState)
{
    /* O(n^2) loop to check if any entity collides with another. */
    for (s32 I = 0; I < MAX_ENTITY_COUNT; ++I)
    {
        entity *EntityA = GameState->Entities + I;

        if (!EntityA->SpriteType)
        {
            break;
        }

        for (s32 J = I + 1; J < MAX_ENTITY_COUNT; ++J)
        {
            entity *EntityB = GameState->Entities + J;

            if (!EntityB->SpriteType)
            {
                break;
            }

            b32 Collides = CollideRectangles(EntityA, EntityB);

            if (Collides)
            {
                Rectangle SpriteA = GetSpriteRectangle(EntityA);
                Rectangle SpriteB = GetSpriteRectangle(EntityB);

                DrawRectangleLinesEx(SpriteA, 2.0f, (Color){220,40,220,255});
                DrawRectangleLinesEx(SpriteB, 2.0f, (Color){220,40,220,255});
            }
        }
    }
}

internal void UpdateAndRender(void *VoidGameState)
{
    game_state *GameState = (game_state *)VoidGameState;

    { /* update timer */
        f32 Time = GetTime();
        f32 DeltaTime = Time - GameState->LastTime;
        GameState->DeltaTime = MinF32(MAX_DELTA_TIME, DeltaTime);
        GameState->LastTime = Time;
    }

    HandleUserInput(GameState);

    f32 StartTime = GetTime();

    BeginDrawing();
    ClearBackground(BackgroundColor);

    { /* draw coral background */
        entity *CoralEntity = GameState->CoralEntity;
        Rectangle CoralSpriteRectangle = GetSpriteRectangle(CoralEntity);

        s32 WallColumnCount = 2 + (SCREEN_WIDTH / CoralSpriteRectangle.width);
        s32 WallRowCount = 2 + (SCREEN_HEIGHT / CoralSpriteRectangle.height);

        for (s32 Y = 0; Y < WallRowCount; ++Y)
        {
            for (s32 X = 0; X < WallColumnCount; ++X)
            {
                CoralEntity->Position.x = (X - 1) * CoralSpriteRectangle.width;
                CoralEntity->Position.y = (Y - 1) * CoralSpriteRectangle.height;

                DrawSprite(GameState, CoralEntity);
            }
        }
    }

    { /* draw eel */
        UpdateEntity(GameState, GameState->EelEntity);
        DrawSprite(GameState, GameState->EelEntity);
    }

    { /* draw player */
        UpdateEntity(GameState, GameState->PlayerEntity);
        DrawSprite(GameState, GameState->PlayerEntity);
    }

    DebugDrawCollisions(GameState);

    { /* debug draw time */
        char DebugTextBuffer[128] = {};
        f32 DeltaTime = GetTime() - StartTime;
        sprintf(DebugTextBuffer, "dt %.4f", 1000 * DeltaTime);
        DrawText(DebugTextBuffer, 11, 11, 12, (Color){0,0,0,255});
        DrawText(DebugTextBuffer, 10, 10, 12, (Color){255,255,255,255});
    }

    EndDrawing();
}

internal game_state InitGameState(Texture2D ScubaTexture)
{
    game_state GameState = {0};

    GameState.ScubaTexture = ScubaTexture;

    Sprites[sprite_type_Fish].SourceRectangle = R2(5,3,12,9);
    Sprites[sprite_type_Eel].SourceRectangle = R2(1,27,34,20);
    Sprites[sprite_type_Coral].SourceRectangle = R2(464,7,24,24);

    { /* init entities */
        GameState.PlayerEntity = AddEntity(&GameState);
        GameState.PlayerEntity->SpriteType = sprite_type_Fish;

        GameState.EelEntity = AddEntity(&GameState);
        GameState.EelEntity->SpriteType = sprite_type_Eel;
        GameState.EelEntity->Position.x = 0.0f;
        GameState.EelEntity->Position.y = 200.0f;

        GameState.CoralEntity = AddEntity(&GameState);
        GameState.CoralEntity->SpriteType = sprite_type_Coral;
        GameState.CoralEntity->Position.x = 100.0f;
        GameState.CoralEntity->Position.y = 200.0f;
    }

    GameState.LastTime = GetTime();

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
