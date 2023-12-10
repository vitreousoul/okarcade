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
    Vector2 OldPosition;
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

typedef struct
{
    Vector2 Start;
    Vector2 End;
} line;

typedef struct
{
    f32 I;
    f32 J;
    f32 R;
} circle;

typedef struct
{
    s32 Count;
    Vector2 Collisions[2];
} collision_result;

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


global_variable entity NullEntity;


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

    b32 CollidesInX = !(A.x + A.width < B.x || B.x + B.width < A.x);
    b32 CollidesInY = !(A.y + A.height < B.y || B.y + B.height < A.y);

    b32 Collides = CollidesInX && CollidesInY;

    return Collides;
}

#define MAX_COLLISION_COUNT 16

internal void UpdateEntity(game_state *GameState, entity *Entity)
{
    f32 AccelerationThreshold = 0.0001f;
    f32 VelocityThreshold = 1.0f;

    Entity->OldPosition = Entity->Position;

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

internal void UpdateEntities(game_state *GameState)
{
    /* TODO: Define new loop that:
       finds soonest collision,
       update entities using collision,
       repeat until all entities are out of movement.
    */
    f32 LeastTimeTaken = INFINITY;
    { /* find nearest collision */
        for (s32 I = 0; I < GameState->EntityCount; ++I)
        {
            entity *Entity = GameState->Entities + I;
            UpdateEntity(GameState, Entity); /* TODO this is just during collision testing, please delete!!!!!! */

            for (s32 J = I + 1; J < GameState->EntityCount; ++J)
            {
                entity *TestEntity = GameState->Entities + J;

                f32 TimeTaken = CollideEntities(GameState, Entity, TestEntity);
            }
        }

    }
#if 0
    for (s32 I = 0; I < GameState->EntityCount; ++I)
    {
        entity *Entity = GameState->Entities + I;
        sprite_type CollisionTypes[MAX_COLLISION_COUNT];
        s32 CollisionIndex = 0;

        if (!Entity->Type)
        {
            break;
        }

        if (I == 0)
        {
            UpdateEntity(GameState, Entity);
        }

        for (s32 J = I + 1; J < GameState->EntityCount; ++J)
        {
            entity *TestEntity = GameState->Entities + J;

            if (!TestEntity->Type)
            {
                break;
            }

            UpdateEntity(GameState, TestEntity);
            b32 Collides = CollideEntities(GameState, Entity, TestEntity);

            sprite_type SpriteType = Entity->Sprites[0].Type;
            sprite_type TestSpriteType = TestEntity->Sprites[0].Type;
            b32 PlayerAndEel = ((SpriteType == sprite_type_Fish && TestSpriteType == sprite_type_Eel) ||
                                (SpriteType == sprite_type_Eel && TestSpriteType == sprite_type_Fish));

            if (Collides && PlayerAndEel)
            {
                GameState->Mode = game_mode_GameOver;
            }

            if (Collides && CollisionIndex < MAX_COLLISION_COUNT)
            {
                CollisionTypes[CollisionIndex++] = TestEntity->Sprites[0].Type;
            }
        }
    }
#endif
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

internal collision_result CollideLineAndLine(line FirstLine, line SecondLine)
{
    collision_result Result = {0};

    f32 X1 = FirstLine.Start.x;
    f32 Y1 = FirstLine.Start.y;
    f32 X2 = FirstLine.End.x;
    f32 Y2 = FirstLine.End.y;

    f32 X3 = SecondLine.Start.x;
    f32 Y3 = SecondLine.Start.y;
    f32 X4 = SecondLine.End.x;
    f32 Y4 = SecondLine.End.y;

    /* NOTE: If the divisor is close to 0, then _both_ lines are short. */
    f32 Divisor = (X1 - X2)*(Y3 - Y4) - (Y1 - Y2)*(X3 - X4);

    /* TODO: What value should Threshold be set at? */
    f32 Threshold = 0.001f;

    if (Divisor > Threshold || Divisor < Threshold)
    {
        f32 FirstDeterminant = X1*Y2 - Y1*X2;
        f32 SecondDeterminant = X3*Y4 - Y3*X4;

        f32 X = (FirstDeterminant*(X3 - X4) - (X1 - X2)*SecondDeterminant) / Divisor;
        f32 Y = (FirstDeterminant*(Y3 - Y4) - (Y1 - Y2)*SecondDeterminant) / Divisor;

        f32 FirstMinX = MinS32(FirstLine.Start.x, FirstLine.End.x);
        f32 FirstMinY = MinS32(FirstLine.Start.y, FirstLine.End.y);
        f32 FirstMaxX = MaxS32(FirstLine.Start.x, FirstLine.End.x);
        f32 FirstMaxY = MaxS32(FirstLine.Start.y, FirstLine.End.y);

        f32 SecondMinX = MinS32(SecondLine.Start.x, SecondLine.End.x);
        f32 SecondMinY = MinS32(SecondLine.Start.y, SecondLine.End.y);
        f32 SecondMaxX = MaxS32(SecondLine.Start.x, SecondLine.End.x);
        f32 SecondMaxY = MaxS32(SecondLine.Start.y, SecondLine.End.y);

        f32 CollisionIsInFirst = (X >= FirstMinX && X <= FirstMaxX &&
                                  Y >= FirstMinY && Y <= FirstMaxY);
        f32 CollisionIsInSecond = (X >= SecondMinX && X <= SecondMaxX &&
                                   Y >= SecondMinY && Y <= SecondMaxY);

        if (CollisionIsInFirst && CollisionIsInSecond)
        {
            Result.Collisions[0] = V2(X, Y);
            Result.Count = 1;
        }
    }

    return Result;
}

internal collision_result CollideCircleAndLine(circle Circle, line Line)
{
    collision_result Result = {0};
    f32 Threshold = 8.0f;

    /* (X - I)^2 + (Y - J)^2 - R = 0 */
    f32 I = Circle.I;
    f32 J = Circle.J;
    f32 R = Circle.R;

    /* M*X + N*Y + P = 0 */
    f32 M = Line.End.y - Line.Start.y;
    f32 N = Line.Start.x - Line.End.x;
    f32 P = Line.End.x * Line.Start.y - Line.Start.x * Line.End.y;

    f32 A;
    f32 B;
    f32 C;

    f32 SmallN = N < Threshold && N > -Threshold;

    if (SmallN)
    {

        A = M*M + N*N;
        B = 2 * (N*P + M*N*I - M*M*J);
        C = P*P + 2*M*P*I - M*M*(R*R - I*I - J*J);
    }
    else
    {
        A = M*M + N*N;
        B = 2 * (M*P + M*N*J - N*N*I);
        C = P*P + 2*N*P*J - N*N*(R*R - I*I - J*J);
    }

    f32 Discriminant = B*B - 4*A*C;

    if (Discriminant >= 0.0f)
    {
        f32 MinX = MinF32(Line.Start.x, Line.End.x);
        f32 MinY = MinF32(Line.Start.y, Line.End.y);
        f32 MaxX = MaxF32(Line.Start.x, Line.End.x);
        f32 MaxY = MaxF32(Line.Start.y, Line.End.y);

        f32 SqrtDiscriminant = sqrt(Discriminant);
        f32 DividendPlus = -B + SqrtDiscriminant;
        f32 DividendMinus = -B - SqrtDiscriminant;
        f32 Divisor = 2 * A;

        f32 CollisionXPlus;
        f32 CollisionXMinus;
        f32 CollisionYPlus;
        f32 CollisionYMinus;

        if (SmallN)
        {
            CollisionYPlus = DividendPlus / Divisor;
            CollisionYMinus = DividendMinus / Divisor;

            CollisionXPlus = -(N * CollisionYPlus + P) / M;
            CollisionXMinus = -(N * CollisionYMinus + P) / M;
        }
        else
        {
            CollisionXPlus = DividendPlus / Divisor;
            CollisionXMinus = DividendMinus / Divisor;

            CollisionYPlus = -(M * CollisionXPlus + P) / N;
            CollisionYMinus = -(M * CollisionXMinus + P) / N;
        }

        if (CollisionXPlus >= MinX && CollisionXPlus <= MaxX &&
            CollisionYPlus >= MinY && CollisionYPlus <= MaxY)
        {
            Result.Collisions[Result.Count++] = V2(CollisionXPlus, CollisionYPlus);
        }

        if (CollisionXMinus >= MinX && CollisionXMinus <= MaxX &&
            CollisionYMinus >= MinY && CollisionYMinus <= MaxY)
        {
            Result.Collisions[Result.Count++] = V2(CollisionXMinus, CollisionYMinus);
        }
    }

    return Result;
}

internal void UpdateAndRender(void *VoidGameState)
{
    game_state *GameState = (game_state *)VoidGameState;
    ui *UI = &GameState->UI;

    { /* TODO: Delete this one we are done testing collisions... */
        typedef enum
        {
            collision_type_Circle = 0x1,
            collision_type_Line = 0x2,
        } collision_type;

        typedef struct
        {
            collision_type Type;
            union
            {
                line Line;
                circle Circle;
            };
        } collision_item;

        Color CircleColor = (Color){100, 40, 100, 100};
        Color LineColor = (Color){40, 200, 200, 100};
        Color RectColor = (Color){200, 200, 40, 150};

        Vector2 MouseP = GetMousePosition();

        collision_item Items[] = {
            {collision_type_Line, {{V2(500, 300), V2(800, 400)}}},
            {collision_type_Circle, {MouseP.x, MouseP.y, 128}},
            {collision_type_Line, {{V2(MouseP.x, MouseP.y), V2(700, 200)}}},
            /* {collision_type_Line, {{V2(100, 100), V2(700, 500)}}}, */
        };

        f32 BoxRadius = 4.0f;

        BeginDrawing();
        ClearBackground((Color){0,0,0,255});

        for (u32 IndexA = 0; IndexA < ArrayCount(Items); ++IndexA)
        {
            collision_item ItemA = Items[IndexA];

            for (u32 IndexB = IndexA + 1; IndexB < ArrayCount(Items); ++IndexB)
            {
                collision_result Collision = {0};
                collision_item ItemB = Items[IndexB];

                collision_type Circle = collision_type_Circle;
                collision_type Line = collision_type_Line;

                if (ItemA.Type == Circle && ItemB.Type == Line)
                {
                    circle Circle = ItemA.Circle;
                    line Line = ItemB.Line;

                    DrawCircle(Circle.I, Circle.J, Circle.R, CircleColor);
                    DrawLineEx(Line.Start, Line.End, 2.0f, LineColor);

                    Collision = CollideCircleAndLine(Circle, Line);
                }
                else if (ItemA.Type == Line && ItemB.Type == Circle)
                {
                    circle Circle = ItemB.Circle;
                    line Line = ItemA.Line;

                    DrawCircle(Circle.I, Circle.J, Circle.R, CircleColor);
                    DrawLineEx(Line.Start, Line.End, 2.0f, LineColor);

                    Collision = CollideCircleAndLine(Circle, Line);
                }
                else if (ItemA.Type == Line && ItemB.Type == Line)
                {
                    DrawLineEx(ItemA.Line.Start, ItemA.Line.End, 2.0f, LineColor);
                    DrawLineEx(ItemB.Line.Start, ItemB.Line.End, 2.0f, LineColor);

                    Collision = CollideLineAndLine(ItemA.Line, ItemB.Line);
                }
                else
                {
                    /* TODO: handle other collisions like circle-circle */
                    Assert(0);
                }

                for (s32 I = 0; I < Collision.Count; ++I)
                {
                    Vector2 Point = Collision.Collisions[I];
                    DrawRectangle(Point.x - BoxRadius, Point.y - BoxRadius, 2*BoxRadius, 2*BoxRadius, RectColor);
                }
            }
        }

        EndDrawing();
        return;
    }

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
            { /* draw entity dots */
                for (s32 I = 0; I < GameState->EntityCount; ++I)
                {
                    entity *Entity = GameState->Entities + I;

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
            b32 ShouldCloseWindow = WindowShouldClose();// && !IsKeyPressed(KEY_ESCAPE); /* TODO add back in escape key ignore */
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
