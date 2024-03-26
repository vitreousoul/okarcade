/*
    ==============================
    ¡Estudioso!
    ==============================

    A flash-card typing game for learning Spanish.

    TODO: Create some UI to toggle different modes, or at least filter-out/select quiz-items.
    TODO: DOING: Create a lookup table to use to sort and permutate quiz-items.
    TODO: Prevent getting multiple failure counts by repeatidly pressing the Enter key with an incorrect answer.
    TODO: Allow a history of quiz items, so you can scroll back and view previous answers.
    TODO: Allow moving cursor between characters and splice editing. Currently cursor is always at the end of the input.
    TODO: Display a message showing that save-file has been written. Also, maybe disable the save button for a bit...
    TODO: Should we ignore whitespace in the user input (like trailing space after an answer)
    TODO: How do we merge an existing save with a version of the app with new quiz-items? Should save file versions line up with versions of the app, so to upgrade a version of the save file is to upgrade save file itself? (For this to work, we would need an update function that can take any version of a save-file and convert it to the newest version)
    TODO: Implement line break for long lines, especially now that our quiz sentences are getting longer.
    TODO: Improve the quality of fonts on web? They look pixellated but I'm not sure why :(
*/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "../lib/raylib.h"

#include "../src/types.h"
#include "../src/core.c"

#if !defined(PLATFORM_WEB)
#include "../src/platform.h"
#else
#include <emscripten/emscripten.h>
#endif

#include "../gen/roboto_regular.h"

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

#include "math.c"
#include "raylib_helpers.h"
#include "ui.c"


#define TEST 0
#define SAVE_FILE_PATH ((u8 *)"../save/save_file_0.estudioso")


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
    u8 *Prompt;
    u8 *Answer;
} quiz_text;

typedef struct
{
    conjugation Conjugation;
    u8 *Prompt;
    u8 *Answer;
} quiz_conjugation;

typedef struct
{
    quiz_item_type Type;
    b32 Complete;
    u16 PassCount;
    u16 FailCount;

    union
    {
        quiz_text Text;
        quiz_conjugation Conjugation;
    };
} quiz_item;

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

#define Quiz_Item_Max 256
    quiz_item QuizItems[Quiz_Item_Max];
    s32 QuizItemsLookup[Quiz_Item_Max];
    u32 QuizItemCount;
    s32 QuizLookupIndex;
    quiz_mode QuizMode;

    ui UI;
} state;


typedef enum
{
    ui_NULL,
    ui_Next,
    ui_Previous,
    ui_ShowAnswer,
    ui_Save,
} estudioso_ui_ids;

typedef struct
{
    u16 MagicNumber;
    u16 Version; /* TODO: Think about how to represent version in a consise way... */
    u32 QuizItemCount;
    u32 FileSize;
    u32 OffsetToStringData;
} save_file_header;

#define Save_File_Magic_Number 0xe54d
#define Save_File_Header_Version 0

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

internal inline b32 IsValidQuizItem(quiz_item *QuizItem)
{
    b32 IsValid = 1;

    if (QuizItem->Type == quiz_item_Text)
    {
        if (!(QuizItem->Text.Prompt && QuizItem->Text.Answer))
        {
            IsValid = 0;
        }
    }
    else if (QuizItem->Type == quiz_item_Conjugation)
    {
        if (!(QuizItem->Text.Prompt && QuizItem->Text.Answer))
        {
            IsValid = 0;
        }
    }
    else
    {
        IsValid = 0;
    }

    return IsValid;
}

internal u32 GetQuizItemCount(state *State)
{
    u32 QuizItemCount;
    for (QuizItemCount = 0; QuizItemCount < Quiz_Item_Max; ++QuizItemCount)
    {
        quiz_item QuizItem = State->QuizItems[QuizItemCount];

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

internal u8 *GetStringFromSaveFile(buffer *SaveFile, u64 StringOffset)
{
    save_file_header *Header = (save_file_header *)SaveFile->Data;
    u8 *Text = &SaveFile->Data[Header->OffsetToStringData + StringOffset];

    return Text;
}

/*
   TODO: We reserve space solely for the save file because the web
   platform doesn't yet have an api for arena-style allocators.
*/
#define Max_Bytes_For_Save_File 16000
global_variable u8 SaveFileBuffer[Max_Bytes_For_Save_File];

internal u64 PrepareSaveFile(state *State)
{
    b32 FileSize = 0;
    u32 HeaderSize = sizeof(save_file_header);

    u8 *SaveFileQuizItemsLocation = SaveFileBuffer + HeaderSize;
    u32 QuizItemCount = GetQuizItemCount(State);
    u32 QuizItemSize = sizeof(quiz_item) * QuizItemCount;

    s32 FreeSpaceForStringData = Max_Bytes_For_Save_File - (QuizItemSize + HeaderSize);

    if (FreeSpaceForStringData <= 0)
    {
        printf("Not enough space in save file for quiz-items and header (not to speak of string data).");
        return 0;
    }

    save_file_header SaveHeader;
    SaveHeader.MagicNumber = Save_File_Magic_Number;
    SaveHeader.Version = Save_File_Header_Version;
    SaveHeader.QuizItemCount = QuizItemCount;
    SaveHeader.OffsetToStringData = HeaderSize + QuizItemSize;

    u8 *StringLocation = (u8 *)(SaveFileBuffer + SaveHeader.OffsetToStringData);
    u8 *StringStart = StringLocation;

    CopyMemory((u8 *)State->QuizItems, SaveFileQuizItemsLocation, QuizItemSize);

    for (u32 I = 0; I < Quiz_Item_Max; ++I)
    {
        quiz_item *SaveFileQuizItem = (quiz_item *)SaveFileQuizItemsLocation + I;

        if(!IsValidQuizItem(SaveFileQuizItem))
        {
            break;
        }

        u8 **Prompt = 0;
        u8 **Answer = 0;

        switch (SaveFileQuizItem->Type) {
        case quiz_item_Text:
        {
            Prompt = (u8 **)&SaveFileQuizItem->Text.Prompt;
            Answer = (u8 **)&SaveFileQuizItem->Text.Answer;
        } break;
        case quiz_item_Conjugation:
        {
            Prompt = (u8 **)&SaveFileQuizItem->Conjugation.Prompt;
            Answer = (u8 **)&SaveFileQuizItem->Conjugation.Answer;
        } break;
        default: break;
        }

        u32 PromptLength = GetStringLength(*Prompt) + 1;
        u32 AnswerLength = GetStringLength(*Answer) + 1;
        u32 FullLength = PromptLength + AnswerLength;

        if (FreeSpaceForStringData - FullLength > 0)
        {
            FreeSpaceForStringData -= FullLength;

            CopyMemory(*Prompt, StringLocation, PromptLength);
            *(StringLocation + PromptLength) = 0; /* NOTE: Null terminate */
            u64 PromptOffset = (u64)StringLocation - (u64)StringStart;
            *Prompt = (u8 *)(0xffff & PromptOffset);
            StringLocation += PromptLength;

            CopyMemory(*Answer, StringLocation, AnswerLength);
            *(StringLocation + AnswerLength) = 0; /* NOTE: Null terminate */
            u64 AnswerOffset = (u64)StringLocation - (u64)StringStart;
            *Answer = (u8 *)(0xffff & AnswerOffset);
            StringLocation += AnswerLength;
        }
        else
        {
            printf("Overwrote the save-file buffer!\n");
            return 0;
        }
    }

    FileSize = StringLocation - SaveFileBuffer;
    SaveHeader.FileSize = FileSize;
    printf("FileSize %d\n", FileSize);

    CopyMemory((u8 *)&SaveHeader, SaveFileBuffer, HeaderSize);

    return FileSize;
}

internal void WriteQuizItemsToFile(state *State, u8 *FilePath)
{
    FILE *File = fopen((char *)FilePath, "wb");

    u64 FileSize = PrepareSaveFile(State);

    if (FileSize && FileSize < Max_Bytes_For_Save_File)
    {
        fwrite(SaveFileBuffer, 1, FileSize, File);
    }
    else
    {
        Assert(0);
    }
}

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
                        /* NOTE: Acute accent '´' */
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
                        /* NOTE: Tilde '˜' */
                        /* State->QuizInput[State->QuizInputIndex] = '~'; */
                        State->QuizInput[State->QuizInputIndex] = 0xCB;
                        State->QuizInput[State->QuizInputIndex+1] = 0x9C;
                        State->AccentMode = 1;
                        State->CurrentAccent = accent_Tilde;
                    }
                } break;
                case KEY_SLASH:
                {
                    if (State->ModifierKeys.Shift &&
                        State->QuizInputIndex + 2 < Test_Buffer_Count)
                    {
                        /* NOTE: Inverted question mark '¿' */
                        State->QuizInput[State->QuizInputIndex] = 0xC2;
                        State->QuizInput[State->QuizInputIndex+1] = 0xBF;
                        State->QuizInputIndex = State->QuizInputIndex + 2;
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

debug_variable b32 DEBUGCanHandleEnterKeyInWinMode = 0; /* TODO: Just fix the Enter key-press bug, where the user hits the enter key, gets the last quiz item correct, so the win screen only appears for a single frame. */

internal void GetNextRandomQuizItem(state *State)
{
    State->QuizLookupIndex = (State->QuizLookupIndex + 1);
    State->ShowAnswer = 0;

    ClearQuizInput(State);

    State->QuizInputIndex = 0;

    if (State->QuizLookupIndex >= (s32)State->QuizItemCount)
    {
        SetQuizMode(State, quiz_mode_Win);
        DEBUGCanHandleEnterKeyInWinMode = 0;
        State->QuizLookupIndex = 0;
    }
    else
    {
        SetQuizMode(State, quiz_mode_Typing);
        FrameIndex = 0;
    }
}

internal quiz_item *GetActiveQuizItem(state *State)
{
    u32 Index = State->QuizItemsLookup[State->QuizLookupIndex];
    quiz_item *Item = State->QuizItems + Index;

    return Item;
}

internal void DrawQuizPrompt(state *State, u32 LetterSpacing)
{
    quiz_item *QuizItem = GetActiveQuizItem(State);

    u8 *Prompt;
    u8 *Answer;

    switch (QuizItem->Type)
    {
    case quiz_item_Text:
    {
        Prompt = QuizItem->Text.Prompt;
        Answer = QuizItem->Text.Answer;
    } break;
    case quiz_item_Conjugation:
    {
        Prompt = QuizItem->Conjugation.Prompt;
        Answer = QuizItem->Conjugation.Answer;
    } break;
    default:
    {
        Prompt = (u8 *)"";
        Answer = (u8 *)"";
    } break;
    }

    { /* DEBUG: Draw pass/fail counts */
        char Buff[32];
        sprintf(Buff, "P:%d F:%d", QuizItem->PassCount, QuizItem->FailCount);
        DrawTextEx(State->UI.Font, Buff, V2(20, 78), State->UI.FontSize, LetterSpacing, FONT_COLOR);
    }

    Vector2 PromptSize = MeasureTextEx(State->UI.Font, (char *)Prompt, State->UI.FontSize, 1);
    Vector2 AnswerSize = MeasureTextEx(State->UI.Font, (char *)Answer, State->UI.FontSize, 1);
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
    }

    if (QuizItem->Type == quiz_item_Conjugation)
    {
        char *ConjugationText = "";

        switch (QuizItem->Conjugation.Conjugation)
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
        DrawTextEx(State->UI.Font, (char *)Answer, V2(AnswerX, AnswerY), State->UI.FontSize, LetterSpacing, ANSWER_COLOR);
    }

    { /* Draw the index of the quiz item. */
        char Buff[64];
        sprintf(Buff, "%d/%d  index=%d", State->QuizLookupIndex, State->QuizItemCount, State->QuizItemsLookup[State->QuizLookupIndex]);
        DrawTextEx(State->UI.Font, Buff, V2(20, 48), State->UI.FontSize, LetterSpacing, FONT_COLOR);
    }

    DrawTextEx(State->UI.Font, (char *)Prompt, V2(PromptX, PromptY), State->UI.FontSize, LetterSpacing, FONT_COLOR);
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

internal b32 TryToLoadSaveFile(state *State)
{
    b32 SaveFileHasLoaded = 0;
#ifndef PLATFORM_WEB

    /* quiz_item *QuizItemsFromFile = ReadQuizItemsFromFile(SAVE_FILE_PATH); */
    buffer *Buffer = ReadFileIntoBuffer(SAVE_FILE_PATH);
    s32 HeaderSize = sizeof(save_file_header);

    if (Buffer)
    {
        if (Buffer->Size > HeaderSize)
        {
            save_file_header *Header = (save_file_header *)Buffer->Data;
            u32 QuizItemSize = Header->QuizItemCount * sizeof(quiz_item);
            u32 StringDataSize = Buffer->Size - (HeaderSize + QuizItemSize);
            Assert(Buffer->Size == (s32)(HeaderSize + QuizItemSize + StringDataSize));
            Assert((u32)Buffer->Size == Header->FileSize);
            printf("StringDataSize %d\n", StringDataSize);

            for (u32 I = 0; I < Header->QuizItemCount; ++I)
            {
                quiz_item *Item = (quiz_item *)(Buffer->Data + HeaderSize) + I;

                switch (Item->Type)
                {
                case quiz_item_Text:
                {
                    Item->Text.Prompt = GetStringFromSaveFile(Buffer, (u64)Item->Text.Prompt);
                    Item->Text.Answer = GetStringFromSaveFile(Buffer, (u64)Item->Text.Answer);
                } break;
                case quiz_item_Conjugation:
                {
                    Item->Conjugation.Prompt = GetStringFromSaveFile(Buffer, (u64)Item->Conjugation.Prompt);
                    Item->Conjugation.Answer = GetStringFromSaveFile(Buffer, (u64)Item->Conjugation.Answer);
                } break;
                default: break;
                }
            }

            if (Header->QuizItemCount)
            {
                u64 Size = Header->QuizItemCount * sizeof(quiz_item);
                CopyMemory(Buffer->Data + HeaderSize, (u8 *)State->QuizItems, Size);

                SaveFileHasLoaded = 1;
            }
        }
    }

#endif
    return SaveFileHasLoaded;
}

internal void InitializeDefaultQuizItems(state *State);

internal void InitializeQuizItems(state *State)
{
    b32 SaveFileHasLoaded = TryToLoadSaveFile(State);
    State->QuizLookupIndex = 0;

    if (!SaveFileHasLoaded)
    {
        InitializeDefaultQuizItems(State);
    }
    else
    {
        /* NOTE: Set all complete fields to 0, just in case they were saved as Complete=1 */
        for (u32 I = 0; I < State->QuizItemCount; ++I)
        {
            State->QuizItems[I].Complete = 0;
        }
    }
    printf("quiz item count %d\n", State->QuizItemCount);

    { /* NOTE: Permute the lookup table */
        s32 PermutationCount = State->QuizItemCount; /* TODO: What value should permutation-count be? */

        for (u32 I = 0; I < State->QuizItemCount; ++I)
        {
            State->QuizItemsLookup[I] = I;
        }

        for (s32 I = 0; I < PermutationCount; ++I)
        {
            s32 RandomIndex = rand() % State->QuizItemCount;

            /* NOTE: Swap the quiz item indices */
            s32 Temp = State->QuizItemsLookup[I];
            State->QuizItemsLookup[I] = State->QuizItemsLookup[RandomIndex];
            State->QuizItemsLookup[RandomIndex] = Temp;
        }

#if 1
        { /* DEBUG: Test that the list does not contain duplicates */
            for (u32 I = 0; I < State->QuizItemCount; ++I)
            {
                printf("%d %d\n", I, State->QuizItemsLookup[I]);
            }

            for (s32 I = 0; I < PermutationCount; ++I)
            {
                for (s32 J = 0; J < PermutationCount; ++J)
                {
                    if (I != J)
                    {
                        Assert(State->QuizItemsLookup[I] != State->QuizItemsLookup[J]);
                    }
                }
            }
        }
#endif
    }

}

internal void DisplayWinMessage(state *State, u32 LetterSpacing)
{
    if (DEBUGCanHandleEnterKeyInWinMode)
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
                State->QuizItems[I].Complete = 0;
            }
            ClearQuizInput(State);

            SetQuizMode(State, quiz_mode_Typing);

            InitializeQuizItems(State);
        }
    }
    else if (!IsKeyPressed(KEY_ENTER))
    {
        DEBUGCanHandleEnterKeyInWinMode = 1;
    }
}

internal void HandleQuizItem(state *State, quiz_item *QuizItem, u8 *Answer)
{
    int LetterSpacing = 1;

    if (IsKeyPressed(KEY_ENTER))
    {
        if (State->QuizMode == quiz_mode_Typing)
        {
            if (StringsMatch((char *)Answer, State->QuizInput))
            {
                SetQuizMode(State, quiz_mode_Correct);
                QuizItem->Complete = 1;
                QuizItem->PassCount += 1;
            }
            else
            {
                QuizItem->FailCount += 1;
            }
        }
        else if (State->QuizMode == quiz_mode_Correct)
        {
            GetNextRandomQuizItem(State);
        }
    }

    if (State->QuizMode == quiz_mode_Typing || State->QuizMode == quiz_mode_Correct)
    {
        DrawQuizPrompt(State, LetterSpacing);
    }
    else if (State->QuizMode == quiz_mode_Win)
    {
        DisplayWinMessage(State, LetterSpacing);
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

    quiz_item *QuizItem = GetActiveQuizItem(State);

    BeginDrawing();

    if (/* TODO: Fix and reintroduce lazy-drawing */1 || ForceDraw || State->InputOccured)
    {
        ClearBackground(BACKGROUND_COLOR);

#if 0
        { /* DEBUG: Draw frame index */
            char DebugBuffer[256];
            sprintf(DebugBuffer, "%d", FrameIndex);
            DrawText(DebugBuffer, 0, 0, 20, (Color){255,255,255,255});
        }
#endif

        switch(QuizItem->Type)
        {
        case quiz_item_Conjugation:
        {
            HandleQuizItem(State, QuizItem, QuizItem->Conjugation.Answer);
        } break;
        case quiz_item_Text:
        {
            HandleQuizItem(State, QuizItem, QuizItem->Text.Answer);
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

#ifndef PLATFORM_WEB
        { /* NOTE: Overlay graphics on top of all view types. */
            f32 Spacing = State->UI.FontSize;
            Vector2 ButtonPosition = V2(SCREEN_WIDTH - Spacing, Spacing);
            b32 SavePressed = DoButtonWith(&State->UI, ui_Save, (u8 *)"Save", ButtonPosition, alignment_TopRight);

            if (SavePressed)
            {
                WriteQuizItemsToFile(State, SAVE_FILE_PATH);
            }
        }
#endif
    }

    EndDrawing();
}

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

    srand(time(NULL));
    InitializeQuizItems(&State);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Estudioso");
#if !defined(PLATFORM_WEB)
    SetWindowPosition(0, 0);
#endif
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
            "[]{}\\|;:'\",.<>/?¿¡"
            "´˜áéíóúÁÉÍÓÚñÑ");

        Codepoints = LoadCodepoints(TextWithAllCodepoints, &CodepointCount);

        u32 FontScale = 4;
        State.UI.Font = LoadFontFromMemory(".ttf", FontData, ArrayCount(FontData), State.UI.FontSize*FontScale, Codepoints, CodepointCount);
        UnloadCodepoints(Codepoints);
    }

    SetQuizMode(&State, quiz_mode_Typing);

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

internal void AddQuizItem(state *State, quiz_item QuizItem)
{
    if (State->QuizItemCount < Quiz_Item_Max)
    {
        State->QuizItems[State->QuizItemCount] = QuizItem;
        State->QuizItemCount += 1;
    }
}

#define AddQuizText(P, A) AddQuizText_(State, (P), (A))
internal void AddQuizText_(state *State, char *Prompt, char *Answer)
{
    quiz_item QuizItem;

    QuizItem.Type = quiz_item_Text;
    QuizItem.Complete = 0;
    QuizItem.PassCount = 0;
    QuizItem.FailCount = 0;
    QuizItem.Text.Prompt = (u8 *)Prompt;
    QuizItem.Text.Answer = (u8 *)Answer;

    AddQuizItem(State, QuizItem);
}

#define AddQuizConjugation(C, P, A) AddQuizConjugation_(State, C, P, A)
internal void AddQuizConjugation_(state *State, conjugation Conjugation, char *Prompt, char *Answer)
{
    quiz_item QuizItem;

    QuizItem.Type = quiz_item_Conjugation;
    QuizItem.Complete = 0;
    QuizItem.PassCount = 0;
    QuizItem.FailCount = 0;
    QuizItem.Conjugation.Conjugation = Conjugation;
    QuizItem.Conjugation.Prompt = (u8 *)Prompt;
    QuizItem.Conjugation.Answer = (u8 *)Answer;

    AddQuizItem(State, QuizItem);
}


internal void InitializeDefaultQuizItems(state *State)
{
#if 1
    AddQuizText(
        "_ tarea",
        "la"
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
    /* 006_gustar_homework */
    AddQuizText(
        "A María no (me/te/le/nos/les gustar) este libro.",
        "le gusta"
    );
    AddQuizText(
        "A mí (me/te/le/nos/les gustar) mucho escribir composiciones.",
        "me gusta"
    );
    AddQuizText(
        "A ellos no (me/te/le/nos/les gustar) el clima de Alaska.",
        "les gusta"
    );
    AddQuizText(
        "A este joven (me/te/le/nos/les gustar) todas las muchachas del club.",
        "le gustan"
    );
    AddQuizText(
        "A los animales (me/te/le/nos/les gustar) la carne cruda.",
        "les gusta"
    );
    AddQuizText(
        "A Eneida no (me/te/le/nos/les gustar) los perros.",
        "le gustan"
    );
    AddQuizText(
        "No (me/te/le/nos/les gustar) las ciudades que visitamos ayer.",
        "nos gustan"
    );
    AddQuizText(
        "A mis primas (me/te/le/nos/les gustar) mucho ir a los bailes.",
        "les gusta"
    );
    AddQuizText(
        "¿(me/te/le/nos/les gustar) a ti las clases de español?",
        "Te gustan"
    );
    AddQuizText(
        "A nosotros no (me/te/le/nos/les gustar) los exámenes.",
        "nos gustan"
    );
    AddQuizText(
        "No (me/te/le/nos/les gustar) estudiar los sábados.",
        "me gusta"
    );
    AddQuizText(
        "A mis vecinos (me/te/le/nos/les gustar) mucho viajar.",
        "les gusta"
    );
    AddQuizText(
        "A nosotros no (me/te/le/nos/les gustar) los viajes muy largos.",
        "nos gustan"
    );
    AddQuizText(
        "Al maestro (me/te/le/nos/les gustar) escribir los ejercicios en la pizarra.",
        "le gusta"
    );
    AddQuizText(
        "¿(me/te/le/nos/les gustar) a Enrique trabajar en esa tienda?",
        "Le gusta"
    );
    AddQuizText(
        "A mi médico (me/te/le/nos/les gustar) escribir los ejercicios en la pizarra.",
        "le gusta"
    );
    AddQuizText(
        "A mí (me/te/le/nos/les gustar) mucho la película de anoche.",
        "me gusta"
    );
    AddQuizText(
        "A mi madre (me/te/le/nos/les gustar) sembrar flores.",
        "le gusta"
    );
    AddQuizText(
        "No (me/te/le/nos/les gustar) el discurso del señor Gómez en la reunión de ayer.",
        "me gusta"
    );
    AddQuizText(
        "A ellos no (me/te/le/nos/les gustar) el paseo de la semana pasada.",
        "les gusta"
    );
    /* 008_mas_verbos_homework EJERCICIO A (1 Singular) */
    AddQuizText(
        "En la mañana yo (beber) café con leche.",
        "bebo"
    );
    AddQuizText(
        "Tú no (comer) carne.",
        "comes"
    );
    AddQuizText(
        "Nosotros (comprender) español.",
        "comprendemos"
    );
    AddQuizText(
        "Ellos (correr) en el parque.",
        "corren"
    );
    AddQuizText(
        "Ella (aprender) español en la Cooperativa.",
        "aprende"
    );
    AddQuizText(
        "Él (vender) limonada.",
        "vende"
    );
    AddQuizText(
        "Usted (leer) un nuevo libro.",
        "lee"
    );
    AddQuizText(
        "María no (beber) café.",
        "bebe"
    );
    AddQuizText(
        "Carlos y Anita (comer) una hamburguesa.",
        "comen"
    );
    AddQuizText(
        "Mi hermana (deber) estudiar más.",
        "debe"
    );
    /* 008_mas_verbos_homework EJERCICIO A (2 Plurares) */
    AddQuizText(
        "Yo (asistir) a la escuela todos los días.",
        "asisto"
    );
    AddQuizText(
        "Mis padres (vivir) en un pueblo pequeño.",
        "viven"
    );
    AddQuizText(
        "Yo (recibir) muchas cartas de los amigos.",
        "recibo"
    );
    AddQuizText(
        "Yo (escribir) a mis amigos.",
        "escribo"
    );
    AddQuizText(
        "Tú (abrir) las ventanas.",
        "abres"
    );
    AddQuizText(
        "El fin de semana nosotros (subir) las montañas.",
        "subimos"
    );
    AddQuizText(
        "Yo (compartir) mi casa con mi hermano.",
        "comparto"
    );
#endif
    AddQuizText(
        "Él (insistir) en viajar a Guatemala.",
        "insiste"
    );
    AddQuizText(
        "Yo (partir) la próxima semana.",
        "parto"
    );
    AddQuizText(
        "En Guatemala (existir) muchos lugares bonitos.",
        "existen"
    );
    AddQuizText(
        "Yo (hablar) español en la escuela.",
        "hablo"
    );
    AddQuizText(
        "En San Pedro yo no (necesitar) mucho dinero.",
        "necesito"
    );
    AddQuizText(
        "Yo (viajar) solo.",
        "viajo"
    );
    AddQuizText(
        "Tú (ayudar) a tu hermano.",
        "ayudas"
    );
    AddQuizText(
        "Tú (cocinar) mucha comida.",
        "cocinas"
    );
    AddQuizText(
        "Nosotros (visitar) muchas partes de Guatemala.",
        "visitamos"
    );
    AddQuizText(
        "En la tarde ellos (descansar) por una hora.",
        "descansan"
    );
    AddQuizText(
        "Cuando yo (tomar) cerveza, yo (fumar) mucho.",
        "tomo fumo"
    );
    AddQuizText(
        "Él (regresar) a su país la próxima semana.",
        "regresa"
    );
    AddQuizText(
        "Ella (lavar) su ropa cada semana.",
        "lava"
    );
    AddQuizText(
        "Yo (pagar) veinticinco quetzales por una comida.",
        "pago"
    );
    AddQuizText(
        "Ellos (desear) estudiar español.",
        "desean"
    );
    AddQuizText(
        "Yo (estudiar) español en la Cooperativa.",
        "estudio"
    );
    AddQuizText(
        "Mi padre (trabajar) ocho horas cada día.",
        "trabaja"
    );
    /* 007_verbos_homework EJERCICIO A */
    AddQuizText(
        "Yo (hablar) español en la escuela.",
        "hablo"
    );
    AddQuizText(
        "En San Pedro yo no (necesitar) mucho dinero.",
        "necesito"
    );
    AddQuizText(
        "Yo (viajar) solo.",
        "viajo"
    );
    AddQuizText(
        "Tú (ayudar) a tu hermano.",
        "ayudas"
    );
    AddQuizText(
        "Tú (cocinar) mucha comida.",
        "cocinas"
    );
    AddQuizText(
        "Nosotros (visitar) muchas partes de Guatemala.",
        "visitamos"
    );
    AddQuizText(
        "En la tarde ellos (descansar) por una hora.",
        "descansan"
    );
    AddQuizText(
        "Cuando yo (tomar) cerveza, yo (fumar) mucho.",
        "tomo fumo"
    );
    AddQuizText(
        "Él (regresar) a su país la próxima semana.",
        "regresa"
    );
    AddQuizText(
        "Ella (lavar) su ropa cada semana.",
        "lava"
    );
    AddQuizText(
        "Yo (pagar) veinticinco quetzales por una comida.",
        "pago"
    );
    AddQuizText(
        "Ellos (desear) estudiar español.",
        "desean"
    );
    AddQuizText(
        "Yo (estudiar) español en la Cooperativa.",
        "estudio"
    );
    AddQuizText(
        "Mi padre (trabajar) ocho horas cada día.",
        "trabaja"
    );
    AddQuizText(
        "Yo (mirar) una película cada semana en la escuela.",
        "miro"
    );





    /* 009_y_mas_verbos_homework */
    AddQuizText(
        "Yo (salir) a las doce de la escuela.",
        "salgo"
    );
    AddQuizText(
        "Yo (hacer) la tarea todas las tardes.",
        "hago"
    );
    AddQuizText(
        "Yo (traer) libros a la escuela.",
        "traigo"
    );
    AddQuizText(
        "Yo (poner) mis libros en la mesa.",
        "pongo"
    );
    AddQuizText(
        "Yo (saber) hablar poco español.",
        "sé"
    );
    AddQuizText(
        "Yo (dar) la tarea al profesor.",
        "doy"
    );
    AddQuizText(
        "Yo (hacer) ejercicios físicos.",
        "hago"
    );
    AddQuizText(
        "Yo (ver) un programa interesante.",
        "veo"
    );
    AddQuizText(
        "Yo no (salir) en este espacio.",
        "salgo"
    );
    AddQuizText(
        "Yo (salir) la próxima semana.",
        "salgo"
    );
    AddQuizText(
        "¿Qué tipo de música (oír) tú?",
        "oyes"
    );
    AddQuizText(
        "Yo (venir) a la escuela.",
        "vengo"
    );
    AddQuizText(
        "Cuando yo salgo de la casa (decir) adiós.",
        "digo"
    );
    AddQuizText(
        "Yo (oír) las noticias en la radio.",
        "oigo"
    );
    AddQuizText(
        "Mis amigos no (venir) a la escuela hoy.",
        "vienen"
    );
    AddQuizText(
        "¿Qué (decir) tú cuando recibes un regalo?",
        "dices"
    );
    AddQuizText(
        "Yo (decir) gracias.",
        "digo"
    );
    AddQuizText(
        "Nosotros (venir) temprano a la escuela.",
        "venimos"
    );
    AddQuizText(
        "Cuando él (venir) a la escuela, siempre (oír) música.",
        "viene oye"
    );
    AddQuizText(
        "Ellos (oír) el pronóstico del tiempo.",
        "oyen"
    );
    AddQuizText(
        "Cerca de mi casa yo (oír) mucho ruido.",
        "oigo"
    );
    AddQuizText(
        "¿A qué hora (venir) tú a la casa?",
        "vienes"
    );
    AddQuizText(
        "Mis amigos y yo siempre (decir) la verdad.",
        "decimos"
    );
    AddQuizText(
        "Cuando yo estoy en el centro del pueblo (oír) mucho ruido.",
        "oigo"
    );
    AddQuizText(
        "El maestro (oír) mis respuestas con atención.",
        "oye"
    );
    AddQuizText(
        "Yo (poder) hablar alemán.",
        "puedo"
    );
    AddQuizText(
        "Nosotros no (recordar) mucho.",
        "recordamos"
    );
    AddQuizText(
        "Mi hermano (dormir) ocho horas cada día.",
        "duerme"
    );
    AddQuizText(
        "¿Dónde (almorzar) tú normalmente?",
        "almuerzas"
    );
    AddQuizText(
        "Yo (volver) a las cuatro del trabajo.",
        "vuelvo"
    );
    AddQuizText(
        "Mi mejor amigo (jugar) fútbol.",
        "juega"
    );
    AddQuizText(
        "¿Te (acordar) (tú) de la fecha de la fiesta?",
        "acuerdas"
    );
    AddQuizText(
        "¿Cuántas horas (volar) (tú) desde Europa hasta América?",
        "vuelas"
    );
    AddQuizText(
        "¿A qué hora (almorzar) tu familia normalmente?",
        "almuerza"
    );
    AddQuizText(
        "Los estudiantes (devolver) los libros a la escuela.",
        "devuelven"
    );
    AddQuizText(
        "¿Cuánto (costar) una cámara digital?",
        "cuesta"
    );
    AddQuizText(
        "Mi abuelo (contar) sus experiencias.",
        "cuenta"
    );
    AddQuizText(
        "Muchas plantas (morir) en verano.",
        "mueren"
    );
    AddQuizText(
        "Nosotros (mostrar) las fotos a nuestros amigos.",
        "mostramos"
    );
    AddQuizText(
        "En invierno (llover) mucho.",
        "llueve"
    );
}
