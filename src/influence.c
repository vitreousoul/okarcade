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

#define Kilobyte (1024)
#define Megabyte (1024*1024)
#define Gigabyte (1024*1024*1024)

#define ref_struct(name)\
    typedef struct name name;\
    struct name

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
    sentence_type__Null,
    sentence_type_Declarative,
    sentence_type_Interrogative,
    sentence_type_Imperative,
    sentence_type_Exclamatory,
} sentence_type;

typedef u32 word_id;

#define Word_Chunk_Size 64
ref_struct(word_chunk)
{
    word_chunk *Next;
    word_id Words[Word_Chunk_Size];
    s32 Count;
};

typedef struct
{
    part_of_speech_flag PartOfSpeech;
    word_chunk Words;
} part_of_speech;

ref_struct(part_of_speech_list)
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

ref_struct(sentence_list)
{
    sentence Sentence;
    sentence_list *Next;
};

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
    v2s32 ConversationDirection;
} user_input;

ref_struct(message)
{
    sentence_list Sentences;
    message *Next;
    message *Responses;
};

typedef struct
{
    message *FirstMessage;
} conversation;

typedef enum
{
    world_mode_Null,
    world_mode_Walking,
    world_mode_Talking,
} world_mode;

#define Message_Pool_Size 1024
typedef struct
{
    message Messages[Message_Pool_Size];
    message *Free;
    s32 Index;
} message_pool;

#define Word_Table_Size 128
typedef struct
{
    relationship Relationships[Relationship_Count];
    entity Entities[Max_Entities];
    Vector2 CameraPosition;
    Texture2D AssetTexture;
    conversation Conversation;
    world_mode Mode;
    message_pool MessagePool;
} world;

global_variable ryn_string WordTable[Word_Table_Size];

typedef struct
{
    world World;
    ui Ui;
    user_input UserInput;
    ryn_memory_arena FrameArena;
} game;

typedef enum
{
    ui_element__Null,
    ui_element_CommandLine,
    ui_element__Count,
} ui_element_name;

internal ryn_string GetWord(word_id Id)
{
    ryn_string String = WordTable[Id];
    return String;
}

internal entity *GetEntity(world *World, entity_id Id)
{
    Assert(Id > 0 && Id < Max_Entities);
    return World->Entities + Id;
}

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
    X(NULL, 0) \
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
    word__Count
} word;
#undef X

#define AddWordToTable__(c_string) \
    Assert(I < Word_Table_Size);\
    WordTable[I] = ryn_string_CreateString(c_string);\
    WordTable[I++].Size -= 1;

internal void InitializeWordTable(world *World)
{
    s32 I = 0;

    #define X(word, pos) AddWordToTable__(#word);
    All_Words_List
    #undef X
}

internal void DebugPrintString(ryn_string String)
{
    for (u64 I = 0; I < String.Size; ++I)
    {
        printf("%c", String.Bytes[I]);
    }
}

internal void DebugPrintSentence(sentence *Sentence)
{
    if (Sentence != 0)
    {
        part_of_speech_list *Part = &Sentence->FirstPartOfSpeech;
        while (Part != 0)
        {
            word_chunk *Words = &Part->Part.Words;
            while (Words != 0)
            {
                for (s32 I = 0; I < Words->Count; ++I)
                {
                    word_id WordId = Words->Words[I];
                    ryn_string WordString = GetWord(WordId);
                    DebugPrintString(WordString);
                    printf(" ");
                }

                Words = Words->Next;
            }
            Part = Part->Next;
        }
        printf("\n");
    }
}

internal void DrawEntity(game *Game, entity_id EntityId)
{
    world *World = &Game->World;

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

internal void PushWord(ryn_memory_arena *Arena, word Word, u8 TrailingCharacter)
{
    Assert(Word >= 0 && Word < word__Count);
    ryn_string String = WordTable[Word];
    u64 Size = String.Size;
    u64 PushSize = Size;

    if (TrailingCharacter)
    {
        PushSize += 1;
    }

    u8 *At = ryn_memory_PushSize(Arena, PushSize);

    for (u64 I = 0; I < Size; ++I)
    {
        At[I] = String.Bytes[I];
    }

    if (TrailingCharacter)
    {
        At[Size] = TrailingCharacter;
    }
}

internal void DrawMessage(game *Game, message *Message)
{
    sentence_list *Sentences = &Message->Sentences;
    ryn_memory_arena *FrameArena = &Game->FrameArena;
    u64 MessageOffset = FrameArena->Offset;

    do
    {
        part_of_speech_list *Parts = &Sentences->Sentence.FirstPartOfSpeech;

        do
        {
            part_of_speech Part = Parts->Part;

            for (s32 I = 0; I < Part.Words.Count; ++I)
            {
                PushWord(FrameArena, Part.Words.Words[I], ' ');
            }

            Parts = Parts->Next;
        } while (Parts);

        Sentences = Sentences->Next;
    } while (Sentences);

    u8 *NullCharacter = ryn_memory_PushSize(FrameArena, 1);
    *NullCharacter = 0;

    f32 MaxWidth = 0.8f * Screen_Width;
    f32 PaddingX = 0.5f*(Screen_Width - MaxWidth);
    f32 PaddingY = 0.8f * Screen_Height;
    u8 *Text = FrameArena->Data + MessageOffset;
    f32 LineHeight = Game->Ui.FontSize + 4.0f;
    f32 LetterSpacing = 1.0f;
    Color FontColor = (Color){230, 240, 220, 255};

    DrawWrappedText(&Game->Ui, Text, MaxWidth, PaddingX, PaddingY, LineHeight, LetterSpacing, FontColor, alignment_BottomCenter);
}

internal void DrawConversation(game *Game)
{
    world *World = &Game->World;

    if (World->Conversation.FirstMessage)
    {
        message *Message = World->Conversation.FirstMessage;
        DrawMessage(Game, Message);
    }
}

internal void DrawWorld(game *Game)
{
    DrawEntity(Game, entity_id_Player);
    DrawEntity(Game, entity_id_Somebody);

    if (Game->World.Mode == world_mode_Talking)
    {
        DrawConversation(Game);
    }
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
    World->Mode = world_mode_Walking;

    entity *Player = GetEntity(World, entity_id_Player);
    entity *Somebody = GetEntity(World, entity_id_Somebody);

    LoadAssetTexture(World);

    Player->WorldPosition.Tile = (v2s32){1, 2};
    Somebody->WorldPosition.Tile = (v2s32){8, 5};

    World->CameraPosition = Player->WorldPosition.Offset;
}

internal message *CreateMessage(world *World)
{
    message *Message = 0;

    if (World->MessagePool.Free)
    {
        Message = World->MessagePool.Free;
        World->MessagePool.Free = World->MessagePool.Free->Next;
    }
    else if (World->MessagePool.Index < Message_Pool_Size)
    {
        Message = World->MessagePool.Messages + World->MessagePool.Index;
    }

    return Message;
}

internal sentence GenerateSentence(sentence_type Type)
{
    sentence Sentence = {};
    Sentence.Type = Type;

    switch (Type)
    {
    case sentence_type_Declarative:
    {
        NotImplemented;
    } break;
    case sentence_type_Interrogative:
    {
        NotImplemented;
    } break;
    case sentence_type_Imperative:
    {
        NotImplemented;
    } break;
    case sentence_type_Exclamatory:
    {
        Sentence.FirstPartOfSpeech.Part.PartOfSpeech = part_of_speech_Interjection;
        u64 Count = 0;
        Sentence.FirstPartOfSpeech.Part.Words.Words[Count++] = _hello;
        Sentence.FirstPartOfSpeech.Part.Words.Words[Count++] = _there;
        Sentence.FirstPartOfSpeech.Part.Words.Words[Count++] = _world;
        Sentence.FirstPartOfSpeech.Part.Words.Count = Count;
    } break;
    default:
    {
        NotImplemented;
    } break;
    }

    return Sentence;
}

internal void TestUpdatePlayer(world *World, user_input *UserInput)
{
    if (World->Mode == world_mode_Walking)
    {
        v2s32 *Direction = &UserInput->Direction;

        if (Direction->x ^ Direction->y)
        {
            entity *Player = GetEntity(World, entity_id_Player);
            v2s32 NewTile = Addv2s32(*Direction, Player->WorldPosition.Tile);
            b32 PlayerCanMove = 1;
            entity_id AdjacentEntityId = 0;

            for (s32 Id = 1; Id < Max_Entities; ++Id)
            {
                if (Id != entity_id_Player)
                {
                    entity *TestEntity = GetEntity(World, Id);
                    /* v2s32 Tile = Player->WorldPosition.Tile; */
                    v2s32 TestTile = TestEntity->WorldPosition.Tile;

                    v2s32 Difference = Subtractv2s32(NewTile, TestTile);

                    if (Difference.x == 0 && Difference.y == 0)
                    {
                        PlayerCanMove = 0;
                        break;
                    }
                    else if ((Difference.x > -2 && Difference.x < 2) &&
                             (Difference.y > -2 && Difference.y < 2))
                    {
                        AdjacentEntityId = Id;
                        break;
                    }
                }
            }

            if (PlayerCanMove)
            {
                Player->WorldPosition.Tile = NewTile;
            }

            if (AdjacentEntityId)
            {
                World->Mode = world_mode_Talking;
            }
        }
    }
    else if (World->Mode == world_mode_Talking)
    {
        if (!World->Conversation.FirstMessage)
        {
            message *Message = CreateMessage(World);
            sentence_list Greeting = {};
            Greeting.Sentence = GenerateSentence(sentence_type_Exclamatory);
            Message->Sentences = Greeting;
            Assert(Message != 0);
            World->Conversation.FirstMessage = Message;
        }
        else
        {

        }
    }
}

internal void HandleUserInput(game *Game, text_element *TextElement)
{
    ui *Ui = &Game->Ui;
    user_input *UserInput = &Game->UserInput;

    UserInput->Direction = (v2s32){0,0};
    UserInput->ConversationDirection = (v2s32){0,0};

    if (IsKeyPressed(KEY_W))
    {
        UserInput->Direction.y -= 1;
    }
    else if (IsKeyPressed(KEY_D))
    {
        UserInput->Direction.x += 1;
    }
    else if (IsKeyPressed(KEY_S))
    {
        UserInput->Direction.y += 1;
    }
    else if (IsKeyPressed(KEY_A))
    {
        UserInput->Direction.x -= 1;
    }

    if (IsKeyPressed(KEY_I))
    {
        UserInput->ConversationDirection.y -= 1;
    }
    else if (IsKeyPressed(KEY_L))
    {
        UserInput->ConversationDirection.x += 1;
    }
    else if (IsKeyPressed(KEY_K))
    {
        UserInput->ConversationDirection.y += 1;
    }
    else if (IsKeyPressed(KEY_J))
    {
        UserInput->ConversationDirection.x -= 1;
    }

    for (s32 I = 0; I < Ui->QueueIndex; ++I)
    {
        s32 Key = Ui->KeyEventQueue[I].Key;
        HandleKey(Ui, TextElement, Key);
    }
}

#define Debug_Text_Size 32
global_variable u8 DebugText[Debug_Text_Size];

int main(void)
{
    u64 MaxArenaSize = 1*Megabyte;
    Assert(sizeof(game) < MaxArenaSize);

    ryn_memory_arena Arena = ryn_memory_CreateArena(sizeof(game));

    game *Game = ryn_memory_PushStruct(&Arena, game);
    world *World = &Game->World;
    Game->FrameArena = ryn_memory_CreateArena(Megabyte);

    InitWindow(Screen_Width, Screen_Height, "Influence");
    SetTargetFPS(60);

    InitializeWorld(World);
    InitializeWordTable(World);


    { /* NOTE: Initialize Ui. */
        Game->Ui.Font = GetFontDefault();
        Game->Ui.FontSize = 18.0f;
        Game->Ui.CursorBlinkRate = 1.1f;
    }

    sentence Sentence = {0};
    {
        Sentence.FirstPartOfSpeech.Part.PartOfSpeech = part_of_speech_Interjection;
        Sentence.FirstPartOfSpeech.Part.Words.Words[0] = _world;
        Sentence.FirstPartOfSpeech.Part.Words.Count = 1;

        part_of_speech_list SecondPartOfSpeech = {0};
        SecondPartOfSpeech.Part.PartOfSpeech = part_of_speech_Noun;
        SecondPartOfSpeech.Part.Words.Words[0] = _hello;
        SecondPartOfSpeech.Part.Words.Count = 1;

        Sentence.FirstPartOfSpeech.Next = &SecondPartOfSpeech;
    }

    text_element DebugTextElement = CreateText(V2(20.0f, 20.0f), ui_element_CommandLine, DebugText, Debug_Text_Size);

    f32 Time = GetTime();

    while (!WindowShouldClose())
    {
        {
            f32 CurrentTime = GetTime();
            Game->Ui.DeltaTime = CurrentTime - Time;
            Time = CurrentTime;
        }

        UpdateUserInputForUi(&Game->Ui);
        HandleUserInput(Game, &DebugTextElement);
        TestUpdatePlayer(World, &Game->UserInput);

        BeginDrawing();
        ClearBackground((Color){40,0,50,255});
        DoTextElement(&Game->Ui, &DebugTextElement, alignment_TopLeft);
        /* DrawWorld(Game); */
        Game->FrameArena.Offset = 0;
        EndDrawing();
    }

    /* printf("sizeof(world) %lu\n", sizeof(world)); */

    /* DebugPrintSentence(&Sentence); */
    printf("Word count %d\n", _WORD_COUNT);

    return 0;
}
