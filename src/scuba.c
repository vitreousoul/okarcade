#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "../lib/raylib.h"

#include "types.h"

int SCREEN_WIDTH = 1200;
int SCREEN_HEIGHT = 700;

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#include "raylib_defines.c"
#endif

#include "../gen/scuba.h"
#include "core.c"
#include "math.c"
#include "raylib_helpers.h"
#include "ui.c"

#define TEXTURE_MAP_SCALE 7
#define MAX_ENTITY_COUNT 256
#define MAX_COLLISION_AREA_COUNT 256
#define MAX_DELTA_TIME (1.0f/50.0f)

global_variable Color BackgroundColor = (Color){20, 116, 92, 255};

#define TILE_SIZE 24
#define MAP_WIDTH 16
#define MAP_HEIGHT 16
#define o '\0' /* open space */
#define W 'w' /* wall */
global_variable u8 Map[MAP_HEIGHT][MAP_WIDTH] = {
    {W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W},
    {W,o,W,o,o,o,o,o,o,o,o,o,o,o,o,W},
    {W,o,W,o,o,o,o,o,o,o,o,o,o,o,o,W},
    {W,o,W,W,o,o,o,o,o,o,o,o,o,o,o,W},
    {W,o,o,o,o,o,o,o,o,o,o,o,o,o,o,W},
    {W,W,W,W,o,o,o,o,o,o,o,o,o,o,o,W},
    {W,o,o,o,o,o,o,o,o,o,o,o,o,o,o,W},
    {W,o,o,o,o,o,o,o,o,o,o,o,o,o,o,W},
    {W,o,o,o,o,o,o,o,o,o,o,o,o,o,o,W},
    {W,o,o,o,o,o,o,o,o,o,o,o,o,o,o,W},
    {W,o,o,o,o,o,o,o,o,o,o,o,o,o,o,W},
    {W,o,o,o,o,o,o,o,o,o,o,o,o,o,o,W},
    {W,o,o,o,o,o,o,o,o,o,o,o,o,o,o,W},
    {W,o,o,o,o,o,o,o,o,o,o,o,o,o,o,W},
    {W,o,o,o,o,o,o,o,o,o,o,o,o,o,o,W},
    {W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W},
};
#undef o
#undef W

typedef enum
{
    sprite_type_NONE,
    sprite_type_Fish,
    sprite_type_Eel,
    sprite_type_Coral,
    sprite_type_Wall,
    sprite_type_Cage,
    sprite_type_Crab,
    sprite_type_Count,
} sprite_type;

typedef struct
{
    sprite_type Type;
    Rectangle SourceRectangle;
    s32 DepthZ;
} sprite;

struct collision_area
{
    Rectangle Area;
    struct collision_area *Next;
};
typedef struct collision_area collision_area;

#define ENTITY_SPRITE_COUNT 3

typedef enum
{
    entity_type_Undefined,
    entity_type_Base,
} entity_type;

typedef struct
{
    entity_type Type;

    Vector2 Position;
    Vector2 Velocity;
    Vector2 Acceleration;

    sprite Sprites[ENTITY_SPRITE_COUNT];

    collision_area *CollisionArea;
} entity;

typedef struct
{
    f32 DeltaTime;
    f32 LastTime;

    s32 EntityCount;
    entity Entities[MAX_ENTITY_COUNT];

    s32 CollisionAreaCount;
    collision_area CollisionAreas[MAX_COLLISION_AREA_COUNT];

    Vector2 CameraPosition;

    /* TODO: don't store these entity pointers. Instead, loop through entities to update/render them */
    entity *PlayerEntity;
    entity *EelEntity;
    entity *CoralEntity;
    entity *WallEntity;
    entity *CageEntity;
    entity *CrabEntity;

    Texture2D ScubaTexture;
} game_state;

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

internal Vector2 WorldToScreenPosition(game_state *GameState, Vector2 P)
{
    Vector2 HalfScreen = MultiplyV2S(V2(SCREEN_WIDTH, SCREEN_HEIGHT), 0.5f);
    Vector2 Result = AddV2(SubtractV2(P, GameState->CameraPosition), HalfScreen);

    return Result;
}

internal Rectangle WorldToScreenRectangle(game_state *GameState, Rectangle R)
{
    Vector2 HalfScreen = MultiplyV2S(V2(SCREEN_WIDTH, SCREEN_HEIGHT), 0.5f);
    Vector2 RectPosition = V2(R.x, R.y);
    Vector2 Position = AddV2(SubtractV2(RectPosition, GameState->CameraPosition), HalfScreen);
    Rectangle Result = R2(Position.x, Position.y, R.width, R.height);

    return Result;
}

internal Vector2 ScreenToWorldPosition(game_state *GameState, Vector2 P)
{
    Vector2 HalfScreen = MultiplyV2S(V2(SCREEN_WIDTH, SCREEN_HEIGHT), 0.5f);
    Vector2 Result = SubtractV2(AddV2(P, GameState->CameraPosition), HalfScreen);

    return Result;
}

internal void DrawSprite(game_state *GameState, entity *Entity, s32 DepthZ)
{
    for (s32 I = 0; I < ENTITY_SPRITE_COUNT; ++I)
    {
        sprite Sprite = Entity->Sprites[I];
        Rectangle SourceRectangle = Sprite.SourceRectangle;
        Vector2 SpriteSize = (Vector2){TEXTURE_MAP_SCALE*SourceRectangle.width,
                                       TEXTURE_MAP_SCALE*SourceRectangle.height};

        Vector2 EntityScreenPosition = WorldToScreenPosition(GameState, Entity->Position);
        Vector2 SpriteOffset = MultiplyV2S(SpriteSize, -0.5f);
        Vector2 Position = AddV2(EntityScreenPosition, SpriteOffset);

        if (Sprite.Type)
        {
            if (Sprite.DepthZ == DepthZ)
            {
                Rectangle DestRectangle = R2(Position.x, Position.y, SpriteSize.x, SpriteSize.y);
                Color Tint = (Color){255,255,255,255};
                Vector2 Origin = (Vector2){0,0};

                DrawTexturePro(GameState->ScubaTexture, SourceRectangle, DestRectangle, Origin, 0.0f, Tint);
            }
        }
        else
        {
            break;
        }
    }
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
    Rectangle SourceRectangle = Entity->Sprites[0].SourceRectangle;

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

internal b32 CollideEntities(game_state *GameState, entity *EntityA, entity *EntityB)
{
    sprite SpriteA = EntityA->Sprites[0];
    sprite SpriteB = EntityB->Sprites[0];

    if (SpriteA.Type == sprite_type_Coral || SpriteB.Type == sprite_type_Coral)
    {
        return 0;
    }

    /* TODO: handle multiple collision areas by traversing the collision_area list */

    Rectangle A = EntityA->CollisionArea->Area;
    A.x += EntityA->Position.x - (A.width / 2.0f);
    A.y += EntityA->Position.y - (A.height / 2.0f);

    Rectangle B = EntityB->CollisionArea->Area;
    B.x += EntityB->Position.x - (B.width / 2.0f);
    B.y += EntityB->Position.y - (B.height / 2.0f);

    b32 XCollides = CollideRanges(A.x, A.width, B.x, B.width);
    b32 YCollides = CollideRanges(A.y, A.height, B.y, B.height);

    b32 Collides = XCollides && YCollides;

#if 1
    if (Collides)
    {
        Rectangle ScreenRectangleA = WorldToScreenRectangle(GameState, A);
        Rectangle ScreenRectangleB = WorldToScreenRectangle(GameState, B);

        DrawRectangleLinesEx(ScreenRectangleA, 2.0f, (Color){220,40,220,255});
        DrawRectangleLinesEx(ScreenRectangleB, 2.0f, (Color){220,40,220,255});
    }
#endif

    return Collides;
}

internal void DebugDrawCollisions(game_state *GameState)
{
    /* O(n^2) loop to check if any entity collides with another. */
    for (s32 I = 0; I < MAX_ENTITY_COUNT; ++I)
    {
        entity *EntityA = GameState->Entities + I;
        sprite SpriteA = EntityA->Sprites[0];

        if (!SpriteA.Type)
        {
            break;
        }

        for (s32 J = I + 1; J < MAX_ENTITY_COUNT; ++J)
        {
            entity *EntityB = GameState->Entities + J;
            sprite SpriteB = EntityB->Sprites[0];

            if (!SpriteB.Type)
            {
                break;
            }

            b32 Collides = CollideEntities(GameState, EntityA, EntityB);
        }
    }
}

internal void UpdateAndRender(void *VoidGameState)
{
    game_state *GameState = (game_state *)VoidGameState;

    if (!IsTextureReady(GameState->ScubaTexture))
    {
        EndDrawing();
        return;
    }

    { /* update timer */
        f32 Time = GetTime();
        f32 DeltaTime = Time - GameState->LastTime;
        GameState->DeltaTime = MinF32(MAX_DELTA_TIME, DeltaTime);
        GameState->LastTime = Time;
    }

    HandleUserInput(GameState);

    f32 StartTime = GetTime();

    {
        UpdateEntity(GameState, GameState->EelEntity);
        UpdateEntity(GameState, GameState->CrabEntity);
        UpdateEntity(GameState, GameState->PlayerEntity);
        GameState->CameraPosition = GameState->PlayerEntity->Position;
    }

    BeginDrawing();
    ClearBackground(BackgroundColor);

    { /* draw coral background */
        entity *CoralEntity = GameState->CoralEntity;
        entity *WallEntity = GameState->WallEntity;

        Rectangle CoralRect = GetSpriteRectangle(CoralEntity);
        Vector2 SpriteScale = MultiplyV2S(V2(CoralRect.width, CoralRect.height), 1);

        Vector2 MinCorner = ScreenToWorldPosition(GameState, V2(0.0f, 0.0f));
        Vector2 MaxCorner = ScreenToWorldPosition(GameState, V2(SCREEN_WIDTH, SCREEN_HEIGHT));

        MinCorner = DivideV2(MinCorner, SpriteScale);
        MaxCorner = DivideV2(MaxCorner, SpriteScale);

        s32 Padding = 2;

        s32 MinX = MaxS32(0, MinCorner.x - Padding);
        s32 MaxX = MinS32(MAP_WIDTH - Padding, MaxCorner.x + Padding);
        s32 MinY = MaxS32(0, MinCorner.y - Padding);
        s32 MaxY = MinS32(MAP_HEIGHT - Padding, MaxCorner.y + Padding);

        s32 SpriteCount = 0;

        for (s32 Y = MinY; Y < MaxY; ++Y)
        {
            for (s32 X = MinX; X < MaxX; ++X)
            {
                entity *Entity = 0;

                switch (Map[Y][X])
                {
                case '\0': Entity = CoralEntity; break;
                case 'w': Entity = WallEntity; break;
                }

                if (Entity)
                {
                    Entity->Position.x = X * CoralRect.width;
                    Entity->Position.y = Y * CoralRect.height;

                    ++SpriteCount;
                    DrawSprite(GameState, Entity, 0);
                }
            }
        }
    }

    /* TODO: loop through entities to do update/render (have to do z-sorting) */
    { /* entities */
        DrawSprite(GameState, GameState->CageEntity, 0);
        DrawSprite(GameState, GameState->EelEntity, 1);
        DrawSprite(GameState, GameState->CrabEntity, 1);
        DrawSprite(GameState, GameState->PlayerEntity, 1);
        DrawSprite(GameState, GameState->CageEntity, 1);
    }

    { /* debug graphics */
        DebugDrawCollisions(GameState);


        { /* draw entity dots */
            for (s32 I = 0; I < MAX_ENTITY_COUNT; ++I)
            {
                entity *Entity = GameState->Entities + I;

                if (!Entity->Type)
                {
                    break;
                }

                Vector2 Position = WorldToScreenPosition(GameState, Entity->Position);
                DrawRectangle(Position.x, Position.y, 4.0f, 4.0f, (Color){220,40,220,255});
            }
        }

        { /* draw time */
            char DebugTextBuffer[128] = {};
            f32 DeltaTime = GetTime() - StartTime;
            sprintf(DebugTextBuffer, "dt %.4f", 1000 * DeltaTime);
            DrawText(DebugTextBuffer, 11, 11, 12, (Color){0,0,0,255});
            DrawText(DebugTextBuffer, 10, 10, 12, (Color){255,255,255,255});
        }
    }

    EndDrawing();
}

internal collision_area *AddCollisionArea(game_state *GameState)
{
    collision_area *CollisionArea = 0;

    if (GameState->CollisionAreaCount < MAX_COLLISION_AREA_COUNT)
    {
        CollisionArea = GameState->CollisionAreas + GameState->CollisionAreaCount;
        CollisionArea->Area = (Rectangle){0};
        CollisionArea->Next = 0;
        GameState->CollisionAreaCount += 1;
    }

    return CollisionArea;
}

internal game_state InitGameState(Texture2D ScubaTexture)
{
    game_state GameState = {0};

    GameState.ScubaTexture = ScubaTexture;

    { /* init entities */
        GameState.PlayerEntity = AddEntity(&GameState);
        entity *PlayerEntity = GameState.PlayerEntity;
        PlayerEntity->Type = entity_type_Base;
        PlayerEntity->Sprites[0].Type = sprite_type_Fish;
        PlayerEntity->Sprites[0].SourceRectangle = R2(5,3,12,9);
        PlayerEntity->Sprites[0].DepthZ = 1;
        PlayerEntity->Position = V2(0.0f, 0.0f);
        PlayerEntity->Position = MultiplyV2S(V2(1.0f, 1.0f), TILE_SIZE * TEXTURE_MAP_SCALE);
        PlayerEntity->CollisionArea = AddCollisionArea(&GameState);
        PlayerEntity->CollisionArea->Area = R2(1 * TEXTURE_MAP_SCALE,
                                               1 * TEXTURE_MAP_SCALE,
                                               10 * TEXTURE_MAP_SCALE,
                                               6 * TEXTURE_MAP_SCALE);

        GameState.EelEntity = AddEntity(&GameState);
        entity *EelEntity = GameState.EelEntity;
        EelEntity->Type = entity_type_Base;
        EelEntity->Sprites[0].Type = sprite_type_Eel;
        EelEntity->Sprites[0].SourceRectangle = R2(1,27,34,20);
        EelEntity->Sprites[0].DepthZ = 1;
        EelEntity->Position = MultiplyV2S(V2(0.25f, 4.0f), TILE_SIZE * TEXTURE_MAP_SCALE);
        EelEntity->CollisionArea = AddCollisionArea(&GameState);
        EelEntity->CollisionArea->Area = R2(6 * TEXTURE_MAP_SCALE,
                                            6 * TEXTURE_MAP_SCALE,
                                            22 * TEXTURE_MAP_SCALE,
                                            8 * TEXTURE_MAP_SCALE);

        GameState.CrabEntity = AddEntity(&GameState);
        entity *CrabEntity = GameState.CrabEntity;
        CrabEntity->Type = entity_type_Base;
        CrabEntity->Sprites[0].Type = sprite_type_Crab;
        CrabEntity->Sprites[0].SourceRectangle = R2(14,69,39,20);
        CrabEntity->Sprites[0].DepthZ = 1;
        CrabEntity->Position = MultiplyV2S(V2(9.0f, 7.0f), TILE_SIZE * TEXTURE_MAP_SCALE);
        CrabEntity->CollisionArea = AddCollisionArea(&GameState);
        CrabEntity->CollisionArea->Area = R2(13 * TEXTURE_MAP_SCALE,
                                             3 * TEXTURE_MAP_SCALE,
                                             16 * TEXTURE_MAP_SCALE,
                                             9 * TEXTURE_MAP_SCALE);

        GameState.CoralEntity = AddEntity(&GameState);
        entity *CoralEntity = GameState.CoralEntity;
        CoralEntity->Type = entity_type_Base;
        CoralEntity->Sprites[0].Type = sprite_type_Coral;
        CoralEntity->Sprites[0].SourceRectangle = R2(13,118,TILE_SIZE,TILE_SIZE);
        CoralEntity->Sprites[0].DepthZ = 0;

        GameState.WallEntity = AddEntity(&GameState);
        entity *WallEntity = GameState.WallEntity;
        WallEntity->Type = entity_type_Base;
        WallEntity->Sprites[0].Type = sprite_type_Wall;
        WallEntity->Sprites[0].SourceRectangle = R2(13,147,TILE_SIZE,TILE_SIZE);
        WallEntity->Sprites[0].DepthZ = 0;
        WallEntity->CollisionArea = AddCollisionArea(&GameState);
        WallEntity->CollisionArea->Area = R2(0 * TEXTURE_MAP_SCALE,
                                             0 * TEXTURE_MAP_SCALE,
                                             TILE_SIZE * TEXTURE_MAP_SCALE,
                                             TILE_SIZE * TEXTURE_MAP_SCALE);

        GameState.CageEntity = AddEntity(&GameState);
        entity *CageEntity = GameState.CageEntity;
        CageEntity->Type = entity_type_Base;
        CageEntity->Sprites[0].Type = sprite_type_Cage;
        CageEntity->Sprites[0].SourceRectangle = R2(90,6,110,70);
        CageEntity->Sprites[0].DepthZ = 0;
        CageEntity->Sprites[1].Type = sprite_type_Cage;
        CageEntity->Sprites[1].SourceRectangle = R2(90,101,110,70);
        CageEntity->Sprites[1].DepthZ = 1;
        CageEntity->Position = MultiplyV2S(V2(9.0f, 7.0f), TILE_SIZE * TEXTURE_MAP_SCALE);
        CageEntity->CollisionArea = AddCollisionArea(&GameState);
        CageEntity->CollisionArea->Area = R2(1 * TEXTURE_MAP_SCALE,
                                             1 * TEXTURE_MAP_SCALE,
                                             110 * TEXTURE_MAP_SCALE,
                                             70 * TEXTURE_MAP_SCALE);
    }

    GameState.CameraPosition = GameState.PlayerEntity->Position;

    GameState.LastTime = GetTime();

    return GameState;
}

internal b32 LoadScubaTexture(Texture2D *Texture)
{
    b32 Error = 0;
    s32 AssetSize = ArrayCount(ScubaAssetData);

    if (AssetSize > 2)
    {
        u32 AssetWidth = ScubaAssetData[0];
        u32 AssetHeight = ScubaAssetData[1];
        u64 PixelCount = AssetWidth * AssetHeight;

        Image ScubaImage = GenImageColor(AssetWidth, AssetHeight, (Color){255,0,255,255});

        for (u64 I = 2; I < PixelCount + 2; ++I)
        {
            u32 X = (I - 2) % AssetWidth;
            u32 Y = (I - 2) / AssetWidth;
            if (X < AssetWidth && X >= 0 && Y < AssetHeight && Y >= 0)
            {
                /* NOTE: We could pack our colors and just cast the u32,
                 * but for now we break the color components out and re-write them. */
                u8 R = (ScubaAssetData[I] >> 24) & 0xff;
                u8 G = (ScubaAssetData[I] >> 16) & 0xff;
                u8 B = (ScubaAssetData[I] >>  8) & 0xff;
                u8 A = (ScubaAssetData[I]      ) & 0xff;
                Color PixelColor = (Color){R, G, B, A};

                ImageDrawPixel(&ScubaImage, X, Y, PixelColor);
            }
            else
            {
                LogError("out of bounds scuba asset access");
                break;
            }
        }

        *Texture = LoadTextureFromImage(ScubaImage);
    }
    else
    {
        LogError("Scuba asset is too small to be useful, there was likely an error generating game assets.");
    }

    return Error;
}

int main(void)
{
    printf("entity size %lu\n", sizeof(entity));
#if defined(PLATFORM_WEB)
    InitRaylibCanvas();
#endif

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "SCUBA");

    Texture2D ScubaTexture = {0};
    b32 TextureError = LoadScubaTexture(&ScubaTexture);

    if (TextureError)
    {
        LogError("loading scuba asset texture.");
    }
    else
    {
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
    }

    CloseWindow();
}
