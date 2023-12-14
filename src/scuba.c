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

typedef struct
{
    Vector2 Start;
    Vector2 End;
} line;

typedef struct
{
    f32 X;
    f32 Y;
    f32 R;
} circle;

typedef struct
{
    s32 Count;
    Vector2 Collisions[2];
} collision_result;

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

struct collision_area
{
    collision_type Type;
    union
    {
        line Line;
        circle Circle;
    };
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
    Vector2 Position;
    Vector2 Velocity;
} entity_movement;

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
    s32 SortedEntityTable[MAX_ENTITY_COUNT];

    s32 CollisionAreaCount;
    collision_area CollisionAreas[MAX_COLLISION_AREA_COUNT];

    Vector2 CameraPosition;

    entity *PlayerEntity;

    Texture2D ScubaTexture;
} game_state;

/* BEGIN Debug Draw Commands */
typedef enum
{
   debug_draw_type_Circle,
   debug_draw_type_Line,
   debug_draw_type_Rectangle,
} debug_draw_type;

typedef union
{
    circle Circle;
    line Line;
    Rectangle Rectangle;
} debug_draw_shape;

typedef struct
{
    debug_draw_type Type;
    debug_draw_shape Shape;
    Color Color;
} debug_draw_command;

#define DEBUG_DRAW_COMMAND_MAX 1024
global_variable debug_draw_command DebugDrawCommands[DEBUG_DRAW_COMMAND_MAX] = {0};
global_variable s32 DebugDrawCommandCount = 0;

internal void ResetDebugDrawCommands(void)
{
    DebugDrawCommandCount = 0;
}

internal void PushDebugDrawCommand(debug_draw_command Command)
{
    if (DebugDrawCommandCount < DEBUG_DRAW_COMMAND_MAX)
    {
        DebugDrawCommands[DebugDrawCommandCount] = Command;
        DebugDrawCommandCount += 1;
    }
}

internal void PushDebugCircle(circle Circle, Color Color)
{
    debug_draw_command Command;

    Command.Type = debug_draw_type_Circle;
    Command.Shape.Circle = Circle;
    Command.Color = Color;

    PushDebugDrawCommand(Command);
}

internal void PushDebugLine(line Line, Color Color)
{
    debug_draw_command Command;

    Command.Type = debug_draw_type_Line;
    Command.Shape.Line = Line;
    Command.Color = Color;

    PushDebugDrawCommand(Command);
}

internal void RenderDebugDrawCommands(void)
{
    for (s32 I = 0; I < DebugDrawCommandCount; ++I)
    {
        debug_draw_command Command = DebugDrawCommands[I];

        switch(Command.Type)
        {
        case debug_draw_type_Circle:
        {
            circle Circle = Command.Shape.Circle;

            DrawCircleLines(Circle.X, Circle.Y, Circle.R, Command.Color);
        } break;
        case debug_draw_type_Line:
        {
            line Line = Command.Shape.Line;

            DrawLine(Line.Start.x, Line.Start.y, Line.End.x, Line.End.y, Command.Color);
        } break;
        case debug_draw_type_Rectangle:
        {
            Rectangle Rectangle = Command.Shape.Rectangle;

            DrawRectangleLinesEx(Rectangle, 2.0f, Command.Color);
        } break;
        }
    }
}
/* END Debug Draw Commands */

/* BEGIN Debug frame-rate historgram */
#define FRAME_RATE_HISTORY_COUNT_LOG2 6
#define FRAME_RATE_HISTORY_COUNT (1 << FRAME_RATE_HISTORY_COUNT_LOG2)
#define FRAME_RATE_HISTORY_MASK (FRAME_RATE_HISTORY_COUNT - 1)

global_variable f32 FrameRateHistory[FRAME_RATE_HISTORY_COUNT];
global_variable u32 FrameRateHistoryIndex = 0;

internal void RecordFrameRate(f32 FrameRate)
{
    FrameRateHistory[FrameRateHistoryIndex] = FrameRate;
    FrameRateHistoryIndex = (FrameRateHistoryIndex + 1) & FRAME_RATE_HISTORY_MASK;
}

internal void DrawFrameRateHistory(void)
{
    Color RectangleColor = (Color){255,255,255,100};

    for (u32 I = 0; I < FRAME_RATE_HISTORY_COUNT; ++I)
    {
        s32 HistogramItemWidth = 2;
        s32 HistogramFullWidth = HistogramItemWidth * FRAME_RATE_HISTORY_COUNT;

        f32 HistogramHeight = 42.0f;
        f32 FrameRate = 1000.0f * AbsF32(FrameRateHistory[I]);
        f32 FrameRateScaled = FrameRate;

        s32 FrameRateItemHeight = (s32)(FrameRateScaled);

        s32 FrameRateItemX = SCREEN_WIDTH - 24 - HistogramFullWidth + I * HistogramItemWidth;
        s32 FrameRateItemY = HistogramHeight + 24 - FrameRateItemHeight;

        DrawRectangle(FrameRateItemX, FrameRateItemY, 2, FrameRateItemHeight, RectangleColor);
    }
}
/* END Debug frame-rate historgram */

internal collision_area *AddCollisionArea(game_state *GameState)
{
    collision_area *CollisionArea = 0;
    collision_area NullArea = {0};

    if (GameState->CollisionAreaCount < MAX_COLLISION_AREA_COUNT)
    {
        CollisionArea = GameState->CollisionAreas + GameState->CollisionAreaCount;
        *CollisionArea = NullArea;
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
        *Entity = NullEntity;
        GameState->EntityCount += 1;

        switch (SpriteType)
        {
        case sprite_type_Fish:
        {
            Entity->Type = entity_type_Base;
            Entity->Sprites[0].Type = SpriteType;
            Entity->Sprites[0].SourceRectangle = R2(5,3,12,9);
            Entity->Sprites[0].DepthZ = 2;
            Entity->Position = V2(0.0f, 0.0f);
            Entity->CollisionArea = AddCollisionArea(GameState);
            Entity->CollisionArea->Type = collision_type_Circle;
            Entity->CollisionArea->Circle = (circle){0.0f, -3.0f, 20.0f};
        } break;
        case sprite_type_Eel:
        {
            Entity->Type = entity_type_Base;
            Entity->Sprites[0].Type = SpriteType;
            Entity->Sprites[0].SourceRectangle = R2(1,27,34,20);
            Entity->Sprites[0].DepthZ = 1;
            Entity->Position = V2(0.0f, 0.0f);
            Entity->CollisionArea = AddCollisionArea(GameState);
            Entity->CollisionArea->Type = collision_type_Circle;
            Entity->CollisionArea->Circle = (circle){40.0f, 20.0f, 32.0f};
        } break;
        case sprite_type_Coral:
        {
            Entity->Type = entity_type_Base;
            Entity->Sprites[0].Type = SpriteType;
            Entity->Sprites[0].SourceRectangle = R2(13,118,TILE_SIZE,TILE_SIZE);
            Entity->Sprites[0].DepthZ = -1;
        } break;
        case sprite_type_Wall:
        {
            f32 TileScale = TILE_SIZE * TEXTURE_MAP_SCALE;
            f32 TileHalfScale = TileScale / 2.0f;
            Entity->Type = entity_type_Base;
            Entity->Sprites[0].Type = SpriteType;
            Entity->Sprites[0].SourceRectangle = R2(13,147,TILE_SIZE,TILE_SIZE);
            Entity->Sprites[0].DepthZ = -1;
            Entity->CollisionArea = AddCollisionArea(GameState);
            Entity->CollisionArea->Type = collision_type_Line;
            Entity->CollisionArea->Line.Start = V2(-TileHalfScale, -TileHalfScale);
            Entity->CollisionArea->Line.End = V2(-TileHalfScale, TileHalfScale);
        } break;
        case sprite_type_Cage:
        {
            Entity->Type = entity_type_Base;
            Entity->Sprites[0].Type = SpriteType;
            Entity->Sprites[0].SourceRectangle = R2(90,6,110,70);
            Entity->Sprites[0].DepthZ = 0;
            Entity->Sprites[1].Type = SpriteType;
            Entity->Sprites[1].SourceRectangle = R2(90,101,110,70);
            Entity->Sprites[1].DepthZ = 3;
            Entity->Position = V2(0.0f, 0.0f);
            Entity->CollisionArea = AddCollisionArea(GameState);
            Entity->CollisionArea->Type = collision_type_Circle;
            Entity->CollisionArea->Circle = (circle){0.0f, 0.0f, 162.0f};
        } break;
        case sprite_type_Crab:
        {
            Entity->Type = entity_type_Base;
            Entity->Sprites[0].Type = SpriteType;
            Entity->Sprites[0].SourceRectangle = R2(14,69,39,20);
            Entity->Sprites[0].DepthZ = 1;
            Entity->Position = MultiplyV2S(V2(9.0f, 7.0f), TILE_SIZE * TEXTURE_MAP_SCALE);
            Entity->CollisionArea = AddCollisionArea(GameState);
            Entity->CollisionArea->Type = collision_type_Circle;
            Entity->CollisionArea->Circle = (circle){0.0f, 0.0f, 32.0f};
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
            Rectangle DestRectangle = R2(Position.x, Position.y, SpriteSize.x, SpriteSize.y);
            Color Tint = (Color){255,255,255,255};
            Vector2 Origin = (Vector2){0,0};

            DrawTexturePro(GameState->ScubaTexture, SourceRectangle, DestRectangle, Origin, 0.0f, Tint);
        }
        else
        {
            break;
        }
    }
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

        /* TODO: What should ErrorMargin be set to???? */
        f32 ErrorMargin = 0.7f;

        f32 FirstMinX = MinS32(FirstLine.Start.x, FirstLine.End.x) - ErrorMargin;
        f32 FirstMinY = MinS32(FirstLine.Start.y, FirstLine.End.y) - ErrorMargin;
        f32 FirstMaxX = MaxS32(FirstLine.Start.x, FirstLine.End.x) + ErrorMargin;
        f32 FirstMaxY = MaxS32(FirstLine.Start.y, FirstLine.End.y) + ErrorMargin;

        f32 SecondMinX = MinS32(SecondLine.Start.x, SecondLine.End.x) - ErrorMargin;
        f32 SecondMinY = MinS32(SecondLine.Start.y, SecondLine.End.y) - ErrorMargin;
        f32 SecondMaxX = MaxS32(SecondLine.Start.x, SecondLine.End.x) + ErrorMargin;
        f32 SecondMaxY = MaxS32(SecondLine.Start.y, SecondLine.End.y) + ErrorMargin;

        b32 CollisionIsInFirst = (X >= FirstMinX && X <= FirstMaxX &&
                                  Y >= FirstMinY && Y <= FirstMaxY);
        b32 CollisionIsInSecond = (X >= SecondMinX && X <= SecondMaxX &&
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
    f32 I = Circle.X;
    f32 J = Circle.Y;
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

internal collision_result CollideCircleAndCircle(circle FirstCircle, circle SecondCircle)
{
    /*
              /|\
         R0 /  |H \ R1
          /    |    \
         ---A--P--B---
          \    |    /
         R0 \  |H / R1
              \|/

         D = A + B
     */
    collision_result Result = {0};

    f32 R0 = FirstCircle.R;
    f32 R1 = SecondCircle.R;

    Vector2 FirstPosition = V2(FirstCircle.X, FirstCircle.Y);
    Vector2 SecondPosition = V2(SecondCircle.X, SecondCircle.Y);

    f32 D = LengthV2(SubtractV2(FirstPosition, SecondPosition));

    b32 CirclesAreCloseEnough = D < R0 + R1;
    b32 OneCircleIsInsideTheOther = D < AbsF32(R0 - R1);
    b32 CirclesInSamePosition = D < 1.0f;

    if (CirclesAreCloseEnough &&
        !OneCircleIsInsideTheOther &&
        !CirclesInSamePosition)
    {
        f32 A = (R0*R0 - R1*R1 + D*D) / (2*D);
        f32 RadiusMinusA = R0*R0 - A*A;

        if (RadiusMinusA > 0.0f)
        {
            f32 H = sqrt(RadiusMinusA);
            Vector2 P = AddV2(FirstPosition,
                              DivideV2S(MultiplyV2S(SubtractV2(SecondPosition, FirstPosition),
                                                    A),
                                        D));

            f32 DeltaX = SecondPosition.x - FirstPosition.x;
            f32 DeltaY = SecondPosition.y - FirstPosition.y;

            Vector2 CollisionPlus = V2(P.x + H * DeltaY / D, P.y - H * DeltaX / D);
            Vector2 CollisionMinus = V2(P.x - H * DeltaY / D, P.y + H * DeltaX / D);

            Result.Collisions[0] = CollisionPlus;
            Result.Collisions[1] = CollisionMinus;
            Result.Count = 2;
        }
    }

    return Result;
}

internal entity_movement GetEntityMovement(game_state *GameState, entity *Entity)
{
    entity_movement Movement = {0};

    f32 AccelerationThreshold = 0.0001f;
    f32 VelocityThreshold = 1.0f;

    if ((LengthSquaredV2(Entity->Acceleration) > AccelerationThreshold ||
         LengthSquaredV2(Entity->Velocity) > VelocityThreshold))
    {
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

        Movement.Position = SubtractV2(NewPosition, Entity->Position);
        Movement.Velocity = SubtractV2(NewVelocity, Entity->Velocity);
    }

    return Movement;
}

internal void UpdateEntity(game_state *GameState, entity *Entity)
{
    entity_movement Movement = GetEntityMovement(GameState, Entity);

    Entity->Velocity = AddV2(Entity->Velocity, Movement.Velocity);
    Entity->Position = AddV2(Entity->Position, Movement.Position);
}

internal circle GetOffsetCircle(circle Circle, Vector2 Offset)
{
    circle Result = Circle;
    Result.X += Offset.x;
    Result.Y += Offset.y;

    return Result;
}

internal line GetOffsetLine(line Line, Vector2 Offset)
{
    line Result = Line;
    Result.Start.x += Offset.x;
    Result.Start.y += Offset.y;
    Result.End.x += Offset.x;
    Result.End.y += Offset.y;

    return Result;
}

internal void UpdateEntities(game_state *GameState)
{
    /* TODO: Define new loop that:
       finds soonest collision,
       update entities using collision,
       repeat until all entities are out of movement.
    */
    /* find nearest collision */
    for (s32 I = 0; I < GameState->EntityCount; ++I)
    {
        entity *Entity = GameState->Entities + I;
        b32 EntityCollided = 0;

        entity_movement EntityMovement = GetEntityMovement(GameState, Entity);

        for (s32 J = I + 1; J < GameState->EntityCount; ++J)
        {
            entity *TestEntity = GameState->Entities + J;

            if (!(Entity->CollisionArea && TestEntity->CollisionArea))
            {
                continue;
            }

            u32 CollisionTypes = Entity->CollisionArea->Type | TestEntity->CollisionArea->Type;
            collision_result CollisionResult = {0};

            switch(CollisionTypes)
            {
            case collision_type_Circle:
            {
                Vector2 OffsetEntity = AddV2(Entity->Position, EntityMovement.Position);

                circle CircleA = GetOffsetCircle(Entity->CollisionArea->Circle, OffsetEntity);
                circle CircleB = GetOffsetCircle(TestEntity->CollisionArea->Circle, TestEntity->Position);

                CollisionResult = CollideCircleAndCircle(CircleA, CircleB);

                if (CollisionResult.Count)
                {
                    EntityCollided = 1;
                }

                if (Entity->Sprites[0].Type == sprite_type_Fish)
                {
                    f32 DeltaX = CircleA.X - CircleB.X;
                    f32 DeltaY = CircleA.Y - CircleB.Y;

                    f32 Distance = sqrt(DeltaX*DeltaX + DeltaY*DeltaY);
                    f32 Overlap = (CircleA.R + CircleB.R) - Distance;

                    if (Overlap > 0.0f)
                    {
                        /* TODO: move colliding entities */

                        { /* debug drawing */
                            Vector2 ScreenCircleA = WorldToScreenPosition(GameState, V2(CircleA.X, CircleA.Y));
                            Vector2 ScreenCircleB = WorldToScreenPosition(GameState, V2(CircleB.X, CircleB.Y));

                            circle Circle = (circle){ScreenCircleA.x, ScreenCircleA.y, CircleA.R};
                            circle TestCircle = (circle){ScreenCircleB.x, ScreenCircleB.y, CircleB.R};

                            PushDebugCircle(Circle, (Color){255,255,0,255});
                            PushDebugCircle(TestCircle, (Color){255,0,255,255});
                        }
                    }
                }
            } break;
            case collision_type_Line:
            {
                line LineA = Entity->CollisionArea->Line;
                line LineB = TestEntity->CollisionArea->Line;

                CollisionResult = CollideLineAndLine(LineA, LineB);
            } break;
            case (collision_type_Circle | collision_type_Line):
            {
                circle Circle;
                line Line;

                if (Entity->CollisionArea->Type == collision_type_Circle)
                {
                    circle AreaCircle = Entity->CollisionArea->Circle;
                    line AreaLine = TestEntity->CollisionArea->Line;

                    Vector2 CircleOffset = AddV2(Entity->Position, V2(AreaCircle.X, AreaCircle.Y));

                    Circle = (circle){CircleOffset.x, CircleOffset.y, AreaCircle.R};
                    Line = (line){AddV2(AreaLine.Start, TestEntity->Position),
                                  AddV2(AreaLine.End, TestEntity->Position)};
                }
                else
                {
                    circle AreaCircle = TestEntity->CollisionArea->Circle;
                    line AreaLine = TestEntity->CollisionArea->Line;

                    Vector2 CircleOffset = AddV2(TestEntity->Position, V2(AreaCircle.X, AreaCircle.Y));

                    Circle = (circle){CircleOffset.x, CircleOffset.y, AreaCircle.R};
                    Line = (line){AddV2(AreaLine.Start, Entity->Position),
                                  AddV2(AreaLine.End, Entity->Position)};
                }

                CollisionResult = CollideCircleAndLine(Circle, Line);
            } break;
            default: { Assert(0); } break;
            }
        }

        if (EntityCollided)
        {
            Entity->Acceleration = V2(0, 0);
            Entity->Velocity = V2(0, 0);
        }
        else
        {
            UpdateEntity(GameState, Entity);
        }
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

internal void GenerateSortedEntityTable(game_state *GameState)
{
    s32 SortedEntityIndex = 0;

    for (s32 I = 0; I < GameState->EntityCount; ++I)
    {
        entity Entity = GameState->Entities[I];

        if (SortedEntityIndex == 0)
        {
            GameState->SortedEntityTable[0] = I;
            SortedEntityIndex += 1;
        }
        else
        {
            b32 Added = 0;

            for (s32 J = 0; J < SortedEntityIndex; ++J)
            {
                s32 TestEntityIndex = GameState->SortedEntityTable[J];
                entity TestEntity = GameState->Entities[TestEntityIndex];

                if (Entity.Sprites[0].DepthZ < TestEntity.Sprites[0].DepthZ)
                {
                    Added = 1;
                    s32 TempIndex = I;

                    for (s32 K = J; K <= SortedEntityIndex + 1; ++K)
                    {
                        Assert(SortedEntityIndex + 1 < MAX_ENTITY_COUNT);
                        s32 AnotherTemp = GameState->SortedEntityTable[K];
                        GameState->SortedEntityTable[K] = TempIndex;
                        TempIndex = AnotherTemp;
                    }

                    SortedEntityIndex += 1;
                    break;
                }
            }

            if (!Added)
            {
                Assert(SortedEntityIndex < MAX_ENTITY_COUNT);
                GameState->SortedEntityTable[SortedEntityIndex] = I;
                SortedEntityIndex += 1;
            }
        }
    }
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

        entity *EelEntity = AddEntity(GameState, sprite_type_Eel);
        EelEntity->Position = MultiplyV2S(V2(0.25f, 4.0f), TILE_SIZE * TEXTURE_MAP_SCALE);

        entity *CrabEntity = AddEntity(GameState, sprite_type_Crab);
        CrabEntity->Position = MultiplyV2S(V2(9.0f, 7.0f), TILE_SIZE * TEXTURE_MAP_SCALE);

        /* TODO: Fix z-sorting, so that the front of the cage is in front of entities
                 like the player and the crab. The easiest way is probably to split
                 the cage into two entities, with their own individual depths.
        */
        entity *CageEntity = AddEntity(GameState, sprite_type_Cage);
        CageEntity->Position = MultiplyV2S(V2(9.0f, 7.0f), TILE_SIZE * TEXTURE_MAP_SCALE);

        for (s32 J = 0; J < MAP_HEIGHT; ++J)
        {
            for (s32 I = 0; I < MAP_WIDTH; ++I)
            {
                switch(Map[J][I])
                {
                case 'w':
                {
                    f32 Scale = TEXTURE_MAP_SCALE * TILE_SIZE;
                    Vector2 Start = V2(Scale * I, Scale * J);
                    Vector2 End = V2(Start.x, Start.y + Scale);

                    entity *Entity = AddEntity(GameState, sprite_type_Wall);
                    Entity->Position = Start;
                } break;
                default:
                    break;
                }
            }
        }
    }

    GenerateSortedEntityTable(GameState);

    GameState->CameraPosition = GameState->PlayerEntity->Position;
    GameState->Mode = game_mode_Play;//game_mode_Start;
    GameState->LastTime = GetTime();
}

internal void UpdateAndRender(void *VoidGameState)
{
    game_state *GameState = (game_state *)VoidGameState;
    ui *UI = &GameState->UI;

    ResetDebugDrawCommands();

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
            RecordFrameRate(DeltaTime);
            GameState->DeltaTime = MinF32(MAX_DELTA_TIME, DeltaTime);
            GameState->LastTime = StartTime;
        }

        { /* update entities */
            UpdateEntities(GameState);
            GameState->CameraPosition = GameState->PlayerEntity->Position;
        }

        ClearBackground(BackgroundColor);

        { /* draw entities */
            for (s32 I = 0; I < GameState->EntityCount; ++I)
            {
                s32 EntityIndex = GameState->SortedEntityTable[I];
                Assert(EntityIndex >= 0 && EntityIndex < MAX_ENTITY_COUNT);
                entity *Entity = GameState->Entities + EntityIndex;
                DrawSprite(GameState, Entity, 0);
            }
        }

        { /* debug graphics */
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

            RenderDebugDrawCommands();

            DrawFrameRateHistory();
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
