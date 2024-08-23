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

#define Screen_Width 960
#define Screen_Height 540
#define Tile_Size 20

#include "../src/types.h"
#include "../src/core.c"
#include "../src/math.c"
#include "../src/raylib_helpers.h"
#include "../src/ui.c"

#include "../gen/influence.h"

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
    /* air, food, drink, shelter, warmth, sex, sleep, etc. */
    need_kind_Physiological = 0x1,

    /* protection from elements, security, order, law, stability, freedom from fear. */
    need_kind_Safety = 0x2,

    /* friendship, intimacy, trust, and acceptance, receiving and giving affection and love. */
    need_kind_Love = 0x4,

    /* (i) esteem for oneself and (ii) the need to be accepted and valued by others . */
    need_kind_Esteem = 0x8,

    /* knowledge and understanding, curiosity, exploration, need for meaning and predictability. */
    need_kind_Cognitive = 0x10,

    /* appreciation and search for beauty, balance, form, etc. */
    need_kind_SelfActualization = 0x20,

    /* A person is motivated by values that transcend beyond the personal self. */
    need_kind_Transcendence = 0x40,
} need_kind;

#define Need_Type_XList\
    X(Drink,    need_kind_Physiological)\
    X(Food,     need_kind_Physiological)\
    X(Sleep,    need_kind_Physiological)\
    X(Shelter,  need_kind_Physiological)\
    X(Security, need_kind_Safety       )\
    X(Trust,    need_kind_Love         )

#define X(need, kind) need_type_##need,
typedef enum
{
    Need_Type_XList
    need_type__Count
} need_type;
#undef X

#define X(need, kind) [need_type_##need] = kind,
need_kind NeedKindTable[need_type__Count] = {
    Need_Type_XList
};
#undef X

typedef struct need need;

struct need
{
    need_type Type;
    need_kind Kind;
    need *Next;
};

typedef s32 entity_id;

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
    v2s32 Tile;
    Vector2 Offset;
} world_position;

typedef enum
{
    entity_id__Null,
    entity_id_Player,
    entity_id_Somebody,
    entity_id__Count,
} entity_id_name;

typedef struct
{
    entity_id Id;
    emotion Emotion;
    need Needs;
    world_position WorldPosition;
} entity;

#define Max_Entities 64
#define Relationship_Count (Max_Entities * Max_Entities)

typedef struct
{
    v2s32 Direction;
} user_input;

#define Word_Table_Size 128
typedef struct
{
    user_input UserInput;
    ui Ui;
    relationship Relationships[Relationship_Count];
    ryn_string WordTable[Word_Table_Size];
    entity Entities[Max_Entities];
    Vector2 CameraPosition;
    Texture2D AssetTexture;
} world;

internal ryn_string GetWord(world *World, word_id Id)
{
    ryn_string String = World->WordTable[Id];
    return String;
}

internal entity *GetEntity(world *World, entity_id Id)
{
    Assert(Id > 0 && Id < Max_Entities);
    return World->Entities + Id;
}

#define PushWord(c_string) \
    Assert(I < Word_Table_Size); \
    World->WordTable[I++] = CreateString(c_string);

#define Adjective    part_of_speech_Adjective
#define Adverb       part_of_speech_Adverb
#define Conjunction  part_of_speech_Conjunction
#define Determiner   part_of_speech_Determiner
#define Interjection part_of_speech_Interjection
#define Noun         part_of_speech_Noun
#define Preposition  part_of_speech_Preposition
#define Pronoun      part_of_speech_Pronoun
#define Verb         part_of_speech_Verb

#define All_Words_List \
    X(hello, (Interjection | Noun | Verb)) \
    X(world, (Noun)) \
    X(there, (Adverb | Pronoun | Noun | Adjective | Interjection)) \
    X(WORD_COUNT, 0)

#undef Adjective
#undef Adverb
#undef Conjunction
#undef Determiner
#undef Interjection
#undef Noun
#undef Preposition
#undef Pronoun
#undef Verb

#define X(word, pos) _##word,
typedef enum
{
    All_Words_List
} word;
#undef X

internal void InitializeWordTable(world *World)
{
    s32 I = 0;

    #define X(word, pos) PushWord(#word);
    All_Words_List
    #undef X
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

internal void DrawEntity(world *World, entity_id EntityId)
{
    if (IsTextureReady(World->AssetTexture))
    {
        entity *Entity = GetEntity(World, EntityId);
        Rectangle Source = {0.0f, 0.0f, 100.0f, 200.0f};
        b32 ShouldDraw = 1;

        switch (EntityId)
        {
        case entity_id_Player: break;
        case entity_id_Somebody: Source.x = 0; break;
        default: ShouldDraw = 0; break;
        }

        if (ShouldDraw)
        {
            Vector2 ScreenHalfSize = MultiplyV2S(V2(Screen_Width, Screen_Height), 0.5f);
            Vector2 Position = MultiplyV2S(V2(Entity->WorldPosition.Tile.x, Entity->WorldPosition.Tile.y), Tile_Size);
            Vector2 ScreenPosition = AddV2(ScreenHalfSize,
                                           SubtractV2(Position, World->CameraPosition));
            Rectangle Destination = (Rectangle){ScreenPosition.x, ScreenPosition.y, Tile_Size, 2.0f*Tile_Size};
            DrawTexturePro(World->AssetTexture, Source, Destination, V2(0, 0), 0, (Color){255, 255, 255, 255});
        }
    }
    else
    {
        printf("Asset texture not ready\n");
    }
}

internal void DrawWorld(world *World)
{
    DrawEntity(World, entity_id_Player);
    DrawEntity(World, entity_id_Somebody);
}

internal b32 LoadAssetTexture(world *World)
{
    b32 Error = 0;
#if 0
    s32 AssetSize = ArrayCount(AssetData);

    if (AssetSize > 2)
    {
        u32 AssetWidth = AssetData[0];
        u32 AssetHeight = AssetData[1];
        u64 PixelCount = AssetWidth * AssetHeight;

        Image Image = GenImageColor(AssetWidth, AssetHeight, (Color){255,0,255,255});

        for (u64 I = 2; I < PixelCount + 2; ++I)
        {
            u32 X = (I - 2) % AssetWidth;
            u32 Y = (I - 2) / AssetWidth;
            if (X < AssetWidth && X >= 0 && Y < AssetHeight && Y >= 0)
            {
                /* NOTE: We could pack our colors and just cast the u32,
                 * but for now we break the color components out and re-write them. */
                u8 R = (AssetData[I] >> 24) & 0xff;
                u8 G = (AssetData[I] >> 16) & 0xff;
                u8 B = (AssetData[I] >>  8) & 0xff;
                u8 A = (AssetData[I]      ) & 0xff;
                Color PixelColor = (Color){R, G, B, A};

                ImageDrawPixel(&Image, X, Y, PixelColor);
            }
            else
            {
                printf("[Error] Out of bounds asset access.");
                break;
            }
        }

        World->AssetTexture = LoadTextureFromImage(Image);
    }
    else
    {
        printf("[Error] Asset is too small to be useful, there was likely an error generating game assets.");
    }
#else
    World->AssetTexture = LoadTexture("../assets/influence.png");
#endif

    return Error;
}

internal void InitializeWorld(world *World)
{
    entity *Player = GetEntity(World, entity_id_Player);
    entity *Somebody = GetEntity(World, entity_id_Somebody);

    LoadAssetTexture(World);

    Player->WorldPosition.Tile = (v2s32){1, 2};
    Somebody->WorldPosition.Tile = (v2s32){8, 5};

    World->CameraPosition = Player->WorldPosition.Offset;
}

internal void TestUpdatePlayer(world *World)
{
    v2s32 *Direction = &World->UserInput.Direction;

    if (Direction->x ^ Direction->y)
    {
        entity *Player = GetEntity(World, entity_id_Player);
        v2s32 NewTile = Addv2s32(*Direction, Player->WorldPosition.Tile);
        b32 PlayerCanMove = 1;

        for (s32 I = 1; I < Max_Entities; ++I)
        {
            if (I != entity_id_Player)
            {
                entity *TestEntity = GetEntity(World, I);

                if (AreEqualv2s32(TestEntity->WorldPosition.Tile, NewTile))
                {
                    PlayerCanMove = 0;
                    break;
                }
            }
        }

        if (PlayerCanMove)
        {
            Player->WorldPosition.Tile = NewTile;
        }
    }
}

internal void HandleUserInput(world *World)
{
    World->UserInput.Direction = (v2s32){0,0};

    if (IsKeyPressed(KEY_W))
    {
        World->UserInput.Direction.y -= 1;
    }
    else if (IsKeyPressed(KEY_D))
    {
        World->UserInput.Direction.x += 1;
    }
    else if (IsKeyPressed(KEY_S))
    {
        World->UserInput.Direction.y += 1;
    }
    else if (IsKeyPressed(KEY_A))
    {
        World->UserInput.Direction.x -= 1;
    }
}

int main(void)
{
    u64 MaxArenaSize = 1*Megabyte;
    Assert(sizeof(world) < MaxArenaSize);

    ryn_memory_arena Arena = ryn_memory_CreateArena(sizeof(world));
    ryn_memory_arena FrameArena = ryn_memory_CreateArena(Megabyte);
    world *World = ryn_memory_PushStruct(&Arena, world);

    InitWindow(Screen_Width, Screen_Height, "Influence");
    SetTargetFPS(60);

    InitializeWorld(World);
    InitializeWordTable(World);


    { /* NOTE: Initialize world. */
        World->Ui.Font = GetFontDefault();
        World->Ui.FontSize = 16.0f;
    }

    sentence Sentence = {0};
    {
        Sentence.FirstPartOfSpeech.Part.PartOfSpeech = part_of_speech_Interjection;
        Sentence.FirstPartOfSpeech.Part.Words.Id = _world;

        part_of_speech_list SecondPartOfSpeech = {0};
        SecondPartOfSpeech.Part.PartOfSpeech = part_of_speech_Noun;
        SecondPartOfSpeech.Part.Words.Id = _hello;

        Sentence.FirstPartOfSpeech.Next = &SecondPartOfSpeech;
    }

    while (!WindowShouldClose())
    {
        HandleUserInput(World);
        TestUpdatePlayer(World);

        BeginDrawing();
        ClearBackground((Color){40,0,50,255});
        DrawWorld(World);
        EndDrawing();
    }

    /* printf("sizeof(world) %lu\n", sizeof(world)); */

    DebugPrintSentence(World, &Sentence);
    printf("Word count %d\n", _WORD_COUNT);

    return 0;
}
