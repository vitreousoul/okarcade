/*|
  * TITLE
  This is something
|*/

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

void RUN_LSystem(void);

#define EXPANSION_BUFFER_SIZE (1 << 12)
#define EXPANSION_MAX_DEPTH 7
#define RULE_SIZE_MAX 16

#define BUTTON_PADDING 8

internal void PrintError_(char *Message, char *FileName, s32 LineNumber)
{
    printf("Error in file %s line %d: %s\n", FileName, LineNumber, Message);
}
#define PrintError(message) PrintError_((message), __FILE__, __LINE__)

global_variable Color BackgroundColor = (Color){58, 141, 230, 255};
global_variable Color ClearColor = (Color){0, 0, 0, 255};

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

char *ButtonText[button_kind_Count] = {
    [button_kind_Expansion] = "Expansion",
    [button_kind_Movement] = "Movement",
};

typedef struct
{
    turtle Turtle;
    Image Canvas;
    Texture2D FrameBuffer;
    s32 LineDrawsPerFrame;
    symbol Rules[symbol_Count][RULE_SIZE_MAX];
    button Buttons[button_kind_Count];
    s32 FontSize;
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

internal char *GetSymbolText(symbol Symbol)
{
    switch (Symbol)
    {
    case symbol_A: return (char *)"A";
    case symbol_B: return (char *)"B";
    case symbol_Push: return (char *)"+";
    case symbol_Pop: return (char *)"-";
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

internal void PrintLSystem(Image *Canvas, turtle *Turtle, symbol Rules[symbol_Count][RULE_SIZE_MAX], s32 Depth)
{
    if (Depth >= EXPANSION_MAX_DEPTH)
    {
        s32 MaxDepthMinusOne = EXPANSION_MAX_DEPTH - 1;
        printf("Max depth for PrintLSystem is %d but was passed a depth of %d\n", MaxDepthMinusOne, Depth);
        PrintError("DepthError\n");
    }

    expansion Expansion = {0};

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
            PrintError("Rule index out of bounds\n");
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
                {
                    /* draw random line */
                    f32 X = ((f32)rand()/(f32)(RAND_MAX)) * SCREEN_WIDTH;
                    f32 Y = ((f32)rand()/(f32)(RAND_MAX)) * SCREEN_HEIGHT;
                    ImageDrawLine(Canvas, Turtle->Position.x, Turtle->Position.y, X, Y, (Color){0,0,0,255});

                    Turtle->Position.x = X;
                    Turtle->Position.y = Y;
                }

                CurrentItem.Index += 1;
                OutputSymbol = GetSymbolFromRules(Rules, CurrentItem);
            }

            PopAndIncrementParent(&Expansion);
        }
    }
}

internal Vector2 CreateVector2(f32 X, f32 Y)
{
    return (Vector2){X,Y};
}

internal void DrawRuleSet(app_state *AppState)
{
    Color FontColor = (Color){0, 0, 0, 255};
    Color BackgroundColor = (Color){80, 80, 80, 255};
    s32 ItemWidth = 24;
    s32 Padding = 8;
    s32 Width = (ItemWidth * RULE_SIZE_MAX) + (2 * Padding);
    s32 Height = (2 * AppState->FontSize) + (3 * Padding);
    s32 Y = SCREEN_HEIGHT - (Height + Padding);
    s32 X = Padding;
    s32 RowIndex = 0;

    DrawRectangle(X, Y, Width, Height, BackgroundColor);

    for (s32 I = symbol_A; I <= symbol_B; I++)
    {
        char *LabelText = GetSymbolText(I);
        s32 LabelX = X + Padding;
        s32 YOffset = RowIndex * (AppState->FontSize + Padding);
        RowIndex += 1;
        s32 LabelY = Y + YOffset + Padding;

        DrawText(LabelText, LabelX, LabelY, AppState->FontSize, FontColor);

        for (s32 J = 0; J < RULE_SIZE_MAX; J++)
        {
            char *ItemText = GetSymbolText(AppState->Rules[I][J]);

            if (ItemText)
            {
                s32 XOffset = (J + 1) * (ItemWidth + 0);//Padding);
                DrawText(ItemText, LabelX + XOffset, LabelY, AppState->FontSize, FontColor);
            }
            else
            {
                break;
            }
        }
    }
}

internal void UpdateAndRender(void *VoidAppState)
{
    /* we have to pass our data in as a void-star because of emscripten, so we just cast it to app_state and pray */
    app_state *AppState = (app_state *)VoidAppState;
    Vector2 MousePosition = GetMousePosition();
    b32 ButtonDown = IsMouseButtonPressed(0);

    BeginDrawing();
    ClearBackground(BackgroundColor);
    ImageDrawLine(&AppState->Canvas, 0, 0, MousePosition.x, MousePosition.y, (Color){0,0,0,255});

    if (ButtonDown)
    {
        ImageClearBackground(&AppState->Canvas, BackgroundColor);
    }

    {
        /* draw simulation image */
        Color *Pixels = LoadImageColors(AppState->Canvas);
        UpdateTexture(AppState->FrameBuffer, Pixels);
        UnloadImageColors(Pixels);
        DrawTexture(AppState->FrameBuffer, 0, 0, (Color){255,255,255,255});
    }
    {
        /* draw ui */
        for (s32 I = 1; I < button_kind_Count; I++)
        {
            button Button = AppState->Buttons[I];
            Color ButtonColor = (Color){20,180,180,255};
            Color TextColor = (Color){0,0,0,255};
            DrawRectangle(Button.Position.x, Button.Position.y, Button.Size.x, Button.Size.y, ButtonColor);
            DrawText(Button.Text,
                     Button.Position.x + BUTTON_PADDING, Button.Position.y + BUTTON_PADDING,
                     AppState->FontSize, TextColor);

            DrawRuleSet(AppState);
        }
    }
    EndDrawing();
}

internal void InitRules(symbol Rules[symbol_Count][RULE_SIZE_MAX])
{
    symbol A = symbol_A;
    symbol B = symbol_B;
    symbol End = symbol_Undefined;

    symbol InitialRules[symbol_Count][RULE_SIZE_MAX] = {
        [symbol_Root] = {A,B,A,B,End},
        [symbol_A] = {A,B,A,End},
        [symbol_B] = {B,B,End},
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

    for (s32 I = 0; I < button_kind_Count; I++)
    {
        char *Text = ButtonText[I];
        s32 Width = MeasureText(Text, AppState->FontSize);

        AppState->Buttons[I].Text = Text;
        AppState->Buttons[I].Position = (Vector2){10.0f, Y};
        AppState->Buttons[I].Size = (Vector2){Width + TwicePadding,
                                              AppState->FontSize + TwicePadding};
        AppState->Buttons[I].Active = 0;

        Y += TwicePadding + AppState->FontSize + 10;
    }
}

internal app_state InitAppState(void)
{
    app_state AppState;

    AppState.Turtle = (turtle){CreateVector2(0.0f, 0.0f), CreateVector2(0.0f, 0.0f)};
    AppState.LineDrawsPerFrame = 100;
    AppState.Canvas = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BackgroundColor);
    AppState.FrameBuffer = LoadTextureFromImage(AppState.Canvas);
    AppState.FontSize = 16;

    InitUi(&AppState);

    InitRules(AppState.Rules);

    return AppState;
}

void RUN_LSystem(void)
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

    /* PrintLSystem(&Canvas, &Turtle, Rules, 6); */

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
