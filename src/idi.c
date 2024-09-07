#include "../lib/ryn_macro.h"
#include "../lib/ryn_memory.h"
#include "../lib/ryn_string.h"

#include "../src/types.h"
#include "../src/core.c"

typedef enum
{
    tokenizer_state__Error,
    tokenizer_state_Begin,
    tokenizer_state_Space,
    tokenizer_state_Digit,
    tokenizer_state_Identifier,
    tokenizer_state_Done,
    tokenizer_state__Count,
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

typedef enum
{
    token_type__Null,

#define X(name, literal)\
    token_type_##name = literal,
    SingleCharTokenList
#undef X

    token_type_Space = 256,
    token_type_Digit,
    token_type_Identifier,

    token_type__Error,
} token_type;

global_variable b32 SingleTokenCharTable[256] = {
#define X(name, literal)\
    [token_type_##name] = 1,
    SingleCharTokenList
#undef X
};

ref_struct(token)
{
    token *Next;
    token_type Type;
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

typedef struct
{
    tokenizer_state State;
} token_done_result;

global_variable token_type StateToTypeTable[tokenizer_state__Count] = {
    [tokenizer_state_Space]      = token_type_Space,
    [tokenizer_state_Digit]      = token_type_Digit,
    [tokenizer_state_Identifier] = token_type_Identifier,
};

global_variable tokenizer_state TokenDoneTable[tokenizer_state__Count] = {
    [tokenizer_state_Space]      = tokenizer_state_Begin,
    [tokenizer_state_Digit]      = tokenizer_state_Begin,
    [tokenizer_state_Identifier] = tokenizer_state_Begin,
};

global_variable tokenizer_state TheTable[tokenizer_state__Count][256];

void SetupTheTable(void)
{
    tokenizer_state Begin = tokenizer_state_Begin;
    tokenizer_state Space = tokenizer_state_Space;
    tokenizer_state Digit = tokenizer_state_Digit;
    tokenizer_state Identifier = tokenizer_state_Identifier;
    tokenizer_state Done = tokenizer_state_Done;

    for (s32 I = 0; I < 255; ++I)
    {
        TheTable[Space][I] = Done;
        TheTable[Digit][I] = Done;
        TheTable[Identifier][I] = Done;
    }

#define X(_name, value)                                              \
    TheTable[Begin][value] = Done;
    SingleCharTokenList;
#undef X

#define X(_name, value)                                              \
    TheTable[Begin][value] = Space; \
    TheTable[Space][value] = Space;
    SpaceCharList;
#undef X

    for (s32 I = 'a'; I < 'z'; ++I)
    {
        TheTable[Begin][I] = Identifier;
        TheTable[Begin][I-32] = Identifier;

        TheTable[Identifier][I] = Identifier;
        TheTable[Identifier][I-32] = Identifier;
    }

    for (s32 I = '0'; I < '9'; ++I)
    {
        TheTable[Begin][I] = Digit;
        TheTable[Digit][I] = Digit;
    }
}

token *Tokenize(ryn_memory_arena *Arena, ryn_string Source)
{
    token HeadToken = {};
    token *CurrentToken = &HeadToken;
    tokenizer_state State = tokenizer_state_Begin;
    u64 I = 0;
    u8 PreviousChar = Source.Bytes[0];

    do
    {
        u8 Char = Source.Bytes[I];
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
        else
        {
            ++I;
        }

        State = NextState;
        PreviousChar = Char;
    } while (I < Source.Size && CurrentToken != 0 && State != tokenizer_state__Error);

    return HeadToken.Next;
}

int main(void)
{
    SetupTheTable();
    ryn_string Source = ryn_string_CreateString("( 123 + abc)- (asdlkfj / 423)");
    ryn_memory_arena Arena = ryn_memory_CreateArena(Megabytes(1));

    token *Token = Tokenize(&Arena, Source);

    while (Token)
    {
        printf(SingleTokenCharTable[Token->Type] ? "type=%c\n" : "type=%d\n", Token->Type);
        Token = Token->Next;
    }

    printf("sizeof(TheTable) %lu\n", sizeof(TheTable));

    return 0;
}
