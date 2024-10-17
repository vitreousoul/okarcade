/*
  TODO:
    [ ] Once things are more stable, rename all the confusing variable names (rows, columns, variable versions of table/class/index).
    [ ] Setup tests against a C lexer (stb or cuik or something) to begin building parse tables for lexing C.
*/

#include <stdlib.h> /* NOTE: stdio and time are included because platform current requires it... */
#include <time.h>

#include "../lib/ryn_macro.h"
#include "../lib/ryn_memory.h"
#include "../lib/ryn_string.h"

#include "../src/types.h"
#include "../src/core.c"
#include "../src/platform.h"

#define Test_Tokenizer     1
#define Test_Preprocessor  1
#define Test_Parser        0

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
    X(Directive,           Directive,           0)\
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
    X(CharLiteral,  CharLiteral,  0)

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
    X(DirectiveEscape,    DirectiveEscape,    0)\
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

#define NonNewlineSpaceCharList\
    X(Space,   ' ')\
    X(Tab,     '\t')\
    X(Return,  '\r')

#define SpaceCharList\
    NonNewlineSpaceCharList\
    X(Newline, '\n')

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

/* Unimplemented preprocessor directives...
     assert Obsolete Features
     pragma endregion {tokens}... Pragmas
     pragma GCC dependency Pragmas
     pragma GCC error Pragmas
     pragma GCC poison Pragmas
     pragma GCC system_header System Headers
     pragma GCC system_header Pragmas
     pragma GCC warning Pragmas
     pragma once Pragmas
     pragma region {tokens}... Pragmas
     unassert Obsolete Features
*/
#define Directives_XList\
    X(_define,        Define,       0)\
    X(_elif,          Elif,         0)\
    X(_else,          Else,         0)\
    X(_endif,         Endif,        0)\
    X(_error,         Error,        0)\
    X(_ident,         Ident,        0)\
    X(_if,            If,           0)\
    X(_ifdef,         Ifdef,        0)\
    X(_ifndef,        Ifndef,       0)\
    X(_import,        Import,       0)\
    X(_include,       Include,      0)\
    X(_include_next,  IncludeNext,  0)\
    X(_line,          Line,         0)\
    X(_sccs,          Sccs,         0)\
    X(_undef,         Undef,        0)\
    X(_warning,       Warning,      0)


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
    X(NewlineEscape,       NewlineEscape,       299) /* TODO: delete newline-escape if we aren't going to use it.... */\
    X(LessThan,            LessThan,            300)\
    X(LessThanOrEqual,     LessThanOrEqual,     301)\
    X(GreaterThan,         GreaterThan,         302)\
    X(GreaterThanOrEqual,  GreaterThanOrEqual,  303)\
    X(NotEqual,            NotEqual,            304)\
    X(Arrow,               Arrow,               305)\
    X(Not,                 Not,                 306)\
    X(Dash,                Dash,                307)\
    X(Newline,             Newline,             308)\
    X(Directive,           Directive,           309)

#define token_type_MaxValue 500 /* TODO: This number can be way less now... */

#define token_type_All_XList\
    X(_Null, _Null, 0)\
    token_type_Valid_XList\
    X(__Error, __Error, token_type_MaxValue)

typedef enum
{
#define X(_name, typename, _literal)\
    token_type_##typename,
    token_type_All_XList
#undef X
    token_type__Count
} token_type;

typedef enum
{
    directive_type__Error,
#define X(_name, typename, _literal)\
    directive_type_##typename,
    Directives_XList
#undef X
    directive_type__Count
} directive_type;

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

ref_struct(lookup_node)
{
    lookup_node *Child;
    lookup_node *Sibling;
    u8 Char;
    b32 IsTerminal;
    u32 Type; /* TODO: Consider a more type-safe option here... */
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

typedef struct
{
    char *CString;
    ryn_string String;
    u64 Type; /* TODO: u64 so we can store either an enum or a pointer. */
} keyword;


/**************************************/
/* Globals */

global_variable u8 TokenizerTable[tokenizer_state__Count][256];
global_variable u8 TheDebugTable[tokenizer_state__Count][256];

#define Max_Keywords 100 /* TODO: Please get rid of Max_Keywords :( */
/* From GNU C manual */
global_variable keyword GlobalHackedUpKeywords[] = {
#define X(name, typename, _value)\
    {#name,{},token_type_##typename},
    Keywords_XList
#undef X
};

global_variable keyword GlobalHackedUpDirectives[] = {
#define X(name, typename, _value)\
    {#name,{},directive_type_##typename},
    Directives_XList
#undef X
};

keyword GlobalKeywordStrings[Max_Keywords] = {};

/**************************************/
/* Functions */

#define DebugPrint(format, ...) printf(format __VA_OPT__(,) __VA_ARGS__)

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
#define X(name, typename, _literal)\
    case token_type_##typename: String = ryn_string_CreateString(#name); break;
    token_type_All_XList;
#undef X
    default: String = ryn_string_CreateString("[Error]");
    }

    return String;
}

internal ryn_string GetDirectiveTypeString(directive_type DirectiveType)
{
    ryn_string String;

    switch (DirectiveType)
    {
#define X(name, typename, _literal)\
    case directive_type_##typename: String = ryn_string_CreateString(#name); break;
    Directives_XList;
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

internal b32 AddToLookup(ryn_memory_arena *Arena, lookup_node *Lookup, keyword Keyword)
{
    b32 KeywordAlreadyExists = 0;
    ryn_string KeywordString = Keyword.String;

    /* TODO: This control flow for this loop feels off, especially with the mutliple places where we check if we need to set "IsTerminal". */
    for (u32 StringIndex = 0; StringIndex < KeywordString.Size; ++StringIndex)
    {
        u8 Char = KeywordString.Bytes[StringIndex];

        if (Lookup->Char)
        {
            for (;;)
            {
                if (Lookup->Char == Char)
                {
                    if (StringIndex == KeywordString.Size - 1)
                    {
                        KeywordAlreadyExists = Lookup->IsTerminal;
                        Lookup->IsTerminal = 1;
                    }
                    else if (Lookup->Child)
                    {
                        Lookup = Lookup->Child;
                    }
                    else
                    {
                        Lookup->Child = ryn_memory_PushZeroStruct(Arena, lookup_node);
                        Lookup = Lookup->Child;
                    }

                    break;
                }
                else if (Lookup->Sibling == 0)
                {
                    Lookup->Sibling = ryn_memory_PushZeroStruct(Arena, lookup_node);
                    Lookup->Sibling->Char = Char;
                    Lookup = Lookup->Sibling;
                    Lookup->Type = Keyword.Type;

                    if (StringIndex == KeywordString.Size - 1)
                    {
                        KeywordAlreadyExists = Lookup->IsTerminal;
                        Lookup->IsTerminal = 1;
                    }

                    Lookup->Child = ryn_memory_PushZeroStruct(Arena, lookup_node);
                    Lookup = Lookup->Child;
                    break;
                }
                else
                {
                    Lookup = Lookup->Sibling;
                }
            }
        }
        else
        {
            Assert(Lookup->Child == 0);
            Lookup->Char = Char;
            Lookup->Type = Keyword.Type;

            if (StringIndex == KeywordString.Size - 1)
            {
                KeywordAlreadyExists = Lookup->IsTerminal;
                Lookup->IsTerminal = 1;
            }

            Lookup->Child = ryn_memory_PushZeroStruct(Arena, lookup_node);
            Lookup = Lookup->Child;
        }
    }

    return KeywordAlreadyExists;
}

internal lookup_node *BuildLookup(ryn_memory_arena *Arena, keyword *Keywords, u32 KeywordCount)
{
    lookup_node *RootLookup = ryn_memory_PushZeroStruct(Arena, lookup_node);

    for (u32 I = 0; I < KeywordCount; ++I)
    {
        GlobalKeywordStrings[I].String = ryn_string_CreateString(Keywords[I].CString);
        GlobalKeywordStrings[I].String.Bytes = GlobalKeywordStrings[I].String.Bytes + 1;
        GlobalKeywordStrings[I].String.Size -= 2; /* NOTE: Subtract 2 to ignore hacked prepended underscore and the null-terminator. */
        GlobalKeywordStrings[I].Type = Keywords[I].Type;
    }

    RootLookup->Char = GlobalKeywordStrings[0].String.Bytes[0];

    for (u32 KeywordIndex = 0; KeywordIndex < KeywordCount; ++KeywordIndex)
    {
        keyword Keyword = GlobalKeywordStrings[KeywordIndex];
        AddToLookup(Arena, RootLookup, Keyword);
    }

    return RootLookup;
}

internal lookup_node LookupString(lookup_node *Lookup, ryn_string String)
{
    lookup_node Result = {};
    lookup_node *CurrentLookup = Lookup;
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

    if (CurrentLookup)
    {
        Result = *CurrentLookup;
    }

    return Result;
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
        TokenizerTable[Directive][I] = Directive;
        TokenizerTable[DirectiveEscape][I] = Directive;

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
    NonNewlineSpaceCharList;
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

        TokenizerTable[Begin]['#'] = Directive;
        TokenizerTable[Directive]['\n'] = Done;
        TokenizerTable[Directive]['\\'] = DirectiveEscape;
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

token_list *Tokenize(ryn_memory_arena *Arena, lookup_node *KeywordLookup, ryn_string Source)
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

            NextToken->Token.String.Bytes = Source.Bytes + StartOfToken;
            NextToken->Token.String.Size = I - StartOfToken;
            StartOfToken = I;

            if (NextToken->Token.Type == token_type_Identifier)
            {
                lookup_node Lookup_Node = LookupString(KeywordLookup, NextToken->Token.String);

                if (Lookup_Node.IsTerminal)
                {
                    /* TODO: overwrite token-type of identifier and set as type of the particular keyword. */
                    NextToken->Token.Type = Lookup_Node.Type;
                }
            }

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
            printf("Tokenizer error! char_index=%llu    state=%d\n", I, State);
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

internal token_list *ConsumeNonNewlineSpace(token_list *Node)
{
    Assert(Node != 0);
    token_list *Result = Node;

    while (Result &&
           (Result->Token.Type == token_type_Space ||
           Result->Token.Type == token_type_NewlineEscape))
    {
        Result = Result->Next;
    }

    return Result;
}

u8 SpaceCharacters[] = {
#define X(_name, literal)\
    literal,
    SpaceCharList
#undef X
};

internal ryn_string GetStringUntilNextWhitespace(ryn_string String)
{
    ryn_string Result = {};
    u32 SpaceCharCount = ArrayCount(SpaceCharacters);
    u32 I = 0;
    b32 IsEscape = 0;
    b32 Looping = 1;

    for (; Looping && I < String.Size; ++I)
    {
        if (IsEscape)
        {
            /* NOTE: Nothing to see here... keep it movin' */
        }
        else
        {
            if (String.Bytes[I] == '\\')
            {
                IsEscape = 1;
            }
            else
            {
                for (u32 J = 0; J < SpaceCharCount; ++J)
                {
                    if (String.Bytes[I] == SpaceCharacters[J])
                    {
                        Looping = 0;
                        break;
                    }
                }
            }
        }
    }

    if (I > 0)
    {
        u32 SizeOffset = Looping ? 0 : 1;  /* NOTE: Subtract one from Size if we broke at a space character, so that we don't include the space in the identifier. If Looping is 1 then we reached the end of the string without hitting space char, so don't subtract anything from Size. */
        Result.Size = I - SizeOffset;
        Result.Bytes = String.Bytes;
    }

    return Result;
}

internal void TrimSpaceAtStart(ryn_string *String)
{
    for (u32 I = 0; I < String->Size; ++I)
    {
        b32 IsSpace = 0;

        for (u32 J = 0; J < ArrayCount(SpaceCharacters); ++J)
        {
            if (String->Bytes[I] == SpaceCharacters[J])
            {
                IsSpace = 1;
                String->Bytes += 1;
                String->Size -= 1;
                break;
            }
        }

        if (!IsSpace)
        {
            break;
        }
    }
}

internal void Preprocess(ryn_memory_arena *Arena, token_list *FirstToken, lookup_node *DirectiveLookup)
{
    token_list *Node = FirstToken;

    while (Node)
    {
        u64 OldOffset = Arena->Offset;

        if (Node->Token.Type == token_type_Directive)
        {
            /* TODO: Parse directive string. */
            ryn_string String = Node->Token.String;

            if (String.Bytes[0] == '#')
            {
                ryn_string StartOfIdentifier = String;
                StartOfIdentifier.Size -= 1;
                StartOfIdentifier.Bytes = StartOfIdentifier.Bytes + 1;

                ryn_string IdentifierString = GetStringUntilNextWhitespace(StartOfIdentifier);
                u8 *CString = Arena->Data + Arena->Offset;
                ryn_string_ToCString(IdentifierString, CString);
                Arena->Offset += IdentifierString.Size;

                lookup_node Lookup = LookupString(DirectiveLookup, IdentifierString);

                if (Lookup.IsTerminal)
                {
                    switch (Lookup.Type)
                    {
                    case directive_type_Define:
                    {
                        printf("Directive type \"Define\"\n");
                    } break;
                    case directive_type_Elif:
                    {
                        printf("Directive type \"Elif\"\n");
                    } break;
                    case directive_type_Else:
                    {
                        printf("Directive type \"Else\"\n");
                    } break;
                    case directive_type_Endif:
                    {
                        printf("Directive type \"Endif\"\n");
                    } break;
                    case directive_type_Error:
                    {
                        printf("Directive type \"Error\"\n");
                    } break;
                    case directive_type_Ident:
                    {
                        printf("Directive type \"Ident\"\n");
                    } break;
                    case directive_type_If:
                    {
                        printf("Directive type \"If\"\n");
                    } break;
                    case directive_type_Ifdef:
                    {
                        printf("Directive type \"Ifdef\"\n");
                    } break;
                    case directive_type_Ifndef:
                    {
                        printf("Directive type \"Ifndef\"\n");
                    } break;
                    case directive_type_Import:
                    {
                        printf("Directive type \"Import\"\n");
                    } break;
                    case directive_type_Include:
                    {
                        printf("Directive type \"Include\"\n");
                        /* NOTE: So far, this is the only directive that cannot be tokenized using the same tokenizer we used to call Preprocess. This is because of the angle-bracket strings for system includes... */
                        ryn_string IncludeString;
                        IncludeString.Size = StartOfIdentifier.Size - IdentifierString.Size;
                        IncludeString.Bytes = StartOfIdentifier.Bytes + IdentifierString.Size;
                        TrimSpaceAtStart(&IncludeString);
                        /* TODO: Parse the include........ */
                        /* ryn_string_ToCString(IncludeString, Arena->Data + Arena->Offset); */
                        /* printf("Include text ........%s\n", Arena->Data + Arena->Offset); */
                    } break;
                    case directive_type_IncludeNext:
                    {
                        printf("Directive type \"IncludeNext\"\n");
                    } break;
                    case directive_type_Line:
                    {
                        printf("Directive type \"Line\"\n");
                    } break;
                    case directive_type_Sccs:
                    {
                        printf("Directive type \"Sccs\"\n");
                    } break;
                    case directive_type_Undef:
                    {
                        printf("Directive type \"Undef\"\n");
                    } break;
                    case directive_type_Warning:
                    {
                        printf("Directive type \"Warning\"\n");
                    } break;
                    }
                }
                else
                {
                    if (ryn_memory_GetArenaFreeSpace(Arena) > IdentifierString.Size)
                    {
                        DebugPrint("[Error] in Preprocess: unknown directive \"%s\"\n", CString);
                    }
                    else
                    {
                        DebugPrint("[Error] in Preprocess: unknown directive\n");
                    }
                }
            }
            else
            {
                DebugPrint("[Error] in Preprocess: Directive does not start with a hash '#'");
            }
        }

        if (Node)
        {
            Node = Node->Next;
        }
        Arena->Offset = OldOffset;
    }
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

internal ryn_string GetIdiSource(ryn_memory_arena *Arena)
{
    u8 *FileSource = ryn_memory_GetArenaWriteLocation(Arena);
    ReadFileIntoAllocator(Arena, (u8 *)"../src/idi.c");
    ryn_string FileSourceString = ryn_string_CreateString((char *)FileSource);
    return FileSourceString;
}

#define Max_Token_Count_For_Testing 2048

typedef struct {
    ryn_string Source;
    token Tokens[Max_Token_Count_For_Testing];
} test_case;

test_case TestCases[50];

#define BeginTestCase() TestIndex = 0; TokenIndex = 0
#define EndTestCase() TestIndex += 1

internal void InitTestCases(void)
{
    u32 TestIndex = 0;
    u32 TokenIndex = 0;

#define T(token_type) TestCases[TestIndex].Tokens[TokenIndex] = (token){token_type,{}}

#define X(name, typename, _literal)\
    token_type name = token_type_##typename;
    token_type_Valid_XList;
#undef X

    BeginTestCase();
    TestCases[TestIndex].Source = ryn_string_CreateString("( 193 + abc)- (as3fj / 423)");
    T(OpenParenthesis); T(Space); T(Digit); T(Space); T(Cross); T(Space); T(Identifier); T(CloseParenthesis); T(Dash); T(Space); T(OpenParenthesis); T(Identifier); T(Space); T(ForwardSlash); T(Space); T(Digit); T(CloseParenthesis);
    EndTestCase();

#if 0
    {"bar = (3*4)^x;",
     {T(Identifier), T(Space), T(Equal), T(Space), T(OpenParenthesis), T(Digit), T(Star), T(Digit), T(CloseParenthesis), T(Carrot), T(Identifier), T(Semicolon)}},
    {"bar != 0b010110",
     {T(Identifier), T(Space), T(NotEqual), T(Space), T(BinaryDigit)}},
    {"bar= 0b010110",
     {T(Identifier), T(Equal), T(Space), T(BinaryDigit)}},
    {"bar ||0b010110",
     {T(Identifier), T(Space), T(Pipe), T(Pipe), T(BinaryDigit)}},
    {"\"this is a string\"",
     {T(String)}},
    {"\"string \\t with \\\\ escapes!\"",
     {T(String)}},
    {"\"escape \\t \\n \\r this\"",
     {T(String)}},
    {"char A = \'f\'",
     {T(_char), T(Space), T(Identifier), T(Space), T(Equal), T(Space), T(CharLiteral)}},
    {"= >= == <=",
     {T(Equal), T(Space), T(GreaterThanOrEqual), T(Space), T(DoubleEqual), T(Space), T(LessThanOrEqual)}},
    {"(&foo, 234, bar.baz)",
     {T(OpenParenthesis), T(Ampersand), T(Identifier), T(Comma), T(Space), T(Digit), T(Comma), T(Space), T(Identifier), T(Dot), T(Identifier), T(CloseParenthesis)}},
    {"/* This is a comment * and is has stars ** in it */",
     {T(Comment)}},
    {"int Foo[3] = {1,2, 4};",
     {T(_int), T(Space), T(Identifier), T(OpenSquare), T(Digit), T(CloseSquare), T(Space), T(Equal), T(Space), T(OpenBracket), T(Digit), T(Comma), T(Digit), T(Comma), T(Space), T(Digit), T(CloseBracket), T(Semicolon)}},
    {"#define Foo 3",
     {T(Directive)}},
    {"((0xabc123<423)>3)",
     {T(OpenParenthesis), T(OpenParenthesis), T(HexDigit), T(LessThan), T(Digit), T(CloseParenthesis), T(GreaterThan), T(Digit), T(CloseParenthesis)}},
    {"#define Foo\\\n3\n#define Bar 4",
     {T(Directive), T(Newline), T(Directive)}},
    {"a=4;   \n    b=5;",
     {T(Identifier), T(Equal), T(Digit), T(Semicolon), T(Space), T(Newline), T(Space), T(Identifier), T(Equal), T(Digit), T(Semicolon)}},
    {"case A: x=4; break; case B: x=5;",
     {T(_case), T(Space), T(Identifier), T(Colon), T(Space), T(Identifier), T(Equal), T(Digit), T(Semicolon), T(Space), T(_break), T(Semicolon), T(Space), T(_case), T(Space), T(Identifier), T(Colon), T(Space), T(Identifier), T(Equal), T(Digit), T(Semicolon)}},
    {"int Foo = Bar ? 3 : 2;",
     {T(_int), T(Space), T(Identifier), T(Space), T(Equal), T(Space), T(Identifier), T(Space), T(Question), T(Space), T(Digit), T(Space), T(Colon), T(Space), T(Digit), T(Semicolon)}},
    {"A->Size != B->Size",
     {T(Identifier), T(Arrow), T(Identifier), T(Space), T(NotEqual), T(Space), T(Identifier), T(Arrow), T(Identifier)}},
    {"!Foo != Bar",
     {T(Not), T(Identifier), T(Space), T(NotEqual), T(Space), T(Identifier)}},
    {"auto break case char const continue default do double else enum extern float for goto if int long register return short signed sizeof static struct switch typedef union unsigned void volatile while",
     {T(_auto), T(Space), T(_break), T(Space), T(_case), T(Space), T(_char), T(Space), T(_const), T(Space), T(_continue), T(Space), T(_default), T(Space), T(_do), T(Space), T(_double), T(Space), T(_else), T(Space), T(_enum), T(Space), T(_extern), T(Space), T(_float), T(Space), T(_for), T(Space), T(_goto), T(Space), T(_if), T(Space), T(_int), T(Space), T(_long), T(Space), T(_register), T(Space), T(_return), T(Space), T(_short), T(Space), T(_signed), T(Space), T(_sizeof), T(Space), T(_static), T(Space), T(_struct), T(Space), T(_switch), T(Space), T(_typedef), T(Space), T(_union), T(Space), T(_unsigned), T(Space), T(_void), T(Space), T(_volatile), T(Space), T(_while)}},
#endif
    /*{FileSourceString, {T(OpenParenthesis)}},*/
#undef T
};

internal void TestTokenizer(ryn_memory_arena *Arena, lookup_node *KeywordLookup)
{
    u64 OldArenaOffset = Arena->Offset;

    ryn_string FileSourceString = GetIdiSource(Arena);
    s32 TotalTestCount = ArrayCount(TestCases);
    s32 TotalPassedTests = 0;

    //TestCases[22].Source = FileSourceString;
    //TestCases[22].Tokens[0] = (token){token_type_OpenParenthesis,{}};

    for (s32 I = 0; I < TotalTestCount; ++I)
    {
        u64 TestArenaOffset = Arena->Offset;
        test_case TestCase = TestCases[I];

        if (TestCase.Source.Size == 0 || TestCase.Source.Bytes == 0)
        {
            continue;
        }

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
                printf("%s ", GetTokenTypeString(Token->Token.Type).Bytes);

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
            printf("******** Pass ********\n");
            TotalPassedTests += 1;
        }
        else
        {
            printf("******** FAIL ******** index=%d   type=%d\n", TestTokenIndex, Token ? Token->Token.Type : 9999999);
        }

        printf("\n\n");
        Arena->Offset = TestArenaOffset;
    }

    printf("(%d / %d) passed tests\n", TotalPassedTests, TotalTestCount);
    printf("%d failed\n", TotalTestCount - TotalPassedTests);
    Arena->Offset = OldArenaOffset;
}

internal void TestParser(ryn_memory_arena *Arena, lookup_node *KeywordLookup)
{
    ryn_string FileSourceString = GetIdiSource(Arena);

    token_list *Token = Tokenize(Arena, KeywordLookup, FileSourceString);
    while (Token)
    {
        Token = Token->Next;
    }
}

int main(void)
{
    ryn_memory_arena Arena = ryn_memory_CreateArena(Megabytes(500));
    ryn_memory_arena StaticArena = ryn_memory_CreateArena(Megabytes(500));

    ryn_string FileSourceString = GetIdiSource(&Arena);
    lookup_node *KeywordLookup = BuildLookup(&StaticArena, GlobalHackedUpKeywords, ArrayCount(GlobalHackedUpKeywords));
    lookup_node *DirectiveLookup = BuildLookup(&StaticArena, GlobalHackedUpDirectives, ArrayCount(GlobalHackedUpDirectives));
    u64 BaseOffset = Arena.Offset;
    SetupTokenizerTable();

#if Test_Tokenizer
    printf("======== Testing Tokenizer ========\n");
    TestTokenizer(&Arena, KeywordLookup);
    printf("\n\n");
    Arena.Offset = BaseOffset;
#endif

#if Test_Preprocessor
    {
        printf("======== Testing Preprocessor ========\n");
        token_list *FirstToken = Tokenize(&Arena, KeywordLookup, FileSourceString);
        Preprocess(&Arena, FirstToken, DirectiveLookup);
        Arena.Offset = BaseOffset;
    }
#endif

#if Test_Parser
    printf("======== Testing Parser ========\n");
    TestParser(&Arena, KeywordLookup);
    Arena.Offset = BaseOffset;
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

    return 0;
}
