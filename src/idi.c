/*
  TODO: The tokenizer testing passes even though the tokenized result sometimes has a null token at the end.
*/
#include "../lib/ryn_macro.h"
#include "../lib/ryn_memory.h"
#include "../lib/ryn_string.h"

#include "../src/types.h"
#include "../src/core.c"

#define End_Of_Source_Char 0

#define tokenizer_state_Accepting_XList\
    X(Space)\
    X(Digit)\
    X(BinaryDigitValue)\
    X(HexDigitValue)\
    X(IdentifierStart)\
    X(IdentifierRest)\
    X(StringEnd)\
    X(Equal)\
    X(DoubleEqual)

#define tokenizer_state_NonError_XList\
    X(Begin)\
    tokenizer_state_Accepting_XList\
    X(BaseDigit)\
    X(String)\
    X(StringEscape)\
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
    X(Dash,             '-')

#define token_type_Valid_XList\
    SingleCharTokenList\
    X(Space, 256)\
    X(Digit, 257)\
    X(BinaryDigit, 258)\
    X(Identifier, 259)\
    X(String, 260)\
    X(Equal, 261)\
    X(DoubleEqual, 262)

#define token_type_All_XList\
    X(_Null, 0)\
    token_type_Valid_XList\
    X(__Error, 500)

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
    [tokenizer_state_BinaryDigitValue] = token_type_BinaryDigit,
    [tokenizer_state_IdentifierStart] = token_type_Identifier,
    [tokenizer_state_IdentifierRest] = token_type_Identifier,
    [tokenizer_state_StringEnd] = token_type_String,
    [tokenizer_state_Equal] = token_type_Equal,
    [tokenizer_state_DoubleEqual] = token_type_DoubleEqual,
};

global_variable tokenizer_state TokenDoneTable[tokenizer_state__Count] = {
    [tokenizer_state_Space]      = tokenizer_state_Begin,
    [tokenizer_state_Digit]      = tokenizer_state_Begin,
    [tokenizer_state_BinaryDigitValue] = tokenizer_state_Begin,
    [tokenizer_state_IdentifierStart] = tokenizer_state_Begin,
    [tokenizer_state_IdentifierRest] = tokenizer_state_Begin,
    [tokenizer_state_StringEnd] = tokenizer_state_Begin,
    [tokenizer_state_Equal] = tokenizer_state_Begin,
    [tokenizer_state_DoubleEqual] = tokenizer_state_Begin,
};

global_variable tokenizer_state AcceptingStates[] = {
#define X(name)\
    tokenizer_state_##name,
    tokenizer_state_Accepting_XList
#undef X
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
        TheTable[Equal][I] = Done;
        TheTable[String][I] = String;
    }

    for (s32 I = Begin; I < tokenizer_state__Count; ++I)
    {
        TheTable[I][End_Of_Source_Char] = Done;
    }

    for (u32 I = 0; I < ArrayCount(AcceptingStates); ++I)
    {
        tokenizer_state AcceptingState = AcceptingStates[I];

#define X(_name, value)\
        TheTable[AcceptingState][value] = Done;
        SingleCharTokenList;
#undef X

#define X(_name, value)\
        TheTable[AcceptingState][value] = Done;
        SpaceCharList;
#undef X

        TheTable[AcceptingState]['='] = Done;
    }

#define X(_name, value)\
    TheTable[Begin][value] = Done;
    SingleCharTokenList;
#undef X

#define X(_name, value)\
    TheTable[Begin][value] = Space;\
    TheTable[Space][value] = Space;
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

        TheTable[HexDigitValue][I] = HexDigitValue;
    }

    for (s32 I = 'a'; I < 'f'; ++I)
    {
        TheTable[HexDigitValue][I] = HexDigitValue;
    }

    TheTable[Begin]['0'] = BaseDigit;

    TheTable[BaseDigit]['b'] = BinaryDigitValue;
    TheTable[BaseDigit]['x'] = HexDigitValue;

    TheTable[BinaryDigitValue]['0'] = BinaryDigitValue;
    TheTable[BinaryDigitValue]['1'] = BinaryDigitValue;

    TheTable[Begin]['"'] = String;
    TheTable[String]['\\'] = StringEscape;
    TheTable[String]['"'] = StringEnd;


    TheTable[StringEscape]['a'] = String; // Alert (Beep, Bell) (added in C89)[1]
    TheTable[StringEscape]['b'] = String; // Backspace
    TheTable[StringEscape]['e'] = String; // Escape character
    TheTable[StringEscape]['f'] = String; // Formfeed Page Break
    TheTable[StringEscape]['n'] = String; // Newline (Line Feed); see below
    TheTable[StringEscape]['r'] = String; // Carriage Return
    TheTable[StringEscape]['t'] = String; // Horizontal Tab
    TheTable[StringEscape]['v'] = String; // Vertical Tab
    TheTable[StringEscape]['\\'] = String; // Backslash
    TheTable[StringEscape]['\''] = String; // Apostrophe or single quotation mark
    TheTable[StringEscape]['"'] = String; // Double quotation mark
    TheTable[StringEscape]['?'] = String; // Question mark (used to avoid trigraphs)
    /* TheTable[StringEscape]['nnn'] = String; // The byte whose numerical value is given by nnn interpreted as an octal number */
    /* TheTable[StringEscape]['xhh'] = String; // The byte whose numerical value is given by hh… interpreted as a hexadecimal number */
    /* TheTable[StringEscape]['u'] = String; // Unicode code point below 10000 hexadecimal (added in C99)[1]: 26  */
    /* TheTable[StringEscape]['U'] = String; // Unicode code point where h is a hexadecimal digit */

    TheTable[Begin]['='] = Equal;
    TheTable[Equal]['='] = DoubleEqual;
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

    if (State == tokenizer_state__Error)
    {
        /* NOTE: Give a zero-token first, which signals an error, but include the tokens that were parsed during the process. */
        token *ErrorToken = ryn_memory_PushZeroStruct(Arena, token);
        ErrorToken->Next = HeadToken.Next;
        HeadToken.Next = ErrorToken;
    }

    return HeadToken.Next;
}

ref_struct(char_map)
{
    char_map *Next;
    s32 Column;
};

internal s32 DebugPrintEquivalenceClasses(ryn_memory_arena *Arena, u8 *Table, s32 Rows, s32 Columns)
{
    s32 ClassCount = 0;
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

    for (s32 R = 0; R < Rows; ++R)
    {
        ClassCount = 0;
        CurrentCharMap = FirstCharMap;
        while (CurrentCharMap)
        {
            s32 Index = Columns * R + CurrentCharMap->Column;
            printf("%c ", Table[Index] == 0 ? '.' : Table[Index]+97);

            ClassCount += 1;
            CurrentCharMap = CurrentCharMap->Next;
        }
        printf("   [%c %s]\n", R==0 ? '.' : R+97, GetTokenizerStateString(R).Bytes);
    }

    return ClassCount;
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
        {ryn_string_CreateString("( 123 + abc)- (as3fj / 423)"),
         {T(OpenParenthesis), T(Space), T(Digit), T(Space), T(Cross), T(Space), T(Identifier), T(CloseParenthesis), T(Dash), T(Space), T(OpenParenthesis), T(Identifier), T(Space), T(ForwardSlash), T(Space), T(Digit), T(CloseParenthesis)}},
        {ryn_string_CreateString("bar = (3*4)^x"),
         {T(Identifier), T(Space), T(Equal), T(Space), T(OpenParenthesis), T(Digit), T(Star), T(Digit), T(CloseParenthesis), T(Carrot), T(Identifier)}},
        {ryn_string_CreateString("bar = 0b010110"),
         {T(Identifier), T(Space), T(Equal), T(Space), T(BinaryDigit)}},
        {ryn_string_CreateString("bar= 0b010110"),
         {T(Identifier), T(Equal), T(Space), T(BinaryDigit)}},
        {ryn_string_CreateString("bar =0b010110"),
         {T(Identifier), T(Space), T(Equal), T(BinaryDigit)}},
        {ryn_string_CreateString("\"this is a string\""),
         {T(String)}},
        {ryn_string_CreateString("\"escape \t \n \r this\""),
         {T(String)}},
        {ryn_string_CreateString("= == ="),
         {T(Equal), T(Space), T(DoubleEqual), T(Space), T(Equal)}},
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
        printf("tokenize result %d\n", Token->Type);

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

            if (TestTokenIndex >= TestTokenCount ||
                Token->Type == 0 ||
                Token->Type != TestCase.Tokens[TestTokenIndex].Type)
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
    s32 ClassCount = DebugPrintEquivalenceClasses(&Arena, (u8 *)TheTable, Rows, Columns);
    printf("\n");

    printf("StateCount = %d\n", tokenizer_state__Count);
    printf("CharCount = %d\n", Columns);
    printf("sizeof(TheTable) %lu\n", sizeof(TheTable));
    Assert(tokenizer_state__Count * Columns == sizeof(TheTable));

    printf("EqClass Count %d\n", ClassCount);
    printf("256 + EqClassCount*StateCount = %d\n", Columns + ClassCount*tokenizer_state__Count);

    return 0;
}
