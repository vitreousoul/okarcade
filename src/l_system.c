/*|
  * TITLE
  This is something
|*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "../lib/raylib.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#include "types.h"
#include "core.c"
#include "math.c"
#include "ui.c"

int SCREEN_WIDTH = 600;
int SCREEN_HEIGHT = 400;
#define TARGET_FPS 30

#define EXPANSION_BUFFER_SIZE (1 << 12)
#define EXPANSION_MAX_DEPTH 7
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
    Texture2D FrameBuffer;
    s32 LineDrawsPerFrame;
    symbol Rules[symbol_Count][RULE_SIZE_MAX];
    button Buttons[button_kind_Count];
    ui UI;
} app_state;


#if defined(PLATFORM_WEB)
EM_JS(void, UpdateCanvasDimensions, (), {
    var canvas = document.getElementById("canvas");
    var body = document.querySelector("body");

    if (canvas && body) {
        var rect = body.getBoundingClientRect();
        canvas.width = rect.width;
        canvas.height = rect.height;
    }
});

EM_JS(s32, GetCanvasWidth, (), {
    var canvas = document.getElementById("canvas");
    if (canvas) {
        var rect = canvas.getBoundingClientRect();
        return canvas.width;
    } else {
        return -1.0;
    }
});

EM_JS(s32, GetCanvasHeight, (), {
    var canvas = document.getElementById("canvas");
    if (canvas) {
        var rect = canvas.getBoundingClientRect();
        return canvas.height;
    } else {
        return -1.0;
    }
});
#endif

internal u8 *GetSymbolText(symbol Symbol)
{
    switch (Symbol)
    {
    case symbol_A: return (u8 *)"A";
    case symbol_B: return (u8 *)"B";
    case symbol_Push: return (u8 *)"+";
    case symbol_Pop: return (u8 *)"-";
    default: return 0;
    }
}

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

internal void DrawLSystem(Image *Canvas, turtle *InitialTurtle, symbol Rules[symbol_Count][RULE_SIZE_MAX], s32 Depth)
{
    if (Depth >= EXPANSION_MAX_DEPTH)
    {
        s32 MaxDepthMinusOne = EXPANSION_MAX_DEPTH - 1;
        printf("Max depth for DrawLSystem is %d but was passed a depth of %d\n", MaxDepthMinusOne, Depth);
        LogError("DepthError\n");
    }

    expansion Expansion = {0};
    turtle TurtleStack[TURTLE_STACK_MAX];
    s32 TurtleStackIndex = 0;
    CopyTurtle(InitialTurtle, &TurtleStack[TurtleStackIndex]);

    {
        expansion_item RootItem = CreateExpansionItem(symbol_Root, 0);
        PushExpansionItem(&Expansion, RootItem);
    }

    while (Expansion.Index > 0)
    {
        expansion_item CurrentItem = PeekExpansionItem(&Expansion);
        b32 RuleIndexInBounds = CurrentItem.Index >= 0 && CurrentItem.Index < RULE_SIZE_MAX;

        if (!RuleIndexInBounds)
        {
            LogError("Rule index out of bounds\n");
            break;
        }

        symbol ChildSymbol = Rules[CurrentItem.Symbol][CurrentItem.Index];

        if (ChildSymbol == symbol_Undefined)
        {
            PopAndIncrementParent(&Expansion);
        }
        else if (Expansion.Index <= Depth)
        {
            expansion_item ChildItem = CreateExpansionItem(ChildSymbol, 0);
            PushExpansionItem(&Expansion, ChildItem);
        }
        else
        {
            /* output symbols */
            symbol OutputSymbol = GetSymbolFromRules(Rules, CurrentItem);

            while (OutputSymbol != symbol_Undefined)
            {
                turtle *Turtle = &TurtleStack[TurtleStackIndex];
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
                    if (TurtleStackIndex < TURTLE_STACK_MAX - 1)
                    {
                        Vector2 NewHeading = RotateVector2(Turtle->Heading, 0.75f);
                        TurtleStackIndex += 1;
                        CopyTurtle(Turtle, &TurtleStack[TurtleStackIndex]);
                        TurtleStack[TurtleStackIndex].Heading = NewHeading;
                    }
                } break;
                case symbol_Pop:
                {
                    if (TurtleStackIndex > 0)
                    {
                        TurtleStackIndex -= 1;
                        Vector2 NewHeading = RotateVector2(TurtleStack[TurtleStackIndex].Heading, -0.75f);
                        TurtleStack[TurtleStackIndex].Heading = NewHeading;
                    }
                } break;
                default: break;
                }

                if (ShouldDraw)
                {
                    turtle *Turtle = &TurtleStack[TurtleStackIndex];
                    f32 NewX = Turtle->Position.x + (10.0f * Turtle->Heading.x);
                    f32 NewY = Turtle->Position.y + (10.0f * Turtle->Heading.y);
                    Color LineColor = (Color){0,0,0,255};

                    ImageDrawLine(Canvas, Turtle->Position.x, Turtle->Position.y, NewX, NewY, LineColor);

                    Turtle->Position.x = NewX;
                    Turtle->Position.y = NewY;
                }

                CurrentItem.Index += 1;
                OutputSymbol = GetSymbolFromRules(Rules, CurrentItem);
            }

            PopAndIncrementParent(&Expansion);
        }
    }
}

internal void DrawRuleSet(app_state *AppState)
{
    Color FontColor = (Color){0, 0, 0, 255};
    s32 ItemWidth = 24;
    s32 Padding = 8;
    s32 Height = (2 * AppState->UI.FontSize) + (3 * Padding);
    s32 Y = SCREEN_HEIGHT - (Height + Padding);
    s32 X = Padding;
    s32 RowIndex = 0;

    /* DrawRectangle(X, Y, Width, Height, BackgroundColor); */

    for (s32 I = symbol_A; I <= symbol_B; I++)
    {
        u8 *LabelText = GetSymbolText(I);
        s32 LabelX = X + Padding;
        s32 YOffset = RowIndex * (AppState->UI.FontSize + Padding);
        RowIndex += 1;
        s32 LabelY = Y + YOffset + Padding;

        ui_id RuleButtonId = button_kind_Count + 1 + I;
        Vector2 ButtonPosition = CreateVector2(LabelX, LabelY);
        button RuleButton = CreateButton(ButtonPosition, LabelText, RuleButtonId);

        DoButton(&AppState->UI, RuleButton, AppState->UI.FontSize);

        for (s32 J = 0; J < RULE_SIZE_MAX; J++)
        {
            u8 *ItemText = GetSymbolText(AppState->Rules[I][J]);

            if (ItemText)
            {
                s32 XOffset = (J + 1) * (ItemWidth + 0);//Padding);
                DrawText((char *)ItemText, LabelX + XOffset, LabelY, AppState->UI.FontSize, FontColor);
            }
            else
            {
                break;
            }
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

internal void UpdateAndRender(void *VoidAppState)
{
    /* we have to pass our data in as a void-star because of emscripten, so we just cast it to app_state and pray */
    app_state *AppState = (app_state *)VoidAppState;
    UpdateUI(AppState);

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
        for (s32 I = 1; I < button_kind_Count; I++)
        {
            button Button = AppState->Buttons[I];

            if (DoButton(UI, Button, AppState->UI.FontSize))
            {
                ImageClearBackground(&AppState->Canvas, BackgroundColor);
            }
        }

        DrawRuleSet(AppState);
    }

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
        [symbol_A] = {B,Push,A,Pop,A,End},
        [symbol_B] = {B,B,End},
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
    s32 TwicePadding = 2 * BUTTON_PADDING;

    AppState->UI.Active = 0;

    for (s32 I = 1; I < button_kind_Count; I++)
    {
        u8 *Text = ButtonText[I];

        AppState->Buttons[I].Id = I;
        AppState->Buttons[I].Text = Text;
        AppState->Buttons[I].Position = (Vector2){10.0f, Y};
        AppState->Buttons[I].Active = 0;

        Y += TwicePadding + AppState->UI.FontSize + 10;
    }
}

internal app_state InitAppState(void)
{
    app_state AppState;

    Vector2 TurtlePosition = CreateVector2(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
    Vector2 TurtleHeading = CreateVector2(1.0f, 0.0f);

    AppState.Turtle = (turtle){TurtlePosition, TurtleHeading};
    AppState.LineDrawsPerFrame = 100;
    AppState.Canvas = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BackgroundColor);
    AppState.FrameBuffer = LoadTextureFromImage(AppState.Canvas);
    AppState.UI.FontSize = 18;

    InitUi(&AppState);

    InitRules(AppState.Rules);

    return AppState;
}

int main(void)
{
#if defined(PLATFORM_WEB)
    UpdateCanvasDimensions();

    s32 CanvasWidth = GetCanvasWidth();
    s32 CanvasHeight = GetCanvasHeight();
    if (CanvasWidth > 0.0f && CanvasHeight > 0.0f)
    {
        SCREEN_WIDTH = CanvasWidth;
        SCREEN_HEIGHT = CanvasHeight;
    }
#endif

    printf("InitWindow %d %d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Look at the l-systems");

    app_state AppState = InitAppState();

    DrawLSystem(&AppState.Canvas, &AppState.Turtle, AppState.Rules, 5);

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
