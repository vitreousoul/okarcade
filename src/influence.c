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

#include "../src/types.h"
#include "../lib/ryn_memory.h"

#define Kilobyte (1024)
#define Megabyte (1024*1024)
#define Gigabyte (1024*1024*1024)

typedef enum
{
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

typedef struct
{
    relationship Relationships[Relationship_Count];
} world;

int main(void)
{
    ryn_memory_arena Arena = ryn_memory_CreateArena(10*Megabyte);
    printf("Arena.Capacity %llu\n", Arena.Capacity);
    return 0;
}
