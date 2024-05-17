/*
    ==============================
    ¡Estudioso!
    ==============================

    A flash-card typing game for learning Spanish.

    Save File
    TODO: Create a new/load game screen, to choose between default quiz items and an existing save.
    TODO: How do we merge an existing save with a version of the app with new quiz-items? Should save file versions line up with versions of the app, so to upgrade a version of the save file is to upgrade save file itself? (For this to work, we would need an update function that can take any version of a save-file and convert it to the newest version)
    TODO: Draw a message showing that save-file has been written. Also, maybe disable the save button for a bit...

    Meta Game
    TODO: Create some UI to toggle different modes, or at least filter-out/select quiz-items.

    Text Input
    TODO: Should we ignore whitespace in the user input (like trailing space after an answer)
    TODO: Allow moving cursor between characters and splice editing. Currently cursor is always at the end of the input.

    Quiz Features
    TODO: Add a "Show Answer" button, instead of relying on undocumented hotkeys.
    TODO: Allow a history of quiz items, so you can scroll back and view previous answers.
    TODO: Add automated number quizes, where random numbers are picked and the user enters the spelled out name for the number.
    TODO: Allow multiple potential answers, for things that may have multiple translations.
    TODO: Create grammar structures in order to randomize sentences with similar structure.

    Debugging
    TODO: Allow marking a quiz-item as inaccurate or in need of a review. This would be helpful when using the app and noticing typos or other errors in the item content.
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
#include "../gen/sfx_correct.h"

#if 0
global_variable int SCREEN_WIDTH = TARGET_SCREEN_WIDTH;
global_variable int SCREEN_HEIGHT = TARGET_SCREEN_HEIGHT;
#else
global_variable int SCREEN_WIDTH = 800;
global_variable int SCREEN_HEIGHT = 600;
#endif

#include "math.c"
#include "raylib_helpers.h"
#include "ui.c"
#include "sound.c"

#define DEBUG_GRAPHICS 1

#define TEST 0
#define SAVE_FILE_PATH ((u8 *)"../save/save_file_0.estudioso")


#define ArrayCount(a) (sizeof(a)/sizeof((a)[0]))

#define IS_SPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')
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
    quiz_item_VariableAnswer,
    quiz_item_Translation,
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

/* TODO: Don't hard-code the variable size stuff in variable_answer.
   Instead, use a linked list or some other dynamic structure (this will be easier once
   estudioso is using some kind of allocator like arenas.
*/
#define Max_Variable_Answers 4
#define Max_Variables 4

typedef struct
{
    char *PromptParts[Max_Variables + 1];
    s32 PromptPartCount;
    char *Variables[Max_Variable_Answers][Max_Variables];
    char *Answers[Max_Variable_Answers];
    s32 VariableAnswerCount;
    s32 VariableAnswerIndex;
} variable_answer;

typedef struct
{
    quiz_item_type Type;
    b32 Complete;
    s16 PassCount;
    s16 FailCount;

    union
    {
        quiz_text Text;
        quiz_conjugation Conjugation;
        variable_answer VariableAnswer;
    };
} quiz_item;

typedef enum
{
    frequency_High,
    frequency_Medium,
    frequency_Low,
    frequency_Count
} frequency;

typedef struct
{
    f32 DeltaTime;
    f32 Time;

    u32 KeyStateIndex;
    key_state_chunk KeyStates[2][Key_State_Chunk_Count];
#define Key_Press_Queue_Size 16
    u32 KeyPressQueue[Key_Press_Queue_Size];

    b32 ShowAnswer;
    b32 CurrentQuizItemFailed;

    b32 InputOccured;
    key_code LastKeyPressed;

#define Quiz_Item_Max 512
    quiz_item QuizItems[Quiz_Item_Max];
    s32 QuizItemsLookup[Quiz_Item_Max];
    s32 QuizItemsLookupWorkingSet[Quiz_Item_Max];
    u32 QuizItemCount;

    s32 FrequencyThreshold[frequency_Count];
    s32 Bucket[frequency_Count];
    s32 BucketCount[frequency_Count];
    s32 BucketSize[frequency_Count];
    s32 BucketIndex;

    quiz_mode QuizMode;

    ui UI;
    text_element TextElement;

#define Max_Line_Count 6
#define Max_Line_Buffer_Size 256
    u8 LineBuffer[Max_Line_Count][Max_Line_Buffer_Size];
    s32 LineBufferIndex;

    Sound Correct;
    Sound Wrong;
    Sound Win;
} state;

typedef enum
{
    ui_NULL,
    ui_Next,
    ui_Previous,
    ui_ShowAnswer,
    ui_Save,
} estudioso_ui_id;

typedef struct
{
    u16 MagicNumber;
    u16 Version; /* TODO: Think about how to represent version in a consise way... */
    u32 QuizItemCount;
    s32 FrequencyThreshold[frequency_Count];
    s32 Bucket[frequency_Count];
    u32 FileSize;
    u32 OffsetToStringData;
    u32 OffsetToQuizItemsLookupData;
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

internal void AddQuizItem(state *State, quiz_item QuizItem)
{
    if (State->QuizItemCount < Quiz_Item_Max)
    {
        State->QuizItems[State->QuizItemCount] = QuizItem;
        State->QuizItemCount += 1;
    }
    else
    {
        Assert(0);
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

#define AddQuizTranslation(P, A) AddQuizTranslation_(State, (P), (A))
internal void AddQuizTranslation_(state *State, char *Prompt, char *Answer)
{
    quiz_item QuizItem;

    QuizItem.Type = quiz_item_Translation;
    QuizItem.Complete = 0;
    QuizItem.PassCount = 0;
    QuizItem.FailCount = 0;
    QuizItem.Text.Prompt = (u8 *)Prompt;
    QuizItem.Text.Answer = (u8 *)Answer;

    AddQuizItem(State, QuizItem);
}

internal void AddQuizVariableAnswer_(state *State, variable_answer VA)
{
    Assert(VA.VariableAnswerCount >= 0 && VA.VariableAnswerCount < Max_Variable_Answers);
    Assert(VA.PromptPartCount >= 0);

    quiz_item QuizItem = {0};

    QuizItem.Type = quiz_item_VariableAnswer;
    QuizItem.Complete = 0;
    QuizItem.PassCount = 0;
    QuizItem.FailCount = 0;
    QuizItem.VariableAnswer.PromptPartCount = VA.PromptPartCount;
    QuizItem.VariableAnswer.VariableAnswerCount = VA.VariableAnswerCount;

    for (s32 I = 0; I < VA.PromptPartCount; ++I)
    {
        QuizItem.VariableAnswer.PromptParts[I] = VA.PromptParts[I];
    }

    for (s32 I = 0; I < Max_Variable_Answers; ++I)
    {
        for (s32 J = 0; J < VA.PromptPartCount - 1; ++J)
        {
            QuizItem.VariableAnswer.Variables[I][J] = VA.Variables[I][J];
        }

        QuizItem.VariableAnswer.Answers[I] = VA.Answers[I];
    }

    AddQuizItem(State, QuizItem);
}

#include "estudioso_data.h"

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
    u32 QuizLookupSize = sizeof(State->QuizItemsLookup);

    s32 FreeSpaceForStringData = Max_Bytes_For_Save_File - (QuizItemSize + HeaderSize);

    if (FreeSpaceForStringData <= 0)
    {
        printf("Not enough space in save file for quiz-items and header (not to speak of string data).");
        return 0;
    }

    save_file_header SaveHeader;
    {
        SaveHeader.MagicNumber = Save_File_Magic_Number;
        SaveHeader.Version = Save_File_Header_Version;
        SaveHeader.QuizItemCount = QuizItemCount;
        SaveHeader.OffsetToQuizItemsLookupData = HeaderSize + QuizItemSize;
        SaveHeader.OffsetToStringData = HeaderSize + QuizItemSize + QuizLookupSize;

        for (s32 I = 0; I < frequency_Count; ++I)
        {
            SaveHeader.FrequencyThreshold[I] = State->FrequencyThreshold[I];
            SaveHeader.Bucket[I] = State->Bucket[I];
        }
    }

    u8 *StringLocation = (u8 *)(SaveFileBuffer + SaveHeader.OffsetToStringData);
    u8 *StringStart = StringLocation;

    CopyMemory((u8 *)State->QuizItems, SaveFileQuizItemsLocation, QuizItemSize);

    u8 *QuizLookupData = SaveFileBuffer + SaveHeader.OffsetToQuizItemsLookupData;
    CopyMemory((u8 *)State->QuizItemsLookup, QuizLookupData, QuizLookupSize);

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

    CopyMemory((u8 *)&SaveHeader, SaveFileBuffer, HeaderSize);

    return FileSize;
}

internal void WriteSaveFileToDisk(state *State, u8 *FilePath)
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

internal void ClearAccentKey(text_element *TextElement)
{
    switch (TextElement->CurrentAccent)
    {
    case accent_Acute:
    case accent_Tilde:
    {
        if (TextElement->Text)
        {
            TextElement->Text[TextElement->Index] = 0;
            if (TextElement->Index + 1 < TextElement->TextSize)
            {
                TextElement->Text[TextElement->Index+1] = 0;
            }
            TextElement->AccentMode = 0;
            TextElement->CurrentAccent = 0;
        }
    } break;
    default:
    {
        Assert(!"Not Implemented.");
    }
    }
}

internal void HandleKey(ui *UI, text_element *TextElement, key_code Key)
{
    if (PrintableKeys[Key])
    {
        if (TextElement->Index + 1 < TextElement->TextSize)
        {
            if (TextElement->AccentMode)
            {
                unsigned char SecondByte = 0;

                if (TextElement->Index + 2 < TextElement->TextSize)
                {
                    if (TextElement->CurrentAccent == accent_Acute)
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
                            ClearAccentKey(TextElement);
                        }
                        else
                        {
                            if (!UI->ModifierKeys.Shift)
                            {
                                SecondByte += 32;
                            }

                            TextElement->Text[TextElement->Index] = 0xC3;
                            TextElement->Text[TextElement->Index+1] = SecondByte;
                            TextElement->Index += 2;
                        }
                    }
                    else if (TextElement->CurrentAccent == accent_Tilde && Key == KEY_N)
                    {
                        char SecondByte = 0x91;

                        if (!UI->ModifierKeys.Shift)
                        {
                            SecondByte += 32;
                        }

                        TextElement->Text[TextElement->Index] = 0xC3;
                        TextElement->Text[TextElement->Index+1] = SecondByte;
                        TextElement->Index += 2;
                    }
                    else
                    {
                        ClearAccentKey(TextElement);
                    }
                }

                TextElement->AccentMode = 0;
                TextElement->CurrentAccent = 0;
            }
            else if (UI->ModifierKeys.Alt)
            {
                switch (Key)
                {
                case KEY_E:
                {
                    if (TextElement->Index + 2 < TextElement->TextSize)
                    {
                        /* NOTE: Acute accent '´' */
                        TextElement->Text[TextElement->Index] = 0xC2;
                        TextElement->Text[TextElement->Index+1] = 0xB4;
                        TextElement->AccentMode = 1;
                        TextElement->CurrentAccent = accent_Acute;
                    }
                } break;
                case KEY_N:
                {
                    if (TextElement->Index + 2 < TextElement->TextSize)
                    {
                        /* NOTE: Tilde '˜' */
                        /* TextElement->Text[TextElement->Index] = '~'; */
                        TextElement->Text[TextElement->Index] = 0xCB;
                        TextElement->Text[TextElement->Index+1] = 0x9C;
                        TextElement->AccentMode = 1;
                        TextElement->CurrentAccent = accent_Tilde;
                    }
                } break;
                case KEY_SLASH:
                {
                    if (UI->ModifierKeys.Shift &&
                        TextElement->Index + 2 < TextElement->TextSize)
                    {
                        /* NOTE: Inverted question mark '¿' */
                        TextElement->Text[TextElement->Index] = 0xC2;
                        TextElement->Text[TextElement->Index+1] = 0xBF;
                        TextElement->Index = TextElement->Index + 2;
                    }
                } break;
                }
            }
            else if (UI->ModifierKeys.Shift && Key == KEY_SLASH)
            {
                TextElement->Text[TextElement->Index] = '?';
                TextElement->Index = TextElement->Index + 1;
            }
            else
            {
                if (!UI->ModifierKeys.Shift && IS_UPPER_CASE(Key))
                {
                    Key += 32;
                }

                TextElement->Text[TextElement->Index] = Key;
                TextElement->Index = TextElement->Index + 1;
            }
        }
    }
    else
    {
        switch (Key)
        {
        case KEY_BACKSPACE:
        {
            if (TextElement->AccentMode)
            {
                ClearAccentKey(TextElement);
            }
            else
            {
                int TailBytesSkipped = 0;

                for (;;)
                {
                    if (TextElement->Index > 0)
                    {
                        TextElement->Index = TextElement->Index - 1;
                    }

                    if (TextElement->Index < 0)
                    {
                        break;
                    }

                    char Char = TextElement->Text[TextElement->Index];

                    if (Is_Utf8_Tail_Byte(Char))
                    {
                        TextElement->Text[TextElement->Index] = 0;
                        TailBytesSkipped += 1;
                    }
                    else if (TailBytesSkipped == 1 && Is_Utf8_Header_Byte2(Char))
                    {
                        if (TextElement->Index < 0) break;
                        TextElement->Text[TextElement->Index] = 0;
                        break;
                    }
                    else if (TailBytesSkipped == 2 && Is_Utf8_Header_Byte3(Char))
                    {
                        if (TextElement->Index < 0) break;
                        TextElement->Text[TextElement->Index] = 0;
                        break;
                    }
                    else if (TailBytesSkipped == 3 && Is_Utf8_Header_Byte4(Char))
                    {
                        if (TextElement->Index < 0) break;
                        TextElement->Text[TextElement->Index] = 0;
                        break;
                    }
                    else
                    {
                        /* NOTE: Delete an ASCII character. */
                        TextElement->Text[TextElement->Index] = 0;
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
        State->UI.ModifierKeys.Shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        State->UI.ModifierKeys.Control = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        State->UI.ModifierKeys.Alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
        State->UI.ModifierKeys.Super = IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
    }

    {
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
            /* TODO: Do we need to still consume all key-presses with GetKeyPressed?
                     Right now we break if the queue is full, but maybe we really
                     should not do the code below, but still loop through calling
                     GetKeyPressed in order to flush all the key events?
            */
            break;
        }

        if (State->UI.ModifierKeys.Control && Key == KEY_H)
        {
            State->ShowAnswer = 1;
            KeysWerePressed = 1;
            State->InputOccured = 1;
        }
        else
        {
            State->KeyPressQueue[QueueIndex] = Key;
            QueueIndex += 1;
            KeysWerePressed = 1;
            State->TextElement.KeyRepeatTime = 0.0f;
            State->TextElement.KeyHasRepeated = 0;
            State->LastKeyPressed = Key;

            HandleKey(&State->UI, &State->TextElement, Key);
            State->InputOccured = 1;
        }
    }

    if (!KeysWerePressed && IsKeyDown(State->LastKeyPressed))
    {
        /* NOTE: Handle keyboard repeat */
        State->TextElement.KeyRepeatTime += State->DeltaTime;

        b32 InitialKeyRepeat = !State->TextElement.KeyHasRepeated && State->TextElement.KeyRepeatTime > 0.3f;
        /* NOTE: If the key repeat time is short enough, we may need to handle multiple repeats in a single frame. */
        b32 TheOtherKeyRepeats = State->TextElement.KeyHasRepeated && State->TextElement.KeyRepeatTime > 0.04f;

        if (InitialKeyRepeat || TheOtherKeyRepeats)
        {
            State->TextElement.KeyRepeatTime = 0.0f;
            State->TextElement.KeyHasRepeated = 1;

            HandleKey(&State->UI, &State->TextElement, State->LastKeyPressed);
            State->InputOccured = 1;
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
            key_state NextState = KeyStateMap[CurrentState][IsDown]; /* TODO: What is NextState used for? */
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

internal void ClearTextElement(text_element *TextElement)
{
    for (s32 I = 0; I <= TextElement->TextSize; ++I)
    {
        TextElement->Text[I] = 0;
    }
    TextElement->Index = 0;
}

debug_variable b32 FrameIndex = 0;

internal inline void SetQuizMode(state *State, quiz_mode Mode)
{
    State->QuizMode = Mode;
    FrameIndex = 0; /* NOTE: Set FrameIndex to 0 to force a redraw. */
}

debug_variable b32 DEBUGCanHandleEnterKeyInWinMode = 0; /* TODO: Just fix the Enter key-press bug, where the user hits the enter key, gets the last quiz item correct, so the win screen only appears for a single frame. */

internal s32 GetBucketSize(state *State, s32 BucketIndex)
{
    Assert(BucketIndex >= 0 && BucketIndex < frequency_Count);

    s32 Size = 0;
    /* @CopyPasta */
    s32 HighFrequencyThreshold = State->FrequencyThreshold[frequency_High];
    s32 MediumFrequencyThreshold = State->FrequencyThreshold[frequency_Medium];
    s32 LowFrequencyThreshold = State->QuizItemCount;

    switch(BucketIndex)
    {
    case frequency_High:
        Size = HighFrequencyThreshold;
        break;
    case frequency_Medium:
        Size = MediumFrequencyThreshold - HighFrequencyThreshold;
        break;
    case frequency_Low:
        Size = LowFrequencyThreshold - MediumFrequencyThreshold;
        break;
    }

    return Size;
}

internal s32 GetActiveQuizItemIndex(state *State)
{
    s32 LookupIndex = State->Bucket[State->BucketIndex];

    s32 Offset;

    s32 HighFrequencyThreshold = State->FrequencyThreshold[frequency_High];
    s32 MediumFrequencyThreshold = State->FrequencyThreshold[frequency_Medium];

    switch(State->BucketIndex)
    {
    case frequency_Medium:
        Offset = HighFrequencyThreshold;
        break;
    case frequency_Low:
        Offset = MediumFrequencyThreshold;
        break;
    default:
        Offset = 0;
    }

    s32 OffsetLookupIndex = LookupIndex + Offset;
    s32 Index = State->QuizItemsLookup[OffsetLookupIndex];
    Assert(Index >= 0 && Index < Quiz_Item_Max);

    return Index;
}

internal void GetNextRandomQuizItem(state *State)
{
    Assert(State->BucketIndex >= 0 && State->BucketIndex < frequency_Count);

    s32 MaxCount;
    s32 FrequencySize = GetBucketSize(State, State->BucketIndex);

    switch(State->BucketIndex)
    {
    case frequency_High:    MaxCount = 8; break;
    case frequency_Medium:  MaxCount = 2; break;
    /* NOTE: Low frequency. */
    default:                MaxCount = 1; break;
    }

    s32 FrequencyModulo = FrequencySize;

    if (!FrequencySize)
    {
        State->BucketIndex = 0;
        FrequencyModulo = 1;
    }

    State->Bucket[State->BucketIndex] += 1;
    State->Bucket[State->BucketIndex] = State->Bucket[State->BucketIndex] % FrequencyModulo;
    State->BucketCount[State->BucketIndex] += 1;

    if (State->BucketCount[State->BucketIndex] >= MaxCount ||
        FrequencySize == 1)
    {
        /* TODO: Permute the frequency bucket, but not the other buckets. */
        State->BucketCount[State->BucketIndex] = 0;

        for (s32 Tries = 0; Tries < frequency_Count; ++Tries)
        {
            State->BucketIndex = (State->BucketIndex + 1) % frequency_Count;

            if (GetBucketSize(State, State->BucketIndex) > 0)
            {
                break;
            }
        }

    }

    State->ShowAnswer = 0;

    ClearTextElement(&State->TextElement);
    State->TextElement.Index = 0;
    State->CurrentQuizItemFailed = 0;

    { /* NOTE: Randomly pick index for variable-answer quiz-items. */
        s32 Index = GetActiveQuizItemIndex(State);
        Assert(Index >= 0 && Index < Quiz_Item_Max);
        quiz_item *QuizItem = State->QuizItems + Index;

        switch(QuizItem->Type)
        {
        case quiz_item_VariableAnswer:
        {
            s32 RandomIndex = rand() % QuizItem->VariableAnswer.VariableAnswerCount;
            QuizItem->VariableAnswer.VariableAnswerIndex = RandomIndex;
        } break;
        default: break;
        }
    }

    if (State->Bucket[State->BucketIndex] >= (s32)State->QuizItemCount)
    {
        SetQuizMode(State, quiz_mode_Win);
        OkPlaySound(State->Win);
        DEBUGCanHandleEnterKeyInWinMode = 0;
    }
    else
    {
        SetQuizMode(State, quiz_mode_Typing);
        FrameIndex = 0;
    }
}

internal quiz_item *GetActiveQuizItem(state *State)
{
    s32 Index = GetActiveQuizItemIndex(State);
    Assert(Index >= 0 && Index < Quiz_Item_Max);
    quiz_item *Item = State->QuizItems + Index;

    return Item;
}

internal f32 GetCenteredTextX(state *State, u8 *Text, s32 LetterSpacing)
{
    Vector2 TextSize = MeasureTextEx(State->UI.Font, (char *)Text, State->UI.FontSize, LetterSpacing);
    f32 X = (SCREEN_WIDTH - TextSize.x) / 2.0f;

    return X;
}

internal void DrawWrappedText(state *State, u8 *Text, f32 MaxWidth, f32 Y, f32 LineHeight, f32 LetterSpacing)
{
    /* NOTE: Assumes that Y passed in is the Y position for the _last_ line.
       We probably want to allow the caller to specify which direction the lines grow.
    */
    Vector2 TextSize = MeasureTextEx(State->UI.Font, (char *)Text, State->UI.FontSize, 1);
    State->LineBufferIndex = 0;

    if (TextSize.x < MaxWidth)
    {
        f32 X = GetCenteredTextX(State, Text, LetterSpacing);
        DrawTextEx(State->UI.Font, (char *)Text, V2(X, Y), State->UI.FontSize, LetterSpacing, FONT_COLOR);
    }
    else
    {
        s32 I = 0;
        s32 J = 0;
        s32 Offset = 0;

        /* TODO: @Speed We should be able do better than a linear search */
        while (Text[I] && State->LineBufferIndex < Max_Line_Count)
        {
            J = I - Offset;
            Assert(J >= 0 && J < Max_Line_Buffer_Size);

            u8 *Line = State->LineBuffer[State->LineBufferIndex];

            Line[J] = Text[I];
            Line[J + 1] = 0;
            Assert(J > 0 || !IS_SPACE(Line[J]));
            Vector2 LineSize = MeasureTextEx(State->UI.Font, (char *)Line, State->UI.FontSize, LetterSpacing);

            if (LineSize.x > MaxWidth)
            {
                b32 SpaceFound = 0;

                /* NOTE: Seek back to the last space, and break the line there */
                while (J >= 0)
                {
                    if (IS_SPACE(Line[J]))
                    {
                        SpaceFound = 1;
                        Line[J] = 0;
                        Offset += J + 1;
                        I = Offset;

                        ++State->LineBufferIndex;

                        break;
                    }
                    else
                    {
                        while (Is_Utf8_Tail_Byte(Line[J--]));
                    }
                }

                /* NOTE: No space was found, which means we have a word that is longer than
                   the maximum line width. Break the word and put a dash at the end of the line.
                */
                if (!SpaceFound)
                {
                    s32 LastCharIndex = I - Offset;
                    Line[LastCharIndex] = '-';

                    Offset = I;
                }

                Assert(Offset == I);
            }
            else
            {
                ++I;
            }
        }

        f32 OffsetY = Y - (LineHeight * State->LineBufferIndex);

        /* NOTE: Draw the accumulated lines */
        for (s32 LineIndex = 0; LineIndex < State->LineBufferIndex; ++LineIndex)
        {
            u8 *Line = State->LineBuffer[LineIndex];
            f32 X = GetCenteredTextX(State, Line, LetterSpacing);
            DrawTextEx(State->UI.Font, (char *)Line, V2(X, OffsetY), State->UI.FontSize, LetterSpacing, FONT_COLOR);
            OffsetY += LineHeight;
        }

        /* NOTE: Draw any text remaining after the last line. */
        if (I > Offset)
        {
            u8 *RemainingText = Text + Offset;
            f32 X = GetCenteredTextX(State, RemainingText, LetterSpacing);
            DrawTextEx(State->UI.Font, (char *)RemainingText, V2(X, OffsetY), State->UI.FontSize, LetterSpacing, FONT_COLOR);
        }
    }
}

internal void DrawQuizItem(state *State, u32 LetterSpacing)
{
    quiz_item *QuizItem = GetActiveQuizItem(State);

    u8 *Prompt;
    u8 *Answer;

    f32 BorderPadding = 18.0f;
    f32 TextPadding = 4.0f;
    f32 LineHeight = TextPadding + State->UI.FontSize;

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
    case quiz_item_VariableAnswer:
    {
        variable_answer VA = QuizItem->VariableAnswer;
        s32 VariableAnswerIndex = VA.VariableAnswerIndex;
        Assert(VariableAnswerIndex >= 0);
        s32 PromptPartCount = VA.PromptPartCount;
        u8 Buffer[1024]; /* TODO: We should use some type of temp memory instead of this inline buffer. */
        s32 At = 0;

        for (s32 I = 0; I < PromptPartCount; ++I)
        {
            if (I > 0)
            {
                u8 *Variable = (u8 *)VA.Variables[VariableAnswerIndex][I - 1];
                s32 VariableSize = GetStringLength(Variable);
                CopyString(Variable, Buffer + At, 1024 - At);
                At += VariableSize;
            }

            u8 *PromptPart = (u8 *)VA.PromptParts[I];

            if (!PromptPart)
            {
                break;
            }

            s32 PromptPartSize = GetStringLength(PromptPart);
            CopyString(PromptPart, Buffer + At, 1024 - At);
            At += PromptPartSize;
        }

        s32 NullIndex = At < 1024 ? At : 1023;
        Buffer[NullIndex] = 0;

        Prompt = Buffer;
        Answer = (u8 *)VA.Answers[VariableAnswerIndex];
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
        DrawTextEx(State->UI.Font, Buff, V2(BorderPadding, BorderPadding), State->UI.FontSize, LetterSpacing, FONT_COLOR);
    }

    f32 MaxPromptWidth = MinF32(500.0f, SCREEN_WIDTH - 2.0f * BorderPadding);

    Vector2 AnswerSize = MeasureTextEx(State->UI.Font, (char *)Answer, State->UI.FontSize, 1);

    f32 AnswerOffsetX = AnswerSize.x / 2.0f;
    f32 AnswerX = SCREEN_HALF_WIDTH - AnswerOffsetX;
    f32 AnswerY = SCREEN_HALF_HEIGHT + (1 * LineHeight);

    /* f32 CursorX = SCREEN_HALF_WIDTH + (InputSize.x / 2.0f); */
    /* f32 CursorY = InputY; */

    if (State->ShowAnswer)
    {
        DrawTextEx(State->UI.Font, (char *)Answer, V2(AnswerX, AnswerY), State->UI.FontSize, LetterSpacing, ANSWER_COLOR);
    }

#if DEBUG_GRAPHICS
    { /* Draw the index of the quiz item. */
        char Buff[64];
        s32 Index = GetActiveQuizItemIndex(State);
        sprintf(Buff, "quiz_item_count=%d  index=%d", State->QuizItemCount, Index);
        DrawTextEx(State->UI.Font, Buff, V2(BorderPadding, BorderPadding + LineHeight), State->UI.FontSize, LetterSpacing, FONT_COLOR);
    }
    { /* Draw the bucket index. */
        char Buff[64];
        sprintf(Buff, "bucket_index=%d", State->BucketIndex);
        DrawTextEx(State->UI.Font, Buff, V2(BorderPadding, BorderPadding + 2 * LineHeight), State->UI.FontSize, LetterSpacing, FONT_COLOR);
    }
#endif


    f32 WrappedTextY = SCREEN_HALF_HEIGHT - (2.0f * LineHeight);
    DrawWrappedText(State, Prompt, MaxPromptWidth, WrappedTextY, LineHeight, LetterSpacing);

    if (State->QuizMode == quiz_mode_Correct)
    {
        char *CorrectString = "Correct";
        /* TODO: Scale up the Correct font size... but that requires increasing the font size that is loaded on web, to reduce blurry text... */
        f32 CorrectFontSize = State->UI.FontSize;
        f32 X = GetCenteredTextX(State, (u8 *)CorrectString, LetterSpacing);
        f32 OffsetY = (State->LineBufferIndex + 4) * LineHeight;
        Vector2 CorrectPosition = V2(X, SCREEN_HALF_HEIGHT - OffsetY);

        DrawTextEx(State->UI.Font, CorrectString, CorrectPosition, CorrectFontSize, LetterSpacing, (Color){20, 200, 40, 255});
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
        Vector2 TextPosition = V2((SCREEN_WIDTH / 2) - (TextSize.x / 2), (SCREEN_HEIGHT / 2) - (LineHeight));
        DrawTextEx(State->UI.Font, ConjugationText, TextPosition, State->UI.FontSize, LetterSpacing, CONJUGATION_TYPE_COLOR);
    }

    if (State->InputOccured)
    {
        State->UI.CursorBlinkTime = 0.0f;
    }

    DoText(&State->UI, &State->TextElement, State->DeltaTime);

    Vector2 NextButtonPosition = V2(SCREEN_WIDTH - BorderPadding, SCREEN_HEIGHT - BorderPadding);
    b32 NextPressed = DoButtonWith(&State->UI, ui_Next, (u8 *)"Next", NextButtonPosition, alignment_BottomRight);
    if (NextPressed)
    {
        GetNextRandomQuizItem(State);
    }

    Vector2 PreviousButtonPosition = V2(BorderPadding, SCREEN_HEIGHT - BorderPadding);
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

    buffer *Buffer = ReadFileIntoBuffer(SAVE_FILE_PATH);
    s32 HeaderSize = sizeof(save_file_header);

    if (Buffer)
    {
        if (Buffer->Size > HeaderSize)
        {
            save_file_header *Header = (save_file_header *)Buffer->Data;
            u32 QuizItemSize = Header->QuizItemCount * sizeof(quiz_item);
            u32 StringDataSize = Buffer->Size - (HeaderSize + QuizItemSize);

            Assert(Header->Version == Save_File_Header_Version);
            Assert(Buffer->Size == (s32)(HeaderSize + QuizItemSize + StringDataSize));
            Assert((u32)Buffer->Size == Header->FileSize);

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
                State->QuizItemCount = Header->QuizItemCount;
            }

            {
                u8 *QuizItemsLookupData = Buffer->Data + Header->OffsetToQuizItemsLookupData;
                CopyMemory(QuizItemsLookupData, (u8 *)State->QuizItemsLookup, sizeof(State->QuizItemsLookup));
            }

            State->Bucket[State->BucketIndex] = Header->Bucket[0];

            SaveFileHasLoaded = 1;
        }
    }

#endif
    return SaveFileHasLoaded;
}

internal void InitializeDefaultQuizItems(state *State);

internal inline s32 GetQuizItemScoreFromIndex(state *State, s32 Index)
{
    s32 Score = 0;

    if (Index >= 0 && Index < Quiz_Item_Max)
    {
        quiz_item Item = State->QuizItems[Index];
        Score = Item.PassCount - Item.FailCount;
    }

    return Score;
}

/* NOTE: @CopyPasta from Wikipedia... */
void MergeSortQuizItems(state *State)
{
    s32 *A = State->QuizItemsLookup;
    s32 *B = State->QuizItemsLookupWorkingSet;

    // NOTE: array A[] has the items to sort; array B[] is a work array
    // Each 1-element run in A is already "sorted".
    // Make successively longer sorted runs of length 2, 4, 8, 16... until the whole array is sorted.
    for (u32 Width = 1; Width < State->QuizItemCount; Width = 2 * Width)
    {
        // Array A is full of runs of length Width.
        for (u32 I = 0; I < State->QuizItemCount; I = I + 2 * Width)
        {
            // Merge two runs: A[I:I+Width-1] and A[I+Width:I+2*Width-1] to B[]
            // or copy A[I:Length-1] to B[] ( if (I+Width >= Length) )
            s32 iLeft = I;
            s32 iRight = MinS32(I+Width, State->QuizItemCount);
            s32 iEnd = MinS32(I+2*Width, State->QuizItemCount);

            s32 j = iRight;
            // While there are elements in the left or right runs...
            for (s32 k = iLeft; k < iEnd; k++)
            {
                s32 LeftValue = GetQuizItemScoreFromIndex(State, A[iLeft]);
                s32 JValue = GetQuizItemScoreFromIndex(State, A[j]);
                b32 Comparison = LeftValue <= JValue;
                // If left run head exists and is <= existing right run head.
                if (iLeft < iRight && (j >= iEnd || Comparison))
                {
                    B[k] = A[iLeft];
                    iLeft = iLeft + 1;
                }
                else
                {
                    B[k] = A[j];
                    j = j + 1;
                }
            }
        }

        // Now work array B is full of runs of length 2*Width.
        // Copy array B to array A for the next iteration.
        // A more efficient implementation would swap the roles of A and B.
        for (u32 I = 0; I < State->QuizItemCount; I++)
        {
            A[I] = B[I];
        }
        // Now array A is full of runs of length 2*Width.
    }
}

internal void ResetQuizItems(state *State)
{
    for (s32 I = 0; I < frequency_Count; ++I)
    {
        State->BucketCount[I] = 0;
    }

    for (u32 I = 0; I < State->QuizItemCount; ++I)
    {
        State->QuizItems[I].Complete = 0;
    }

    MergeSortQuizItems(State);

    s32 MaxHighFrequencyScore = 0;
    s32 MaxMediumFrequencyScore = 2;

    for (u32 I = 0; I < State->QuizItemCount; ++I)
    {
        s32 Index = State->QuizItemsLookupWorkingSet[I];
        quiz_item Item = State->QuizItems[Index];
        s32 Score = Item.PassCount - Item.FailCount;

        State->QuizItemsLookup[I] = Index; /* NOTE: Copy sorted array back into the lookup array. */

        if (Score < MaxHighFrequencyScore)
        {
            State->FrequencyThreshold[frequency_High] = I + 1;
        }
        else if (Score < MaxMediumFrequencyScore)
        {
            State->FrequencyThreshold[frequency_Medium] = I + 1;
        }
        else
        {
            break;
        }
    }

    if (State->FrequencyThreshold[frequency_High])
    {
        State->BucketIndex = frequency_High;
    }
    else if (State->FrequencyThreshold[frequency_Medium])
    {
        State->BucketIndex = frequency_Medium;
    }
    else if (State->QuizItemCount)
    {
        State->BucketIndex = frequency_Low;
    }
    else
    {
        State->BucketIndex = frequency_High;
    }
}

internal void PermuteQuizItemRange(state *State, s32 BeginIndex, s32 EndIndex)
{
    s32 PermutationCount = EndIndex - BeginIndex;

    if (PermutationCount)
    {
        /* NOTE: Set initial lookup index */
        for (s32 I = BeginIndex; I <= EndIndex; ++I)
        {
            State->QuizItemsLookup[I] = I;
        }

        for (s32 I = 0; I < PermutationCount; ++I)
        {
            s32 Index = I + BeginIndex;
            s32 RandomIndex = (rand() % PermutationCount) + BeginIndex;

            /* NOTE: Swap the quiz item indices */
            s32 Temp = State->QuizItemsLookup[Index];
            State->QuizItemsLookup[Index] = State->QuizItemsLookup[RandomIndex];
            State->QuizItemsLookup[RandomIndex] = Temp;
        }
    }
}

internal void InitializeQuizItems(state *State, b32 InitializeDefault)
{
    b32 SaveFileHasLoaded = TryToLoadSaveFile(State);

    for (s32 I = 0; I < frequency_Count; ++I)
    {
        State->Bucket[I] = 0;
    }
    State->BucketIndex = 0;

    if (!SaveFileHasLoaded)
    {
        if (InitializeDefault)
        {
            State->QuizItemCount = 0;
            InitializeDefaultQuizItems(State);
        }
    }

    ResetQuizItems(State);

    s32 HighThreshold = State->FrequencyThreshold[frequency_High];
    s32 MediumThreshold = State->FrequencyThreshold[frequency_Medium];
    s32 LowThreshold = State->QuizItemCount;

    PermuteQuizItemRange(State, 0, HighThreshold);
    PermuteQuizItemRange(State, HighThreshold, MediumThreshold);
    PermuteQuizItemRange(State, MediumThreshold, LowThreshold);

#if 1
    { /* DEBUG: Test that the list does not contain duplicates */
        for (u32 I = 0; I < State->QuizItemCount; ++I)
        {
            for (u32 J = 0; J < State->QuizItemCount; ++J)
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

internal void DrawWinMessage(state *State, u32 LetterSpacing)
{
    if (DEBUGCanHandleEnterKeyInWinMode)
    {
        char *WinMessage = "Tú ganas!";
        Vector2 TextSize = MeasureTextEx(State->UI.Font, WinMessage, State->UI.FontSize, LetterSpacing);
        Vector2 TextPosition = V2((SCREEN_WIDTH / 2) - (TextSize.x / 2), SCREEN_HEIGHT / 2);
        DrawTextEx(State->UI.Font, WinMessage, TextPosition, State->UI.FontSize, LetterSpacing, FONT_COLOR);

        if (IsKeyPressed(KEY_ENTER))
        {
            ResetQuizItems(State);
            ClearTextElement(&State->TextElement);

            SetQuizMode(State, quiz_mode_Typing);

            InitializeQuizItems(State, 0);
        }
    }
    else if (!IsKeyPressed(KEY_ENTER))
    {
        DEBUGCanHandleEnterKeyInWinMode = 1;
    }
}

internal void DrawUnknownQuizItem(state *State)
{
    DrawText("Unknown quiz type\n", 20, 20, 22, (Color){220,100,180,255});

    Vector2 NextButtonPosition = V2(SCREEN_WIDTH - State->UI.FontSize, SCREEN_HEIGHT - State->UI.FontSize);
    b32 NextPressed = DoButtonWith(&State->UI, ui_Next, (u8 *)"Next", NextButtonPosition, alignment_BottomRight);

    if (NextPressed)
    {
        GetNextRandomQuizItem(State);
    }
}

internal void HandleQuizItem(state *State, quiz_item *QuizItem)
{
    int LetterSpacing = 1;
    u8 *Answer;

    switch(QuizItem->Type)
    {
    case quiz_item_Conjugation: Answer = QuizItem->Conjugation.Answer; break;
    case quiz_item_Text: Answer = QuizItem->Text.Answer; break;
    case quiz_item_VariableAnswer:
    {
        s32 RandomIndex = QuizItem->VariableAnswer.VariableAnswerIndex;
        Answer = (u8 *)QuizItem->VariableAnswer.Answers[RandomIndex];
    } break;
    default:
    {
        Answer = (u8 *)"";
    }
    }

    if (IsKeyPressed(KEY_ENTER))
    {
        if (State->QuizMode == quiz_mode_Typing)
        {
            if (StringsMatch((char *)Answer, (char *)State->TextElement.Text))
            {
                SetQuizMode(State, quiz_mode_Correct);
                OkPlaySound(State->Correct);
                QuizItem->Complete = 1;
                QuizItem->PassCount += 1;
            }
            else
            {
                OkPlaySound(State->Wrong);

                if (!State->CurrentQuizItemFailed)
                {
                    QuizItem->FailCount += 1;
                    State->CurrentQuizItemFailed = 1;
                }
            }
        }
        else if (State->QuizMode == quiz_mode_Correct)
        {
            GetNextRandomQuizItem(State);
        }
    }

    if (State->QuizMode == quiz_mode_Typing || State->QuizMode == quiz_mode_Correct)
    {
        DrawQuizItem(State, LetterSpacing);
    }
    else if (State->QuizMode == quiz_mode_Win)
    {
        DrawWinMessage(State, LetterSpacing);
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

        switch(QuizItem->Type)
        {
        case quiz_item_Conjugation:
        case quiz_item_Text:
        case quiz_item_VariableAnswer:
        {
            HandleQuizItem(State, QuizItem);
        } break;
        default:
        {
            DrawUnknownQuizItem(State);
        }
        }

#ifndef PLATFORM_WEB
        { /* NOTE: Overlay graphics on top of all view types. */
            f32 Spacing = State->UI.FontSize;
            Vector2 ButtonPosition = V2(SCREEN_WIDTH - Spacing, Spacing);
            b32 SavePressed = DoButtonWith(&State->UI, ui_Save, (u8 *)"Save", ButtonPosition, alignment_TopRight);

            if (SavePressed)
            {
                WriteSaveFileToDisk(State, SAVE_FILE_PATH);
            }
        }
#endif
    }

    EndDrawing();
}

int main(void)
{
    Assert(key_state_Count >= (1 << Bits_Per_Key_State) - 1); /* NOTE: What is this assertion about? */
    Assert((1 << Key_State_Chunk_Size_Log2) == 8 * sizeof(key_state_chunk));
    Assert((1 << Key_State_Chunk_Count_Log2) == Key_State_Chunk_Count);

    state State = {0};
    State.UI.CursorBlinkRate = 1.0f;

    u8 TextBuffer[1024] = {0};
    State.TextElement.TextSize = 1024;
    State.TextElement.Text = TextBuffer;
    State.TextElement.Position = V2(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
    State.TextElement.Color = (Color){255, 255, 255, 255};

#if !PLATFORM_WEB
    /* TODO: Update raylib binary to see if we can init audio device on web!
       Currently, we get an error that leads to the following issue:
           https://github.com/raysan5/raylib/issues/3441
    */
    {
        InitAudioDevice();

        Wave Correct = LoadWaveFromMemory(".wav", SfxCorrectData, sizeof(SfxCorrectData));
        /* TODO: Generate data for wrong and win, then use LoadWaveFromMemory */
        Wave Wrong = LoadWave("../assets/sfx_wrong.wav");
        Wave Win = LoadWave("../assets/sfx_win.wav");

        if (IsWaveReady(Correct) && IsWaveReady(Wrong) && IsWaveReady(Win))
        {
            State.Correct = LoadSoundFromWave(Correct);
            SetSoundVolume(State.Correct, 0.55f);
            State.Wrong = LoadSoundFromWave(Wrong);
            State.Win = LoadSoundFromWave(Win);
            SetSoundVolume(State.Win, 0.55f);
        }
        else
        {
            printf("Error: Wave not ready!\n");
        }
    }
#endif


#if defined(PLATFORM_WEB)
    InitRaylibCanvas();
    State.UI.FontSize = 28;
#else
    State.UI.FontSize = 28;
#endif

    int Result = 0;
    b32 ShouldClose = 0;

    srand(time(NULL));
    InitializeQuizItems(&State, 1);
    /* TODO: Check if any anti-aliasing flags (like FLAG_MSAA_4X_HINT) affect the web-platform text quality. */
    /* SetConfigFlags(FLAG_MSAA_4X_HINT); */

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Estudioso");
#if !defined(PLATFORM_WEB)
    SetWindowPosition(0, 0);
#endif
    SetTargetFPS(60);

    int CodepointCount = 0;
    int *Codepoints = 0;

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

#if PLATFORM_WEB
        /* TODO: It initially seemed that scaling fonts at all on the web produced blurry text.
           Investigate whether that is _really_ true, and if so, set up web fonts so that they
           are loaded and drawn as 1:1 fonts. This means loading fonts per font sizes in the
           game? Not sure about this one at the moment...
        */
        u32 FontScale = 1;
        /* TODO: Fix loading font-data-files with LoadFontFromMemory for web,
           somehow it doesn't work anymore :(
        */
        State.UI.Font = GetFontDefault();
#else
        u32 FontScale = 4;
        State.UI.Font = LoadFontFromMemory(".ttf", FontData, ArrayCount(FontData), State.UI.FontSize*FontScale, Codepoints, CodepointCount);
#endif

        if (IsFontReady(State.UI.Font))
        {
            UnloadCodepoints(Codepoints);
        }
        else
        {
            ShouldClose = 1;
        }

    }

    SetQuizMode(&State, quiz_mode_Typing);

    for (;;)
    {
        if (ShouldClose || WindowShouldClose())
        {
            /* WriteSaveFileToDisk(&State, SAVE_FILE_PATH); */
            break;
        }

        { /* time calculation */
            f32 CurrentTime = GetTime();
            State.DeltaTime = CurrentTime - State.Time;
            State.Time = CurrentTime;
        }

        b32 ForceDraw = FrameIndex == 0;

        UpdateAndRender(&State, ForceDraw);
    }

    CloseWindow();

    return Result;
}
