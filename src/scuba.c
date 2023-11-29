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

typedef struct
{
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

internal void DrawSprite(game_state *GameState, entity *Entity, s32 DepthZ)
{
    /* Rectangle SourceRectangle = Sprites[Entity->SpriteType].SourceRectangle; */

    for (s32 I = 0; I < ENTITY_SPRITE_COUNT; ++I)
    {
        sprite Sprite = Entity->Sprites[I];

        if (Sprite.Type)
        {
            if (Sprite.DepthZ == DepthZ)
            {
                Rectangle SourceRectangle = Sprite.SourceRectangle;
                Rectangle DestRectangle = R2(Entity->Position.x,
                                             Entity->Position.y,
                                             TEXTURE_MAP_SCALE*SourceRectangle.width,
                                             TEXTURE_MAP_SCALE*SourceRectangle.height);
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

internal b32 CollideEntities(entity *EntityA, entity *EntityB)
{
    sprite SpriteA = EntityA->Sprites[0];
    sprite SpriteB = EntityB->Sprites[0];

    if (SpriteA.Type == sprite_type_Coral || SpriteB.Type == sprite_type_Coral)
    {
        return 0;
    }

    /* TODO: handle multiple collision areas by traversing the collision_area list */

    Rectangle A = EntityA->CollisionArea->Area;
    A.x += EntityA->Position.x;
    A.y += EntityA->Position.y;

    Rectangle B = EntityB->CollisionArea->Area;
    B.x += EntityB->Position.x;
    B.y += EntityB->Position.y;

    b32 XCollides = CollideRanges(A.x, A.width, B.x, B.width);
    b32 YCollides = CollideRanges(A.y, A.height, B.y, B.height);

    b32 Collides = XCollides && YCollides;

#if 1
    if (Collides)
    {
        DrawRectangleLinesEx(A, 2.0f, (Color){220,40,220,255});
        DrawRectangleLinesEx(B, 2.0f, (Color){220,40,220,255});
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

            b32 Collides = CollideEntities(EntityA, EntityB);
        }
    }
}

internal void UpdateAndRender(void *VoidGameState)
{
    game_state *GameState = (game_state *)VoidGameState;

    if (!IsTextureReady(GameState->ScubaTexture))
    {
        printf("texture not ready\n");
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

    BeginDrawing();
    ClearBackground(BackgroundColor);

    { /* draw coral background */
        entity *CoralEntity = GameState->CoralEntity;
        entity *WallEntity = GameState->WallEntity;

        Rectangle CoralSpriteRectangle = GetSpriteRectangle(CoralEntity);

        Vector2 CameraPosition = GameState->CameraPosition;

        f32 HalfWidth = SCREEN_WIDTH / 2.0f;
        f32 HalfHeight = SCREEN_HEIGHT / 2.0f;

        s32 TilesPerScreenX = SCREEN_WIDTH / CoralSpriteRectangle.width;
        s32 TilesPerScreenY = SCREEN_HEIGHT / CoralSpriteRectangle.height;

        s32 MinX = (CameraPosition.x - HalfWidth) / CoralSpriteRectangle.width;
        s32 MinY = (CameraPosition.y - HalfHeight) / CoralSpriteRectangle.height;

        s32 MaxX = MinX + TilesPerScreenX;
        s32 MaxY = MinY + TilesPerScreenY;

        MinX = MaxS32(0, MinX);
        MaxX = MinS32(MAP_WIDTH - 1, MaxX);
        MinY = MaxS32(0, MinY);
        MaxY = MinS32(MAP_HEIGHT - 1, MaxY);

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
                    Entity->Position.x = X * CoralSpriteRectangle.width;
                    Entity->Position.y = Y * CoralSpriteRectangle.height;


                    DrawSprite(GameState, Entity, 0);
                }
            }
        }
    }

    /* TODO: loop through entities to do update/render (have to do z-sorting) */

    { /* draw back of cage */
        DrawSprite(GameState, GameState->CageEntity, 0);
    }

    { /* draw eel */
        UpdateEntity(GameState, GameState->EelEntity);
        DrawSprite(GameState, GameState->EelEntity, 1);
    }

    { /* draw crab */
        UpdateEntity(GameState, GameState->CrabEntity);
        DrawSprite(GameState, GameState->CrabEntity, 1);
    }

    { /* draw player */
        UpdateEntity(GameState, GameState->PlayerEntity);
        DrawSprite(GameState, GameState->PlayerEntity, 1);
    }

    { /* draw front of cage */
        DrawSprite(GameState, GameState->CageEntity, 1);
    }

    { /* debug graphics */
        DebugDrawCollisions(GameState);

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

    GameState.CameraPosition = V2(500.0f, 500.0f);

    { /* init entities */
        GameState.PlayerEntity = AddEntity(&GameState);
        GameState.PlayerEntity->Sprites[0].Type = sprite_type_Fish;
        GameState.PlayerEntity->Sprites[0].SourceRectangle = R2(5,3,12,9);
        GameState.PlayerEntity->Sprites[0].DepthZ = 1;
        GameState.PlayerEntity->CollisionArea = AddCollisionArea(&GameState);
        GameState.PlayerEntity->CollisionArea->Area = R2(1 * TEXTURE_MAP_SCALE,
                                                         1 * TEXTURE_MAP_SCALE,
                                                         10 * TEXTURE_MAP_SCALE,
                                                         6 * TEXTURE_MAP_SCALE);

        GameState.EelEntity = AddEntity(&GameState);
        GameState.EelEntity->Sprites[0].Type = sprite_type_Eel;
        GameState.EelEntity->Sprites[0].SourceRectangle = R2(1,27,34,20);
        GameState.EelEntity->Sprites[0].DepthZ = 1;
        GameState.EelEntity->Position = V2(0.0f, 200.0f);
        GameState.EelEntity->CollisionArea = AddCollisionArea(&GameState);
        GameState.EelEntity->CollisionArea->Area = R2(12 * TEXTURE_MAP_SCALE,
                                                      12 * TEXTURE_MAP_SCALE,
                                                      22 * TEXTURE_MAP_SCALE,
                                                      8 * TEXTURE_MAP_SCALE);

        GameState.CrabEntity = AddEntity(&GameState);
        GameState.CrabEntity->Sprites[0].Type = sprite_type_Crab;
        GameState.CrabEntity->Sprites[0].SourceRectangle = R2(14,69,39,20);
        GameState.CrabEntity->Sprites[0].DepthZ = 1;
        GameState.CrabEntity->Position = V2(340.0f, 280.0f);
        GameState.CrabEntity->CollisionArea = AddCollisionArea(&GameState);
        GameState.CrabEntity->CollisionArea->Area = R2(13 * TEXTURE_MAP_SCALE,
                                                       3 * TEXTURE_MAP_SCALE,
                                                       16 * TEXTURE_MAP_SCALE,
                                                       9 * TEXTURE_MAP_SCALE);

        GameState.CoralEntity = AddEntity(&GameState);
        GameState.CoralEntity->Sprites[0].Type = sprite_type_Coral;
        GameState.CoralEntity->Sprites[0].SourceRectangle = R2(464,7,24,24);
        GameState.CoralEntity->Sprites[0].DepthZ = 0;

        GameState.WallEntity = AddEntity(&GameState);
        GameState.WallEntity->Sprites[0].Type = sprite_type_Wall;
        GameState.WallEntity->Sprites[0].SourceRectangle = R2(464,36,24,24);
        GameState.WallEntity->Sprites[0].DepthZ = 0;
        GameState.WallEntity->CollisionArea = AddCollisionArea(&GameState);
        GameState.WallEntity->CollisionArea->Area = R2(0 * TEXTURE_MAP_SCALE,
                                                       0 * TEXTURE_MAP_SCALE,
                                                       24 * TEXTURE_MAP_SCALE,
                                                       24 * TEXTURE_MAP_SCALE);

        GameState.CageEntity = AddEntity(&GameState);
        GameState.CageEntity->Sprites[0].Type = sprite_type_Cage;
        GameState.CageEntity->Sprites[0].SourceRectangle = R2(90,6,110,70);
        GameState.CageEntity->Sprites[0].DepthZ = 0;
        GameState.CageEntity->Sprites[1].Type = sprite_type_Cage;
        GameState.CageEntity->Sprites[1].SourceRectangle = R2(90,101,110,70);
        GameState.CageEntity->Sprites[1].DepthZ = 1;
        GameState.CageEntity->Position = V2(250.0f, 130.0f);
        GameState.CageEntity->CollisionArea = AddCollisionArea(&GameState);
        GameState.CageEntity->CollisionArea->Area = R2(1 * TEXTURE_MAP_SCALE,
                                                       1 * TEXTURE_MAP_SCALE,
                                                       110 * TEXTURE_MAP_SCALE,
                                                       70 * TEXTURE_MAP_SCALE);
    }

    GameState.LastTime = GetTime();

    return GameState;
}

int main(void)
{
    printf("entity size %lu\n", sizeof(entity));
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
