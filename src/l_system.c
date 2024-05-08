/*
    L-Systems

    Just trying to find beautiful patterns using l-systems.

    TODO: Detect if mouse position is unchanged during drag, if so, keep drawing so the tree fills out more as you drag.
    TODO: Display rules being displayed.
    TODO: Copy-paste the text-input code from Estudioso, and use to edit rules.
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "../lib/raylib.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#include "types.h"

int SCREEN_WIDTH = 1024;
int SCREEN_HEIGHT = 600;
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

global_variable char SymbolToCharTable[symbol_Count] = {
    [symbol_Undefined] = ' ',
    [symbol_Root] = '.',
    [symbol_A] = 'a',
    [symbol_B] = 'b',
    [symbol_Push] = '[',
    [symbol_Pop] = ']',
};

#define Is_Valid_Symbol(s) ((s) > symbol_Undefined && (s) < symbol_Count)

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

#define Temp_Text_Buffer_Size 256
    char TempTextBuffer[Temp_Text_Buffer_Size];
} state;


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

internal void DrawLSystem(state *State, s32 Depth)
{
    if (Depth >= EXPANSION_MAX_DEPTH)
    {
        s32 MaxDepthMinusOne = EXPANSION_MAX_DEPTH - 1;
        printf("Max depth for DrawLSystem is %d but was passed a depth of %d\n", MaxDepthMinusOne, Depth);
        LogError("DepthError\n");
        return;
    }

    expansion *Expansion = &State->Expansion;
    turtle *TurtleStack = State->TurtleStack;
    s32 *TurtleStackIndex = &State->TurtleStackIndex;
    f32 TurtleSpeed = 1.0f;

    s32 LineDrawCount = 0;

    while (Expansion->Index > 0 && LineDrawCount < State->LineDrawsPerFrame)
    {
        expansion_item CurrentItem = PeekExpansionItem(Expansion);
        b32 RuleIndexInBounds = CurrentItem.Index >= 0 && CurrentItem.Index < RULE_SIZE_MAX;

        if (!RuleIndexInBounds)
        {
            LogError("Rule index out of bounds\n");
            break;
        }

        symbol ChildSymbol = State->Rules[CurrentItem.Symbol][CurrentItem.Index];

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
            symbol OutputSymbol = GetSymbolFromRules(State->Rules, CurrentItem);

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
                        f32 RotationAmount = State->RotationAmount;
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
                        f32 RotationAmount = -State->RotationAmount;
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

                    ImageDrawLine(&State->Canvas, Turtle->Position.x, Turtle->Position.y, NewX, NewY, LineColor);
                    LineDrawCount += 1;

                    Turtle->Position.x = NewX;
                    Turtle->Position.y = NewY;
                }

                CurrentItem.Index += 1;
                OutputSymbol = GetSymbolFromRules(State->Rules, CurrentItem);
            }

            PopAndIncrementParent(Expansion);
        }
    }
}

internal void UpdateUI(state *State)
{
    State->UI.Hot = -1;
    State->UI.MouseButtonPressed = IsMouseButtonPressed(0);
    State->UI.MouseButtonReleased = IsMouseButtonReleased(0);
    State->UI.MousePosition = GetMousePosition();
}

internal void InitTurtleState(state *State, f32 OffsetX, f32 OffsetY)
{
    SetMemory((u8 *)&State->Expansion, 0, sizeof(expansion));
    SetMemory((u8 *)&State->TurtleStack, 0, sizeof(turtle) * TURTLE_STACK_MAX);

    State->TurtleStackIndex = 0;

    CopyTurtle(&State->Turtle, &State->TurtleStack[State->TurtleStackIndex]);

    State->TurtleStack[State->TurtleStackIndex].Position.x += OffsetX;
    State->TurtleStack[State->TurtleStackIndex].Position.y += OffsetY;

    expansion_item RootItem = CreateExpansionItem(symbol_Root, 0);
    PushExpansionItem(&State->Expansion, RootItem);
}

internal void UpdateAndRender(void *VoidAppState)
{
    /* We have to pass our data in as a void-star because of emscripten:
       https://github.com/raysan5/raylib/blob/master/examples/core/core_basic_window_web.c
    */
    state *State = (state *)VoidAppState;
    UpdateUI(State);
    b32 UiInteractionOccured = 0;

    BeginDrawing();
    ClearBackground(BackgroundColor);

    { /* draw simulation image */
        Color *Pixels = LoadImageColors(State->Canvas);
        UpdateTexture(State->FrameBuffer, Pixels);
        UnloadImageColors(Pixels);
        DrawTexture(State->FrameBuffer, 0, 0, (Color){255,255,255,255});
    }

    { /* draw ui */
        ui *UI = &State->UI;

#if 0
        for (s32 I = 1; I < button_kind_Count; I++)
        {
            button Button = State->Buttons[I];

            if (DoButton(UI, &Button))
            {
                UiInteractionOccured = 1;
                ImageClearBackground(&State->Canvas, BackgroundColor);
            }
        }
#endif

        { /* NOTE: Angle slider */
            b32 SliderUpdated = DoSlider(UI, &State->Slider);
            slider Slider = State->Slider;

            if (SliderUpdated)
            {
                UiInteractionOccured = 1;
                State->RotationAmount = Slider.Value * 8.0f;
                ImageClearBackground(&State->Canvas, BackgroundColor);
                InitTurtleState(State, 0.0f, 0.0f);
            }

            Vector2 TextPosition = (Vector2){
                Slider.Position.x + Slider.Size.x + 8.0f,
                Slider.Position.y + (Slider.Size.y - UI->FontSize) / 2.0f
            };

            sprintf(State->TempTextBuffer, "%.04f", Slider.Value);
            DrawText(State->TempTextBuffer, TextPosition.x, TextPosition.y, UI->FontSize, (Color){255,255,255,255});
        }

        { /* NOTE: Draw rules. */
            Assert(RULE_SIZE_MAX + 4 < Temp_Text_Buffer_Size);

            symbol RuleSymbols[2] = {symbol_A, symbol_B};
            f32 X = 8.0f;
            f32 Y = 8.0f;

            for (u32 I = 0; I < ArrayCount(RuleSymbols); ++I)
            {
                s32 BufferIndex = 0;
                symbol RuleSymbol = RuleSymbols[I];

                State->TempTextBuffer[BufferIndex++] = SymbolToCharTable[RuleSymbol];
                State->TempTextBuffer[BufferIndex++] = ':';
                State->TempTextBuffer[BufferIndex++] = ' ';

                for (s32 RuleIndex = 0; RuleIndex < RULE_SIZE_MAX; ++RuleIndex)
                {
                    symbol Symbol = State->Rules[RuleSymbol][RuleIndex];

                    if (Is_Valid_Symbol(Symbol))
                    {
                        char Char = SymbolToCharTable[Symbol];
                        State->TempTextBuffer[BufferIndex++] = Char;
                    }
                }

                State->TempTextBuffer[BufferIndex] = 0;
                DrawText(State->TempTextBuffer, X, Y, UI->FontSize, (Color){255, 255, 255, 255});
                Y += UI->FontSize + 4.0f;
            }
        }

        if (!UiInteractionOccured)
        {
            b32 TabletChanged = DoUiElement(UI, &State->Tablet);
            tablet *Tablet = &State->Tablet.Tablet;

            if (TabletChanged)
            {
                ImageClearBackground(&State->Canvas, BackgroundColor);

                if (Tablet->MouseReleased)
                {
                    State->Turtle.Position.x += Tablet->Offset.x;
                    State->Turtle.Position.y += Tablet->Offset.y;
                    InitTurtleState(State, 0.0f, 0.0f);
                }
                else
                {
                    Vector2 Offset = Tablet->Offset;
                    InitTurtleState(State, Offset.x, Offset.y);
                }
            }
        }
    }

    DrawLSystem(State, 8);

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

internal void InitUi(state *State)
{
    s32 Y = 10.0f;
    s32 I = 1;
    s32 TwicePadding = 2 * BUTTON_PADDING;

    State->UI.Active = 0;

    for (; I < button_kind_Count; I++)
    {
        u8 *Text = ButtonText[I];

        State->Buttons[I].Id = I;

        State->Buttons[I].Text = Text;
        State->Buttons[I].Position = (Vector2){10.0f, Y};

        Y += TwicePadding + State->UI.FontSize + 10;
    }

    { /* init slider */
        State->Slider.Id = I;

        State->Slider.Position.x = 10.0f;
        State->Slider.Position.y = Y;

        State->Slider.Size.x = 240.0f;
        State->Slider.Size.y = 40.0f;

        State->Slider.Value = 0.3f;

        I += 1;
    }

    { /* init tablet */
        State->Tablet.Tablet.Id = I;
        State->Tablet.Type = ui_element_type_Tablet;

        State->Tablet.Tablet.Position.x = 0.0f;
        State->Tablet.Tablet.Position.y = 0.0f;

        State->Tablet.Tablet.Size.x = SCREEN_WIDTH;
        State->Tablet.Tablet.Size.y = SCREEN_HEIGHT;

        State->Tablet.Tablet.Offset.x = 0.0f;
        State->Tablet.Tablet.Offset.y = 0.0f;

        I += 1;
    }
}

internal state InitAppState(void)
{
    state State;

    Vector2 TurtlePosition = V2(120.0f, SCREEN_HEIGHT / 2.0f);
    Vector2 TurtleHeading = V2(1.0f, 0.0f);

    State.Turtle = (turtle){TurtlePosition, TurtleHeading};

    State.RotationAmount = 0.25f;
    State.LineDrawsPerFrame = 20000;

    State.Canvas = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BackgroundColor);
    State.FrameBuffer = LoadTextureFromImage(State.Canvas);
    State.UI.FontSize = 22;

    InitTurtleState(&State, 0.0f, 0.0f);

    InitUi(&State);

    InitRules(State.Rules);

    return State;
}

int main(void)
{
#if defined(PLATFORM_WEB)
    InitRaylibCanvas();
#endif

    printf("InitWindow %d %d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Look at the l-systems");

    state State = InitAppState();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop_arg(UpdateAndRender, &State, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        UpdateAndRender(&State);
    }
#endif

    CloseWindow();
}
