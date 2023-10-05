#include <stdlib.h>
#include <stdio.h>

#include "../lib/raylib.h"

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


#define EXPANSION_BUFFER_SIZE (1 << 12)
#define EXPANSION_MAX_DEPTH 7
#define RULE_SIZE_MAX 16

internal void PrintError_(char *Message, char *FileName, s32 LineNumber)
{
    printf("Error in file %s line %d: %s\n", FileName, LineNumber, Message);
}
#define PrintError(message) PrintError_((message), __FILE__, __LINE__)

int SCREEN_WIDTH = 600;
int SCREEN_HEIGHT = 400;
#define TARGET_FPS 30

global_variable Color BackgroundColor = (Color){58, 141, 230, 255};
global_variable Color ClearColor = (Color){0, 0, 0, 255};

/* TODO put state into a struct and pass as (void *) arg in emscripten_set_main_loop */
global_variable Image Canvas;
global_variable Texture2D FrameBuffer;

typedef enum
{
    symbol_Undefined,
    symbol_Root,
    symbol_A,
    symbol_B,
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

internal void UpdateAndRender()
{
    Vector2 MousePosition = GetMousePosition();
    b32 ButtonDown = IsMouseButtonPressed(0);

    BeginDrawing();
    ClearBackground(BackgroundColor);
    ImageDrawLine(&Canvas, 0, 0, MousePosition.x, MousePosition.y, (Color){0,0,0,255});

    if (ButtonDown)
    {
        ImageClearBackground(&Canvas, BackgroundColor);
    }

    {
        /* draw simulation image */
        Color *Pixels = LoadImageColors(Canvas);
        UpdateTexture(FrameBuffer, Pixels);
        UnloadImageColors(Pixels);
        DrawTexture(FrameBuffer, 0, 0, (Color){255,255,255,255});
    }
    EndDrawing();
}

int main(void)
{
    int Result = 0;

    s32 LineDrawsPerFrame = 100;

    symbol A = symbol_A;
    symbol B = symbol_B;
    symbol End = symbol_Undefined;

    symbol Rules[symbol_Count][RULE_SIZE_MAX] = {
        [symbol_Root] = {A,B,A,B,End},
        [symbol_A] = {A,B,A,End},
        [symbol_B] = {B,B,End},
    };

    turtle Turtle = {CreateVector2(0.0f, 0.0f), CreateVector2(0.0f, 0.0f)};

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Look at the l-systems");

    Canvas = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BackgroundColor);
    FrameBuffer = LoadTextureFromImage(Canvas);

    /* PrintLSystem(&Canvas, &Turtle, Rules, 6); */

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateAndRender, 0, 2);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        UpdateAndRender();
    }
#endif

    CloseWindow();

    return Result;
}
