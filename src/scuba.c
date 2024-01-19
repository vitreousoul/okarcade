#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

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
    b32 Collides;
    b32 Bounded; /* NOTE: Denotes if the collision is a bound within the shapes being collided. */
    s32 Count;
    Vector2 Collisions[4];
    f32 TimeTaken;
    Vector2 Normal;
} collision_result;

typedef enum
{
    collision_type_Circle = 0x1,
    collision_type_Line = 0x2,
    collision_type_Rectangle = 0x4,
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
        Rectangle Rectangle;
    };
    /* struct collision_area *Next; */ /* TODO: reinstate multiple areas for entities like eels */
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
} entity_movement_type; /* TODO: maybe these should be flags? */

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
    /* TODO: Use eithers lines or rectangles and delete the other. */
    line Lines[4];
    Rectangle Rectangles[2];
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

#define ENTITY_IS_PLAYER(e) ((e)->Sprites[0].Type == sprite_type_Fish)

typedef enum
{
    game_mode_Start,
    game_mode_Play,
    game_mode_GameOver,
    game_mode_Quit,
} game_mode;

typedef struct
{
    b32 KeyboardEnter;
} user_input;

typedef struct
{
    f32 DeltaTime;
    f32 LastTime;

    game_mode Mode;
    ui UI;
    user_input Input;

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

internal void PushDebugRectangle(Rectangle R, Color Color)
{
    debug_draw_command Command;

    Command.Type = debug_draw_type_Rectangle;
    Command.Shape.Rectangle = R;
    Command.Color = Color;

    PushDebugDrawCommand(Command);
}

internal Vector2 WorldToScreenPosition(game_state *GameState, Vector2 P)
{
    Vector2 HalfScreen = MultiplyV2S(V2(SCREEN_WIDTH, SCREEN_HEIGHT), 0.5f);
    Vector2 Result = AddV2(SubtractV2(P, GameState->CameraPosition), HalfScreen);

    return Result;
}

internal line WorldToScreenLine(game_state *GameState, line Line)
{
    line Result = (line){WorldToScreenPosition(GameState, Line.Start), WorldToScreenPosition(GameState, Line.End)};
    return Result;
}

internal circle WorldToScreenCircle(game_state *GameState, circle Circle)
{
    Vector2 CirclePosition = WorldToScreenPosition(GameState, V2(Circle.X, Circle.Y));
    circle Result = (circle){CirclePosition.x, CirclePosition.y, Circle.R};
    return Result;
}

internal Rectangle WorldToScreenRectangle(game_state *GameState, Rectangle R)
{
    Vector2 P = WorldToScreenPosition(GameState, V2(R.x, R.y));
    Rectangle Result = (Rectangle){P.x, P.y, R.width, R.height};
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

            DrawRectangleLinesEx(Rectangle, 1.0f, Command.Color);
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
            Entity->CollisionArea = AddCollisionArea(GameState);
            collision_area *Area = Entity->CollisionArea;
            Area->Type = collision_type_Rectangle;
            Area->Rectangle = R2(-TileHalfScale, -TileHalfScale, TileScale, TileScale);
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
            Entity->MovementType = entity_movement_type_None;
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

    GameState->Input.KeyboardEnter = IsKeyDown(KEY_ENTER);

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

internal inline Rectangle GetOffsetRectangle(Rectangle R, Vector2 Offset)
{
    Rectangle Result = (Rectangle){R.x + Offset.x, R.y + Offset.y, R.width, R.height};
    return Result;
}

internal minkowski_circle_and_rectangle MinkowskiSumCircleAndRectangle(circle Circle, Rectangle Rectangle)
{
    minkowski_circle_and_rectangle Shape;

    f32 CR = Circle.R;
    f32 CD = 2 * Circle.R;

    f32 RX = Rectangle.x;
    f32 RY = Rectangle.y;
    f32 RW = Rectangle.width;
    f32 RH = Rectangle.height;

    Shape.Circles[0] = (circle){RX,      RY,      CR};
    Shape.Circles[1] = (circle){RX + RW, RY,      CR};
    Shape.Circles[2] = (circle){RX + RW, RY + RH, CR};
    Shape.Circles[3] = (circle){RX,      RY + RH, CR};

    Shape.Lines[0] = (line){RX - CR,      RY,           RX - CR,      RY + RH};
    Shape.Lines[1] = (line){RX + RW + CR, RY,           RX + RW + CR, RY + RH};
    Shape.Lines[2] = (line){RX,           RY - CR,      RX + RW,      RY - CR};
    Shape.Lines[2] = (line){RX,           RY + RH + CR, RX + RW,      RY + RH + CR};

    Shape.Rectangles[0] = R2(RX - CR, RY,      RW + CD, RH);
    Shape.Rectangles[1] = R2(RX,      RY - CR, RW,      RH + CD);

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

        Result.Collides = 1;
        Result.Count = 1;
        Result.Collisions[0] = V2(X, Y);

        if (CollisionIsInFirst && CollisionIsInSecond)
        {
            Result.Bounded = 1;
        }
    }

    return Result;
}

internal collision_result CollideLineAndCircle(line Line, circle Circle)
{
    collision_result Result = {0};

    Vector2 CirclePosition = (Vector2){Circle.X, Circle.Y};

    f32 CircleDistanceStart = LengthV2(SubtractV2(Line.Start, CirclePosition));
    f32 CircleDistanceEnd = LengthV2(SubtractV2(Line.End, CirclePosition));

    f32 IsInsideCircle = CircleDistanceStart < Circle.R && CircleDistanceEnd < Circle.R;

    Result.Normal = NormalizeV2(RotateV2(SubtractV2(Line.End, Line.Start), PI_OVER_2));

    if (IsInsideCircle)
    {
        /* TODO: Project line start/end onto the circle for collision result */
        /* Vector2 DirectionStart = MultiplyV2S(NormalizeV2(SubtractV2(Line.Start, CirclePosition)), Circle.R); */
        Vector2 DirectionEnd = MultiplyV2S(NormalizeV2(SubtractV2(Line.End, CirclePosition)), Circle.R);

        /* Vector2 CollisionPoint0 = AddV2(DirectionStart, CirclePosition); */
        Vector2 CollisionPoint = AddV2(DirectionEnd, CirclePosition);

        Result.Collides = 1;
        Result.Count = 1;
        Result.Collisions[0] = CollisionPoint;
        Result.Bounded = 1;
        /* Result.Collisions[1] = CollisionPoint1; */
    }
    else
    {
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
                Result.Count = 1;
                Result.Bounded = 1;
            }
        }
    }

    return Result;
}

internal b32 IsPointInsideRectangle(Vector2 Point, Rectangle Rectangle)
{
    b32 OutsideTheLeft = Point.x < Rectangle.x;
    b32 OutsideTheRight = Point.x > Rectangle.x + Rectangle.width;
    b32 OutsideTheTop = Point.y < Rectangle.y;
    b32 OutsideTheBottom = Point.y > Rectangle.y + Rectangle.height;

    b32 IsInside = !(OutsideTheLeft || OutsideTheRight || OutsideTheTop || OutsideTheBottom);

    return IsInside;
}

internal b32 IsPointInsideCircle(Vector2 Point, circle Circle)
{
    f32 Distance = LengthV2(SubtractV2(Point, V2(Circle.X, Circle.Y)));

    b32 IsInside = Distance < Circle.R;

    return IsInside;
}

internal collision_result CollideLineAndRectangle(line Line, Rectangle R)
{
    collision_result Result = {0};

    Vector2 Point0 = V2(R.x,           R.y);
    Vector2 Point1 = V2(R.x + R.width, R.y);
    Vector2 Point2 = V2(R.x + R.width, R.y + R.height);
    Vector2 Point3 = V2(R.x,           R.y + R.height);

    line Lines[4] = {
        (line){Point0, Point1},
        (line){Point1, Point2},
        (line){Point2, Point3},
        (line){Point3, Point0}
    };

    Vector2 Normals[4] = {
        NormalizeV2(RotateV2(SubtractV2(Lines[0].End, Lines[0].Start), PI_OVER_2)),
        NormalizeV2(RotateV2(SubtractV2(Lines[1].Start, Lines[1].End), PI_OVER_2)),
        NormalizeV2(RotateV2(SubtractV2(Lines[2].End, Lines[2].Start), PI_OVER_2)),
        NormalizeV2(RotateV2(SubtractV2(Lines[3].Start, Lines[3].End), PI_OVER_2)),
    };

    f32 MinimumDistance = 99999999999.0f;

    for (s32 I = 0; I < 4; ++I)
    {
        collision_result Collision = CollideLineAndLine(Line, Lines[I]);

        if (Collision.Count)
        {
            f32 Distance = LengthSquaredV2(SubtractV2(Collision.Collisions[0], Line.Start));

            if (Collision.Bounded && Distance < MinimumDistance)
            {
                MinimumDistance = Distance;
                Result = Collision;
                Result.Normal = Normals[I];
            }
        }
    }

    return Result;
}

internal collision_result GetCollisionPointForRectangle(Vector2 P, Rectangle R)
{
    collision_result Result = {0};

    Vector2 Center = V2(R.x + R.width / 2.0f, R.y + R.height / 2.0f);
    f32 ConservativeSize = R.width + R.height;
    Vector2 Direction = MultiplyV2S(NormalizeV2(SubtractV2(P, Center)), ConservativeSize);
    Vector2 ExtendedPoint = AddV2(P, Direction);
    line Line = (line){Center, ExtendedPoint};

    Vector2 Point0 = V2(R.x,           R.y);
    Vector2 Point1 = V2(R.x + R.width, R.y);
    Vector2 Point2 = V2(R.x + R.width, R.y + R.height);
    Vector2 Point3 = V2(R.x,           R.y + R.height);

    line Lines[4] = {
        (line){Point0, Point1},
        (line){Point1, Point2},
        (line){Point2, Point3},
        (line){Point3, Point0}
    };

    Vector2 Normals[4] = {
        NormalizeV2(RotateV2(SubtractV2(Lines[0].Start, Lines[0].End), PI_OVER_2)),
        NormalizeV2(RotateV2(SubtractV2(Lines[1].Start, Lines[1].End), PI_OVER_2)),
        NormalizeV2(RotateV2(SubtractV2(Lines[2].Start, Lines[2].End), PI_OVER_2)),
        NormalizeV2(RotateV2(SubtractV2(Lines[3].Start, Lines[3].End), PI_OVER_2)),
    };

    f32 MinimumDistance = 99999999999.0f;

    for (s32 I = 0; I < 4; ++I)
    {
        collision_result Collision = CollideLineAndLine(Line, Lines[I]);

        if (Collision.Count)
        {
            f32 Distance = LengthSquaredV2(SubtractV2(Collision.Collisions[0], Line.End));

            if (Collision.Bounded && Distance < MinimumDistance)
            {
                MinimumDistance = Distance;
                Result = Collision;
                Result.Normal = Normals[I];
            }
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

internal Rectangle UnionRectangles(Rectangle RectangleA, Rectangle RectangleB)
{
    f32 MinX = MinF32(RectangleA.x, RectangleB.x);
    f32 MinY = MinF32(RectangleA.y, RectangleB.y);

    f32 MaxX = MaxF32(RectangleA.x + RectangleA.width, RectangleB.x + RectangleB.width);
    f32 MaxY = MaxF32(RectangleA.y + RectangleA.width, RectangleB.y + RectangleB.width);

    Rectangle Result = (Rectangle){MinX, MinY, MaxX - MinX, MaxY - MinY};

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

internal entity_movement GetEntityMovement(entity *Entity, f32 DeltaTime)
{
    entity_movement Movement = {0};

    f32 AccelerationThreshold = 0.0001f;
    f32 VelocityThreshold = 1.0f;

    b32 HasSufficientVelocity = LengthSquaredV2(Entity->Velocity) > VelocityThreshold;
    b32 HasSufficientAcceleration = LengthSquaredV2(Entity->Acceleration) > AccelerationThreshold;

    if (HasSufficientVelocity || HasSufficientAcceleration)
    {
        f32 AccelerationScale = 1000.0f;
        f32 DT = DeltaTime;

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

internal void UpdateEntity(game_state *GameState, entity *Entity, f32 DeltaTime)
{
    ryn_BEGIN_TIMED_BLOCK(TB_UpdateEntity);
    entity_movement Movement = GetEntityMovement(Entity, DeltaTime);

    f32 Threshold = 0.8f;

    if(LengthSquaredV2(Movement.Position) < Threshold)
    {
        Movement.Position = V2(0.0f, 0.0f);
    }

    if(LengthSquaredV2(Movement.Velocity) < Threshold)
    {
        Movement.Velocity = V2(0.0f, 0.0f);
    }

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

internal collision_result CollideEntities(game_state *GameState, entity *Entity, entity *TestEntity, f32 DeltaTime, b32 ShouldUpdateEntities)
{
    collision_result Result = {0};
    f32 NearestCollisionDistance = 999999999999.0f;

    Color DebugColor = (Color){255,255,0,255};

    collision_area *Area = Entity->CollisionArea;
    collision_area *TestArea = TestEntity->CollisionArea;

    entity_movement EntityMovement = GetEntityMovement(Entity, DeltaTime);
    entity_movement TestEntityMovement = GetEntityMovement(TestEntity, DeltaTime);

    Vector2 EntityEndPosition = AddV2(Entity->Position, EntityMovement.Position);

    line MovementLine = (line){Entity->Position, EntityEndPosition};

    if (Area->Type == collision_type_Circle && TestArea->Type == collision_type_Rectangle)
    {
        /* NOTE: Assert circles are moveable and rectangles are _not_ moveable, just to make things simpler for now. */
        Assert(Entity->MovementType == entity_movement_type_Moveable && TestEntity->MovementType != entity_movement_type_Moveable);
        minkowski_circle_and_rectangle MinkowskiRectangle = MinkowskiSumCircleAndRectangle(Area->Circle, TestArea->Rectangle);

        Rectangle R0 = GetOffsetRectangle(MinkowskiRectangle.Rectangles[0], TestEntity->Position);
        Rectangle R1 = GetOffsetRectangle(MinkowskiRectangle.Rectangles[1], TestEntity->Position);

        circle C0 = GetOffsetCircle(MinkowskiRectangle.Circles[0], TestEntity->Position);
        circle C1 = GetOffsetCircle(MinkowskiRectangle.Circles[1], TestEntity->Position);
        circle C2 = GetOffsetCircle(MinkowskiRectangle.Circles[2], TestEntity->Position);
        circle C3 = GetOffsetCircle(MinkowskiRectangle.Circles[3], TestEntity->Position);

        collision_result Collision = {0};

        b32 EntityPositionInRectangle0 = IsPointInsideRectangle(Entity->Position, R0);
        b32 EntityEndPositionInRectangle0 = IsPointInsideRectangle(EntityEndPosition, R0);

        b32 EntityPositionInRectangle1 = IsPointInsideRectangle(Entity->Position, R1);
        b32 EntityEndPositionInRectangle1 = IsPointInsideRectangle(EntityEndPosition, R1);

        b32 EntityInCircle0 = IsPointInsideCircle(Entity->Position, C0);
        b32 EntityEndInCircle0 = IsPointInsideCircle(EntityEndPosition, C0);

        b32 EntityInCircle1 = IsPointInsideCircle(Entity->Position, C1);
        b32 EntityEndInCircle1 = IsPointInsideCircle(EntityEndPosition, C1);

        b32 EntityInCircle2 = IsPointInsideCircle(Entity->Position, C2);
        b32 EntityEndInCircle2 = IsPointInsideCircle(EntityEndPosition, C2);

        b32 EntityInCircle3 = IsPointInsideCircle(Entity->Position, C3);
        b32 EntityEndInCircle3 = IsPointInsideCircle(EntityEndPosition, C3);

        /* b32 CollideR0 = EntityPositionInRectangle0 || EntityEndPositionInRectangle0; */
        /* b32 CollideR1 = EntityPositionInRectangle1 || EntityEndPositionInRectangle1; */
        b32 CollideR0 = !EntityPositionInRectangle0 && EntityEndPositionInRectangle0;
        b32 CollideR1 = !EntityPositionInRectangle1 && EntityEndPositionInRectangle1;

        /* b32 CollideC0 = EntityInCircle0 || EntityEndInCircle0; */
        /* b32 CollideC1 = EntityInCircle1 || EntityEndInCircle1; */
        /* b32 CollideC2 = EntityInCircle2 || EntityEndInCircle2; */
        /* b32 CollideC3 = EntityInCircle3 || EntityEndInCircle3; */
        b32 CollideC0 = !EntityInCircle0 && EntityEndInCircle0;
        b32 CollideC1 = !EntityInCircle1 && EntityEndInCircle1;
        b32 CollideC2 = !EntityInCircle2 && EntityEndInCircle2;
        b32 CollideC3 = !EntityInCircle3 && EntityEndInCircle3;

        if (CollideR0)
        {
            Collision = GetCollisionPointForRectangle(EntityEndPosition, R0);
#if DEBUG_DRAW_COLLISIONS
            PushDebugRectangle(WorldToScreenRectangle(GameState, R0), (Color){255,0,255,255});
#endif
        }
        else if (CollideR1)
        {
            Collision = GetCollisionPointForRectangle(EntityEndPosition, R1);
#if DEBUG_DRAW_COLLISIONS
            PushDebugRectangle(WorldToScreenRectangle(GameState, R1), (Color){255,0,255,255});
#endif
        }
        else if (0&&CollideC0)
        {
            Collision = CollideLineAndCircle(MovementLine, C0);
        }
        else if (0&&CollideC1)
        {
            Collision = CollideLineAndCircle(MovementLine, C1);
        }
        else if (0&&CollideC2)
        {
            Collision = CollideLineAndCircle(MovementLine, C2);
        }
        else if (0&&CollideC3)
        {
            Collision = CollideLineAndCircle(MovementLine, C3);
        }

        if (Entity->Sprites[0].Type == sprite_type_Fish)
        {
        }

        if (Collision.Count && Collision.Bounded)
        {
            Result = Collision;

            if (ShouldUpdateEntities)
            {
                Assert(LengthV2(Collision.Normal) > 0.00001f);
                Vector2 OffsetNormal = AddV2(Collision.Normal, EntityEndPosition);
                Vector2 Projection = ProjectV2(EntityEndPosition, Entity->Position, OffsetNormal);
                Vector2 Direction = SubtractV2(Projection, Entity->Position);
                Vector2 NormalizedDirection = NormalizeV2(Direction);
                f32 DirectionLength = LengthV2(Direction);

                Vector2 CollisionToOldPosition = SubtractV2(Entity->Position, Collision.Collisions[0]);
                Vector2 FudgeFactor = MultiplyV2S(NormalizeV2(CollisionToOldPosition), 1.0f);

                f32 VelocityMagnitude = LengthV2(Entity->Velocity);
                f32 AccelerationMagnitude = LengthV2(Entity->Acceleration);

                f32 MovementDistance = LengthV2(SubtractV2(EntityEndPosition, Entity->Position));
                f32 CollisionDistance = LengthV2(SubtractV2(Collision.Collisions[0], Entity->Position));
                f32 PercentOfTimeTaken = ClampF32(CollisionDistance / MovementDistance, 0, 1);
                Result.TimeTaken = DeltaTime * PercentOfTimeTaken;

                if (ENTITY_IS_PLAYER(Entity))
                {
                    if (DirectionLength > 0.01f)
                    {
                        Entity->Position = AddV2(Collision.Collisions[0], FudgeFactor);
                        Entity->Velocity = V2(0.0f, 0.0f);//MultiplyV2S(NormalizedDirection, VelocityMagnitude);
                        Entity->Acceleration = V2(0.0f, 0.0f);//MultiplyV2S(NormalizedDirection, AccelerationMagnitude);
                    }
                    else
                    {
                        Entity->Velocity = V2(0.0f, 0.0f);
                        Entity->Acceleration = V2(0.0f, 0.0f);
                    }
                }
            }
        }
        else
        {
            Result.TimeTaken = DeltaTime + 0.1f;
        }
    }
    else if (Area->Type == collision_type_Circle && TestArea->Type == collision_type_Circle)
    {

    }

    return Result;
}

internal void UpdateEntities(game_state *GameState)
{
    f32 RemainingTime = GameState->DeltaTime;
    s32 UpdateIterationCount = 0;

    while (RemainingTime > 0.0f && UpdateIterationCount < 8)
    {
        UpdateIterationCount += 1;
        f32 SoonestCollisionDistance = FLT_MAX;
        collision_result SoonestCollision = {0};
        s32 SoonestEntity = -1;
        s32 SoonestTestEntity = -1;
        s32 I, J;

        for (I = 0; I < GameState->EntityCount; ++I)
        {
            entity *Entity = GameState->Entities + I;
            b32 EntityIsMoveable = Entity->MovementType == entity_movement_type_Moveable;

            if (!Entity->Sprites[0].Type)
            {
                continue;
            }

            for (J = 1; J < GameState->EntityCount; ++J)
            {
                entity *TestEntity = GameState->Entities + J;
                b32 TestEntityIsMoveable = TestEntity->MovementType == entity_movement_type_Moveable;
                b32 NeitherEntityIsMoveable = !(EntityIsMoveable || TestEntityIsMoveable);

                if (NeitherEntityIsMoveable)
                {
                    continue;
                }

                entity_movement EntityMovement = GetEntityMovement(Entity, RemainingTime);
                entity_movement TestEntityMovement = GetEntityMovement(TestEntity, RemainingTime);

                Rectangle EntitySourceRectangle = GetSpriteRectangle(Entity);
                Vector2 RectanglePosition = V2(EntitySourceRectangle.x, EntitySourceRectangle.y);
                Rectangle RectangleStart = R2(RectanglePosition.x, RectanglePosition.y, EntitySourceRectangle.width, EntitySourceRectangle.height);
                RectanglePosition = AddV2(RectanglePosition, EntityMovement.Position);
                Rectangle RectangleEnd = R2(RectanglePosition.x, RectanglePosition.y, EntitySourceRectangle.width, EntitySourceRectangle.height);

                Rectangle TestEntitySourceRectangle = GetSpriteRectangle(TestEntity);
                Vector2 TestRectanglePosition = V2(TestEntitySourceRectangle.x, TestEntitySourceRectangle.y);
                Rectangle TestRectangleStart = R2(TestRectanglePosition.x, TestRectanglePosition.y, TestEntitySourceRectangle.width, TestEntitySourceRectangle.height);
                TestRectanglePosition = AddV2(TestRectanglePosition, TestEntityMovement.Position);
                Rectangle TestRectangleEnd = R2(TestRectanglePosition.x, TestRectanglePosition.y, TestEntitySourceRectangle.width, TestEntitySourceRectangle.height);

                b32 StartRectanglesCollide = CollideRectangles(RectangleStart, TestRectangleStart);
                b32 EndRectanglesCollide = CollideRectangles(RectangleEnd, TestRectangleEnd);
                b32 EntitiesAreNotTheSame = I != J;

                if ((StartRectanglesCollide || EndRectanglesCollide) && EntitiesAreNotTheSame)
                {
                    collision_result Collision = CollideEntities(GameState, Entity, TestEntity, RemainingTime, 0);

                    if (Collision.Count)
                    {
                        f32 CollisionDistance = LengthV2(SubtractV2(Entity->Position, Collision.Collisions[0]));

                        if (CollisionDistance < SoonestCollisionDistance)
                        {
                            SoonestCollisionDistance = CollisionDistance;
                            SoonestCollision = Collision;
                            SoonestEntity = I;
                            SoonestTestEntity = J;
                        }
                    }
                }
            }
        }

        f32 NewRemainingTime;

        if (SoonestCollision.Count)
        {
            entity *Entity = GameState->Entities + SoonestEntity;
            entity *TestEntity = GameState->Entities + SoonestTestEntity;

            collision_result Collision = CollideEntities(GameState, Entity, TestEntity, RemainingTime, 1);

            NewRemainingTime = RemainingTime - Collision.TimeTaken;
        }

        /* update colliding entities */
        for (I = 0; I < GameState->EntityCount; ++I)
        {
            entity *Entity = GameState->Entities + I;
            /* b32 IsACollisionEntity = SoonestEntity == I || SoonestTestEntity == I; */

            /* if (!(SoonestCollision.Count && IsACollisionEntity)) */
            {
                UpdateEntity(GameState, Entity, RemainingTime);
            }
        }

        if (NewRemainingTime > 0.0001f)
        {
            /* RemainingTime = -1.0f; */
            RemainingTime = NewRemainingTime;
        }
        else
        {
            RemainingTime = -1.0f;
        }
    }
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

        if (Pressed || GameState->Input.KeyboardEnter)
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
