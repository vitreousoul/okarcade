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

#define TEXTURE_MAP_SCALE 5
#define MAX_ENTITY_COUNT 256
#define MAX_COLLISION_AREA_COUNT 256
#define MAX_DELTA_TIME (1.0f/50.0f)

global_variable Color BackgroundColor = (Color){22, 102, 92, 255};

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
    ui_id_Null,
    ui_id_PlayButton,
    ui_id_QuitButton,
} ui_id;

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

typedef enum
{
    game_mode_Start,
    game_mode_Play,
    game_mode_GameOver,
    game_mode_Quit,
} game_mode;

typedef struct
{
    f32 DeltaTime;
    f32 LastTime;

    game_mode Mode;
    ui UI;

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

internal entity *AddEntity(game_state *GameState, sprite_type SpriteType)
{
    entity *Entity = 0;

    if (GameState->EntityCount < MAX_ENTITY_COUNT)
    {
        Entity = GameState->Entities + GameState->EntityCount;
        GameState->EntityCount += 1;

        switch (SpriteType)
        {
        case sprite_type_Fish:
        {
            Entity->Type = entity_type_Base;
            Entity->Sprites[0].Type = SpriteType;
            Entity->Sprites[0].SourceRectangle = R2(5,3,12,9);
            Entity->Sprites[0].DepthZ = 1;
            Entity->Position = V2(0.0f, 0.0f);
            Entity->CollisionArea = AddCollisionArea(GameState);
            Entity->CollisionArea->Area = MultiplyR2S(R2(1, 1, 10, 6), TEXTURE_MAP_SCALE);
        } break;
        case sprite_type_Eel:
        {
            Entity->Type = entity_type_Base;
            Entity->Sprites[0].Type = SpriteType;
            Entity->Sprites[0].SourceRectangle = R2(1,27,34,20);
            Entity->Sprites[0].DepthZ = 1;
            Entity->Position = V2(0.0f, 0.0f);
            Entity->CollisionArea = AddCollisionArea(GameState);
            Entity->CollisionArea->Area = MultiplyR2S(R2(6, 6, 22, 8), TEXTURE_MAP_SCALE);
        } break;
        case sprite_type_Coral:
        {
            Entity->Type = entity_type_Base;
            Entity->Sprites[0].Type = SpriteType;
            Entity->Sprites[0].SourceRectangle = R2(13,118,TILE_SIZE,TILE_SIZE);
            Entity->Sprites[0].DepthZ = 0;
        } break;
        case sprite_type_Wall:
        {
            Entity->Type = entity_type_Base;
            Entity->Sprites[0].Type = SpriteType;
            Entity->Sprites[0].SourceRectangle = R2(13,147,TILE_SIZE,TILE_SIZE);
            Entity->Sprites[0].DepthZ = 0;
            Entity->CollisionArea = AddCollisionArea(GameState);
            Entity->CollisionArea->Area = MultiplyR2S(R2(0, 0, TILE_SIZE, TILE_SIZE), TEXTURE_MAP_SCALE);

        } break;
        case sprite_type_Cage:
        {
            Entity->Type = entity_type_Base;
            Entity->Sprites[0].Type = SpriteType;
            Entity->Sprites[0].SourceRectangle = R2(90,6,110,70);
            Entity->Sprites[0].DepthZ = 0;
            Entity->Sprites[1].Type = SpriteType;
            Entity->Sprites[1].SourceRectangle = R2(90,101,110,70);
            Entity->Sprites[1].DepthZ = 1;
            Entity->Position = V2(0.0f, 0.0f);
            Entity->CollisionArea = AddCollisionArea(GameState);
            Entity->CollisionArea->Area = MultiplyR2S(R2(1, 1, 110, 70), TEXTURE_MAP_SCALE);
        } break;
        case sprite_type_Crab:
        {
            Entity->Type = entity_type_Base;
            Entity->Sprites[0].Type = SpriteType;
            Entity->Sprites[0].SourceRectangle = R2(14,69,39,20);
            Entity->Sprites[0].DepthZ = 1;
            Entity->Position = MultiplyV2S(V2(9.0f, 7.0f), TILE_SIZE * TEXTURE_MAP_SCALE);
            Entity->CollisionArea = AddCollisionArea(GameState);
            Entity->CollisionArea->Area = MultiplyR2S(R2(2, -3, 16, 9), TEXTURE_MAP_SCALE);
        } break;
        default: break;
        }
    }
    else
    {
        LogError("adding entity");
    }

    return Entity;
}

internal void HandleUserInput(game_state *GameState)
{
    GameState->UI.MousePosition = GetMousePosition();
    GameState->UI.MouseButtonPressed = IsMouseButtonPressed(0);
    GameState->UI.MouseButtonReleased = IsMouseButtonReleased(0);

    Vector2 *Acceleration = &GameState->PlayerEntity->Acceleration;

    Acceleration->x = 0;
    Acceleration->y = 0;

    if (IsKeyDown(KEY_D))
    {
        Acceleration->x += 1.0f;
    }

    if (IsKeyDown(KEY_S))
    {
        Acceleration->y += 1.0f;
    }

    if (IsKeyDown(KEY_A))
    {
        Acceleration->x -= 1.0f;
    }

    if (IsKeyDown(KEY_W))
    {
        Acceleration->y -= 1.0f;
    }

    *Acceleration = NormalizeV2(*Acceleration);
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

internal b32 CollideRanges(f32 ValueA, f32 LengthA, f32 ValueB, f32 LengthB)
{
    Assert(LengthA > 0.0f && LengthB > 0.0f);
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

#define MAX_COLLISION_COUNT 16

internal void UpdateEntities(game_state *GameState)
{
    for (s32 I = 0; I < MAX_ENTITY_COUNT; ++I)
    {
        entity *Entity = GameState->Entities + I;
        sprite_type CollisionTypes[MAX_COLLISION_COUNT];
        s32 CollisionIndex = 0;

        if (!Entity->Type)
        {
            break;
        }

        for (s32 J = 0; J < MAX_ENTITY_COUNT; ++J)
        {
            entity *TestEntity = GameState->Entities + J;

            if (I == J)
            {
                continue;
            }

            if (!TestEntity->Type)
            {
                break;
            }

            b32 Collides = CollideEntities(GameState, Entity, TestEntity);

            if (Collides && CollisionIndex < MAX_COLLISION_COUNT)
            {
                CollisionTypes[CollisionIndex++] = TestEntity->Sprites[0].Type;
            }
        }

        f32 AccelerationThreshold = 0.0001f;
        f32 VelocityThreshold = 1.0f;
        if ((LengthSquaredV2(Entity->Acceleration) < AccelerationThreshold &&
             LengthSquaredV2(Entity->Velocity) < VelocityThreshold))
        {
            Entity->Acceleration = V2(0.0f, 0.0f);
            Entity->Velocity = V2(0.0f, 0.0f);
            return;
        }

        f32 AccelerationScale = 1000.0f;
        f32 DT = GameState->DeltaTime;

        Vector2 P = Entity->Position;
        Vector2 V = Entity->Velocity;
        Vector2 A = MultiplyV2S(Entity->Acceleration, AccelerationScale);
        V = AddV2(V, MultiplyV2S(A, 0.01f));
        A = AddV2(A, AddV2S(MultiplyV2S(V, -4.0f), -1.0f)); /* friction */
        V = ClampV2(V, -300.0f, 300.0f);

        Vector2 NewVelocity = AddV2(MultiplyV2S(A, DT), V);
        Vector2 NewPosition = AddV2(AddV2(MultiplyV2S(A, 0.5f * DT * DT),
                                          MultiplyV2S(V, DT)),
                                    P);

        Entity->Velocity = NewVelocity;
        Entity->Position = NewPosition;
    }
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

/* TODO rename this function, since it does more than debug collision drawing now..... */
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

            b32 PlayerAndEel = ((SpriteA.Type == sprite_type_Fish && SpriteB.Type == sprite_type_Eel) ||
                                (SpriteA.Type == sprite_type_Eel && SpriteB.Type == sprite_type_Fish));

            if (Collides && PlayerAndEel)
            {
                GameState->Mode = game_mode_GameOver;
            }
        }
    }
}

static entity NullEntity;

internal void ResetGame(game_state *GameState)
{
    GameState->EntityCount = 0;

    for (s32 I = 0; I < MAX_ENTITY_COUNT; ++I)
    {
        GameState->Entities[I] = NullEntity;
    }

    { /* init entities */
        GameState->PlayerEntity = AddEntity(GameState, sprite_type_Fish);
        entity *PlayerEntity = GameState->PlayerEntity;
        PlayerEntity->Position = MultiplyV2S(V2(1.0f, 1.0f), TILE_SIZE * TEXTURE_MAP_SCALE);

        GameState->EelEntity = AddEntity(GameState, sprite_type_Eel);
        entity *EelEntity = GameState->EelEntity;
        EelEntity->Position = MultiplyV2S(V2(0.25f, 4.0f), TILE_SIZE * TEXTURE_MAP_SCALE);

        GameState->CrabEntity = AddEntity(GameState, sprite_type_Crab);
        entity *CrabEntity = GameState->CrabEntity;
        CrabEntity->Position = MultiplyV2S(V2(9.0f, 7.0f), TILE_SIZE * TEXTURE_MAP_SCALE);

        GameState->CoralEntity = AddEntity(GameState, sprite_type_Coral);

        GameState->WallEntity = AddEntity(GameState, sprite_type_Wall);

        GameState->CageEntity = AddEntity(GameState, sprite_type_Cage);
        entity *CageEntity = GameState->CageEntity;
        CageEntity->Position = MultiplyV2S(V2(9.0f, 7.0f), TILE_SIZE * TEXTURE_MAP_SCALE);
    }

    GameState->CameraPosition = GameState->PlayerEntity->Position;
    GameState->Mode = game_mode_Start;
    GameState->LastTime = GetTime();
}

internal void UpdateAndRender(void *VoidGameState)
{
    game_state *GameState = (game_state *)VoidGameState;
    ui *UI = &GameState->UI;

    HandleUserInput(GameState);
    BeginDrawing();

    switch(GameState->Mode)
    {
    case game_mode_Start:
    {
        f32 CenterX = SCREEN_WIDTH / 2.0;
        f32 StartY = SCREEN_HEIGHT * 0.5;

        ClearBackground(BackgroundColor);

        alignment Alignment = alignment_CenterCenter;
        b32 Pressed = DoButtonWith(UI, ui_id_PlayButton, (u8 *)"Play", V2(CenterX, StartY), Alignment);

        if (Pressed)
        {
            GameState->Mode = game_mode_Play;
        }

        EndDrawing();
    } break;
    case game_mode_GameOver:
    {
        f32 CenterX = SCREEN_WIDTH / 2.0;
        f32 StartY = SCREEN_HEIGHT * 0.4;

        ClearBackground(BackgroundColor);

        alignment Alignment = alignment_CenterCenter;
        b32 PlayPressed = DoButtonWith(UI, ui_id_PlayButton, (u8 *)"Play Again", V2(CenterX, StartY), Alignment);
        b32 QuitPressed = 0;

#if !PLATFORM_WEB
        QuitPressed = DoButtonWith(UI, ui_id_QuitButton, (u8 *)"Quit", V2(CenterX, StartY + 44), Alignment);
#endif

        if (PlayPressed)
        {
            ResetGame(GameState);
            GameState->Mode = game_mode_Play;
        }
        else if (QuitPressed)
        {
            GameState->Mode = game_mode_Quit;
        }

        EndDrawing();
    } break;
    case game_mode_Play:
    {
        if (!IsTextureReady(GameState->ScubaTexture))
        {
            EndDrawing();
            return;
        }

        f32 StartTime = GetTime();

        { /* update timer */
            f32 DeltaTime = StartTime - GameState->LastTime;
            GameState->DeltaTime = MinF32(MAX_DELTA_TIME, DeltaTime);
            GameState->LastTime = StartTime;
        }

        { /* update entities */
            UpdateEntities(GameState);
            GameState->CameraPosition = GameState->PlayerEntity->Position;
        }

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
            s32 MaxX = MinS32(MAP_WIDTH, MaxCorner.x + Padding);
            s32 MinY = MaxS32(0, MinCorner.y - Padding);
            s32 MaxY = MinS32(MAP_HEIGHT, MaxCorner.y + Padding);

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
                Color BackgroundColor = (Color){0, 0, 0, 255};
                Color ForegroundColor = (Color){255, 255, 255, 255};
                char DebugTextBuffer[128] = {};
                f32 DeltaTime = GetTime() - StartTime;
                f32 Spacing = 0;

                sprintf(DebugTextBuffer, "dt (ms) %.4f ", 1000 * DeltaTime);

                DrawTextEx(GameState->UI.Font, DebugTextBuffer, V2(11, 11), 14, Spacing, BackgroundColor);
                DrawTextEx(GameState->UI.Font, DebugTextBuffer, V2(10, 10), 14, Spacing, ForegroundColor);
            }
        }

        EndDrawing();
    } break;
    default: EndDrawing();
    }
}

internal game_state InitGameState(Texture2D ScubaTexture)
{
    game_state GameState = {0};

    GameState.UI.FontSize = 16;
    GameState.ScubaTexture = ScubaTexture;

    ResetGame(&GameState);

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
#if defined(PLATFORM_WEB)
    InitRaylibCanvas();
#endif

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "SCUBA");

    s32 FontSize = 32;
    s32 *Chars = 0;
    s32 GlyphCount = 0;
    Font LoadedFont = LoadFontEx("../assets/Roboto-Regular.ttf", FontSize, Chars, GlyphCount);

    Texture2D ScubaTexture = {0};
    b32 TextureError = LoadScubaTexture(&ScubaTexture);

    if (TextureError)
    {
        LogError("loading scuba asset texture.");
    }
    else if (!IsFontReady(LoadedFont))
    {
        LogError("loading font data");
    }
    else
    {
        game_state GameState = InitGameState(ScubaTexture);
        GameState.UI.Font = LoadedFont;

#if defined(PLATFORM_WEB)
        emscripten_set_main_loop_arg(UpdateAndRender, &GameState, 0, 1);
#else
        SetTargetFPS(60);

        for (;;)
        {
            b32 ShouldCloseWindow = WindowShouldClose() && !IsKeyPressed(KEY_ESCAPE);
            b32 IsQuitMode = GameState.Mode == game_mode_Quit;

            if (ShouldCloseWindow || IsQuitMode)
            {
                break;
            }

            UpdateAndRender(&GameState);
        }
#endif
    }

    CloseWindow();
}
