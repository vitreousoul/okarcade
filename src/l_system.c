/*
    TODO: Detect if mouse position is unchanged during drag, if so, keep drawing so the tree fills out more as you drag.
*/

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
#define TARGET_FPS 30

#include "core.c"
#include "math.c"
#include "raylib_helpers.h"
#include "ui.c"

#define EXPANSION_BUFFER_SIZE (1 << 12)
#define EXPANSION_MAX_DEPTH 16
#define RULE_SIZE_MAX 16
#define TURTLE_STACK_MAX 16


global_variable Color BackgroundColor = (Color){58, 141, 230, 255};

typedef enum
{
    symbol_Undefined,
    symbol_Root,
    symbol_A,
    symbol_B,
    symbol_Push,
    symbol_Pop,
    symbol_Count,
} symbol;

typedef struct
{
    symbol Symbol;
    s32 Index;
} expansion_item;

typedef struct
{
    s32 Index;
    expansion_item Items[EXPANSION_MAX_DEPTH];
} expansion;

typedef struct
{
    Vector2 Position;
    Vector2 Heading;
} turtle;

typedef enum
{
    button_kind_Undefined,
    button_kind_Expansion,
    button_kind_Movement,
    button_kind_Count,
} button_kind;

u8 *ButtonText[button_kind_Count] = {
    [button_kind_Expansion] = (u8 *)"Expansion",
    [button_kind_Movement] = (u8 *)"Movement",
};

typedef struct
{
    turtle Turtle;
    Image Canvas;

    f32 RotationAmount;
    s32 LineDrawsPerFrame;

    symbol Rules[symbol_Count][RULE_SIZE_MAX];
    Texture2D FrameBuffer;

    ui UI;
    button Buttons[button_kind_Count];
    slider Slider;
    ui_element Tablet;

    expansion Expansion;
    turtle TurtleStack[TURTLE_STACK_MAX];
    s32 TurtleStackIndex;

} app_state;


internal expansion_item CreateExpansionItem(symbol Symbol, s32 Index)
{
    expansion_item Item;

    Item.Symbol = Symbol;
    Item.Index = Index;

    return Item;
}

internal void PushExpansionItem(expansion *Expansion, expansion_item Item)
{
    if (Expansion->Index >= 0 && Expansion->Index < EXPANSION_MAX_DEPTH)
    {
        Expansion->Items[Expansion->Index] = Item;
        Expansion->Index += 1;
    }
}

internal expansion_item PopExpansionItem(expansion *Expansion)
{
    if (Expansion->Index < 0)
    {
        return CreateExpansionItem(symbol_Undefined, -1);
    }
    else
    {
        expansion_item Item;

        Item.Symbol = Expansion->Items[Expansion->Index].Symbol;
        Item.Index = Expansion->Items[Expansion->Index].Index;
        Expansion->Index -= 1;

        return Item;
    }
}

internal expansion_item PeekExpansionItem(expansion *Expansion)
{
    s32 PeekIndex = Expansion->Index - 1;

    if (PeekIndex < 0 || PeekIndex >= EXPANSION_MAX_DEPTH)
    {
        return CreateExpansionItem(symbol_Undefined, -1);
    }
    else
    {
        expansion_item Item;

        Item.Symbol = Expansion->Items[PeekIndex].Symbol;
        Item.Index = Expansion->Items[PeekIndex].Index;

        return Item;
    }
}

internal symbol GetSymbolFromRules(symbol Rules[symbol_Count][RULE_SIZE_MAX], expansion_item Item)
{
    b32 RuleIndexInBounds = Item.Index >= 0 && Item.Index < RULE_SIZE_MAX;

    if (RuleIndexInBounds)
    {
        return Rules[Item.Symbol][Item.Index];
    }
    else
    {
        return symbol_Undefined;
    }
}

internal void PopAndIncrementParent(expansion *Expansion)
{
    PopExpansionItem(Expansion);
    s32 ParentIndex = Expansion->Index - 1;

    if (ParentIndex >= 0 && ParentIndex < EXPANSION_MAX_DEPTH)
    {
        Expansion->Items[ParentIndex].Index += 1;
    }
}

internal void CopyTurtle(turtle *SourceTurtle, turtle *DestinationTurtle)
{
    DestinationTurtle->Position.x = SourceTurtle->Position.x;
    DestinationTurtle->Position.y = SourceTurtle->Position.y;

    DestinationTurtle->Heading.x = SourceTurtle->Heading.x;
    DestinationTurtle->Heading.y = SourceTurtle->Heading.y;
}

internal void DrawLSystem(app_state *AppState, s32 Depth)
{
    if (Depth >= EXPANSION_MAX_DEPTH)
    {
        s32 MaxDepthMinusOne = EXPANSION_MAX_DEPTH - 1;
        printf("Max depth for DrawLSystem is %d but was passed a depth of %d\n", MaxDepthMinusOne, Depth);
        LogError("DepthError\n");
        return;
    }

    expansion *Expansion = &AppState->Expansion;
    turtle *TurtleStack = AppState->TurtleStack;
    s32 *TurtleStackIndex = &AppState->TurtleStackIndex;
    f32 TurtleSpeed = 1.0f;

    s32 LineDrawCount = 0;

    while (Expansion->Index > 0 && LineDrawCount < AppState->LineDrawsPerFrame)
    {
        expansion_item CurrentItem = PeekExpansionItem(Expansion);
        b32 RuleIndexInBounds = CurrentItem.Index >= 0 && CurrentItem.Index < RULE_SIZE_MAX;

        if (!RuleIndexInBounds)
        {
            LogError("Rule index out of bounds\n");
            break;
        }

        symbol ChildSymbol = AppState->Rules[CurrentItem.Symbol][CurrentItem.Index];

        if (ChildSymbol == symbol_Undefined)
        {
            PopAndIncrementParent(Expansion);
        }
        else if (Expansion->Index <= Depth)
        {
            expansion_item ChildItem = CreateExpansionItem(ChildSymbol, 0);
            PushExpansionItem(Expansion, ChildItem);
        }
        else
        {
            /* output symbols */
            symbol OutputSymbol = GetSymbolFromRules(AppState->Rules, CurrentItem);

            while (OutputSymbol != symbol_Undefined)
            {
                turtle *Turtle = &TurtleStack[*TurtleStackIndex];
                b32 ShouldDraw = 0;

                switch(OutputSymbol)
                {
                case symbol_A:
                {
                    ShouldDraw = 1;
                } break;
                case symbol_B:
                {
                    ShouldDraw = 1;
                } break;
                case symbol_Push:
                {
                    if (*TurtleStackIndex < TURTLE_STACK_MAX - 1)
                    {
                        f32 RotationAmount = AppState->RotationAmount;
                        Vector2 NewHeading = RotateV2(Turtle->Heading, RotationAmount);
                        *TurtleStackIndex += 1;
                        CopyTurtle(Turtle, &TurtleStack[*TurtleStackIndex]);
                        TurtleStack[*TurtleStackIndex].Heading = NewHeading;
                    }
                } break;
                case symbol_Pop:
                {
                    if (*TurtleStackIndex > 0)
                    {
                        f32 RotationAmount = -AppState->RotationAmount;
                        *TurtleStackIndex -= 1;
                        Vector2 NewHeading = RotateV2(TurtleStack[*TurtleStackIndex].Heading, RotationAmount);
                        TurtleStack[*TurtleStackIndex].Heading = NewHeading;
                    }
                } break;
                default: break;
                }

                if (ShouldDraw)
                {
                    turtle *Turtle = &TurtleStack[*TurtleStackIndex];
                    f32 NewX = Turtle->Position.x + (TurtleSpeed * Turtle->Heading.x);
                    f32 NewY = Turtle->Position.y + (TurtleSpeed * Turtle->Heading.y);
                    Color LineColor = (Color){0,0,0,255};

                    ImageDrawLine(&AppState->Canvas, Turtle->Position.x, Turtle->Position.y, NewX, NewY, LineColor);
                    LineDrawCount += 1;

                    Turtle->Position.x = NewX;
                    Turtle->Position.y = NewY;
                }

                CurrentItem.Index += 1;
                OutputSymbol = GetSymbolFromRules(AppState->Rules, CurrentItem);
            }

            PopAndIncrementParent(Expansion);
        }
    }
}

internal void UpdateUI(app_state *AppState)
{
    AppState->UI.Hot = -1;
    AppState->UI.MouseButtonPressed = IsMouseButtonPressed(0);
    AppState->UI.MouseButtonReleased = IsMouseButtonReleased(0);
    AppState->UI.MousePosition = GetMousePosition();
}

internal void InitTurtleState(app_state *AppState, f32 OffsetX, f32 OffsetY)
{
    SetMemory((u8 *)&AppState->Expansion, 0, sizeof(expansion));
    SetMemory((u8 *)&AppState->TurtleStack, 0, sizeof(turtle) * TURTLE_STACK_MAX);

    AppState->TurtleStackIndex = 0;

    CopyTurtle(&AppState->Turtle, &AppState->TurtleStack[AppState->TurtleStackIndex]);

    AppState->TurtleStack[AppState->TurtleStackIndex].Position.x += OffsetX;
    AppState->TurtleStack[AppState->TurtleStackIndex].Position.y += OffsetY;

    expansion_item RootItem = CreateExpansionItem(symbol_Root, 0);
    PushExpansionItem(&AppState->Expansion, RootItem);
}

internal void UpdateAndRender(void *VoidAppState)
{
    /* We have to pass our data in as a void-star because of emscripten:
       https://github.com/raysan5/raylib/blob/master/examples/core/core_basic_window_web.c
    */
    app_state *AppState = (app_state *)VoidAppState;
    UpdateUI(AppState);
    b32 UiInteractionOccured = 0;

    BeginDrawing();
    ClearBackground(BackgroundColor);

    { /* draw simulation image */
        Color *Pixels = LoadImageColors(AppState->Canvas);
        UpdateTexture(AppState->FrameBuffer, Pixels);
        UnloadImageColors(Pixels);
        DrawTexture(AppState->FrameBuffer, 0, 0, (Color){255,255,255,255});
    }

    { /* draw ui */
        ui *UI = &AppState->UI;

#if 0
        for (s32 I = 1; I < button_kind_Count; I++)
        {
            button Button = AppState->Buttons[I];

            if (DoButton(UI, &Button))
            {
                UiInteractionOccured = 1;
                ImageClearBackground(&AppState->Canvas, BackgroundColor);
            }
        }
#endif

        b32 SliderUpdated = DoSlider(UI, &AppState->Slider);

        if (SliderUpdated)
        {
            UiInteractionOccured = 1;
            AppState->RotationAmount = AppState->Slider.Value * 8.0f;
            ImageClearBackground(&AppState->Canvas, BackgroundColor);
            InitTurtleState(AppState, 0.0f, 0.0f);
        }

        if (!UiInteractionOccured)
        {
            b32 TabletChanged = DoUiElement(UI, &AppState->Tablet);
            tablet *Tablet = &AppState->Tablet.Tablet;

            if (TabletChanged)
            {
                ImageClearBackground(&AppState->Canvas, BackgroundColor);

                if (Tablet->UpdateTurtlePosition)
                {
                    AppState->Turtle.Position.x += Tablet->Offset.x;
                    AppState->Turtle.Position.y += Tablet->Offset.y;
                    InitTurtleState(AppState, 0.0f, 0.0f);
                }
                else
                {
                    Vector2 Offset = Tablet->Offset;
                    InitTurtleState(AppState, Offset.x, Offset.y);
                }
            }
        }
    }

    DrawLSystem(AppState, 8);

    EndDrawing();
}

internal void InitRules(symbol Rules[symbol_Count][RULE_SIZE_MAX])
{
    symbol A = symbol_A;
    symbol B = symbol_B;
    symbol Push = symbol_Push;
    symbol Pop = symbol_Pop;
    symbol End = symbol_Undefined;

    symbol InitialRules[symbol_Count][RULE_SIZE_MAX] = {
        [symbol_Root] = {A,End},
        /* [symbol_A] = {B,Push,A,Pop,A,End}, */
        [symbol_A] = {B, Push, B, Push, B, A, Pop, A, A, Pop, A, End},
        /* [symbol_A] = {B, Push, Push, A, Pop, A, Pop, B, Push, B, A, Pop, A, End}, */
        [symbol_B] = {B,B,End},
        /* NOTE: For now we must define constant symbols as rules that expand to themselves.
           We could _maybe_ change the expansion algorithm to treat symbols without a rule as constant,
           but that might cause confusion. Maybe we should just error if we find a symbol without a rule... */
        [symbol_Push] = {symbol_Push,End},
        [symbol_Pop] = {symbol_Pop,End},
    };

    for (u32 I = 0; I < ArrayCount(InitialRules); I++)
    {
        for (u32 J = 0; J < ArrayCount(InitialRules[I]); J++)
        {
            Rules[I][J] = InitialRules[I][J];
        }
    }
}

internal void InitUi(app_state *AppState)
{
    s32 Y = 10.0f;
    s32 I = 1;
    s32 TwicePadding = 2 * BUTTON_PADDING;

    AppState->UI.Active = 0;

    for (; I < button_kind_Count; I++)
    {
        u8 *Text = ButtonText[I];

        AppState->Buttons[I].Id = I;

        AppState->Buttons[I].Text = Text;
        AppState->Buttons[I].Position = (Vector2){10.0f, Y};

        Y += TwicePadding + AppState->UI.FontSize + 10;
    }

    { /* init slider */
        AppState->Slider.Id = I;

        AppState->Slider.Position.x = 10.0f;
        AppState->Slider.Position.y = Y;

        AppState->Slider.Size.x = 240.0f;
        AppState->Slider.Size.y = 40.0f;

        AppState->Slider.Value = 0.3f;

        I += 1;
    }

    { /* init tablet */
        AppState->Tablet.Tablet.Id = I;
        AppState->Tablet.Type = ui_element_type_Tablet;

        AppState->Tablet.Tablet.Position.x = 0.0f;
        AppState->Tablet.Tablet.Position.y = 0.0f;

        AppState->Tablet.Tablet.Size.x = SCREEN_WIDTH;
        AppState->Tablet.Tablet.Size.y = SCREEN_HEIGHT;

        AppState->Tablet.Tablet.Offset.x = 0.0f;
        AppState->Tablet.Tablet.Offset.y = 0.0f;

        I += 1;
    }
}

internal app_state InitAppState(void)
{
    app_state AppState;

    Vector2 TurtlePosition = V2(120.0f, SCREEN_HEIGHT / 2.0f);
    Vector2 TurtleHeading = V2(1.0f, 0.0f);

    AppState.Turtle = (turtle){TurtlePosition, TurtleHeading};

    AppState.RotationAmount = 0.25f;
    AppState.LineDrawsPerFrame = 20000;

    AppState.Canvas = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BackgroundColor);
    AppState.FrameBuffer = LoadTextureFromImage(AppState.Canvas);
    AppState.UI.FontSize = 18;

    InitTurtleState(&AppState, 0.0f, 0.0f);

    InitUi(&AppState);

    InitRules(AppState.Rules);

    return AppState;
}

int main(void)
{
#if defined(PLATFORM_WEB)
    InitRaylibCanvas();
#endif

    printf("InitWindow %d %d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Look at the l-systems");

    app_state AppState = InitAppState();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop_arg(UpdateAndRender, &AppState, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        UpdateAndRender(&AppState);
    }
#endif

    CloseWindow();
}
