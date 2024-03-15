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
#include "raylib_defines.c"
#endif

global_variable int SCREEN_WIDTH = 640;
global_variable int SCREEN_HEIGHT = 400;

#include "../gen/roboto_regular.h"

#include "math.c"
#include "raylib_helpers.h"
#include "ui.c"

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

typedef struct
{
    s16 Index;
    s16 Shift;
} key_location;

typedef struct
{
    char *Prompt;
    char *ExpectedAnswer;
    b32 Complete;
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

internal inline s32 GetRandomQuizItemIndex(state *State)
{
    s32 RandomIndex;

    u32 QuizItemCount;
    for (QuizItemCount = 0; QuizItemCount < Quiz_Item_Max; ++QuizItemCount)
    {
        if (!QuizItems[QuizItemCount].Prompt || !QuizItems[QuizItemCount].ExpectedAnswer)
        {
            break;
        }
    }

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

internal void GetNextRandomQuizItem(state *State)
{
    State->QuizItemIndex = GetRandomQuizItemIndex(State);
    State->ShowAnswer = 0;

    ClearQuizInput(State);

    State->QuizInputIndex = 0;

    if (State->QuizItemIndex < 0)
    {
        State->QuizMode = quiz_mode_Win;
        State->QuizItemIndex = 0;
    }
    else
    {
        State->QuizMode = quiz_mode_Typing;
    }
}

internal void UpdateAndRender(state *State)
{
    HandleUserInput(State);

    State->KeyStateIndex = !State->KeyStateIndex;

    char *Prompt = QuizItems[State->QuizItemIndex].Prompt;
    char *ExpectedAnswer = QuizItems[State->QuizItemIndex].ExpectedAnswer;

    Vector2 PromptSize = MeasureTextEx(State->UI.Font, Prompt, FONT_SIZE, 1);
    Vector2 AnswerSize = MeasureTextEx(State->UI.Font, ExpectedAnswer, FONT_SIZE, 1);
    Vector2 InputSize = MeasureTextEx(State->UI.Font, State->QuizInput, FONT_SIZE, 1);

    f32 PromptOffsetX = PromptSize.x / 2.0f;
    f32 PromptX = SCREEN_HALF_WIDTH - PromptOffsetX;
    f32 PromptY = SCREEN_HALF_HEIGHT - 24;

    f32 AnswerOffsetX = AnswerSize.x / 2.0f;
    f32 AnswerX = SCREEN_HALF_WIDTH - AnswerOffsetX;
    f32 AnswerY = SCREEN_HALF_HEIGHT - 48;

    f32 InputX = SCREEN_HALF_WIDTH - (InputSize.x / 2.0f);
    f32 InputY = SCREEN_HALF_HEIGHT + 48;

    f32 CursorX = SCREEN_HALF_WIDTH + (InputSize.x / 2.0f);
    f32 CursorY = InputY;

    BeginDrawing();
    ClearBackground(BACKGROUND_COLOR);

    if (StringsMatch(ExpectedAnswer, State->QuizInput))
    {
        State->QuizMode = quiz_mode_Correct;
        QuizItems[State->QuizItemIndex].Complete = 1;
    }

    int LetterSpacing = 1;

    if (State->QuizMode == quiz_mode_Typing || State->QuizMode == quiz_mode_Correct)
    {
        if (State->QuizMode == quiz_mode_Correct)
        {
            DrawTextEx(State->UI.Font, "Correct", V2(20, 20), State->UI.FontSize, LetterSpacing, (Color){20, 200, 40, 255});

            if (IsKeyPressed(KEY_ENTER))
            {
                GetNextRandomQuizItem(State);
            }
        }

        if (State->ShowAnswer)
        {
            DrawTextEx(State->UI.Font, ExpectedAnswer, V2(AnswerX, AnswerY), State->UI.FontSize, LetterSpacing, ANSWER_COLOR);
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
            DrawRectangle(CursorX + Spacing, CursorY, 3, FONT_SIZE, CursorColor);
        }

        Vector2 NextButtonPosition = V2(SCREEN_WIDTH - FONT_SIZE, SCREEN_HEIGHT - FONT_SIZE);
        b32 NextPressed = DoButtonWith(&State->UI, ui_Next, (u8 *)"Next", NextButtonPosition, alignment_BottomRight);
        if (NextPressed)
        {
            GetNextRandomQuizItem(State);
        }

        Vector2 PreviousButtonPosition = V2(FONT_SIZE, SCREEN_HEIGHT - FONT_SIZE);
        b32 PreviousPressed = DoButtonWith(&State->UI, ui_Previous, (u8 *)"Previous", PreviousButtonPosition, alignment_BottomLeft);
        if (PreviousPressed)
        {
            printf("NOT IMPLEMENTED! We need to add a history feature in order to look at previous quiz items.\n");
        }

    }
    else if (State->QuizMode == quiz_mode_Win)
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

            State->QuizMode = quiz_mode_Typing;
            State->QuizItemIndex = GetRandomQuizItemIndex(State);
        }
    }

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

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Estudioso");
    SetTargetFPS(60);

    state State = {0};

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

        u32 FontScale = 2;
        State.UI.Font = LoadFontFromMemory(".ttf", FontData, ArrayCount(FontData), FONT_SIZE*FontScale, Codepoints, CodepointCount);
        UnloadCodepoints(Codepoints);
    }

    State.UI.FontSize = FONT_SIZE;

    State.QuizMode = quiz_mode_Typing;

    srand(time(NULL));

    State.QuizItemIndex = GetRandomQuizItemIndex(&State);

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

        UpdateAndRender(&State);
    }

    CloseWindow();

    return Result;
}

#define Quiz_Item(p, a) ((quiz_item){p, a, 0})

global_variable quiz_item QuizItems[Quiz_Item_Max] = {
    Quiz_Item(
        "In the morning the weather is good in April.",
        "En la mañana hace buen tiempo en abril."
    ),
    Quiz_Item(
        "He is very cold.",
        "Él hace mucho frio."
    ),
    Quiz_Item(
        "_ restaurante",
        "el"
    ),
    Quiz_Item(
        "_ café",
        "el"
    ),
    Quiz_Item(
        "_ coche",
        "el"
    ),
    Quiz_Item(
        "_ viaje",
        "el"
    ),
    Quiz_Item(
        "_ parque",
        "el"
    ),
    Quiz_Item(
        "_ paquete",
        "el"
    ),
    Quiz_Item(
        "_ menú",
        "el"
    ),
    Quiz_Item(
        "_ colibrí",
        "el"
    ),
    Quiz_Item(
        "_ reloj",
        "el"
    ),
    Quiz_Item(
        "_ chocolate",
        "el"
    ),
    Quiz_Item(
        "_ pie",
        "el"
    ),
    Quiz_Item(
        "_ diente",
        "el"
    ),
    Quiz_Item(
        "_ noche",
        "la"
    ),
    Quiz_Item(
        "_ sangre",
        "la"
    ),
    Quiz_Item(
        "_ nube",
        "la"
    ),
    Quiz_Item(
        "_ llave",
        "la"
    ),
    Quiz_Item(
        "_ calle",
        "la"
    ),
    Quiz_Item(
        "_ gente",
        "la"
    ),
    Quiz_Item(
        "_ nieve",
        "la"
    ),
    Quiz_Item(
        "_ leche",
        "la"
    ),
    Quiz_Item(
        "_ carne",
        "la"
    ),
    Quiz_Item(
        "_ ley",
        "la"
    ),
    Quiz_Item(
        "_ imagen",
        "la"
    ),
    Quiz_Item(
        "Félix and Raúl are tall.",
        "Félix y Raúl son altos."
    ),
    Quiz_Item(
        "Antón is very nice.",
        "Antón es muy simpático."
    ),
    Quiz_Item(
        "I am Santiago.",
        "Yo soy Santiago."
    ),
    Quiz_Item(
        "This is the Royal Theater.",
        "Este es el Teatro Real."
    ),
    Quiz_Item(
        "Carlos' family is Catholic.",
        "La familia de Carlos es católica."
    ),
    Quiz_Item(
        "That sheet is from Japan.",
        "Esa lámina es de Japón."
    ),
    Quiz_Item(
        "Tatiana and Sarai are my sisters.",
        "Tatiana y Sarai son mis hermanas."
    ),
    Quiz_Item(
        "These are my friends.",
        "Estos son mis amigos."
    ),
    Quiz_Item(
        "Elisa is my ex-girlfriend.",
        "Elisa es mi exnovia."
    ),
    Quiz_Item(
        "That umbrella is mine.",
        "Ese paraguas es mío."
    ),
    Quiz_Item(
        "The soccor game is in Valencia.",
        "El partido de fútbol es en Valencia."
    ),
    Quiz_Item(
        "The game is on Wednesday.",
        "El partido es el miércoles."
    ),
    Quiz_Item(
        "Today is Sunday.",
        "Hoy es domingo."
    ),
    Quiz_Item(
        "Today is April 1.",
        "Hoy es 1 de abril."
    ),
    Quiz_Item(
        "It's Spring.",
        "Es primavera."
    ),
    Quiz_Item(
        "What time is it? It's ten o-clock.",
        /* "¿Que hora es? Son las diez." */
        "Que hora es? Son las diez."
    )
};
