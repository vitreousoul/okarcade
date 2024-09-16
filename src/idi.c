/*
  TODO: Once things are more stable, rename all the confusing variable names (rows, columns, variable versions of table/class/index).
  TODO: Setup tests against a C lexer (stb?) to begin building parse tables for lexing C.
*/

#include <stdlib.h> /* NOTE: stdio and time are included because platform current requires it... */
#include <time.h>

#include "../lib/ryn_macro.h"
#include "../lib/ryn_memory.h"
#include "../lib/ryn_string.h"

#include "../src/types.h"
#include "../src/core.c"
#include "../src/platform.h"

#define End_Of_Source_Char 0

#define tokenizer_state_Accepting_XList\
    X(Space, Space)\
    X(Digit, Digit)\
    X(BinaryDigitValue, BinaryDigit)\
    X(HexDigitValue, HexDigit)\
    X(IdentifierStart, Identifier)\
    X(IdentifierRest, Identifier)\
    X(StringEnd, String)\
    X(Equal, Equal)\
    X(DoubleEqual, DoubleEqual)\
    X(ForwardSlash, ForwardSlash)\
    X(BaseDigit, Digit)\
    X(NewlineEscape, NewlineEscape)\
    X(Comment, Comment)

#define tokenizer_state_NonError_XList\
    X(Begin, Begin)\
    tokenizer_state_Accepting_XList\
    X(String, String)\
    X(StringEscape, StringEscape)\
    X(CommentBody, CommentBody)\
    X(CommentBodyCheck, CommentBodyCheck)\
    X(TopLevelEscape, TopLevelEscape)\
    X(Done, Done)

#define tokenizer_state_XList\
    X(_Error, _Error)\
    tokenizer_state_NonError_XList

typedef enum
{
#define X(name, _typename)\
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
    X(OpenBracket,      '{')\
    X(CloseBracket,     '}')\
    X(OpenSquare,       '[')\
    X(CloseSquare,      ']')\
    X(Carrot,           '^')\
    X(Star,             '*')\
    X(Cross,            '+')\
    X(Dash,             '-')\
    X(Comma,            ',')\
    X(Semicolon,        ';')\
    X(Hash,             '#')\
    X(LessThan,         '<')\
    X(GreaterThan,      '>')\
    X(Dot,              '.')

#define token_type_Valid_XList\
    SingleCharTokenList\
    X(Space, 256)\
    X(Digit, 257)\
    X(BinaryDigit, 258)\
    X(HexDigit, 259)\
    X(Identifier, 260)\
    X(String, 261)\
    X(Equal, 262)\
    X(DoubleEqual, 263)\
    X(Comment, 264)\
    X(ForwardSlash, 265)\
    X(NewlineEscape, 266)


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

typedef struct
{
    token_type Type;
    union
    {
        u32 Digit;
        ryn_string String;
    };
} token;

ref_struct(token_list)
{
    token_list *Next;
    token Token;
};

typedef struct
{
    token *FirstToken;
    token *LastToken;

    ryn_string Source;
    u64 SourceIndex;
} tokenizer;

global_variable token_type StateToTypeTable[tokenizer_state__Count] = {
#define X(name, typename)\
    [tokenizer_state_##name] = token_type_##typename,
    tokenizer_state_Accepting_XList
#undef X
};

global_variable tokenizer_state TokenDoneTable[tokenizer_state__Count] = {
    #define X(name, _typename)\
    [tokenizer_state_##name] = tokenizer_state_Begin,
    tokenizer_state_Accepting_XList
#undef X
};

ref_struct(char_set)
{
    char_set *Next;
    s32 Column[256];
    s32 ColumnIndex;
};

typedef struct
{
    char_set *CharSet;
    u8 *CharClass;
    u8 *Table;
    s32 CharSetCount;
} equivalent_char_result;

global_variable tokenizer_state AcceptingStates[] = {
#define X(name, _typename)\
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
#define X(name, _typename)\
    case tokenizer_state_##name: String = ryn_string_CreateString(#name); break;
    tokenizer_state_XList;
#undef X
    default: String = ryn_string_CreateString("Unknown token state");
    }

    return String;
}

void SetupTheTable(void)
{
#define X(name, _typename)\
    tokenizer_state name = tokenizer_state_##name;
    tokenizer_state_NonError_XList;
#undef X

    for (s32 I = 0; I < 256; ++I)
    {
        TheTable[Space][I] = Done;
        TheTable[Equal][I] = Done;
        TheTable[ForwardSlash][I] = Done;
        TheTable[NewlineEscape][I] = Done;
        TheTable[String][I] = String;
        TheTable[CommentBody][I] = CommentBody;
        TheTable[CommentBodyCheck][I] = CommentBody;
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
        TheTable[AcceptingState]['\\'] = Done;
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

    for (s32 I = 'a'; I <= 'z'; ++I)
    {
        TheTable[Begin][I] = IdentifierStart;
        TheTable[Begin][I-32] = IdentifierStart;

        TheTable[IdentifierStart][I] = IdentifierRest;
        TheTable[IdentifierStart][I-32] = IdentifierRest;

        TheTable[IdentifierRest][I] = IdentifierRest;
        TheTable[IdentifierRest][I-32] = IdentifierRest;
    }

    for (s32 I = '0'; I <= '9'; ++I)
    {
        TheTable[Begin][I] = Digit;
        TheTable[Digit][I] = Digit;

        TheTable[IdentifierRest][I] = IdentifierRest;

        TheTable[HexDigitValue][I] = HexDigitValue;
    }

    TheTable[IdentifierRest]['_'] = IdentifierRest;

    for (s32 I = 'a'; I <= 'f'; ++I)
    {
        TheTable[HexDigitValue][I] = HexDigitValue;
    }

    TheTable[Begin]['_'] = IdentifierStart;

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

    TheTable[Begin]['/'] = ForwardSlash;
    TheTable[ForwardSlash]['*'] = CommentBody;

    TheTable[Begin]['\\'] = TopLevelEscape;
    TheTable[TopLevelEscape]['\n'] = NewlineEscape; /* TODO: Handle CRLF */

    TheTable[CommentBody]['*'] = CommentBodyCheck;
    TheTable[CommentBodyCheck]['/'] = Comment;
}

/* TODO: Rename rows/columns to something related to chars and tokenizer-states. */
internal equivalent_char_result GetEquivalentChars(ryn_memory_arena *Arena, u8 *Table, s32 Rows, s32 Columns)
{
    equivalent_char_result Result = {};

    for (s32 C = 0; C < Columns; ++C)
    {
        if (Result.CharSet == 0)
        {
            Result.CharSet = ryn_memory_PushZeroStruct(Arena, char_set);
            Result.CharSet->Column[Result.CharSet->ColumnIndex] = C;
            Result.CharSet->ColumnIndex += 1;
            Result.CharSetCount += 1;
        }
        else
        {
            char_set *CurrentCharSet = Result.CharSet;
            b32 Match;

            while (CurrentCharSet)
            {
                Match = 1;

                for (s32 R = 0; R < Rows; ++R)
                {
                    s32 TestC = CurrentCharSet->Column[0];
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
                    CurrentCharSet->Column[CurrentCharSet->ColumnIndex] = C;
                    CurrentCharSet->ColumnIndex += 1;
                    break;
                }

                CurrentCharSet = CurrentCharSet->Next;
            }

            if (!Match)
            {
                char_set *NewCharSet = ryn_memory_PushZeroStruct(Arena, char_set);
                NewCharSet->Column[NewCharSet->ColumnIndex] = C;
                NewCharSet->ColumnIndex += 1;
                Result.CharSetCount += 1;

                NewCharSet->Next = Result.CharSet;
                Result.CharSet = NewCharSet;
            }
        }
    }

    s32 TableSize = Result.CharSetCount*Rows;
    Result.CharClass = ryn_memory_PushSize(Arena, Columns);
    Result.Table = ryn_memory_PushSize(Arena, TableSize);

    char_set *CurrentCharSet = Result.CharSet;
    s32 CharSetIndex = 0;

    while (CurrentCharSet)
    {
        Assert(CurrentCharSet->ColumnIndex > 0);

        for (s32 I = 0; I < CurrentCharSet->ColumnIndex; ++I)
        {
            u8 Char = CurrentCharSet->Column[I];
            Result.CharClass[Char] = CharSetIndex;
        }

        for (s32 R = 0; R < Rows; ++R)
        {
            s32 ClassTableIndex = CharSetIndex * Rows + R;
            u32 Char = CurrentCharSet->Column[0];
            tokenizer_state NextState = TheTable[R][Char];

            Assert(ClassTableIndex < TableSize);
            Result.Table[ClassTableIndex] = NextState;
        }

        CurrentCharSet = CurrentCharSet->Next;
        CharSetIndex += 1;
    }

    Assert(CharSetIndex == Result.CharSetCount);

    return Result;
}

token_list *Tokenize(ryn_memory_arena *Arena, ryn_string Source)
{
    token_list HeadToken = {};
    token_list *CurrentToken = &HeadToken;
    tokenizer_state State = tokenizer_state_Begin;
    u64 I = 0;
    u8 PreviousChar = Source.Bytes[0];
    b32 EndOfSource = 0;
    equivalent_char_result EquivalentChars = GetEquivalentChars(Arena, (u8 *)TheTable, tokenizer_state__Count, 256);

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
        s32 TableIndex = EquivalentChars.CharClass[Char] * tokenizer_state__Count + State;
        tokenizer_state TestState = EquivalentChars.Table[TableIndex];
        Assert(NextState == TestState);

        /* TODO: This seems like a lot of logic, which maybe could be pushed into the table? Or make the logic a table lookup and see which one is faster? */
        if (NextState == tokenizer_state_Done &&
            Char &&
            SingleTokenCharTable[PreviousChar])
        {
                token_list *NextToken = ryn_memory_PushStruct(Arena, token_list);
                Assert(NextToken != 0);
                NextToken->Token.Type = Char;
                NextState = tokenizer_state_Begin;
                ++I;
                CurrentToken = CurrentToken->Next = NextToken;
        }
        else if (NextState == tokenizer_state_Done &&
                 State != tokenizer_state_Begin)
        {
            token_list *NextToken = ryn_memory_PushStruct(Arena, token_list);
            Assert(NextToken != 0);
            NextToken->Token.Type = StateToTypeTable[State];
            NextState = TokenDoneTable[State];
            CurrentToken = CurrentToken->Next = NextToken;
        }
        else
        {
            ++I;
        }

#if 1
        if (NextState == tokenizer_state__Error)
        {
            printf("Tokenizer error! char_index=%llu    %s\n", I, GetTokenizerStateString(State).Bytes);
            s32 Padding = 16;
            s32 Start = I - Padding;
            s32 End = I + Padding;
            if (Start < 0) Start = 0;
            if ((u32)End > Source.Size) Start = Source.Size;
            for (s32 ErrorIndex = Start; ErrorIndex < End; ++ErrorIndex)
            {
                printf("%c", Source.Bytes[ErrorIndex]);
            }
            printf("\n");
        }
#endif


        State = NextState;
        PreviousChar = Char;

    } while (!EndOfSource &&
             CurrentToken != 0 &&
             State != tokenizer_state__Error);

    if (State == tokenizer_state__Error)
    {
        /* NOTE: Give a zero-token first, which signals an error, but include the tokens that were parsed during the process. */
        token_list *ErrorToken = ryn_memory_PushZeroStruct(Arena, token_list);
        ErrorToken->Next = HeadToken.Next;
        HeadToken.Next = ErrorToken;
    }

    return HeadToken.Next;
}

internal s32 DebugPrintEquivalentChars(ryn_memory_arena *Arena, u8 *Table, s32 Rows, s32 Columns)
{
    s32 EquivalentCharCount = 0;
    equivalent_char_result EquivalentChars = GetEquivalentChars(Arena, Table, Rows, Columns);
    char_set *FirstCharSet = EquivalentChars.CharSet;
    char_set *CurrentCharSet = FirstCharSet;

    for (s32 R = 0; R < Rows; ++R)
    {
        EquivalentCharCount = 0;
        CurrentCharSet = FirstCharSet;

        while (CurrentCharSet)
        {
            s32 Index = Columns * R + CurrentCharSet->Column[0];
            printf("%c  ", Table[Index] == 0 ? '.' : Table[Index]+97);

            EquivalentCharCount += 1;
            CurrentCharSet = CurrentCharSet->Next;
        }
        printf("   [%c %s]\n", R==0 ? '.' : R+97, GetTokenizerStateString(R).Bytes);
    }

    s32 DebugPrintCount = 0;

    printf("\n");
    for (s32 I = 0; I < 256; ++I)
    {
        b32 SomethingPrinted = 0;
        CurrentCharSet = FirstCharSet;
        while (CurrentCharSet)
        {
            if (CurrentCharSet->ColumnIndex > I)
            {
                /* printf("%02x ", CurrentCharSet->Column[I]); */
                u8 Char = CurrentCharSet->Column[I];

                if (Char >= 33 && Char <= 126)
                {
                    printf("%c  ", Char);
                }
                else if (Char == 9)
                {
                    printf("\\t ");
                }
                else if (Char == 10)
                {
                    printf("\\n ");
                }
                else if (Char == 13)
                {
                    printf("\\r ");
                }
                else if (Char == 32)
                {
                    printf("\\s ");
                }
                else
                {
                    /* printf(".  "); */
                    printf("%02x ", Char);
                }
                SomethingPrinted = 1;
                DebugPrintCount += 1;
            }
            else
            {
                printf("   ");
            }

            CurrentCharSet = CurrentCharSet->Next;
        }
        printf("\n");

        if (!SomethingPrinted)
        {
            break;
        }
    }

    Assert(DebugPrintCount == 256);

    return EquivalentCharCount;
}

internal void DebugPrintTheTable(void)
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

#define Max_Token_Count_For_Testing 2048

internal void TestTokenizer(ryn_memory_arena *Arena)
{
#define X(name, _value)\
    token_type name = token_type_##name;
    token_type_Valid_XList;
#undef X

    typedef struct {
        ryn_string Source;
        token Tokens[Max_Token_Count_For_Testing];
    } test_case;
#define T(token_type) {token_type,{}}

    u8 *FileSource = ryn_memory_GetArenaWriteLocation(Arena);
    ReadFileIntoAllocator(Arena, (u8 *)"../src/idi.c");
    ryn_string FileSourceString = ryn_string_CreateString((char *)FileSource);

    test_case TestCases[] = {
        {ryn_string_CreateString("( 193 + abc)- (as3fj / 423)"),
         {T(OpenParenthesis), T(Space), T(Digit), T(Space), T(Cross), T(Space), T(Identifier), T(CloseParenthesis), T(Dash), T(Space), T(OpenParenthesis), T(Identifier), T(Space), T(ForwardSlash), T(Space), T(Digit), T(CloseParenthesis)}},
        {ryn_string_CreateString("bar = (3*4)^x;"),
         {T(Identifier), T(Space), T(Equal), T(Space), T(OpenParenthesis), T(Digit), T(Star), T(Digit), T(CloseParenthesis), T(Carrot), T(Identifier), T(Semicolon)}},
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
        {ryn_string_CreateString("(foo, 234, bar.baz)"),
         {T(OpenParenthesis), T(Identifier), T(Comma), T(Space), T(Digit), T(Comma), T(Space), T(Identifier), T(Dot), T(Identifier), T(CloseParenthesis)}},
        {ryn_string_CreateString("/* This is a comment * and is has stars ** in it */"),
         {T(Comment)}},
        {ryn_string_CreateString("int Foo[3] = {1,2, 4};"),
         {T(Identifier), T(Space), T(Identifier), T(OpenSquare), T(Digit), T(CloseSquare), T(Space), T(Equal), T(Space), T(OpenBracket), T(Digit), T(Comma), T(Digit), T(Comma), T(Space), T(Digit), T(CloseBracket), T(Semicolon)}},
        {ryn_string_CreateString("#define Foo 3"),
         {T(Hash), T(Identifier), T(Space), T(Identifier), T(Space), T(Digit)}},
        {ryn_string_CreateString("((0xabc123<423)>3)"),
         {T(OpenParenthesis), T(OpenParenthesis), T(HexDigit), T(LessThan), T(Digit), T(CloseParenthesis), T(GreaterThan), T(Digit), T(CloseParenthesis)}},
        {ryn_string_CreateString("#define Foo\\\n3"),
         {T(Hash), T(Identifier), T(Space), T(Identifier), T(NewlineEscape), T(Digit)}},


        {FileSourceString, {T(OpenParenthesis)}},
    };

#undef T

    s32 TotalTestCount = ArrayCount(TestCases);
    s32 TotalPassedTests = 0;

    for (s32 I = 0; I < TotalTestCount; ++I)
    {
        test_case TestCase = TestCases[I];
        token_list *Token = Tokenize(Arena, TestCase.Source);
        s32 TestTokenCount = ArrayCount(TestCase.Tokens);
        b32 Matches = 1;
        s32 TestTokenIndex = 0;

        /* printf("Test #%d  \"%s\"\n", I, TestCase.Source.Bytes); */
        printf("Test #%d \n", I);

        if (Token)
        {
            while (Token)
            {
                if (SingleTokenCharTable[Token->Token.Type])
                {
                    printf("%c ", Token->Token.Type);
                }
                else
                {
                    printf("%s ", GetTokenTypeString(Token->Token.Type).Bytes);
                }

                if (TestTokenIndex >= TestTokenCount ||
                    Token->Token.Type == 0 ||
                    Token->Token.Type != TestCase.Tokens[TestTokenIndex].Type)
                {
                    Matches = 0;
                    break;
                }

                TestTokenIndex += 1;
                Token = Token->Next;
            }
            printf("\n");
        }
        else
        {
            Matches = 0;
        }

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
    TestTokenizer(&Arena);

#if 0
    DebugPrintTheTable();
#endif

    printf("\nEquivalent Chars\n");
    s32 Columns = 256;
    printf("\n");
    printf("StateCount = %d\n", tokenizer_state__Count);
    printf("CharCount = %d\n", Columns);
    Assert(tokenizer_state__Count * Columns == sizeof(TheTable));

#if 0
    s32 Rows = tokenizer_state__Count;
    s32 EquivalentCharCount = DebugPrintEquivalentChars(&Arena, (u8 *)TheTable, Rows, Columns);
    printf("EqChar Count %d\n", EquivalentCharCount);
    printf("256 + EqCharCount*StateCount = %d\n", Columns + EquivalentCharCount*tokenizer_state__Count);
#endif

    printf("sizeof(TheTable) %lu\n", sizeof(TheTable));

    return 0;
}
