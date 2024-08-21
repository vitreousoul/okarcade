/*
  Experimenting with generating dialogue and npc-actions based on:
  - emotional state
  - needs/goals
  - trust in others

  This kind of system will get deep and complicated very quickly.
  There should be a simple win/loss condition from the start.

  Think about Ursula La Guin's writing about the seed bag holder.
  Less violence and more cultivation, however, let's not let the
  cultivation turn into simple "survival" mechanics.
  This is about gathering influence, whether for good or bad.
*/

#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "../lib/raylib.h"

#include "../lib/ryn_memory.h"
#include "../lib/ryn_string.h"

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540

#include "../src/types.h"
#include "../src/core.c"
#include "../src/math.c"
#include "../src/raylib_helpers.h"
#include "../src/ui.c"

#define Kilobyte (1024)
#define Megabyte (1024*1024)
#define Gigabyte (1024*1024*1024)

typedef enum
{
    part_of_speech_Adjective    = 0x1,   /* describes things */
    part_of_speech_Adverb       = 0x2,   /* tell us about actions */
    part_of_speech_Conjunction  = 0x4,   /* join things */
    part_of_speech_Determiner   = 0x8,   /* specify things or say how many */
    part_of_speech_Interjection = 0x10,  /* express emotions */
    part_of_speech_Noun         = 0x20,  /* name things */
    part_of_speech_Preposition  = 0x40,  /* show relationships between words */
    part_of_speech_Pronoun      = 0x80,  /* replace nouns */
    part_of_speech_Verb         = 0x100, /* are actions */
} part_of_speech_flag;

typedef enum
{
    sentence_type_Declarative   = 0x1,
    sentence_type_Interrogative = 0x2,
    sentence_type_Imperative    = 0x4,
    sentence_type_Exclamatory   = 0x8,
} sentence_type;

typedef u32 word_id;

typedef struct word_list word_list;

struct word_list
{
    word_list *Next;
    word_id Id;
};

typedef struct
{
    part_of_speech_flag PartOfSpeech;
    word_list Words;
} part_of_speech;

typedef struct part_of_speech_list part_of_speech_list;
struct part_of_speech_list
{
    part_of_speech_list *Next;
    part_of_speech Part;
};

typedef struct
{
    part_of_speech Subject;
} context;

typedef struct
{
    sentence_type Type;
    part_of_speech_list FirstPartOfSpeech;
} sentence;

typedef enum
{
    /* TODO: Fill out the other four emotions from Plutchick wheel. */
    emotion_Joy,
    emotion_Sadness,
    emotion_Fear,
    emotion_Anger,
} emotion_type;

typedef struct
{
    emotion_type Type;
    s32 Intensity;
} emotion;

typedef enum
{
    need_flag_Physiological     = 0x1,
    need_flag_Safety            = 0x2,
    need_flag_Love              = 0x4,
    need_flag_Esteem            = 0x8,
    need_flag_Cognitive         = 0x10,
    need_flag_SelfActualization = 0x20,
    need_flag_Transcendence     = 0x40,
} need_flag;

typedef struct
{
    s32 Id;
} entity_id;

typedef struct
{
    entity_id From;
    entity_id To;

    s32 Trust;
    s32 Love;
    s32 Familiarity;
} relationship;

typedef struct
{
    entity_id Id;

    emotion Emotion;
} entity;

#define Max_Entities 64
#define Relationship_Count (Max_Entities * Max_Entities)

#define Word_Table_Size 128
typedef struct
{
    ui Ui;
    relationship Relationships[Relationship_Count];
    ryn_string WordTable[Word_Table_Size];
} world;

internal ryn_string GetWord(world *World, word_id Id)
{
    ryn_string String = World->WordTable[Id];
    return String;
}

#define PushWord(c_string) \
    Assert(I < Word_Table_Size); \
    World->WordTable[I++] = CreateString(c_string);

internal void InitializeWordTable(world *World)
{
    s32 I = 0;

    PushWord("hello");
    PushWord("world");
}
#undef PushWord

internal void DebugPrintString(ryn_string String)
{
    for (u64 I = 0; I < String.Size; ++I)
    {
        printf("%c", String.String[I]);
    }
}

internal void DebugPrintSentence(world *World, sentence *Sentence)
{
    if (Sentence != 0)
    {
        part_of_speech_list *Part = &Sentence->FirstPartOfSpeech;
        while (Part != 0)
        {
            word_list *Word = &Part->Part.Words;
            while (Word != 0)
            {
                ryn_string WordString = GetWord(World, Word->Id);
                DebugPrintString(WordString);
                printf(" ");
                Word = Word->Next;
            }
            Part = Part->Next;
        }
        printf("\n");
    }
}

int main(void)
{
    u64 MaxArenaSize = 10*Megabyte;
    Assert(sizeof(world) < MaxArenaSize);
    ryn_memory_arena Arena = ryn_memory_CreateArena(sizeof(world));
    world *World = ryn_memory_PushStruct(&Arena, world);

    InitializeWordTable(World);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Influence");
    SetTargetFPS(60);

    { /* NOTE: Initialize world. */
        World->Ui.Font = GetFontDefault();
        World->Ui.FontSize = 16.0f;
    }

    f32 LetterSpacing = 1.0f;

    while (!WindowShouldClose())
    {
        ui *Ui = &World->Ui;
        f32 LineHeight = Ui->FontSize + 0.1f*Ui->FontSize;


        BeginDrawing();

        u8 *Text = (u8 *)"This is some text that should be wrapping in some way...";
        Color FontColor = (Color){255, 255, 255, 255};
        f32 MaxTextWidth = 140.0f;
        DrawWrappedText(Ui, Text, MaxTextWidth, 20.0f, 30.0f, LineHeight, LetterSpacing, FontColor);
        DrawRectangleLines(20.0f, 30.0f, MaxTextWidth, 10.0f, (Color){0, 255, 255, 255});

        EndDrawing();
    }

    /* sentence Sentence = {0}; */
    /* Sentence.FirstPartOfSpeech.Part.PartOfSpeech = part_of_speech_Interjection; */
    /* Sentence.FirstPartOfSpeech.Part.Words.Id = 0; */

    /* part_of_speech_list SecondPartOfSpeech = {0}; */
    /* SecondPartOfSpeech.Part.PartOfSpeech = part_of_speech_Noun; */
    /* SecondPartOfSpeech.Part.Words.Id = 1; */

    /* Sentence.FirstPartOfSpeech.Next = &SecondPartOfSpeech; */

    /* printf("sizeof(world) %lu\n", sizeof(world)); */

    /* DebugPrintSentence(World, &Sentence); */

    return 0;
}
