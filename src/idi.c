/*
  TODO:
    [ ] When tokenizing, turn identifiers that are keywords into a specific token_type_* so that the parser doesn't have to do string comparison or other junk.
    [ ] There is shared structure between string escapes and character literal escapes. We should make sure we treat characters inside a string literal as single characters inside a char literal.
    [ ] Once things are more stable, rename all the confusing variable names (rows, columns, variable versions of table/class/index).
    [ ] Setup tests against a C lexer (stb?) to begin building parse tables for lexing C.
*/

#include <stdlib.h> /* NOTE: stdio and time are included because platform current requires it... */
#include <time.h>

#include "../lib/ryn_macro.h"
#include "../lib/ryn_memory.h"
#include "../lib/ryn_string.h"

#include "../src/types.h"
#include "../src/core.c"
#include "../src/platform.h"

#define Test_Tokenizer 1
#define Test_Parser    1

/**************************************/
/* Types/Tables */

#define End_Of_Source_Char 0

#define SingleCharTokenList\
    X(OpenParenthesis,   OpenParenthesis,   '(')\
    X(CloseParenthesis,  CloseParenthesis,  ')')\
    X(OpenBracket,       OpenBracket,       '{')\
    X(CloseBracket,      CloseBracket,      '}')\
    X(OpenSquare,        OpenSquare,        '[')\
    X(CloseSquare,       CloseSquare,       ']')\
    X(Carrot,            Carrot,            '^')\
    X(Star,              Star,              '*')\
    X(Cross,             Cross,             '+')\
    X(Comma,             Comma,             ',')\
    X(Colon,             Colon,             ':')\
    X(Semicolon,         Semicolon,         ';')\
    X(Hash,              Hash,              '#')\
    X(Ampersand,         Ampersand,         '&')\
    X(Pipe,              Pipe,              '|')\
    X(Question,          Question,          '?')\
    X(Dot,               Dot,               '.')

#define tokenizer_state_Accepting_XList\
    SingleCharTokenList\
    X(Space,               Space,               0)\
    X(Digit,               Digit,               0)\
    X(BinaryDigitValue,    BinaryDigit,         0)\
    X(HexDigitValue,       HexDigit,            0)\
    X(IdentifierStart,     Identifier,          0)\
    X(IdentifierRest,      Identifier,          0)\
    X(StringEnd,           String,              0)\
    X(CharLiteralEnd,      CharLiteral,         0)\
    X(Equal,               Equal,               0)\
    X(LessThan,            LessThan,            0)\
    X(LessThanOrEqual,     LessThanOrEqual,     0)\
    X(GreaterThan,         GreaterThan,         0)\
    X(GreaterThanOrEqual,  GreaterThanOrEqual,  0)\
    X(Not,                 Not,                 0)\
    X(DoubleEqual,         DoubleEqual,         0)\
    X(ForwardSlash,        ForwardSlash,        0)\
    X(BaseDigit,           Digit,               0)\
    X(NewlineEscape,       NewlineEscape,       0)\
    X(Arrow,               Arrow,               0)\
    X(NotEqual,            NotEqual,            0)\
    X(Comment,             Comment,             0)\
    X(Dash,                Dash,                0)\
    X(Newline,             Newline,             0)

#define tokenizer_state_Simple_Delimited_XList\
    X(String,       String,       0)\
    X(CharLiteral,  CharLiteral,  0)\

#define tokenizer_state_Delimited_XList\
    tokenizer_state_Simple_Delimited_XList\
    X(CommentBody, CommentBody, 0)

#define tokenizer_state_NonError_XList\
    X(Begin,              Begin,              0)\
    tokenizer_state_Accepting_XList\
    tokenizer_state_Delimited_XList\
    X(StringEscape,       StringEscape,       0)\
    X(CharLiteralEscape,  CharLiteralEscape,  0)\
    X(CommentBodyCheck,   CommentBodyCheck,   0)\
    X(TopLevelEscape,     TopLevelEscape,     0)\
    X(Done,               Done,               0)

#define tokenizer_state_XList\
    X(_Error, _Error, 0)\
    tokenizer_state_NonError_XList

typedef enum
{
#define X(name, _typename, _literal)\
    tokenizer_state_##name,
    tokenizer_state_XList
#undef X

    tokenizer_state__Count
} tokenizer_state;

typedef enum
{
    parser_state__Error,
    parser_state_Top,
    parser_state_MacroStart,
    parser_state_MacroDefinition,
    parser_state__Count
} parser_state;

#define SpaceCharList\
    X(Space,   ' ')\
    X(Tab,     '\t')\
    X(Return,  '\r')

#define escape_character_XList\
    X(Alert,              'a')\
    X(Backspace,          'b')\
    X(EscapeCharacter,    'e')\
    X(FormfeedPageBreak,  'f')\
    X(Newline,            'n')\
    X(CarriageReturn,     'r')\
    X(HorizontalTab,      't')\
    X(VerticalTab,        'v')\
    X(Backslash,          '\\')\
    X(Apostrophe,         '\'')\
    X(Double,             '"')\
    X(Question,           '?')
    /* TODO: Add the following features to escape-character xlist. */
    /* 'nnn'  The byte whose numerical value is given by nnn interpreted as an octal number */
    /* 'xhh'  The byte whose numerical value is given by hh… interpreted as a hexadecimal number */
    /* 'u'  Unicode code point below 10000 hexadecimal (added in C99)[1]: 26  */
    /* 'U'  Unicode code point where h is a hexadecimal digit */

#define Keywords_XList\
    X(_auto,      Auto,     256)\
    X(_break,     Break,    257)\
    X(_case,      Case,     258)\
    X(_char,      Char,     259)\
    X(_const,     Const,    260)\
    X(_continue,  Continue, 261)\
    X(_default,   Default,  262)\
    X(_do,        Do,       263)\
    X(_double,    Double,   264)\
    X(_else,      Else,     265)\
    X(_enum,      Enum,     266)\
    X(_extern,    Extern,   267)\
    X(_float,     Float,    268)\
    X(_for,       For,      269)\
    X(_goto,      Goto,     270)\
    X(_if,        If,       271)\
    X(_int,       Int,      272)\
    X(_long,      Long,     273)\
    X(_register,  Register, 274)\
    X(_return,    Return,   275)\
    X(_short,     Short,    276)\
    X(_signed,    Signed,   277)\
    X(_sizeof,    Sizeof,   278)\
    X(_static,    Static,   279)\
    X(_struct,    Struct,   280)\
    X(_switch,    Switch,   281)\
    X(_typedef,   Typedef,  282)\
    X(_union,     Union,    283)\
    X(_unsigned,  Unsigned, 284)\
    X(_void,      Void,     285)\
    X(_volatile,  Volatile, 286)\
    X(_while,     While,    287)

#define token_type_Valid_XList\
    SingleCharTokenList\
    Keywords_XList\
    X(Space,               Space,               288)\
    X(Digit,               Digit,               289)\
    X(BinaryDigit,         BinaryDigit,         290)\
    X(HexDigit,            HexDigit,            291)\
    X(Identifier,          Identifier,          292)\
    X(String,              String,              293)\
    X(CharLiteral,         CharLiteral,         294)\
    X(Equal,               Equal,               295)\
    X(DoubleEqual,         DoubleEqual,         296)\
    X(Comment,             Comment,             297)\
    X(ForwardSlash,        ForwardSlash,        298)\
    X(NewlineEscape,       NewlineEscape,       299)\
    X(LessThan,            LessThan,            300)\
    X(LessThanOrEqual,     LessThanOrEqual,     301)\
    X(GreaterThan,         GreaterThan,         302)\
    X(GreaterThanOrEqual,  GreaterThanOrEqual,  303)\
    X(NotEqual,            NotEqual,            304)\
    X(Arrow,               Arrow,               305)\
    X(Not,                 Not,                 306)\
    X(Dash,                Dash,                307)\
    X(Newline,             Newline,             308)

#define token_type_MaxValue 500 /* TODO: This number can be way less now... */

#define token_type_All_XList\
    X(_Null, _Null, 0)\
    token_type_Valid_XList\
    X(__Error, __Error, token_type_MaxValue)

typedef enum
{
#define X(name, _typename, _literal)\
    token_type_##name,
    token_type_All_XList
#undef X
    token_type__Count
} token_type;

global_variable b32 SingleTokenCharTable[token_type_MaxValue] = {
#define X(_name, _typename, literal)\
    [literal] = 1,
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
#define X(name, typename, _literal)\
    [tokenizer_state_##name] = token_type_##typename,
    tokenizer_state_Accepting_XList
#undef X
};

global_variable tokenizer_state TokenDoneTable[tokenizer_state__Count] = {
    #define X(name, _typename, _literal)\
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

ref_struct(keyword_lookup)
{
    keyword_lookup *Child;
    keyword_lookup *Sibling;
    u8 Char;
    b32 IsTerminal;
};

global_variable tokenizer_state AcceptingStates[] = {
#define X(name, _typename, _literal)\
    tokenizer_state_##name,
    tokenizer_state_Accepting_XList
#undef X
};

global_variable tokenizer_state DelimitedStates[] = {
#define X(name, _typename, _literal)\
    tokenizer_state_##name,
    tokenizer_state_Delimited_XList
#undef X
};

global_variable char *DeleteMePlease[] = {
};

/**************************************/
/* Globals */

global_variable u8 TokenizerTable[tokenizer_state__Count][256];
global_variable u8 TheDebugTable[tokenizer_state__Count][256];
global_variable u8 ParserTable[parser_state__Count][token_type__Count];

#define Max_Keywords 100 /* TODO: Please get rid of Max_Keywords :( */
/* From GNU C manual */
global_variable char *GlobalHackedUpKeywords[] = {
#define X(name, _typename, _value)\
    #name,
    Keywords_XList
#undef X

    /*   Here is a list of keywords recognized by ANSI C89: */
/*
  Here is a list of keywords recognized by ANSI C89:
  auto break case char const continue default do double else enum extern
  float for goto if int long register return short signed sizeof static
  struct switch typedef union unsigned void volatile while

  ISO C99 adds the following keywords:
  inline _Bool _Complex _Imaginary

  and GNU extensions add these keywords:
  __FUNCTION__ __PRETTY_FUNCTION__ __alignof __alignof__ __asm
  __asm__ __attribute __attribute__ __builtin_offsetof __builtin_va_arg
  __complex __complex__ __const __extension__ __func__ __imag __imag__
  __inline __inline__ __label__ __null __real __real__
  __restrict __restrict__ __signed __signed__ __thread __typeof
  __volatile __volatile__

  In both ISO C99 and C89 with GNU extensions, the following is also recognized as a keyword:
  restrict
*/
};

ryn_string GlobalKeywordStrings[Max_Keywords] = {};

/**************************************/
/* Functions */

internal void CopyTokenizerTableToTheDebugTable(void)
{
    for (s32 Row = 0; Row < tokenizer_state__Count; ++Row)
    {
        for (s32 Col = 0; Col < 256; ++Col)
        {
            TheDebugTable[Row][Col] = TokenizerTable[Row][Col];
        }
    }
}

internal b32 CheckIfTokenizerTableAndTheDebugTableMatch(void)
{
    b32 Match = 1;

    for (s32 Row = 0; Row < tokenizer_state__Count; ++Row)
    {
        for (s32 Col = 0; Col < 256; ++Col)
        {
            if (TheDebugTable[Row][Col] != TokenizerTable[Row][Col])
            {
                Match = 0;
                break;
            }
        }
    }

    return Match;
}

internal ryn_string GetTokenTypeString(token_type TokenType)
{
    ryn_string String;

    switch (TokenType)
    {
#define X(name, _typename, _literal)\
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
#define X(name, _typename, _literal)\
    case tokenizer_state_##name: String = ryn_string_CreateString(#name); break;
    tokenizer_state_XList;
#undef X
    default: String = ryn_string_CreateString("Unknown token state");
    }

    return String;
}

internal keyword_lookup *BuildKeywordLookup(ryn_memory_arena *Arena)
{
    u32 KeywordCount = ArrayCount(GlobalHackedUpKeywords);
    keyword_lookup *RootLookup = ryn_memory_PushZeroStruct(Arena, keyword_lookup);

    for (u32 I = 0; I < KeywordCount; ++I)
    {
        GlobalKeywordStrings[I] = ryn_string_CreateString(GlobalHackedUpKeywords[I]);
        GlobalKeywordStrings[I].Bytes = GlobalKeywordStrings[I].Bytes + 1;
        GlobalKeywordStrings[I].Size -= 2; /* NOTE: Subtract 2 to ignore hacked prepended underscore and the null-terminator. */
    }

    RootLookup->Char = GlobalKeywordStrings[0].Bytes[0];

    for (u32 KeywordIndex = 0; KeywordIndex < KeywordCount; ++KeywordIndex)
    {
        ryn_string KeywordString = GlobalKeywordStrings[KeywordIndex];
        keyword_lookup *CurrentLookup = RootLookup;

        /* TODO: This control flow for this loop feels off, especially with the mutliple places where we check if we need to set "IsTerminal". */
        for (u32 StringIndex = 0; StringIndex < KeywordString.Size; ++StringIndex)
        {
            u8 Char = KeywordString.Bytes[StringIndex];

            if (CurrentLookup->Char)
            {
                for (;;)
                {
                    if (CurrentLookup->Char == Char)
                    {
                        if (StringIndex == KeywordString.Size - 1)
                        {
                            CurrentLookup->IsTerminal = 1;
                        }
                        else if (CurrentLookup->Child)
                        {
                            CurrentLookup = CurrentLookup->Child;
                        }
                        else
                        {
                            CurrentLookup->Child = ryn_memory_PushZeroStruct(Arena, keyword_lookup);
                            CurrentLookup = CurrentLookup->Child;
                        }

                        break;
                    }
                    else if (CurrentLookup->Sibling == 0)
                    {
                        CurrentLookup->Sibling = ryn_memory_PushZeroStruct(Arena, keyword_lookup);
                        CurrentLookup->Sibling->Char = Char;
                        CurrentLookup = CurrentLookup->Sibling;

                        if (StringIndex == KeywordString.Size - 1)
                        {
                            CurrentLookup->IsTerminal = 1;
                        }

                        CurrentLookup->Child = ryn_memory_PushZeroStruct(Arena, keyword_lookup);
                        CurrentLookup = CurrentLookup->Child;
                        break;
                    }
                    else
                    {
                        CurrentLookup = CurrentLookup->Sibling;
                    }
                }
            }
            else
            {
                Assert(CurrentLookup->Child == 0);
                CurrentLookup->Char = Char;

                if (StringIndex == KeywordString.Size - 1)
                {
                    CurrentLookup->IsTerminal = 1;
                }

                CurrentLookup->Child = ryn_memory_PushZeroStruct(Arena, keyword_lookup);
                CurrentLookup = CurrentLookup->Child;
            }
        }
    }

    return RootLookup;
}

internal b32 StringIsKeyword(keyword_lookup *Lookup, ryn_string String)
{
    b32 IsKeyword = 0;
    keyword_lookup *CurrentLookup = Lookup;
    u64 Size = String.Size;

    if (String.Bytes && String.Bytes[Size - 1] == 0)
    {
        Size -= 1;
    }

    for (u32 I = 0; I < Size; ++I)
    {
        u8 Char = String.Bytes[I];
        b32 ShouldBreak = 0;

        while (CurrentLookup)
        {
            if (Char == CurrentLookup->Char)
            {
                if (I == Size - 1)
                {
                    ShouldBreak = 1;
                }
                else
                {
                    CurrentLookup = CurrentLookup->Child;
                }
                break;
            }
            else
            {
                CurrentLookup = CurrentLookup->Sibling;
            }
        }

        if (ShouldBreak)
        {
            break;
        }
    }

    if (CurrentLookup && CurrentLookup->IsTerminal)
    {
        IsKeyword = 1;
    }

    return IsKeyword;
}

internal void SetupTokenizerTable(void)
{
#define X(name, _typename, _literal)\
    tokenizer_state name = tokenizer_state_##name;
    tokenizer_state_NonError_XList;
#undef X

    for (s32 I = 0; I < 256; ++I)
    {
        for (u32 AcceptIndex = 0; AcceptIndex < ArrayCount(AcceptingStates); ++AcceptIndex)
        {
            tokenizer_state AcceptingState = AcceptingStates[AcceptIndex];
            TokenizerTable[AcceptingState][I] = Done;
        }

        for (u32 DelimitedIndex = 0; DelimitedIndex < ArrayCount(DelimitedStates); ++DelimitedIndex)
        {
            tokenizer_state DelimitedState = DelimitedStates[DelimitedIndex];
            TokenizerTable[DelimitedState][I] = DelimitedState;
        }

        TokenizerTable[CommentBodyCheck][I] = CommentBody;
    }

    for (s32 I = Begin; I < tokenizer_state__Count; ++I)
    {
        TokenizerTable[I][End_Of_Source_Char] = Done;
    }

#define X(_name, typename, literal)\
    TokenizerTable[Begin][literal] = typename;
    SingleCharTokenList;
#undef X

#define X(_name, value)\
    TokenizerTable[Begin][value] = Space;\
    TokenizerTable[Space][value] = Space;
    SpaceCharList;
#undef X

    for (s32 I = 'a'; I <= 'z'; ++I)
    {
        TokenizerTable[Begin][I] = IdentifierStart;
        TokenizerTable[Begin][I-32] = IdentifierStart;

        TokenizerTable[IdentifierStart][I] = IdentifierRest;
        TokenizerTable[IdentifierStart][I-32] = IdentifierRest;

        TokenizerTable[IdentifierRest][I] = IdentifierRest;
        TokenizerTable[IdentifierRest][I-32] = IdentifierRest;
    }

    for (s32 I = '0'; I <= '9'; ++I)
    {
        TokenizerTable[Begin][I] = Digit;
        TokenizerTable[Digit][I] = Digit;

        TokenizerTable[IdentifierRest][I] = IdentifierRest;
        TokenizerTable[IdentifierStart][I] = IdentifierRest;

        TokenizerTable[HexDigitValue][I] = HexDigitValue;
    }

    for (s32 I = 'a'; I <= 'f'; ++I)
    {
        TokenizerTable[HexDigitValue][I] = HexDigitValue;
    }

    TokenizerTable[Begin]['_'] = IdentifierStart;
    TokenizerTable[IdentifierStart]['_'] = IdentifierRest;
    TokenizerTable[IdentifierRest]['_'] = IdentifierRest;

    TokenizerTable[Begin]['0'] = BaseDigit;

    TokenizerTable[BaseDigit]['b'] = BinaryDigitValue;
    TokenizerTable[BaseDigit]['x'] = HexDigitValue;

    TokenizerTable[BinaryDigitValue]['0'] = BinaryDigitValue;
    TokenizerTable[BinaryDigitValue]['1'] = BinaryDigitValue;

    { /* Delimited strings */
        TokenizerTable[Begin]['"'] = String;
        TokenizerTable[String]['\\'] = StringEscape;
        TokenizerTable[String]['"'] = StringEnd;

        TokenizerTable[Begin]['\''] = CharLiteral;
        TokenizerTable[CharLiteral]['\\'] = CharLiteralEscape;
        TokenizerTable[CharLiteral]['\''] = CharLiteralEnd;
    }

#define X(_name, value)\
    TokenizerTable[StringEscape][value] = String;\
    TokenizerTable[CharLiteralEscape][value] = CharLiteral;
    escape_character_XList;
#undef X

    TokenizerTable[Begin]['='] = Equal;
    TokenizerTable[Equal]['='] = DoubleEqual;
    TokenizerTable[LessThan]['='] = LessThanOrEqual;
    TokenizerTable[GreaterThan]['='] = GreaterThanOrEqual;
    TokenizerTable[Begin]['<'] = LessThan;
    TokenizerTable[Begin]['>'] = GreaterThan;
    TokenizerTable[Begin]['!'] = Not;
    TokenizerTable[Not]['='] = NotEqual;
    TokenizerTable[Begin]['-'] = Dash;
    TokenizerTable[Dash]['>'] = Arrow;

    TokenizerTable[Begin]['/'] = ForwardSlash;
    TokenizerTable[ForwardSlash]['*'] = CommentBody;
    TokenizerTable[CommentBody]['*'] = CommentBodyCheck;
    TokenizerTable[CommentBodyCheck]['/'] = Comment;

    TokenizerTable[Begin]['\\'] = TopLevelEscape;
    TokenizerTable[TopLevelEscape]['\n'] = NewlineEscape; /* TODO: Handle CRLF */
    TokenizerTable[Begin]['\n'] = Newline;
}

internal void SetupParserTable(void)
{
    ParserTable[parser_state_Top][token_type_Hash] = parser_state_MacroStart;
}

/* TODO: Rename rows/columns to something related to chars and tokenizer-states. */
internal equivalent_char_result GetEquivalentChars(ryn_memory_arena *Arena, u8 *Table, s32 Rows, s32 Columns)
{
    equivalent_char_result Result = {};

    for (s32 C = 0; C < Columns; ++C)
    {
        if (Result.CharSet == 0)
        {
            /* The first char-set is always an equivalent char, so we always instantly add the first char-set. */
            Result.CharSet = ryn_memory_PushZeroStruct(Arena, char_set);
            Result.CharSet->Column[Result.CharSet->ColumnIndex] = C;
            Result.CharSet->ColumnIndex += 1;
            Result.CharSetCount += 1;
        }
        else
        {
            /* Check if the char-set already exists, if not, add it. */
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
            tokenizer_state NextState = TokenizerTable[R][Char];

            Assert(ClassTableIndex < TableSize);
            Result.Table[ClassTableIndex] = NextState;
        }

        CurrentCharSet = CurrentCharSet->Next;
        CharSetIndex += 1;
    }

    Assert(CharSetIndex == Result.CharSetCount);

    return Result;
}

token_list *Tokenize(ryn_memory_arena *Arena, keyword_lookup *KeywordLookup, ryn_string Source)
{
    token_list HeadToken = {};
    token_list *CurrentToken = &HeadToken;
    tokenizer_state State = tokenizer_state_Begin;
    u64 I = 0;
    u64 StartOfToken = I;
    u8 PreviousChar = Source.Bytes[0];
    b32 EndOfSource = 0;
    equivalent_char_result EquivalentChars = GetEquivalentChars(Arena, (u8 *)TokenizerTable, tokenizer_state__Count, 256);

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

        tokenizer_state NextState = TokenizerTable[State][Char];
        s32 TableIndex = EquivalentChars.CharClass[Char] * tokenizer_state__Count + State;

        { /* Make sure that the equivalent-char table behaves the same as the original table. */
            tokenizer_state TestState = EquivalentChars.Table[TableIndex];
            Assert(NextState == TestState);
        }

        if (NextState == tokenizer_state_Done)
        {
            token_list *NextToken = ryn_memory_PushZeroStruct(Arena, token_list);
            Assert(NextToken != 0);
            NextToken->Token.Type = StateToTypeTable[State];
            CurrentToken = CurrentToken->Next = NextToken;

            if (NextToken->Token.Type == token_type_Identifier)
            {
                NextToken->Token.String.Bytes = Source.Bytes + StartOfToken;
                NextToken->Token.String.Size = I - StartOfToken;

                if (StringIsKeyword(KeywordLookup, NextToken->Token.String))
                {
                    /* TODO: overwrite token-type of identifier and set as type of the particular keyword. */
                }
            }

            StartOfToken = I;

            if (SingleTokenCharTable[PreviousChar])
            {
                NextState = tokenizer_state_Begin;
            }
            else
            {
                NextState = TokenDoneTable[State];
            }
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

internal void Parse(token_list *FirstToken)
{
    token_list *Token = FirstToken;

    while (Token)
    {
        Token = Token->Next;
    }
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
            if (Table[Index] == tokenizer_state_Done)
            {
                printf("*  ");
            }
            else if (Table[Index] != 0)
            {
                printf("%02x ", Table[Index]);
            }
            else
            {
                printf(".  ");
            }

            EquivalentCharCount += 1;
            CurrentCharSet = CurrentCharSet->Next;
        }

        if (R == tokenizer_state_Done)
        {
            printf("   [*  %s]\n", GetTokenizerStateString(R).Bytes);
        }
        else if (R != 0)
        {
            printf("   [%02x %s]\n", R, GetTokenizerStateString(R).Bytes);
        }
        else
        {
            printf("   [.  %s]\n", GetTokenizerStateString(R).Bytes);
        }
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

#define Max_Token_Count_For_Testing 2048

internal void TestTokenizer(ryn_memory_arena *Arena, keyword_lookup *KeywordLookup)
{
    u64 OldArenaOffset = Arena->Offset;
#define X(name, _typename, _literal)\
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
        {ryn_string_CreateString("bar != 0b010110"),
         {T(Identifier), T(Space), T(NotEqual), T(Space), T(BinaryDigit)}},
        {ryn_string_CreateString("bar= 0b010110"),
         {T(Identifier), T(Equal), T(Space), T(BinaryDigit)}},
        {ryn_string_CreateString("bar ||0b010110"),
         {T(Identifier), T(Space), T(Pipe), T(Pipe), T(BinaryDigit)}},
        {ryn_string_CreateString("\"this is a string\""),
         {T(String)}},
        {ryn_string_CreateString("\"string \\t with \\\\ escapes!\""),
         {T(String)}},
        {ryn_string_CreateString("\"escape \\t \\n \\r this\""),
         {T(String)}},
        {ryn_string_CreateString("char A = \'f\'"),
         {T(Identifier), T(Space), T(Identifier), T(Space), T(Equal), T(Space), T(CharLiteral)}},
        {ryn_string_CreateString("= >= == <="),
         {T(Equal), T(Space), T(GreaterThanOrEqual), T(Space), T(DoubleEqual), T(Space), T(LessThanOrEqual)}},
        {ryn_string_CreateString("(&foo, 234, bar.baz)"),
         {T(OpenParenthesis), T(Ampersand), T(Identifier), T(Comma), T(Space), T(Digit), T(Comma), T(Space), T(Identifier), T(Dot), T(Identifier), T(CloseParenthesis)}},
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
        {ryn_string_CreateString("a=4;   \n    b=5;"),
         {T(Identifier), T(Equal), T(Digit), T(Semicolon), T(Space), T(Newline), T(Space), T(Identifier), T(Equal), T(Digit), T(Semicolon)}},
        {ryn_string_CreateString("case A: x=4; break; case B: x=5;"),
         {T(Identifier), T(Space), T(Identifier), T(Colon), T(Space), T(Identifier), T(Equal), T(Digit), T(Semicolon), T(Space), T(Identifier), T(Semicolon), T(Space), T(Identifier), T(Space), T(Identifier), T(Colon), T(Space), T(Identifier), T(Equal), T(Digit), T(Semicolon)}},
        {ryn_string_CreateString("int Foo = Bar ? 3 : 2;"),
         {T(Identifier), T(Space), T(Identifier), T(Space), T(Equal), T(Space), T(Identifier), T(Space), T(Question), T(Space), T(Digit), T(Space), T(Colon), T(Space), T(Digit), T(Semicolon)}},
        {ryn_string_CreateString("A->Size != B->Size"),
         {T(Identifier), T(Arrow), T(Identifier), T(Space), T(NotEqual), T(Space), T(Identifier), T(Arrow), T(Identifier)}},
        {ryn_string_CreateString("!Foo != Bar"),
         {T(Not), T(Identifier), T(Space), T(NotEqual), T(Space), T(Identifier)}},
        {ryn_string_CreateString("auto break case char const continue default do double else enum extern float for goto if int long register return short signed sizeof static struct switch typedef union unsigned void volatile while"),
         {T(_auto), T(Space), T(_break), T(Space), T(_case), T(Space), T(_char), T(Space), T(_const), T(Space), T(_continue), T(Space), T(_default), T(Space), T(_do), T(Space), T(_double), T(Space), T(_else), T(Space), T(_enum), T(Space), T(_extern), T(Space), T(_float), T(Space), T(_for), T(Space), T(_goto), T(Space), T(_if), T(Space), T(_int), T(Space), T(_long), T(Space), T(_register), T(Space), T(_return), T(Space), T(_short), T(Space), T(_signed), T(Space), T(_sizeof), T(Space), T(_static), T(Space), T(_struct), T(Space), T(_switch), T(Space), T(_typedef), T(Space), T(_union), T(Space), T(_unsigned), T(Space), T(_void), T(Space), T(_volatile), T(Space), T(_while)}},

        {FileSourceString, {T(OpenParenthesis)}},
    };

#undef T

    s32 TotalTestCount = ArrayCount(TestCases);
    s32 TotalPassedTests = 0;

    for (s32 I = 0; I < TotalTestCount; ++I)
    {
        u64 TestArenaOffset = Arena->Offset;
        test_case TestCase = TestCases[I];
        token_list *Token = Tokenize(Arena, KeywordLookup, TestCase.Source);
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
        Arena->Offset = TestArenaOffset;
    }

    printf("(%d / %d) passed tests\n", TotalPassedTests, TotalTestCount);
    printf("%d failed\n", TotalTestCount - TotalPassedTests);
    Arena->Offset = OldArenaOffset;
}

internal void TestParser(ryn_memory_arena *Arena, keyword_lookup *KeywordLookup)
{
    u8 *FileSource = ryn_memory_GetArenaWriteLocation(Arena);
    ReadFileIntoAllocator(Arena, (u8 *)"../src/idi.c");
    ryn_string FileSourceString = ryn_string_CreateString((char *)FileSource);

    token_list *Token = Tokenize(Arena, KeywordLookup, FileSourceString);
    while (Token)
    {
        Token = Token->Next;
    }
}

int main(void)
{
    ryn_memory_arena Arena = ryn_memory_CreateArena(Megabytes(1));

    keyword_lookup *KeywordLookup = BuildKeywordLookup(&Arena);
    SetupTokenizerTable();

#if Test_Tokenizer
    TestTokenizer(&Arena, KeywordLookup);
#endif

#if Test_Parser
    TestParser(&Arena, KeywordLookup);
#endif

    printf("\nEquivalent Chars\n");
    s32 Columns = 256;
    printf("\n");
    printf("StateCount = %d\n", tokenizer_state__Count);
    printf("CharCount = %d\n", Columns);
    Assert(tokenizer_state__Count * Columns == sizeof(TokenizerTable));

#if 0
    s32 Rows = tokenizer_state__Count;
    s32 EquivalentCharCount = DebugPrintEquivalentChars(&Arena, (u8 *)TokenizerTable, Rows, Columns);
    printf("EqChar Count %d\n", EquivalentCharCount);
    printf("256 + EqCharCount*StateCount = %d\n", Columns + EquivalentCharCount*tokenizer_state__Count);
#endif

    printf("sizeof(TokenizerTable) %lu\n", sizeof(TokenizerTable));

    printf("token_type__Count %d\n", token_type__Count);

    for (u32 I = 0; I < ArrayCount(DeleteMePlease); ++I)
    {
        printf("keyword: \"%s\"\n", DeleteMePlease[I]);
    }

    return 0;
}
