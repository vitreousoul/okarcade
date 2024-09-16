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

#endif /* __RYN_STRING__ */
