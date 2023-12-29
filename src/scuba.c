#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "../lib/raylib.h"

#if PLATFORM_WEB
#define ryn_PROFILER 0
#endif
#include "ryn_prof.h"

#include "types.h"

int SCREEN_WIDTH = 1200;
int SCREEN_HEIGHT = 700;

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#include "raylib_defines.c"
#endif

#include "../gen/scuba.h"
#include "../gen/roboto_regular.h"
#include "core.c"
#include "math.c"
#include "raylib_helpers.h"
#include "ui.c"

#define PI_OVER_2 (PI/2.0f)

#define DEBUG_DRAW_COLLISIONS 1

#define TEXTURE_MAP_SCALE 4
#define MAX_ENTITY_COUNT 256
#define MAX_COLLISION_AREA_COUNT 512
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
    {W,o,W,o,o,W,o,o,o,o,o,o,o,o,o,W},
    {W,o,W,W,W,W,o,W,W,W,o,o,o,o,o,W},
    {W,o,o,o,o,o,o,o,o,o,o,o,o,o,o,W},
    {W,W,W,W,o,W,W,W,W,o,o,o,o,o,o,W},
    {W,o,o,W,o,W,o,o,o,o,o,o,o,o,o,W},
    {W,o,o,W,o,W,o,o,o,o,o,o,o,o,o,W},
    {W,o,o,W,o,W,o,o,o,o,o,o,o,o,o,W},
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

typedef enum
{
    entity_movement_type_None,
    entity_movement_type_Moveable,
} entity_movement_type;

typedef struct
{
    Vector2 Position;
    Vector2 Velocity;
} entity_movement;

typedef enum
{
    minkowski_shape_CircleAndRectangle,
    minkowski_shape_CircleAndLine,
} minkowski_shape_type;

typedef struct
{
    circle Circles[4];
    line Lines[4];
} minkowski_circle_and_rectangle;

typedef struct
{
    circle Circles[2];
    line Lines[2];
} minkowski_circle_and_line;

typedef struct
{
    minkowski_shape_type Type;
    union
    {
        minkowski_circle_and_rectangle CircleAndRectangle;
        minkowski_circle_and_line CircleAndLine;
    };
} minkowski_shape;

typedef struct
{
    entity_type Type;
    entity_movement_type MovementType;

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

typedef enum
{
    TB_UpdateAndRender,
    TB_UpdateEntities,
    TB_DrawEntities,
    TB_UpdateEntity,
    TB_DebugGraphics,
    TB_TimedBlockCount,
} timed_block_kind;

#define DEBUG_DRAW_COMMAND_MAX 1024
global_variable debug_draw_command DebugDrawCommands[DEBUG_DRAW_COMMAND_MAX] = {0};
global_variable s32 DebugDrawCommandCount = 0;
global_variable char DebugTextBuffer[1024];
global_variable u64 CPUFreq = 1;

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

internal Vector2 WorldToScreenPosition(game_state *GameState, Vector2 P)
{
    Vector2 HalfScreen = MultiplyV2S(V2(SCREEN_WIDTH, SCREEN_HEIGHT), 0.5f);
    Vector2 Result = AddV2(SubtractV2(P, GameState->CameraPosition), HalfScreen);

    return Result;
}

internal void PushDebugScreenRectangle(game_state *GameState, Rectangle Rect)
{
    line Left = (line){
        WorldToScreenPosition(GameState, V2(Rect.x,Rect.y)),
        WorldToScreenPosition(GameState, V2(Rect.x,Rect.y+Rect.height)),
    };
    line Top = (line){
        WorldToScreenPosition(GameState, V2(Rect.x,Rect.y)),
        WorldToScreenPosition(GameState, V2(Rect.x+Rect.width,Rect.y)),
    };
    line Right = (line){
        WorldToScreenPosition(GameState, V2(Rect.x+Rect.width,Rect.y)),
        WorldToScreenPosition(GameState, V2(Rect.x+Rect.width,Rect.y+Rect.height)),
    };
    line Bottom = (line){
        WorldToScreenPosition(GameState, V2(Rect.x,Rect.y+Rect.height)),
        WorldToScreenPosition(GameState, V2(Rect.x+Rect.width,Rect.y+Rect.height)),
    };

    PushDebugLine(Left, (Color){255,0,255,255});
    PushDebugLine(Top, (Color){255,0,255,255});
    PushDebugLine(Right, (Color){255,0,255,255});
    PushDebugLine(Bottom, (Color){255,0,255,255});
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

    Assert(GameState->CollisionAreaCount < MAX_COLLISION_AREA_COUNT);

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
            Entity->MovementType = entity_movement_type_Moveable;
            Entity->Sprites[0].Type = SpriteType;
            Entity->Sprites[0].SourceRectangle = R2(5,3,12,9);
            Entity->Sprites[0].DepthZ = 2;
            Entity->Position = V2(0.0f, 0.0f);
            Entity->CollisionArea = AddCollisionArea(GameState);
            Entity->CollisionArea->Type = collision_type_Circle;
            Entity->CollisionArea->Circle = (circle){0.0f, -3.0f, 15.0f};
        } break;
        case sprite_type_Eel:
        {
            Entity->Type = entity_type_Base;
            Entity->MovementType = entity_movement_type_Moveable;
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
            Entity->MovementType = entity_movement_type_None;
            Entity->Sprites[0].Type = SpriteType;
            Entity->Sprites[0].SourceRectangle = R2(13,118,TILE_SIZE,TILE_SIZE);
            Entity->Sprites[0].DepthZ = -1;
        } break;
        case sprite_type_Wall:
        {
            f32 TileScale = TILE_SIZE * TEXTURE_MAP_SCALE;
            f32 TileHalfScale = TileScale / 2.0f;
            Entity->Type = entity_type_Base;
            Entity->MovementType = entity_movement_type_None;
            Entity->Sprites[0].Type = SpriteType;
            Entity->Sprites[0].SourceRectangle = R2(13,147,TILE_SIZE,TILE_SIZE);
            Entity->Sprites[0].DepthZ = -1;
            /* TODO: Clean up the way we do collision area definition */
            /* left line */
            Entity->CollisionArea = AddCollisionArea(GameState);
            collision_area *Area = Entity->CollisionArea;
            Area->Type = collision_type_Line;
            Area->Line.Start = V2(-TileHalfScale, -TileHalfScale);
            Area->Line.End = V2(-TileHalfScale, TileHalfScale);
            /* top line */
            Area->Next = AddCollisionArea(GameState);
            Area = Area->Next;
            Area->Type = collision_type_Line;
            Area->Line.Start = V2(-TileHalfScale, -TileHalfScale);
            Area->Line.End = V2(TileHalfScale, -TileHalfScale);
            /* right line */
            Area->Next = AddCollisionArea(GameState);
            Area = Area->Next;
            Area->Type = collision_type_Line;
            Area->Line.Start = V2(TileHalfScale, -TileHalfScale);
            Area->Line.End = V2(TileHalfScale, TileHalfScale);
            /* bottom line */
            Area->Next = AddCollisionArea(GameState);
            Area = Area->Next;
            Area->Type = collision_type_Line;
            Area->Line.Start = V2(-TileHalfScale, TileHalfScale);
            Area->Line.End = V2(TileHalfScale, TileHalfScale);
        } break;
        case sprite_type_Cage:
        {
            Entity->Type = entity_type_Base;
            Entity->MovementType = entity_movement_type_None;
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
            Entity->MovementType = entity_movement_type_Moveable;
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

internal void DrawSprite(game_state *GameState, entity *Entity)
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

internal inline circle GetOffsetCircle(circle Circle, Vector2 Offset)
{
    circle Result = (circle){ Circle.X + Offset.x, Circle.Y + Offset.y, Circle.R };
    return Result;
}

internal inline line GetOffsetLine(line Line, Vector2 Offset)
{
    line Result = (line){AddV2(Line.Start, Offset), AddV2(Line.End, Offset)};
    return Result;
}

internal minkowski_circle_and_rectangle MinkowskiSumCircleAndRectangle(circle Circle, Rectangle Rectangle)
{
    minkowski_circle_and_rectangle Shape;

    f32 CR = Circle.R;

    f32 RX = Rectangle.x;
    f32 RY = Rectangle.y;
    f32 RW = Rectangle.width;
    f32 RH = Rectangle.height;

    Shape.Circles[0] = (circle){RX,      RY,      Circle.R};
    Shape.Circles[1] = (circle){RX + RW, RY,      Circle.R};
    Shape.Circles[2] = (circle){RX + RW, RY + RH, Circle.R};
    Shape.Circles[2] = (circle){RX,      RY + RH, Circle.R};

    Shape.Lines[0] = (line){{RX - CR,      RY     }, {RX - CR,      RY + RH}};
    Shape.Lines[1] = (line){{RX,           RY - CR}, {RX + RW,      RY - CR}};
    Shape.Lines[2] = (line){{RX + RW + CR, RY     }, {RX + RW + CR, RY + RH}};
    Shape.Lines[2] = (line){{RX,           RY + CR}, {RX + RW,      RY + CR}};

    return Shape;
}

internal minkowski_circle_and_line MinkowskiSumCircleAndLine(circle Circle, line Line)
{
    minkowski_circle_and_line CircleAndLine;

    Vector2 OrthogonalLine = NormalizeV2(RotateV2(SubtractV2(Line.End, Line.Start), PI_OVER_2));
    Vector2 LineOffsetPlus = MultiplyV2S(OrthogonalLine, Circle.R);
    Vector2 LineOffsetMinus = MultiplyV2S(OrthogonalLine, -Circle.R);

    CircleAndLine.Circles[0] = (circle){Line.Start.x, Line.Start.y, Circle.R};
    CircleAndLine.Circles[1] = (circle){Line.End.x, Line.End.y, Circle.R};

    CircleAndLine.Lines[0] = (line){AddV2(Line.Start, LineOffsetPlus),
                                    AddV2(Line.End, LineOffsetPlus)};
    CircleAndLine.Lines[1] = (line){AddV2(Line.Start, LineOffsetMinus),
                                    AddV2(Line.End, LineOffsetMinus)};

    return CircleAndLine;
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

internal b32 CollideRectangles(Rectangle RectangleA, Rectangle RectangleB)
{
    b32 Collides = 0;

    b32 AIsToTheLeft = RectangleA.x + RectangleA.width < RectangleB.x;
    b32 AIsToTheRight = RectangleA.x > RectangleB.x + RectangleB.width;

    b32 AIsAbove = RectangleA.y + RectangleA.height < RectangleB.y;
    b32 AIsBelow = RectangleA.y > RectangleB.y + RectangleB.height;

    Collides = !(AIsToTheLeft || AIsToTheRight || AIsAbove || AIsBelow);

    return Collides;
}

internal f32 GetWhichSideOfALine(Vector2 Point, line Line)
{
    f32 WhichSide;

    f32 X0 = Line.Start.x;
    f32 Y0 = Line.Start.y;
    f32 X1 = Line.End.x;
    f32 Y1 = Line.End.y;

    f32 Threshold = 1.0f;

    f32 DeltaX = X1 - X0;
    f32 DeltaY = Y1 - Y0;

    if (DeltaX > Threshold)
    {
        f32 M = DeltaY / DeltaX;
        f32 B = Y0 - M*X0;

        WhichSide = Point.y - M*Point.x - B;
    }
    else
    {
        WhichSide = Point.x - X0;
    }

    return WhichSide;
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
        A = AddV2(A, AddV2S(MultiplyV2S(V, -4.0f), -2.0f)); /* friction */
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
    ryn_BEGIN_TIMED_BLOCK(TB_UpdateEntity);
    entity_movement Movement = GetEntityMovement(GameState, Entity);

    Entity->Velocity = AddV2(Entity->Velocity, Movement.Velocity);
    Entity->Position = AddV2(Entity->Position, Movement.Position);
    ryn_END_TIMED_BLOCK(TB_UpdateEntity);
}

internal Rectangle GetSpriteRectangle(entity *Entity)
{
    Rectangle SourceRectangle = Entity->Sprites[0].SourceRectangle;

    f32 Width = TEXTURE_MAP_SCALE * SourceRectangle.width;
    f32 Height = TEXTURE_MAP_SCALE * SourceRectangle.height;

    Rectangle SpriteRectangle = (Rectangle){
        Entity->Position.x - (Width / 2.0f),
        Entity->Position.y - (Height / 2.0f),
        Width,
        Height
    };

    return SpriteRectangle;
}

internal b32 IsCollisionCloser(Vector2 Position, collision_result KnownMinimum, collision_result TestCollision)
{
    b32 TestCollisionIsCloser = 0;

    for (s32 I = 0; I < KnownMinimum.Count; ++I)
    {
        f32 DistanceA = LengthSquaredV2(SubtractV2(Position, KnownMinimum.Collisions[I]));

        for (s32 J = 0; J < TestCollision.Count; ++J)
        {
            f32 DistanceB = LengthSquaredV2(SubtractV2(Position, TestCollision.Collisions[I]));

            if (DistanceB < DistanceA)
            {
                TestCollisionIsCloser = 1;
                break;
            }
        }
    }

    return TestCollisionIsCloser;
}

internal void UpdateEntities(game_state *GameState)
{
    ryn_BEGIN_TIMED_BLOCK(TB_UpdateEntities);

    for (s32 I = 0; I < GameState->EntityCount; ++I)
    {
        entity *Entity = GameState->Entities + I;
        entity_movement EntityMovement = GetEntityMovement(GameState, Entity);

        if (Entity->MovementType != entity_movement_type_Moveable)
        {
            continue;
        }

        for (s32 J = 0; J < GameState->EntityCount; ++J)
        {
            entity *TestEntity = GameState->Entities + J;

            {
                /* Assume that the entity's sprite rectangle contains all collision geometry.
                   If the rectangles do _not_ intersect, then skip collision detection.
                */
                Rectangle SpriteRectangle = GetSpriteRectangle(Entity);
                Rectangle TestSpriteRectangle = GetSpriteRectangle(TestEntity);

                b32 RectanglesCollide = CollideRectangles(SpriteRectangle, TestSpriteRectangle);
                b32 BothEntitiesAreTheSame = I == J;

                if (!RectanglesCollide || BothEntitiesAreTheSame)
                {
                    continue;
                }
            }

            for (collision_area *Area = Entity->CollisionArea; Area; Area = Area->Next)
            {
                for (collision_area *TestArea = TestEntity->CollisionArea; TestArea; TestArea = TestArea->Next)
                {
                    if (Area->Type == collision_type_Circle && TestArea->Type == collision_type_Circle)
                    {
                        /* TODO: handle circle-circle collision */
                    }
                    else if (Area->Type == collision_type_Line && TestArea->Type == collision_type_Line)
                    {
                        /* TODO: handle line-line collision */
                    }
                    else if (Area->Type == collision_type_Circle && TestArea->Type == collision_type_Line)
                    {
                        circle AreaCircle = Area->Circle;
                        line AreaLine = TestArea->Line;

                        line MovementLine = (line){Entity->Position, AddV2(Entity->Position, EntityMovement.Position)};
                        circle Circle = GetOffsetCircle(AreaCircle, MovementLine.End);
                        line Line = GetOffsetLine(AreaLine, TestEntity->Position);

                        minkowski_circle_and_line CircleAndLine = MinkowskiSumCircleAndLine(Circle, Line);

#if DEBUG_DRAW_COLLISIONS
                        {
                            line MovementScreenLine = (line){WorldToScreenPosition(GameState, MovementLine.Start), WorldToScreenPosition(GameState, MovementLine.End)};
                            PushDebugLine(MovementScreenLine, (Color){255,255,255,255});
                        }
#endif

                        circle Circle0 = CircleAndLine.Circles[0];
                        circle Circle1 = CircleAndLine.Circles[1];

                        Vector2 CirclePosition0 = V2(Circle0.X, Circle0.Y);
                        Vector2 CirclePosition1 = V2(Circle1.X, Circle1.Y);

                        line Line0 = CircleAndLine.Lines[0];
                        line Line1 = CircleAndLine.Lines[1];

                        Vector2 CircleDelta0 = SubtractV2(MovementLine.End, CirclePosition0);
                        Vector2 CircleDelta1 = SubtractV2(MovementLine.End, CirclePosition1);

                        f32 CircleDistance0 = LengthV2(CircleDelta0);
                        f32 CircleDistance1 = LengthV2(CircleDelta1);

                        f32 InsideCircle0 = CircleDistance0 - Circle0.R;
                        f32 InsideCircle1 = CircleDistance1 - Circle1.R;

                        f32 WhichSide0 = GetWhichSideOfALine(MovementLine.End, Line0);
                        f32 WhichSide1 = GetWhichSideOfALine(MovementLine.End, Line1);
                        f32 WhichSide2 = GetWhichSideOfALine(MovementLine.End, (line){Line0.Start, Line1.Start});
                        f32 WhichSide3 = GetWhichSideOfALine(MovementLine.End, (line){Line0.End, Line1.End});

                        if (((WhichSide0 < 0.0f && WhichSide1 > 0.0f) || (WhichSide0 > 0.0f && WhichSide1 < 0.0f)) &&
                            ((WhichSide2 < 0.0f && WhichSide3 > 0.0f) || (WhichSide2 > 0.0f && WhichSide3 < 0.0f)))
                        {
                            Vector2 Projection0 = ProjectV2(Line0.Start, MovementLine.End, Line0.End);
                            Vector2 Projection1 = ProjectV2(Line1.Start, MovementLine.End, Line1.End);

                            f32 ProjectionDistance0 = LengthSquaredV2(SubtractV2(MovementLine.End, Projection0));
                            f32 ProjectionDistance1 = LengthSquaredV2(SubtractV2(MovementLine.End, Projection1));

                            Vector2 Projection;

                            if (ProjectionDistance0 < ProjectionDistance1)
                            {
                                Projection = Projection0;
                            }
                            else
                            {
                                Projection = Projection1;
                            }

                            Entity->Position = Projection;
                            Entity->Velocity = V2(0.0f, 0.0f);
                            Entity->Acceleration = V2(0.0f, 0.0f);

#if DEBUG_DRAW_COLLISIONS
                            {
                                Vector2 ScreenProjection0 = WorldToScreenPosition(GameState, Projection0);
                                Vector2 ScreenProjection1 = WorldToScreenPosition(GameState, Projection1);

                                line ScreenLine0 = (line){WorldToScreenPosition(GameState, Line0.Start), WorldToScreenPosition(GameState, Line0.End)};
                                line ScreenLine1 = (line){WorldToScreenPosition(GameState, Line1.Start), WorldToScreenPosition(GameState, Line1.End)};

                                PushDebugCircle((circle){ScreenProjection0.x-1, ScreenProjection0.y-1, 2}, (Color){255,255,255,255});
                                PushDebugCircle((circle){ScreenProjection1.x-1, ScreenProjection1.y-1, 2}, (Color){255,255,255,255});

                                PushDebugLine(ScreenLine0, (Color){255,255,255,255});
                                PushDebugLine(ScreenLine1, (Color){255,255,255,255});
                            }
#endif
                        }
                        else if (InsideCircle0 < 0.0f)
                        {
                            Vector2 CircleDeltaNormal = NormalizeV2(CircleDelta0);
                            Vector2 CirclePositionOffset = MultiplyV2S(CircleDeltaNormal, Circle0.R);

                            Entity->Position = AddV2(CirclePosition0, CirclePositionOffset);
                            Entity->Velocity = V2(0.0f, 0.0f);
                            Entity->Acceleration = V2(0.0f, 0.0f);

#if DEBUG_DRAW_COLLISIONS
                            {
                                Vector2 EntityScreenPosition = WorldToScreenPosition(GameState, Entity->Position);
                                Vector2 CirclePosition = WorldToScreenPosition(GameState, V2(Circle0.X, Circle0.Y));
                                PushDebugCircle((circle){CirclePosition.x, CirclePosition.y, Circle0.R}, (Color){255,255,255,255});
                                PushDebugCircle((circle){EntityScreenPosition.x-1, EntityScreenPosition.y-1, 2}, (Color){255,255,255,255});
                            }
#endif
                        }
                        else if (InsideCircle1 < 0.0f)
                        {
                            Vector2 CircleDeltaNormal = NormalizeV2(CircleDelta1);
                            Vector2 CirclePositionOffset = MultiplyV2S(CircleDeltaNormal, Circle1.R);

                            Entity->Position = AddV2(CirclePosition1, CirclePositionOffset);
                            Entity->Velocity = V2(0.0f, 0.0f);
                            Entity->Acceleration = V2(0.0f, 0.0f);

#if DEBUG_DRAW_COLLISIONS
                            {
                                Vector2 EntityScreenPosition = WorldToScreenPosition(GameState, Entity->Position);
                                Vector2 CirclePosition = WorldToScreenPosition(GameState, V2(Circle1.X, Circle1.Y));
                                PushDebugCircle((circle){CirclePosition.x, CirclePosition.y, Circle1.R}, (Color){255,255,255,255});
                                PushDebugCircle((circle){EntityScreenPosition.x-1, EntityScreenPosition.y-1, 2}, (Color){255,255,255,255});
                            }
#endif
                        }
                    }
                    else if (Area->Type == collision_type_Line && TestArea->Type == collision_type_Circle)
                    {
                        /* TODO: Handle line-circle collision */
                    }
                    else
                    {
                        Assert(0);
                    }
                }
            }
        }

        UpdateEntity(GameState, Entity);
    }
    ryn_END_TIMED_BLOCK(TB_UpdateEntities);
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

        /* entity *EelEntity = AddEntity(GameState, sprite_type_Eel); */
        /* EelEntity->Position = MultiplyV2S(V2(0.25f, 4.0f), TILE_SIZE * TEXTURE_MAP_SCALE); */

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

                    entity *Entity = AddEntity(GameState, sprite_type_Wall);
                    Entity->Position = V2(Scale * I, Scale * J);
                } break;
                default:
                    break;
                }
            }
        }
    }

    GenerateSortedEntityTable(GameState);

    GameState->CameraPosition = GameState->PlayerEntity->Position;
    GameState->Mode = game_mode_Start;
    GameState->LastTime = GetTime();
}

internal void DrawDebugProfile(game_state *GameState)
{
#if ryn_PROFILER
    uint64_t TotalElapsedTime = ryn_GlobalProfiler.EndTime - ryn_GlobalProfiler.StartTime;
    s32 TextLineY = 0;
    s32 FontSize = 18;
    s32 LineHeight = 22;
    s32 Spacing = 1;

    if(CPUFreq)
    {
        float TotalElapsedTimeInMs = 1000.0 * (double)TotalElapsedTime / (double)CPUFreq;

        sprintf(DebugTextBuffer, "Total time: %0.4fms", TotalElapsedTimeInMs);
        DrawTextEx(GameState->UI.Font, DebugTextBuffer, V2(10, TextLineY), FontSize, Spacing, (Color){255,255,255,255});
    }

    for(uint32_t TimerIndex = 0; TimerIndex < ArrayCount(ryn_GlobalProfiler.Timers); ++TimerIndex)
    {
        ryn_timer_data *Timer = ryn_GlobalProfiler.Timers + TimerIndex;

        if(Timer->ElapsedInclusive)
        {
            double Percent = 100.0 * ((double)Timer->ElapsedExclusive / (double)TotalElapsedTime);

            if(Timer->ElapsedInclusive != Timer->ElapsedExclusive)
            {
                double PercentWithChildren = 100.0 * ((double)Timer->ElapsedInclusive / (double)TotalElapsedTime);

                sprintf(DebugTextBuffer, "    %s[%llu]: %llu (%.2f%%), %.2f%% w/children", Timer->Label, Timer->HitCount, Timer->ElapsedExclusive, Percent, PercentWithChildren);
                DrawTextEx(GameState->UI.Font, DebugTextBuffer, V2(10, TextLineY += LineHeight), FontSize, Spacing, (Color){255,255,255,255});
            }
            else
            {
                sprintf(DebugTextBuffer, "    %s[%llu]: %llu (%.2f%%)", Timer->Label, Timer->HitCount, Timer->ElapsedExclusive, Percent);
                DrawTextEx(GameState->UI.Font, DebugTextBuffer, V2(10, TextLineY += LineHeight), FontSize, Spacing, (Color){255,255,255,255});
            }
        }
    }
#endif
}

internal void ResetProfilerTimers(void)
{
#if ryn_PROFILER
    for (s32 I = 0; I < TB_TimedBlockCount && I + 1 < ryn_MAX_TIMERS; ++I)
    {
        /* NOTE: ryn_prof reserves the use of the 0th timer, so we add one to I. */
        ryn_GlobalProfiler.Timers[I+1].ElapsedExclusive = 0;
        ryn_GlobalProfiler.Timers[I+1].ElapsedInclusive = 0;
        ryn_GlobalProfiler.Timers[I+1].HitCount = 0;
        ryn_GlobalProfiler.Timers[I+1].ProcessedByteCount = 0;
    }
#endif
}

internal void UpdateAndRender(void *VoidGameState)
{
#if ryn_PROFILER
    ryn_BeginProfile();
#endif
    ryn_BEGIN_TIMED_BLOCK(TB_UpdateAndRender);
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
    } break;
    case game_mode_Play:
    {
        if (!IsTextureReady(GameState->ScubaTexture))
        {
            break;
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
            ryn_BEGIN_TIMED_BLOCK(TB_DrawEntities);
            for (s32 I = 0; I < GameState->EntityCount; ++I)
            {
                s32 EntityIndex = GameState->SortedEntityTable[I];
                Assert(EntityIndex >= 0 && EntityIndex < MAX_ENTITY_COUNT);
                entity *Entity = GameState->Entities + EntityIndex;
                DrawSprite(GameState, Entity);
            }
            ryn_END_TIMED_BLOCK(TB_DrawEntities);
        }

        { /* debug graphics */
            ryn_BEGIN_TIMED_BLOCK(TB_DebugGraphics);
            RenderDebugDrawCommands();
            DrawFrameRateHistory();
            ryn_END_TIMED_BLOCK(TB_DebugGraphics);
        }
    } break;
    default: break;
    }

    ryn_END_TIMED_BLOCK(TB_UpdateAndRender);
#if ryn_PROFILER
    ryn_EndProfile();
#endif

    { /* draw profiler stats */
        DrawDebugProfile(GameState);
        ResetProfilerTimers();
    }

    EndDrawing();

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

#if ryn_PROFILER
    CPUFreq = ryn_EstimateCpuFrequency();
#endif

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "SCUBA");

    s32 FontSize = 32;
    s32 *Chars = 0;
    s32 GlyphCount = 0;
    Font LoadedFont = LoadFontFromMemory(".ttf", RobotoRegularData, ArrayCount(RobotoRegularData), FontSize, Chars, GlyphCount);

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
