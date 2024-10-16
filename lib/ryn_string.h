/*
  TODO:
    [ ] Allow the user to specify their own print functions, for people who don't want to use c-runtime libraries.
*/
#ifndef __RYN_STRING__
#define __RYN_STRING__

#include <stdarg.h>

#include <stdint.h>
#define ryn_string_u8   uint8_t
#define ryn_string_u16  uint16_t
#define ryn_string_u32  uint32_t
#define ryn_string_u64  uint64_t
#define ryn_string_s8   int8_t
#define ryn_string_s16  int16_t
#define ryn_string_s32  int32_t
#define ryn_string_s64  int64_t
#define ryn_string_b8   uint8_t
#define ryn_string_b32  uint32_t
#define ryn_string_f32  float
#define ryn_string_size size_t

typedef struct
{
    ryn_string_u8 *Bytes;
    ryn_string_u64 Size;
} ryn_string;

typedef struct ryn_string_node ryn_string_node;

struct ryn_string_node
{
    ryn_string_node *Next;
    ryn_string String;
};

typedef struct
{
    ryn_string_node *First;
    ryn_string_node *Last;
    ryn_string_u64 NodeCount;
    ryn_string_u64 TotalSize;
} ryn_string_list;

ryn_string ryn_string_CreateString(char *CString)
{
    ryn_string Result = {0};

    if (CString != 0)
    {
        ryn_string_s32 StringSize = 0;
        while ((CString[StringSize++] != 0));
        Result.Bytes = (ryn_string_u8 *)CString;
        Result.Size = StringSize;
    }
    else
    {
        printf("[Error ryn_string_CreateString] CString empty!");
    }

    return Result;
}

ryn_string ryn_string_CreateStringNoNull(char *CString)
{
    /* NOTE: Just kidding! There is a null-terminator we just subtract the string length by 1. Just in case the string gets passed to a CRT function. */
    ryn_string Result = ryn_string_CreateString(CString);
    Result.Size -= 1;

    return Result;
}

/* NOTE: Assume that CString can hold String.Size+1 bytes. */
void ryn_string_ToCString(ryn_string String, ryn_string_u8 *CString)
{
    /* TODO: improve @Speed */
    for (ryn_string_u64 I = 0; I < String.Size; ++I)
    {
        CString[I] = String.Bytes[I];
    }
    CString[String.Size] = 0;
}

ryn_string_u64 ryn_string_GetHash(ryn_string String)
{
    /* NOTE: Copy-paste Polynomial rolling hash function from geeks-for-geeks or something like that... */
    ryn_string_u64 Hash = 0;
    ryn_string_u64 P = 31; /* NOTE: What value should we use here? Should it be paramaterized? */
    ryn_string_u64 Pow = 1;
    ryn_string_u64 Mod = 1e9 + 7;

    for (ryn_string_u64 I = 0; I < String.Size; I++)
    {
        /* NOTE: Do we really need to subtract the value of 'a' or is that only for lowercase-only-strings? */
        Hash = (Hash + (String.Bytes[I] - 'a' + 1) * Pow) % Mod;
        Pow = (Pow * P) % Mod;
    }

    return Hash;
}

#endif /* __RYN_STRING__ */
