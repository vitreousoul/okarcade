/*
  TODO: The tokenizer testing passes even though the tokenized result sometimes has a null token at the end.
*/
#include "../lib/ryn_macro.h"
#include "../lib/ryn_memory.h"
#include "../lib/ryn_string.h"

#include "../src/types.h"
#include "../src/core.c"

#define End_Of_Source_Char 0

#define tokenizer_state_NonError_XList\
    X(Begin)\
    X(Space)\
    X(Digit)\
    X(IdentifierStart)\
    X(IdentifierRest)\
    X(Done)

#define tokenizer_state_XList\
    X(_Error)\
    tokenizer_state_NonError_XList

typedef enum
{
#define X(name)\
    tokenizer_state_##name,
    tokenizer_state_XList
#undef X

    tokenizer_state__Count
} tokenizer_state;

#define SpaceCharList\
    X(Space,   ' ')\
    X(Tab,     '\t')\
    X(Newline, '\n')\
    X(Return,  '\r')

#define SingleCharTokenList\
    X(OpenParenthesis,  '(')\
    X(CloseParenthesis, ')')\
    X(Carrot,           '^')\
    X(Star,             '*')\
    X(ForwardSlash,     '/')\
    X(Cross,            '+')\
    X(Dash,             '-')\
    X(Equal,            '=')

#define token_type_Valid_XList\
    SingleCharTokenList\
    X(Space, 256)\
    X(Digit, 257)\
    X(Identifier, 258)


#define token_type_All_XList\
    X(_Null, 0)\
    token_type_Valid_XList\
    X(__Error, 259)

typedef enum
{
#define X(name, literal)\
    token_type_##name = literal,
    token_type_All_XList
#undef X
} token_type;

global_variable b32 SingleTokenCharTable[256] = {
#define X(name, literal)\
    [token_type_##name] = 1,
    SingleCharTokenList
#undef X
};

ref_struct(token)
{
    token_type Type;
    token *Next;
    union
    {
        u32 Digit;
        ryn_string String;
    };
};

typedef struct
{
    token *FirstToken;
    token *LastToken;

    ryn_string Source;
    u64 SourceIndex;
} tokenizer;

global_variable token_type StateToTypeTable[tokenizer_state__Count] = {
    [tokenizer_state_Space]      = token_type_Space,
    [tokenizer_state_Digit]      = token_type_Digit,
    [tokenizer_state_IdentifierStart] = token_type_Identifier,
    [tokenizer_state_IdentifierRest] = token_type_Identifier,
};

global_variable tokenizer_state TokenDoneTable[tokenizer_state__Count] = {
    [tokenizer_state_Space]      = tokenizer_state_Begin,
    [tokenizer_state_Digit]      = tokenizer_state_Begin,
    [tokenizer_state_IdentifierStart] = tokenizer_state_Begin,
    [tokenizer_state_IdentifierRest] = tokenizer_state_Begin,
};

global_variable u8 TheTable[tokenizer_state__Count][256];


internal ryn_string GetTokenTypeString(token_type TokenType)
{
    ryn_string String;

    switch (TokenType)
    {
#define X(name, _value)\
    case token_type_##name: String = ryn_string_CreateString(#name); break;
    token_type_All_XList;
#undef X
    default: String = ryn_string_CreateString("[Error]");
    }

    return String;
}

internal ryn_string GetTokenizerStateString(tokenizer_state TokenizerState)
{
    ryn_string String;

    switch (TokenizerState)
    {
#define X(name)\
    case tokenizer_state_##name: String = ryn_string_CreateString(#name); break;
    tokenizer_state_XList;
#undef X
    default: String = ryn_string_CreateString("Unknown token state");
    }

    return String;
}

void SetupTheTable(void)
{
#define X(name)\
    tokenizer_state name = tokenizer_state_##name;
    tokenizer_state_NonError_XList;
#undef X

    for (s32 I = 0; I < 256; ++I)
    {
        TheTable[Space][I] = Done;
    }

    for (s32 I = Begin; I < tokenizer_state__Count; ++I)
    {
        TheTable[I][End_Of_Source_Char] = Done;
    }

#define X(_name, value)\
    TheTable[Begin][value] = Done;\
    TheTable[Digit][value] = Done;\
    TheTable[IdentifierStart][value] = Done;\
    TheTable[IdentifierRest][value] = Done;
    SingleCharTokenList;
#undef X

#define X(_name, value)\
    TheTable[Begin][value] = Space;\
    TheTable[Space][value] = Space;\
    TheTable[Digit][value] = Done;\
    TheTable[IdentifierStart][value] = Done;\
    TheTable[IdentifierRest][value] = Done;
    SpaceCharList;
#undef X

    for (s32 I = 'a'; I < 'z'; ++I)
    {
        TheTable[Begin][I] = IdentifierStart;
        TheTable[Begin][I-32] = IdentifierStart;

        TheTable[IdentifierStart][I] = IdentifierRest;
        TheTable[IdentifierStart][I-32] = IdentifierRest;

        TheTable[IdentifierRest][I] = IdentifierRest;
        TheTable[IdentifierRest][I-32] = IdentifierRest;
    }

    for (s32 I = '0'; I < '9'; ++I)
    {
        TheTable[Begin][I] = Digit;
        TheTable[Digit][I] = Digit;

        TheTable[IdentifierRest][I] = IdentifierRest;
        TheTable[IdentifierRest][I-32] = IdentifierRest;
    }
}

token *Tokenize(ryn_memory_arena *Arena, ryn_string Source)
{
    token HeadToken = {};
    token *CurrentToken = &HeadToken;
    tokenizer_state State = tokenizer_state_Begin;
    u64 I = 0;
    u8 PreviousChar = Source.Bytes[0];
    b32 EndOfSource = 0;

    do
    {
        EndOfSource = (I == Source.Size) || (Source.Bytes[I] == 0);
        u8 Char;

        if (EndOfSource)
        {
            Char = End_Of_Source_Char;
        }
        else
        {
            Char = Source.Bytes[I];
        }

        tokenizer_state NextState = TheTable[State][Char];

        if (NextState == tokenizer_state_Done)
        {
            token *NextToken = ryn_memory_PushStruct(Arena, token);
            Assert(NextToken != 0);

            if (SingleTokenCharTable[PreviousChar])
            {
                NextToken->Type = Char;
                NextState = tokenizer_state_Begin;
                ++I;
            }
            else
            {
                NextToken->Type = StateToTypeTable[State];
                NextState = TokenDoneTable[State];
            }

            CurrentToken = CurrentToken->Next = NextToken;
        }
        else if (NextState == tokenizer_state__Error)
        {
            printf("Tokenizer error! char_index=%llu    %s\n", I, GetTokenizerStateString(State).Bytes);
        }
        else
        {
            ++I;
        }

        State = NextState;
        PreviousChar = Char;

    } while (!EndOfSource &&
             CurrentToken != 0 &&
             State != tokenizer_state__Error);

    return HeadToken.Next;
}

ref_struct(char_map)
{
    char_map *Next;
    s32 Column;
};

internal void DebugPrintEquivalenceClasses(ryn_memory_arena *Arena, u8 *Table, s32 Rows, s32 Columns)
{
    char_map *FirstCharMap = 0;
    char_map *CurrentCharMap = 0;

    for (s32 C = 0; C < Columns; ++C)
    {
        if (FirstCharMap == 0)
        {
            FirstCharMap = ryn_memory_PushZeroStruct(Arena, char_map);
            FirstCharMap->Column = C;
        }
        else
        {
            CurrentCharMap = FirstCharMap;
            b32 Match;

            while (CurrentCharMap)
            {
                Match = 1;

                for (s32 R = 0; R < Rows; ++R)
                {
                    s32 TestC = CurrentCharMap->Column;
                    s32 Index = C + Columns * R;
                    s32 TestIndex = TestC + Columns * R;

                    if (Table[Index] != Table[TestIndex])
                    {
                        Match = 0;
                        break;
                    }
                }

                if (Match)
                {
                    break;
                }

                CurrentCharMap = CurrentCharMap->Next;
            }

            if (!Match)
            {
                char_map *NewCharMap = ryn_memory_PushZeroStruct(Arena, char_map);
                NewCharMap->Column = C;

                NewCharMap->Next = FirstCharMap;
                FirstCharMap = NewCharMap;
            }
        }
    }

    CurrentCharMap = FirstCharMap;
    while (CurrentCharMap)
    {
        for (s32 R = 0; R < Rows; ++R)
        {
            s32 Index = Columns * R + CurrentCharMap->Column;
            printf("%c ", Table[Index] == 0 ? '.' : Table[Index]+48);
        }
        printf("\n");

        CurrentCharMap = CurrentCharMap->Next;
    }
}

internal void DebugPringTheTable(void)
{
    for (s32 C = 0; C < 256; ++C)
    {
        if (SingleTokenCharTable[C])
        {
            printf("  %c:  ", C);
        }
        else
        {
            printf("%3d:  ", C);
        }

        for (s32 S = 0; S < tokenizer_state__Count; ++S)
        {
            printf("%d ", TheTable[S][C]);
        }
        printf("\n");
    }
}

#define Max_Token_Count_For_Testing 128

internal void TestTokenizer(ryn_memory_arena Arena)
{
#define X(name, _value)\
    token_type name = token_type_##name;
    token_type_Valid_XList;
#undef X

    typedef struct {
        ryn_string Source;
        token Tokens[Max_Token_Count_For_Testing];
    } test_case;

#define T(token_type) {token_type,0,{}}
    test_case TestCases[] = {
        {
            ryn_string_CreateString("( 123 + abc)- (as3fj / 423)"),
            {T(OpenParenthesis), T(Space), T(Digit), T(Space), T(Cross), T(Space), T(Identifier), T(CloseParenthesis), T(Dash), T(Space), T(OpenParenthesis), T(Identifier), T(Space), T(ForwardSlash), T(Space), T(Digit), T(CloseParenthesis)}
        },
        {
            ryn_string_CreateString("bar = (3*4)^x"),
            {T(Identifier), T(Space), T(Equal), T(Space), T(OpenParenthesis), T(Digit), T(Star), T(Digit), T(CloseParenthesis), T(Carrot), T(Identifier)}
        }
    };
#undef T

    s32 TotalTestCount = ArrayCount(TestCases);
    s32 TotalPassedTests = 0;

        /*
        ryn_string_CreateString("123a56"),
        ryn_string_CreateString("    spaceatstart"),
        ryn_string_CreateString("spaceatend    "),
        ryn_string_CreateString("185"),
        */

    for (s32 I = 0; I < TotalTestCount; ++I)
    {
        test_case TestCase = TestCases[I];
        printf("[%d] \"%s\"\n", I, TestCase.Source.Bytes);

        token *Token = Tokenize(&Arena, TestCase.Source);

        s32 TestTokenCount = ArrayCount(TestCase.Tokens);
        b32 Matches = 1;
        s32 TestTokenIndex = 0;

        while (Token)
        {
            if (SingleTokenCharTable[Token->Type])
            {
                printf("%c ", Token->Type);
            }
            else
            {
                printf("%s ", GetTokenTypeString(Token->Type).Bytes);
            }

            if (TestTokenIndex >= TestTokenCount || Token->Type != TestCase.Tokens[TestTokenIndex].Type)
            {
                Matches = 0;
                break;
            }

            TestTokenIndex += 1;
            Token = Token->Next;
        }
        printf("\n");

        if (Matches)
        {
            printf("pass");
            TotalPassedTests += 1;
        }
        else
        {
            printf("FAIL XXXXXXXXXXXXX\n");
        }

        printf("\n\n");
    }

    printf("(%d / %d) passed tests\n", TotalPassedTests, TotalTestCount);
    printf("%d failed\n", TotalTestCount - TotalPassedTests);
}

int main(void)
{
    ryn_memory_arena Arena = ryn_memory_CreateArena(Megabytes(1));

    SetupTheTable();
    TestTokenizer(Arena);

#if 0
    DebugPringTheTable();
#endif

    printf("\nEquivalence Classes\n");
    s32 Rows = tokenizer_state__Count;
    s32 Columns = 256;
    DebugPrintEquivalenceClasses(&Arena, (u8 *)TheTable, Rows, Columns);
    printf("\n");

    printf("sizeof(TheTable) %lu\n", sizeof(TheTable));

    return 0;
}
