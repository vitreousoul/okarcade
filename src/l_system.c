#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#define internal static
#define global_variable static

typedef uint8_t u8;
typedef uint32_t u32;

typedef uint32_t b32;

typedef int32_t s32;

typedef float f32;

void RUN_LSystem(void);

#define ArrayCount(a) (sizeof(a)/sizeof((a)[0]))

#define EXPANSION_BUFFER_SIZE (1 << 12)
#define EXPANSION_MAX_DEPTH 7
#define RULE_SIZE_MAX 16

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

typedef struct
{
    turtle Turtle;
    Image Canvas;
    Texture2D FrameBuffer;
    s32 LineDrawsPerFrame;
    symbol Rules[symbol_Count][RULE_SIZE_MAX];
} app_state;

#if defined(PLATFORM_WEB)
EM_JS(void, UpdateCanvasDimensions, (), {
    var canvas = document.getElementById("canvas");

    if (canvas) {
        var rect = canvas.getBoundingClientRect();
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

internal app_state InitAppState(void)
{
    app_state AppState;

    AppState.Turtle = (turtle){CreateVector2(0.0f, 0.0f), CreateVector2(0.0f, 0.0f)};
    AppState.LineDrawsPerFrame = 100;
    AppState.Canvas = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BackgroundColor);
    AppState.FrameBuffer = LoadTextureFromImage(AppState.Canvas);

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
