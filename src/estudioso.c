/*
    TODO: Allow a history of quiz items, so you can scroll back and view previous answers.
    TODO: Keep track of what quiz-items have occured and don't show repeat items. (related to item history above)
    TODO: Add upside-down question mark
    TODO: Allow moving cursor between characters and splice editing. Currently cursor is always at the end of the input.
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
#endif

#ifdef TARGET_SCREEN_WIDTH
global_variable int SCREEN_WIDTH = TARGET_SCREEN_WIDTH;
#else
global_variable int SCREEN_WIDTH = 1280;
#endif

#ifdef TARGET_SCREEN_HEIGHT
global_variable int SCREEN_HEIGHT = TARGET_SCREEN_HEIGHT;
#else
global_variable int SCREEN_HEIGHT = 800;
#endif


#include "../gen/roboto_regular.h"

#include "math.c"
#include "raylib_helpers.h"
#include "ui.c"

#define TEST 0

#define ArrayCount(a) (sizeof(a)/sizeof((a)[0]))

#define IS_UPPER_CASE(c) ((c) >= 65 && (c) <= 90)

#define SCREEN_HALF_WIDTH (SCREEN_WIDTH / 2)
#define SCREEN_HALF_HEIGHT (SCREEN_HEIGHT / 2)

#define BACKGROUND_COLOR (Color){40,50,40,255}
#define FONT_COLOR (Color){250,240,245,255}
#define ANSWER_COLOR (Color){200,240,205,255}
#define CONJUGATION_TYPE_COLOR (Color){240,240,205,255}

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
    quiz_mode_Win,
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

    b32 InputOccured;
    key_code LastKeyPressed;
    f32 KeyRepeatTime;
    b32 KeyHasRepeated;

#define Test_Buffer_Count 64
    char QuizInput[Test_Buffer_Count + 1];
    s32 QuizInputIndex;

    s32 QuizItemIndex;
    quiz_mode QuizMode;

    ui UI;
} state;

typedef enum
{
    conjugation_Null,
    conjugation_Presente,
    conjugation_Imperfecto,
    conjugation_Indefinido,
    conjugation_Futuro,
    conjugation_Count,
} conjugation;

typedef struct
{
    s16 Index;
    s16 Shift;
} key_location;

typedef enum
{
    quiz_item_Null,
    quiz_item_Text,
    quiz_item_Conjugation,
    quiz_item_Count,
} quiz_item_type;

typedef struct
{
    char *Prompt;
    char *Answer;
} quiz_text;

typedef struct
{
    conjugation Conjugation;
    char *Prompt;
    char *Answer;
} quiz_conjugation;

typedef struct
{
    quiz_item_type Type;
    b32 Complete;
    union
    {
        quiz_text Text;
        quiz_conjugation Conjugation;
    };
} quiz_item;

typedef enum
{
    ui_NULL,
    ui_Next,
    ui_Previous,
    ui_ShowAnswer,
} estudioso_ui_ids;

#define Quiz_Item_Max 256
global_variable quiz_item QuizItems[Quiz_Item_Max];
global_variable u32 QuizItemCount = 0;

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

internal void ClearAccentKey(state *State)
{
    switch (State->CurrentAccent)
    {
    case accent_Acute:
    case accent_Tilde:
    {
        State->QuizInput[State->QuizInputIndex] = 0;
        if (State->QuizInputIndex + 1 < Test_Buffer_Count)
        {
            State->QuizInput[State->QuizInputIndex+1] = 0;
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
    State->InputOccured = 1;

    if (PrintableKeys[Key])
    {
        if (State->QuizInputIndex + 1 < Test_Buffer_Count)
        {
            if (State->AccentMode)
            {
                unsigned char SecondByte = 0;

                if (State->QuizInputIndex + 2 < Test_Buffer_Count)
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

                            State->QuizInput[State->QuizInputIndex] = 0xC3;
                            State->QuizInput[State->QuizInputIndex+1] = SecondByte;
                            State->QuizInputIndex += 2;
                        }
                    }
                    else if (State->CurrentAccent == accent_Tilde && Key == KEY_N)
                    {
                        char SecondByte = 0x91;

                        if (!State->ModifierKeys.Shift)
                        {
                            SecondByte += 32;
                        }

                        State->QuizInput[State->QuizInputIndex] = 0xC3;
                        State->QuizInput[State->QuizInputIndex+1] = SecondByte;
                        State->QuizInputIndex += 2;
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
                    if (State->QuizInputIndex + 2 < Test_Buffer_Count)
                    {
                        /* NOTE: Acute accent */
                        State->QuizInput[State->QuizInputIndex] = 0xC2;
                        State->QuizInput[State->QuizInputIndex+1] = 0xB4;
                        State->AccentMode = 1;
                        State->CurrentAccent = accent_Acute;
                    }
                } break;
                case KEY_N:
                {
                    if (State->QuizInputIndex + 2 < Test_Buffer_Count)
                    {
                        /* NOTE: Tilde */
                        /* State->QuizInput[State->QuizInputIndex] = '~'; */
                        State->QuizInput[State->QuizInputIndex] = 0xCB;
                        State->QuizInput[State->QuizInputIndex+1] = 0x9C;
                        State->AccentMode = 1;
                        State->CurrentAccent = accent_Tilde;
                    }

                } break;
                }
            }
            else if (State->ModifierKeys.Shift && Key == KEY_SLASH)
            {
                State->QuizInput[State->QuizInputIndex] = '?';
                State->QuizInputIndex = State->QuizInputIndex + 1;
            }
            else
            {
                if (!State->ModifierKeys.Shift && IS_UPPER_CASE(Key))
                {
                    Key += 32;
                }

                State->QuizInput[State->QuizInputIndex] = Key;
                State->QuizInputIndex = State->QuizInputIndex + 1;
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
                    if (State->QuizInputIndex > 0)
                    {
                        State->QuizInputIndex = State->QuizInputIndex - 1;
                    }

                    if (State->QuizInputIndex < 0)
                    {
                        break;
                    }

                    char Char = State->QuizInput[State->QuizInputIndex];

                    if (Is_Utf8_Tail_Byte(Char))
                    {
                        State->QuizInput[State->QuizInputIndex] = 0;
                        TailBytesSkipped += 1;
                    }
                    else if (TailBytesSkipped == 1 && Is_Utf8_Header_Byte2(Char))
                    {
                        Assert(TailBytesSkipped == 1);
                        if (State->QuizInputIndex < 0) break;
                        State->QuizInput[State->QuizInputIndex] = 0;
                        break;
                    }
                    else if (TailBytesSkipped == 2 && Is_Utf8_Header_Byte3(Char))
                    {
                        Assert(!"Three-byte characters have not been tested. Delete this assertion to begin testing three-byte characters.");
                        if (State->QuizInputIndex < 0) break;
                        State->QuizInput[State->QuizInputIndex] = 0;
                        break;
                    }
                    else if (TailBytesSkipped == 3 && Is_Utf8_Header_Byte4(Char))
                    {
                        Assert(!"Four-byte characters have not been tested. Delete this assertion to begin testing four-byte characters.");
                        if (State->QuizInputIndex < 0) break;
                        State->QuizInput[State->QuizInputIndex] = 0;
                        break;
                    }
                    else
                    {
                        /* NOTE: Delete an ASCII character. */
                        State->QuizInput[State->QuizInputIndex] = 0;
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

    {
        State->UI.MouseButtonPressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        State->UI.MouseButtonReleased = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
        State->UI.MousePosition = GetMousePosition();
    }

    for (u32 I = 0; I < Key_Press_Queue_Size; ++I)
    {
        State->KeyPressQueue[I] = 0;
    }

    b32 IsTyping = State->QuizMode == quiz_mode_Typing;
    /* NOTE: Loop through the queue of pressed keys, for operations that rely
       on the order of keys pressed.
    */
    for (;;)
    {
        Key = GetKeyPressed();
        b32 QueueNotEmpty = QueueIndex < Key_Press_Queue_Size;

        if (!(IsTyping && Key && QueueNotEmpty))
        {
            break;
        }

        if (State->ModifierKeys.Control && Key == KEY_H)
        {
            State->ShowAnswer = 1;
            State->InputOccured = 1;
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
        /* NOTE: Handle keyboard repeat */
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
        /* NOTE: Update state for each key */
        u32 NextKeyStateIndex = !State->KeyStateIndex;
        b32 IsDown = IsKeyDown(Key);
        key_location Location = GetKeyLocation(Key);
        key_state CurrentState = key_state_Undefined;

        if (KeyLocationIsValid(Location))
        {
            u32 KeyStateChunk = State->KeyStates[State->KeyStateIndex][Location.Index];
            key_state NextState = KeyStateMap[CurrentState][IsDown];
            CurrentState = (KeyStateChunk >> Location.Shift) & Bits_Per_Key_State_Mask;

            Assert(NextState < key_state_Count);

            if (CurrentState != key_Up || NextState != key_Up)
            {
                /* TODO: do we really want to set InputOccured to 1 here??? */
                State->InputOccured = 1;
            }

            u32 UnsetMask = ~(Bits_Per_Key_State_Mask << Location.Shift);
            u32 NewChunk = (KeyStateChunk & UnsetMask) | (NextState << Location.Shift);

            State->KeyStates[NextKeyStateIndex][Location.Index] = NewChunk;
        }
        else
        {
            /* NOTE: Error */
        }
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

internal u32 GetQuizItemCount()
{
    u32 QuizItemCount;
    for (QuizItemCount = 0; QuizItemCount < Quiz_Item_Max; ++QuizItemCount)
    {
        quiz_item QuizItem = QuizItems[QuizItemCount];

        if (QuizItem.Type == quiz_item_Text)
        {
            if (!(QuizItem.Text.Prompt && QuizItem.Text.Answer))
            {
                break;
            }
        }
        else if (QuizItem.Type == quiz_item_Conjugation)
        {

        }
        else
        {
            break;
        }
    }

    return QuizItemCount;
}

internal inline s32 GetRandomQuizItemIndex(void)
{
    s32 RandomIndex;
    u32 QuizItemCount = GetQuizItemCount();

    RandomIndex = rand() % QuizItemCount;
    s32 LastIndex = -1;

    /* u32 RandomIndex = (LastIndex + 1) % QuizItemCount; */
    for (;;)
    {
        if (!QuizItems[RandomIndex].Complete)
        {
            /* RandomIndex = RandomIndex; */
            break;
        }
        else if (RandomIndex == LastIndex)
        {
            RandomIndex = -1;
            break;
        }

        if (LastIndex == -1)
        {
            /* NOTE: Do this to avoid hitting the (RandomIndex == LastIndex) case on the first iteration...... */
            LastIndex = RandomIndex;
        }

        /* NOTE: Our random guess collided with a complete quiz-item,
           so search for a non-complete quiz-item.
        */
        RandomIndex = (RandomIndex + 1) % QuizItemCount;
    }

    return RandomIndex;
}

internal void ClearQuizInput(state *State)
{
    for (s32 I = 0; I <= State->QuizInputIndex; ++I)
    {
        State->QuizInput[I] = 0;
    }
    State->QuizInputIndex = 0;
}

debug_variable b32 FrameIndex = 0;

internal inline void SetQuizMode(state *State, quiz_mode Mode)
{
    State->QuizMode = Mode;
    FrameIndex = 0; /* NOTE: Set FrameIndex to 0 to force a redraw. */
}

internal void GetNextRandomQuizItem(state *State)
{
    State->QuizItemIndex = GetRandomQuizItemIndex();
    State->ShowAnswer = 0;

    ClearQuizInput(State);

    State->QuizInputIndex = 0;

    if (State->QuizItemIndex < 0)
    {
        SetQuizMode(State, quiz_mode_Win);
        State->QuizItemIndex = 0;
    }
    else
    {
        SetQuizMode(State, quiz_mode_Typing);
        FrameIndex = 0;
    }
}

internal void DrawQuizPrompt(state *State, u32 LetterSpacing)
{
    quiz_item QuizItem = QuizItems[State->QuizItemIndex];

    char *Prompt;
    char *Answer;

    switch (QuizItem.Type)
    {
    case quiz_item_Text:
    {
        Prompt = QuizItems[State->QuizItemIndex].Text.Prompt;
        Answer = QuizItems[State->QuizItemIndex].Text.Answer;
    } break;
    case quiz_item_Conjugation:
    {
        Prompt = QuizItems[State->QuizItemIndex].Conjugation.Prompt;
        Answer = QuizItems[State->QuizItemIndex].Conjugation.Answer;
    } break;
    default:
    {
        Prompt = "";
        Answer = "";
    } break;
    }

    Vector2 PromptSize = MeasureTextEx(State->UI.Font, Prompt, State->UI.FontSize, 1);
    Vector2 AnswerSize = MeasureTextEx(State->UI.Font, Answer, State->UI.FontSize, 1);
    Vector2 InputSize = MeasureTextEx(State->UI.Font, State->QuizInput, State->UI.FontSize, 1);

    /* TODO: Scale the text y positions based off of font-size!!! */
    f32 PromptOffsetX = PromptSize.x / 2.0f;
    f32 PromptX = SCREEN_HALF_WIDTH - PromptOffsetX;
    f32 PromptY = SCREEN_HALF_HEIGHT - 24;

    f32 AnswerOffsetX = AnswerSize.x / 2.0f;
    f32 AnswerX = SCREEN_HALF_WIDTH - AnswerOffsetX;
    f32 AnswerY = SCREEN_HALF_HEIGHT - 52;

    f32 InputX = SCREEN_HALF_WIDTH - (InputSize.x / 2.0f);
    f32 InputY = SCREEN_HALF_HEIGHT + 48;

    f32 CursorX = SCREEN_HALF_WIDTH + (InputSize.x / 2.0f);
    f32 CursorY = InputY;

    if (State->QuizMode == quiz_mode_Correct)
    {
        DrawTextEx(State->UI.Font, "Correct", V2(20, 20), State->UI.FontSize, LetterSpacing, (Color){20, 200, 40, 255});

        if (IsKeyPressed(KEY_ENTER))
        {
            GetNextRandomQuizItem(State);
        }
    }

    if (QuizItem.Type == quiz_item_Conjugation)
    {
        char *ConjugationText = "";

        switch (QuizItems[State->QuizItemIndex].Conjugation.Conjugation)
        {
        case conjugation_Presente:    ConjugationText = "Presente"; break;
        case conjugation_Imperfecto:  ConjugationText = "Imperfecto"; break;
        case conjugation_Indefinido:  ConjugationText = "Indefinido"; break;
        case conjugation_Futuro:      ConjugationText = "Futuro"; break;
        default: break;
        }

        Vector2 TextSize = MeasureTextEx(State->UI.Font, ConjugationText, State->UI.FontSize, LetterSpacing);
        Vector2 TextPosition = V2((SCREEN_WIDTH / 2) - (TextSize.x / 2), (SCREEN_HEIGHT / 2) - 82);
        DrawTextEx(State->UI.Font, ConjugationText, TextPosition, State->UI.FontSize, LetterSpacing, CONJUGATION_TYPE_COLOR);
    }

    if (State->ShowAnswer)
    {
        DrawTextEx(State->UI.Font, Answer, V2(AnswerX, AnswerY), State->UI.FontSize, LetterSpacing, ANSWER_COLOR);
    }

    { /* Draw the index of the quiz item. */
        char Buff[16];
        sprintf(Buff, "%d", State->QuizItemIndex);
        DrawTextEx(State->UI.Font, Buff, V2(20, 48), State->UI.FontSize, LetterSpacing, FONT_COLOR);
    }

    DrawTextEx(State->UI.Font, Prompt, V2(PromptX, PromptY), State->UI.FontSize, LetterSpacing, FONT_COLOR);
    DrawTextEx(State->UI.Font, State->QuizInput, V2(InputX, InputY), State->UI.FontSize, LetterSpacing, FONT_COLOR);

    { /* Draw cursor */
        Color CursorColor = (Color){130,100,250,255};
        f32 Spacing = 3.0f;
        DrawRectangle(CursorX + Spacing, CursorY, 3, State->UI.FontSize, CursorColor);
    }

    Vector2 NextButtonPosition = V2(SCREEN_WIDTH - State->UI.FontSize, SCREEN_HEIGHT - State->UI.FontSize);
    b32 NextPressed = DoButtonWith(&State->UI, ui_Next, (u8 *)"Next", NextButtonPosition, alignment_BottomRight);
    if (NextPressed)
    {
        GetNextRandomQuizItem(State);
    }

    Vector2 PreviousButtonPosition = V2(State->UI.FontSize, SCREEN_HEIGHT - State->UI.FontSize);
    b32 PreviousPressed = DoButtonWith(&State->UI, ui_Previous, (u8 *)"Previous", PreviousButtonPosition, alignment_BottomLeft);
    if (PreviousPressed)
    {
        printf("NOT IMPLEMENTED! We need to add a history feature in order to look at previous quiz items.\n");
    }
}

internal void DisplayWinMessage(state *State, u32 LetterSpacing)
{
    char *WinMessage = "Tú ganas!";
    Vector2 TextSize = MeasureTextEx(State->UI.Font, WinMessage, State->UI.FontSize, LetterSpacing);
    Vector2 TextPosition = V2((SCREEN_WIDTH / 2) - (TextSize.x / 2), SCREEN_HEIGHT / 2);
    DrawTextEx(State->UI.Font, WinMessage, TextPosition, State->UI.FontSize, LetterSpacing, FONT_COLOR);

    if (IsKeyPressed(KEY_ENTER))
    {
        /* NOTE: reset quiz completion */
        for (s32 I = 0; I < Quiz_Item_Max; ++I)
        {
            QuizItems[I].Complete = 0;
        }
        ClearQuizInput(State);

        SetQuizMode(State, quiz_mode_Typing);

        State->QuizItemIndex = GetRandomQuizItemIndex();
    }

}

internal void UpdateAndRender(state *State, b32 ForceDraw)
{
    State->InputOccured = ForceDraw;

    HandleUserInput(State);

    if (State->InputOccured)
    {
        FrameIndex += 1;
    }

    State->KeyStateIndex = !State->KeyStateIndex;

    quiz_item QuizItem = QuizItems[State->QuizItemIndex];
    int LetterSpacing = 1;

    if (ForceDraw || State->InputOccured)
    {
        BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);

        { /* DEBUG: Draw frame index */
            char DebugBuffer[256];
            sprintf(DebugBuffer, "%d", FrameIndex);
            DrawText(DebugBuffer, 0, 0, 20, (Color){255,255,255,255});
        }

        switch(QuizItem.Type)
        {
        case quiz_item_Conjugation:
        {
            if (StringsMatch(QuizItem.Conjugation.Answer, State->QuizInput))
            {
                SetQuizMode(State, quiz_mode_Correct);
                QuizItems[State->QuizItemIndex].Complete = 1;
            }

            if (State->QuizMode == quiz_mode_Typing || State->QuizMode == quiz_mode_Correct)
            {
                DrawQuizPrompt(State, LetterSpacing);
            }
            else if (State->QuizMode == quiz_mode_Win)
            {
                DisplayWinMessage(State, LetterSpacing);
            }
        } break;
        case quiz_item_Text:
        {
            char *Answer = QuizItems[State->QuizItemIndex].Text.Answer;

            if (StringsMatch(Answer, State->QuizInput))
            {
                SetQuizMode(State, quiz_mode_Correct);
                QuizItems[State->QuizItemIndex].Complete = 1;
            }

            if (State->QuizMode == quiz_mode_Typing || State->QuizMode == quiz_mode_Correct)
            {
                DrawQuizPrompt(State, LetterSpacing);
            }
            else if (State->QuizMode == quiz_mode_Win)
            {
                DisplayWinMessage(State, LetterSpacing);
            }
        } break;
        default:
        {
            DrawText("Unknown quiz type\n", 20, 20, 22, (Color){220,100,180,255});

            Vector2 NextButtonPosition = V2(SCREEN_WIDTH - State->UI.FontSize, SCREEN_HEIGHT - State->UI.FontSize);
            b32 NextPressed = DoButtonWith(&State->UI, ui_Next, (u8 *)"Next", NextButtonPosition, alignment_BottomRight);
            if (NextPressed)
            {
                GetNextRandomQuizItem(State);
            }
        }
        }
    }

    EndDrawing();
}

internal void InitializeQuizItems(void);


int main(void)
{
    Assert(key_state_Count >= (1 << Bits_Per_Key_State) - 1);
    Assert((1 << Key_State_Chunk_Size_Log2) == 8 * sizeof(key_state_chunk));
    Assert((1 << Key_State_Chunk_Count_Log2) == Key_State_Chunk_Count);

    state State = {0};

#if defined(PLATFORM_WEB)
    InitRaylibCanvas();
    State.UI.FontSize = 36;
#else
    State.UI.FontSize = 28;
#endif

    int Result = 0;
    InitializeQuizItems();

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Estudioso");
    SetTargetFPS(60);

    int CodepointCount = 0;
    int *Codepoints;

    {
        /* TODO: Pre-bake font bitmaps? */
        char *TextWithAllCodepoints = (
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123450123456789"
            "`~!@#$%^&*()-_=+"
            "[]{}\\|;:'\",.<>/?"
            "´˜áéíóúÁÉÍÓÚñÑ");

        Codepoints = LoadCodepoints(TextWithAllCodepoints, &CodepointCount);

        u32 FontScale = 4;
        State.UI.Font = LoadFontFromMemory(".ttf", FontData, ArrayCount(FontData), State.UI.FontSize*FontScale, Codepoints, CodepointCount);
        UnloadCodepoints(Codepoints);
    }

    SetQuizMode(&State, quiz_mode_Typing);

    srand(time(NULL));

    State.QuizItemIndex = GetRandomQuizItemIndex();

    if (!IsFontReady(State.UI.Font))
    {
        printf("font not ready\n");
        return 1;
    }

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

        b32 ForceDraw = FrameIndex == 0;

        UpdateAndRender(&State, ForceDraw);
    }

    CloseWindow();

    return Result;
}

internal void AddQuizItem(quiz_item QuizItem)
{
    if (QuizItemCount < Quiz_Item_Max)
    {
        QuizItems[QuizItemCount] = QuizItem;
        QuizItemCount += 1;
    }
}

internal void AddQuizText(char *Prompt, char *Answer)
{
    quiz_item QuizItem;

    QuizItem.Type = quiz_item_Text;
    QuizItem.Complete = 0;
    QuizItem.Text.Prompt = Prompt;
    QuizItem.Text.Answer = Answer;

    AddQuizItem(QuizItem);
}

internal void AddQuizConjugation(conjugation Conjugation, char *Prompt, char *Answer)
{
    quiz_item QuizItem;

    QuizItem.Type = quiz_item_Conjugation;
    QuizItem.Complete = 0;
    QuizItem.Conjugation.Conjugation = Conjugation;
    QuizItem.Conjugation.Prompt = Prompt;
    QuizItem.Conjugation.Answer = Answer;

    AddQuizItem(QuizItem);
}


#define QuizItemText(Prompt, Answer) ((quiz_item){quiz_item_Text,0,(quiz_prompt){{Prompt,Answer}}})
#define QuizItemConjugation(Conjugation, Prompt, Answer) ((quiz_item){quiz_item_Conjugation,0,(quiz_conjugation){{Conjugation,Prompt,Answer}}})

internal void InitializeQuizItems(void)
{
    AddQuizConjugation(conjugation_Presente, "yo [ser]",                 "soy");
    AddQuizConjugation(conjugation_Presente, "tú [ser]",                 "eres");
    AddQuizConjugation(conjugation_Presente, "él, ella, usted [ser]",    "es");
    AddQuizConjugation(conjugation_Presente, "nosotros/-as [ser]",       "somos");
    AddQuizConjugation(conjugation_Presente, "vosotros/-as [ser]",       "sois");
    AddQuizConjugation(conjugation_Presente, "ellos/-as, ustedes [ser]", "son");

    AddQuizConjugation(conjugation_Imperfecto, "yo [ser]",                 "era");
    AddQuizConjugation(conjugation_Imperfecto, "tú [ser]",                 "eras");
    AddQuizConjugation(conjugation_Imperfecto, "él, ella, usted [ser]",    "era");
    AddQuizConjugation(conjugation_Imperfecto, "nosotros/-as [ser]",       "éramos");
    AddQuizConjugation(conjugation_Imperfecto, "vosotros/-as [ser]",       "erais");
    AddQuizConjugation(conjugation_Imperfecto, "ellos/-as, ustedes [ser]", "eran");

    AddQuizConjugation(conjugation_Indefinido, "yo [ser]",                 "fui");
    AddQuizConjugation(conjugation_Indefinido, "tú [ser]",                 "fuiste");
    AddQuizConjugation(conjugation_Indefinido, "él, ella, usted [ser]",    "fue");
    AddQuizConjugation(conjugation_Indefinido, "nosotros/-as [ser]",       "fuimos");
    AddQuizConjugation(conjugation_Indefinido, "vosotros/-as [ser]",       "fuisteis");
    AddQuizConjugation(conjugation_Indefinido, "ellos/-as, ustedes [ser]", "fueron");

    AddQuizConjugation(conjugation_Futuro, "yo [ser]",                 "seré");
    AddQuizConjugation(conjugation_Futuro, "tú [ser]",                 "serás");
    AddQuizConjugation(conjugation_Futuro, "él, ella, usted [ser]",    "será");
    AddQuizConjugation(conjugation_Futuro, "nosotros/-as [ser]",       "seremos");
    AddQuizConjugation(conjugation_Futuro, "vosotros/-as [ser]",       "seréis");
    AddQuizConjugation(conjugation_Futuro, "ellos/-as, ustedes [ser]", "serán");

    AddQuizText(
        "In the morning the weather is good in April.",
        "En la mañana hace buen tiempo en abril."
    );
    AddQuizText(
        "He is very cold.",
        "Él hace mucho frio."
    );
    AddQuizText(
        "_ restaurante",
        "el"
    );
    AddQuizText(
        "_ café",
        "el"
    );
    AddQuizText(
        "_ coche",
        "el"
    );
    AddQuizText(
        "_ viaje",
        "el"
    );
    AddQuizText(
        "_ parque",
        "el"
    );
    AddQuizText(
        "_ paquete",
        "el"
    );
    AddQuizText(
        "_ menú",
        "el"
    );
    AddQuizText(
        "_ colibrí",
        "el"
    );
    AddQuizText(
        "_ reloj",
        "el"
    );
    AddQuizText(
        "_ chocolate",
        "el"
    );
    AddQuizText(
        "_ pie",
        "el"
    );
    AddQuizText(
        "_ diente",
        "el"
    );
    AddQuizText(
        "_ noche",
        "la"
    );
    AddQuizText(
        "_ sangre",
        "la"
    );
    AddQuizText(
        "_ nube",
        "la"
    );
    AddQuizText(
        "_ llave",
        "la"
    );
    AddQuizText(
        "_ calle",
        "la"
    );
    AddQuizText(
        "_ gente",
        "la"
    );
    AddQuizText(
        "_ nieve",
        "la"
    );
    AddQuizText(
        "_ leche",
        "la"
    );
    AddQuizText(
        "_ carne",
        "la"
    );
    AddQuizText(
        "_ ley",
        "la"
    );
    AddQuizText(
        "_ imagen",
        "la"
    );
    AddQuizText(
        "Félix and Raúl are tall.",
        "Félix y Raúl son altos."
    );
    AddQuizText(
        "Antón is very nice.",
        "Antón es muy simpático."
    );
    AddQuizText(
        "I am Santiago.",
        "Yo soy Santiago."
    );
    AddQuizText(
        "This is the Royal Theater.",
        "Este es el Teatro Real."
    );
    AddQuizText(
        "Carlos' family is Catholic.",
        "La familia de Carlos es católica."
    );
    AddQuizText(
        "That sheet is from Japan.",
        "Esa lámina es de Japón."
    );
    AddQuizText(
        "Tatiana and Sarai are my sisters.",
        "Tatiana y Sarai son mis hermanas."
    );
    AddQuizText(
        "These are my friends.",
        "Estos son mis amigos."
    );
    AddQuizText(
        "Elisa is my ex-girlfriend.",
        "Elisa es mi exnovia."
    );
    AddQuizText(
        "That umbrella is mine.",
        "Ese paraguas es mío."
    );
    AddQuizText(
        "The soccor game is in Valencia.",
        "El partido de fútbol es en Valencia."
    );
    AddQuizText(
        "The game is on Wednesday.",
        "El partido es el miércoles."
    );
    AddQuizText(
        "Today is Sunday.",
        "Hoy es domingo."
    );
    AddQuizText(
        "Today is April 1.",
        "Hoy es 1 de abril."
    );
    AddQuizText(
        "It's Spring.",
        "Es primavera."
    );
    AddQuizText(
        "What time is it? It's ten o-clock.",
        /* "¿Que hora es? Son las diez." */
        "Que hora es? Son las diez."
    );
}
