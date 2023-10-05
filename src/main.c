#include <stdlib.h>
#include <stdio.h>

#include "../lib/raylib.h"

#define internal static
#define global_variable static

typedef uint8_t u8;
typedef uint32_t u32;

typedef uint32_t b32;

typedef int32_t s32;


#define EXPANSION_BUFFER_SIZE (1 << 12)
#define EXPANSION_MAX_DEPTH 7
#define RULE_SIZE_MAX 16

global_variable u8 ExpansionBuffer[EXPANSION_BUFFER_SIZE];
global_variable s32 ExpansionIndex = 0;

internal void PrintError_(char *Message, char *FileName, s32 LineNumber)
{
    printf("Error in file %s line %d: %s\n", FileName, LineNumber, Message);
}
#define PrintError(message) PrintError_((message), __FILE__, __LINE__)

int SCREEN_WIDTH = 960;
int SCREEN_HEIGHT = 540;
#define TARGET_FPS 30

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

internal void WriteToExpansionBuffer(symbol Symbol)
{
    if (ExpansionIndex >= 0 && ExpansionIndex < EXPANSION_BUFFER_SIZE)
    {
        ExpansionBuffer[ExpansionIndex] = Symbol;
        ExpansionIndex += 1;
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

internal void PrintLSystem(symbol Rules[symbol_Count][RULE_SIZE_MAX], s32 Depth)
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
                WriteToExpansionBuffer(OutputSymbol);
                CurrentItem.Index += 1;
                OutputSymbol = GetSymbolFromRules(Rules, CurrentItem);
            }

            PopAndIncrementParent(&Expansion);
        }
    }
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

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Look at the l-systems");
    SetTargetFPS(TARGET_FPS);

    PrintLSystem(Rules, 6);
    for (s32 I = 0; I < ExpansionIndex; I++)
    {
        switch(ExpansionBuffer[I])
        {
        case symbol_A: {printf("a");} break;
        case symbol_B: {printf("b");} break;
        default: break;
        }
    };
    printf("\n");

    ClearBackground((Color){0,0,0,255});

    while (!WindowShouldClose())
    {
        BeginDrawing();
        EndDrawing();
    }

    CloseWindow();


    return Result;
}
