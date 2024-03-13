/*
    TODO: Allow a history of quiz items, so you can scroll back and view previous answers.
    TODO: Use font that supports displaying raw accents, including small-tilde "˜" (not the standard tilde "~")
    TODO: Add upside-down question mark
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "../lib/raylib.h"

typedef uint8_t    u8;
typedef uint32_t   u32;
typedef uint64_t   u64;

typedef int8_t     s8;
typedef int16_t    s16;
typedef int32_t    s32;
typedef int64_t    s64;

typedef uint8_t    b8;
typedef uint32_t   b32;

typedef float      f32;

typedef size_t     size;

#define internal static
#define global_variable static
#define debug_variable static

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#include "raylib_defines.c"
#endif

global_variable int SCREEN_WIDTH = 640;
global_variable int SCREEN_HEIGHT = 400;

#include "math.c"
#include "raylib_helpers.h"

#define TEST 0

#define ArrayCount(a) (sizeof(a)/sizeof((a)[0]))

#define IS_UPPER_CASE(c) ((c) >= 65 && (c) <= 90)

#define SCREEN_HALF_WIDTH (SCREEN_WIDTH / 2)
#define SCREEN_HALF_HEIGHT (SCREEN_HEIGHT / 2)

#define FONT_SIZE 24
#define BACKGROUND_COLOR (Color){40,50,40,255}
#define FONT_COLOR (Color){250,240,245,255}
#define ANSWER_COLOR (Color){200,240,205,255}

#define Is_Utf8_Header_Byte1(b) (((b) & 0x80) == 0x00)
#define Is_Utf8_Header_Byte2(b) (((b) & 0xE0) == 0xC0)
#define Is_Utf8_Header_Byte3(b) (((b) & 0xF0) == 0xE0)
#define Is_Utf8_Header_Byte4(b) (((b) & 0xF8) == 0xF0)
#define Is_Utf8_Tail_Byte(b)    (((b) & 0xC0) == 0x80)

internal void Assert_(b32 FailIfThisIsFalse, u32 Line)
{
    /* TODO: Change Assert so that it also prints out the location of the assertion. */
    int *Foo = 0;

    if (!FailIfThisIsFalse)
    {
        printf("Assertion failed at line %d.\n", Line);
        *Foo = *Foo;
    }
}
#define Assert(F) (Assert_(F, __LINE__))

#define Key_State_Chunk_Size_Log2 5
#define Key_State_Chunk_Size (1 << Key_State_Chunk_Size_Log2)
#define Key_State_Chunk_Size_Mask (Key_State_Chunk_Size - 1)

#define Key_State_Chunk_Count_Log2 5
#define Key_State_Chunk_Count_Mask ((1 << Key_State_Chunk_Count_Log2) - 1)

typedef u32 key_state_chunk;
typedef u32 key_code;

#define Total_Number_Of_Keys_Log2 9
#define Total_Number_Of_Keys (1 << Total_Number_Of_Keys_Log2)

#define Bits_Per_Key_State_Log2 1
#define Bits_Per_Key_State (1 << Bits_Per_Key_State_Log2)
#define Bits_Per_Key_State_Mask ((1 << Bits_Per_Key_State) - 1)

#define Bits_Per_Key_State_Chunk (sizeof(key_state_chunk) * 8)
#define Key_State_Chunk_Count (Total_Number_Of_Keys / (Bits_Per_Key_State_Chunk / Bits_Per_Key_State))

typedef struct
{
    b32 Shift;
    b32 Control;
    b32 Alt;
    b32 Super;
} modifier_keys;

typedef enum
{
    key_Up,
    key_Pressed,
    key_Down,
    key_Released,
    key_state_Count,
    key_state_Undefined,
} key_state;

typedef enum
{
    quiz_mode_Undefined,
    quiz_mode_Typing,
    quiz_mode_Correct,
    quiz_mode_Wrong,
    quiz_mode_Count,
} quiz_mode;

typedef enum
{
    accent_NULL,
    accent_Acute,
    accent_Tilde,
} accent;

typedef struct
{
    f32 DeltaTime;
    f32 LastTime;

    u32 KeyStateIndex;
    key_state_chunk KeyStates[2][Key_State_Chunk_Count];
#define Key_Press_Queue_Size 16
    u32 KeyPressQueue[Key_Press_Queue_Size];

    modifier_keys ModifierKeys;

    accent CurrentAccent;
    b32 AccentMode;
    b32 ShowAnswer;

    key_code LastKeyPressed;
    f32 KeyRepeatTime;
    b32 KeyHasRepeated;

#define Test_Buffer_Count 64
    char QuizInputBuffer[Test_Buffer_Count + 1];
    s32 QuizInputBufferIndex;

    u32 QuizItemIndex;
    quiz_mode QuizMode;
} state;

typedef struct
{
    s16 Index;
    s16 Shift;
} key_location;

typedef struct
{
    char *Prompt;
    char *ExpectedAnswer;
} quiz_item;

#define Quiz_Item_Max 256
global_variable quiz_item QuizItems[Quiz_Item_Max];

global_variable key_state KeyStateMap[key_state_Count][2] = {
    [key_Up]       =  {key_Up      ,  key_Pressed},
    [key_Pressed]  =  {key_Released,  key_Down},
    [key_Down]     =  {key_Released,  key_Down},
    [key_Released] =  {key_Up      ,  key_Pressed},
};

global_variable b32 PrintableKeys[Total_Number_Of_Keys] = {
    [KEY_SPACE] = 1,
    [KEY_APOSTROPHE] = 1,
    [KEY_COMMA] = 1,
    [KEY_MINUS] = 1,
    [KEY_PERIOD] = 1,
    [KEY_SLASH] = 1,
    [KEY_ZERO] = 1,
    [KEY_ONE] = 1,
    [KEY_TWO] = 1,
    [KEY_THREE] = 1,
    [KEY_FOUR] = 1,
    [KEY_FIVE] = 1,
    [KEY_SIX] = 1,
    [KEY_SEVEN] = 1,
    [KEY_EIGHT] = 1,
    [KEY_NINE] = 1,
    [KEY_SEMICOLON] = 1,
    [KEY_EQUAL] = 1,
    [KEY_A] = 1,
    [KEY_B] = 1,
    [KEY_C] = 1,
    [KEY_D] = 1,
    [KEY_E] = 1,
    [KEY_F] = 1,
    [KEY_G] = 1,
    [KEY_H] = 1,
    [KEY_I] = 1,
    [KEY_J] = 1,
    [KEY_K] = 1,
    [KEY_L] = 1,
    [KEY_M] = 1,
    [KEY_N] = 1,
    [KEY_O] = 1,
    [KEY_P] = 1,
    [KEY_Q] = 1,
    [KEY_R] = 1,
    [KEY_S] = 1,
    [KEY_T] = 1,
    [KEY_U] = 1,
    [KEY_V] = 1,
    [KEY_W] = 1,
    [KEY_X] = 1,
    [KEY_Y] = 1,
    [KEY_Z] = 1,
    [KEY_LEFT_BRACKET] = 1,
    [KEY_BACKSLASH] = 1,
    [KEY_RIGHT_BRACKET] = 1,
    [KEY_GRAVE] = 1,
};


internal inline key_location GetKeyLocation(u32 KeyNumber)
{
     key_location Location;

     Location.Index = Key_State_Chunk_Count_Mask & (KeyNumber >> (Key_State_Chunk_Size_Log2 - Bits_Per_Key_State_Log2));
     Location.Shift = Key_State_Chunk_Size_Mask & (KeyNumber << Bits_Per_Key_State_Log2);

    if (Location.Index >= (s32)Key_State_Chunk_Count)
    {
        Location.Index = -1;
        Location.Shift = -1;
    }

    return Location;
}

internal inline b32 KeyLocationIsValid(key_location Location)
{
    return Location.Index >= 0 && Location.Shift >= 0;
}

internal key_state GetKeyState(state *State, u32 KeyStateIndex, u32 KeyNumber)
{
    Assert(KeyNumber < Total_Number_Of_Keys);
    key_state KeyState = key_state_Undefined;

    key_location Location = GetKeyLocation(KeyNumber);

    if (KeyLocationIsValid(Location))
    {
        u32 KeyStateChunk = State->KeyStates[KeyStateIndex][Location.Index];
        KeyState = (KeyStateChunk >> Location.Shift) & Bits_Per_Key_State_Mask;
    }

    return KeyState;
}

internal b32 SetKeyState(state *State, u32 KeyStateIndex, u32 KeyNumber, key_state KeyState)
{
    b32 Error = 0;
    key_location Location = GetKeyLocation(KeyNumber);
    b32 Valid = KeyLocationIsValid(Location);

    if (Valid)
    {
        Assert(KeyNumber < Total_Number_Of_Keys);
        Assert(KeyState < key_state_Count);

        u32 KeyStateChunk = State->KeyStates[KeyStateIndex][Location.Index];
        u32 UnsetMask = ~(Bits_Per_Key_State_Mask << Location.Shift);
        u32 NewChunk = (KeyStateChunk & UnsetMask) | (KeyState << Location.Shift);

        State->KeyStates[KeyStateIndex][Location.Index] = NewChunk;
    }
    else
    {
        Error = 1;
    }

    return Error;
}

internal inline void UpdateKey(state *State, u32 Key)
{
    u32 NextKeyStateIndex = !State->KeyStateIndex;
    b32 IsDown = IsKeyDown(Key);
    key_state CurrentState = GetKeyState(State, State->KeyStateIndex, Key);
    key_state NextState = KeyStateMap[CurrentState][IsDown];

    SetKeyState(State, NextKeyStateIndex, Key, NextState);
}

internal void ClearAccentKey(state *State)
{
    switch (State->CurrentAccent)
    {
    case accent_Acute:
    case accent_Tilde:
    {
        State->QuizInputBuffer[State->QuizInputBufferIndex] = 0;
        if (State->QuizInputBufferIndex + 1 < Test_Buffer_Count)
        {
            State->QuizInputBuffer[State->QuizInputBufferIndex+1] = 0;
        }
        State->AccentMode = 0;
        State->CurrentAccent = 0;
    } break;
    default:
    {
        Assert(!"Not Implemented.");
    }
    }
}

internal void HandleKey(state *State, key_code Key)
{
    if (PrintableKeys[Key])
    {
        if (State->QuizInputBufferIndex + 1 < Test_Buffer_Count)
        {
            if (State->AccentMode)
            {
                unsigned char SecondByte = 0;

                if (State->QuizInputBufferIndex + 2 < Test_Buffer_Count)
                {
                    if (State->CurrentAccent == accent_Acute)
                    {
                        b32 DoNotUpdate = 0;

                        switch (Key)
                        {
                        case KEY_A: SecondByte = 0x81; break;
                        case KEY_E: SecondByte = 0x89; break;
                        case KEY_I: SecondByte = 0x8d; break;
                        case KEY_O: SecondByte = 0x93; break;
                        case KEY_U: SecondByte = 0x9a; break;
                        default: DoNotUpdate = 1; break;
                        }

                        if (DoNotUpdate)
                        {
                            ClearAccentKey(State);
                        }
                        else
                        {
                            if (!State->ModifierKeys.Shift)
                            {
                                SecondByte += 32;
                            }

                            State->QuizInputBuffer[State->QuizInputBufferIndex] = 0xC3;
                            State->QuizInputBuffer[State->QuizInputBufferIndex+1] = SecondByte;
                            State->QuizInputBufferIndex += 2;
                        }
                    }
                    else if (State->CurrentAccent == accent_Tilde && Key == KEY_N)
                    {
                        char SecondByte = 0x91;

                        if (!State->ModifierKeys.Shift)
                        {
                            SecondByte += 32;
                        }

                        State->QuizInputBuffer[State->QuizInputBufferIndex] = 0xC3;
                        State->QuizInputBuffer[State->QuizInputBufferIndex+1] = SecondByte;
                        State->QuizInputBufferIndex += 2;
                    }
                    else
                    {
                        ClearAccentKey(State);
                    }
                }

                State->AccentMode = 0;
                State->CurrentAccent = 0;
            }
            else if (State->ModifierKeys.Alt)
            {
                switch (Key)
                {
                case KEY_E:
                {
                    if (State->QuizInputBufferIndex + 2 < Test_Buffer_Count)
                    {
                        /* NOTE: Acute accent */
                        State->QuizInputBuffer[State->QuizInputBufferIndex] = 0xC2;
                        State->QuizInputBuffer[State->QuizInputBufferIndex+1] = 0xB4;
                        State->AccentMode = 1;
                        State->CurrentAccent = accent_Acute;
                    }
                } break;
                case KEY_N:
                {
                    if (State->QuizInputBufferIndex + 2 < Test_Buffer_Count)
                    {
                        /* NOTE: Tilde */
                        /* State->QuizInputBuffer[State->QuizInputBufferIndex] = '~'; */
                        State->QuizInputBuffer[State->QuizInputBufferIndex] = 0xCB;
                        State->QuizInputBuffer[State->QuizInputBufferIndex+1] = 0x9C;
                        State->AccentMode = 1;
                        State->CurrentAccent = accent_Tilde;
                    }

                } break;
                }
            }
            else if (State->ModifierKeys.Shift && Key == KEY_SLASH)
            {
                State->QuizInputBuffer[State->QuizInputBufferIndex] = '?';
                State->QuizInputBufferIndex = State->QuizInputBufferIndex + 1;
            }
            else
            {
                if (!State->ModifierKeys.Shift && IS_UPPER_CASE(Key))
                {
                    Key += 32;
                }

                State->QuizInputBuffer[State->QuizInputBufferIndex] = Key;
                State->QuizInputBufferIndex = State->QuizInputBufferIndex + 1;
            }
        }
    }
    else
    {
        switch (Key)
        {
        case KEY_BACKSPACE:
        {
            if (State->AccentMode)
            {
                ClearAccentKey(State);
            }
            else
            {
                int TailBytesSkipped = 0;

                for (;;)
                {
                    if (State->QuizInputBufferIndex > 0)
                    {
                        State->QuizInputBufferIndex = State->QuizInputBufferIndex - 1;
                    }

                    if (State->QuizInputBufferIndex < 0)
                    {
                        break;
                    }

                    char Char = State->QuizInputBuffer[State->QuizInputBufferIndex];

                    if (Is_Utf8_Tail_Byte(Char))
                    {
                        State->QuizInputBuffer[State->QuizInputBufferIndex] = 0;
                        TailBytesSkipped += 1;
                    }
                    else if (TailBytesSkipped == 1 && Is_Utf8_Header_Byte2(Char))
                    {
                        Assert(TailBytesSkipped == 1);
                        if (State->QuizInputBufferIndex < 0) break;
                        State->QuizInputBuffer[State->QuizInputBufferIndex] = 0;
                        break;
                    }
                    else if (TailBytesSkipped == 2 && Is_Utf8_Header_Byte3(Char))
                    {
                        Assert(!"Three-byte characters have not been tested. Delete this assertion to begin testing three-byte characters.");
                        if (State->QuizInputBufferIndex < 0) break;
                        State->QuizInputBuffer[State->QuizInputBufferIndex] = 0;
                        break;
                    }
                    else if (TailBytesSkipped == 3 && Is_Utf8_Header_Byte4(Char))
                    {
                        Assert(!"Four-byte characters have not been tested. Delete this assertion to begin testing four-byte characters.");
                        if (State->QuizInputBufferIndex < 0) break;
                        State->QuizInputBuffer[State->QuizInputBufferIndex] = 0;
                        break;
                    }
                    else
                    {
                        /* NOTE: Delete an ASCII character. */
                        State->QuizInputBuffer[State->QuizInputBufferIndex] = 0;
                        break;
                    }
                }
            }
        } break;
        }
    }
}

internal void HandleUserInput(state *State)
{
    u32 Key;
    u32 QueueIndex = 0;
    b32 KeysWerePressed = 0;

    {
        State->ModifierKeys.Shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        State->ModifierKeys.Control = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        State->ModifierKeys.Alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
        State->ModifierKeys.Super = IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
    }

    for (u32 I = 0; I < Key_Press_Queue_Size; ++I)
    {
        State->KeyPressQueue[I] = 0;
    }

    /* NOTE: Loop through the queue of pressed keys, for operations that rely
       on the order of keys pressed.
    */
    for (;;)
    {
        b32 IsTyping = State->QuizMode == quiz_mode_Typing;
        Key = GetKeyPressed();
        b32 QueueNotEmpty = QueueIndex < Key_Press_Queue_Size;

        if (!(IsTyping && Key && QueueNotEmpty))
        {
            break;
        }

        if (State->ModifierKeys.Control && Key == KEY_H)
        {
            State->ShowAnswer = 1;
        }
        else
        {
            State->KeyPressQueue[QueueIndex] = Key;
            QueueIndex += 1;
            KeysWerePressed = 1;
            State->KeyRepeatTime = 0.0f;
            State->KeyHasRepeated = 0;
            State->LastKeyPressed = Key;

            HandleKey(State, Key);
        }
    }

    if (!KeysWerePressed && IsKeyDown(State->LastKeyPressed))
    {
        State->KeyRepeatTime += State->DeltaTime;

        b32 InitialKeyRepeat = !State->KeyHasRepeated && State->KeyRepeatTime > 0.3f;
        /* NOTE: If the key repeat time is short enough, we may need to handle multiple repeats in a single frame. */
        b32 TheOtherKeyRepeats = State->KeyHasRepeated && State->KeyRepeatTime > 0.04f;

        if (InitialKeyRepeat || TheOtherKeyRepeats)
        {
            State->KeyRepeatTime = 0.0f;
            State->KeyHasRepeated = 1;
            HandleKey(State, State->LastKeyPressed);
        }
    }

    for (u32 Key = 0; Key < Total_Number_Of_Keys; ++Key)
    {
        UpdateKey(State, Key);
    }
}

internal b32 StringsMatch(char *A, char *B)
{
    if (!A ^ !B)
    {
        return 0;
    }

    for (s32 I = 0; ; ++I)
    {
        if (A[I] != B[I])
        {
            return 0;
        }

        if (!A[I] && !B[I])
        {
            return 1;
        }
        else if (!A[I] || !B[I])
        {
            return 0;
        }
    }
}

internal inline u32 GetRandomQuizItemIndex(state *State)
{
    u32 RandomIndex;
    u32 LastIndex = State->QuizItemIndex;

    u32 QuizItemCount;
    for (QuizItemCount = 0; QuizItemCount < Quiz_Item_Max; ++QuizItemCount)
    {
        if (!QuizItems[QuizItemCount].Prompt || !QuizItems[QuizItemCount].ExpectedAnswer)
        {
            break;
        }
    }

    u32 MaxTries = 32;
    for (u32 I = 0; I < MaxTries; ++I)
    {
        RandomIndex = rand() % QuizItemCount;

        if (RandomIndex != LastIndex)
        {
            break;
        }
    }

    return RandomIndex;
}

internal void GetNextRandomQuizItem(state *State)
{
    State->QuizItemIndex = GetRandomQuizItemIndex(State);
    State->ShowAnswer = 0;

    for (s32 I = 0; I <= State->QuizInputBufferIndex; ++I)
    {
        State->QuizInputBuffer[I] = 0;
    }
    State->QuizInputBufferIndex = 0;
    State->QuizMode = quiz_mode_Typing;
}

internal void UpdateAndRender(state *State)
{
    HandleUserInput(State);

    State->KeyStateIndex = !State->KeyStateIndex;

    char *Prompt = QuizItems[State->QuizItemIndex].Prompt;
    char *ExpectedAnswer = QuizItems[State->QuizItemIndex].ExpectedAnswer;

    f32 PromptOffset = MeasureText(Prompt, FONT_SIZE) / 2.0f;
    f32 InputOffset = MeasureText(ExpectedAnswer, FONT_SIZE) / 2.0f;

    if (StringsMatch(ExpectedAnswer, State->QuizInputBuffer))
    {
        State->QuizMode = quiz_mode_Correct;
    }

    if (State->QuizMode == quiz_mode_Correct)
    {
        DrawText("Correct", 20, 20, FONT_SIZE, (Color){20, 200, 40, 255});

        if (IsKeyDown(KEY_ENTER))
        {
            GetNextRandomQuizItem(State);
        }
    }

    if (State->ShowAnswer)
    {
        DrawText(ExpectedAnswer, SCREEN_HALF_WIDTH - InputOffset, SCREEN_HALF_HEIGHT - 48, FONT_SIZE, ANSWER_COLOR);
    }

    if (IsKeyPressed(KEY_RIGHT))
    {
        GetNextRandomQuizItem(State);
    }

    BeginDrawing();
    ClearBackground(BACKGROUND_COLOR);

    { /* Draw the index of the quiz item. */
        char Buff[16];
        sprintf(Buff, "%d", State->QuizItemIndex);
        DrawText(Buff, 20, 48, FONT_SIZE, FONT_COLOR);
    }

    DrawText(Prompt, SCREEN_HALF_WIDTH - PromptOffset, SCREEN_HALF_HEIGHT - 24, FONT_SIZE, FONT_COLOR);
    DrawText(State->QuizInputBuffer, SCREEN_HALF_WIDTH - InputOffset, SCREEN_HALF_HEIGHT + 24, FONT_SIZE, FONT_COLOR);


    EndDrawing();
}

int main(void)
{
    Assert(key_state_Count >= (1 << Bits_Per_Key_State) - 1);
    Assert((1 << Key_State_Chunk_Size_Log2) == 8 * sizeof(key_state_chunk));
    Assert((1 << Key_State_Chunk_Count_Log2) == Key_State_Chunk_Count);

#if defined(PLATFORM_WEB)
    InitRaylibCanvas();
#endif

    int Result = 0;
    state State = {0};

    State.QuizMode = quiz_mode_Typing;

    srand(time(NULL));

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Estudioso");
    SetTargetFPS(60);

    for (;;)
    {
        if (WindowShouldClose())
        {
            break;
        }

        { /* time calculation */
            f32 CurrentTime = GetTime();
            State.DeltaTime = CurrentTime - State.LastTime;
            State.LastTime = CurrentTime;
        }

        UpdateAndRender(&State);
    }

    CloseWindow();

    return Result;
}

global_variable quiz_item QuizItems[Quiz_Item_Max] = {
    {
        "In the morning the weather is good in April.",
        "En la mañana hace buen tiempo en abril."
    },
    {
        "He is very cold.",
        "Él hace mucho frio"
    },
    {
        "_ restaurante",
        "el"
    },
    {
        "_ café",
        "el"
    },
    {
        "_ coche",
        "el"
    },
    {
        "_ viaje",
        "el"
    },
    {
        "_ parque",
        "el"
    },
    {
        "_ paquete",
        "el"
    },
    {
        "_ menú",
        "el"
    },
    {
        "_ colibrí",
        "el"
    },
    {
        "_ reloj",
        "el"
    },
    {
        "_ chocolate",
        "el"
    },
    {
        "_ pie",
        "el"
    },
    {
        "_ diente",
        "el"
    },
    {
        "_ noche",
        "la"
    },
    {
        "_ sangre",
        "la"
    },
    {
        "_ nube",
        "la"
    },
    {
        "_ llave",
        "la"
    },
    {
        "_ calle",
        "la"
    },
    {
        "_ gente",
        "la"
    },
    {
        "_ nieve",
        "la"
    },
    {
        "_ leche",
        "la"
    },
    {
        "_ carne",
        "la"
    },
    {
        "_ ley",
        "la"
    },
    {
        "_ imagen",
        "la"
    },
    {
        "Félix and Raúl are tall.",
        "Félix y Raúl son altos."
    },
    {
        "Antón is very nice.",
        "Antón es muy simpático."
    },
    {
        "I am Santiago.",
        "Yo soy Santiago."
    },
    {
        "This is the Royal Theater.",
        "Este es el Teatro Real."
    },
    {
        "Carlos' family is Catholic.",
        "La familia de Carlos es católica."
    },
    {
        "That sheet is from Japan.",
        "Esa lámina es de Japón."
    },
    {
        "Tatiana and Sarai are my sisters.",
        "Tatiana y Sarai son mis hermanas."
    },
    {
        "These are my friends.",
        "Estos son mis amigos."
    },
    {
        "Elisa is my ex-girlfriend.",
        "Elisa es mi exnovia."
    },
    {
        "That umbrella is mine.",
        "Ese paraguas es mío."
    },
    {
        "The soccor game is in Valencia.",
        "El partido de fútbol es en Valencia."
    },
    {
        "The game is on Wednesday.",
        "El partido es el miércoles."
    },
    {
        "Today is Sunday.",
        "Hoy es domingo."
    },
    {
        "Today is April 1.",
        "Hoy es 1 de abril."
    },
    {
        "It's Spring.",
        "Es primavera."
    },
    {
        "What time is it? It's ten o-clock.",
        /* "¿Que hora es? Son las diez." */
        "Que hora es? Son las diez."
    }
};
