/*
  ryn_memory v0.00 - Memory arena utilities. Very early in development...
*/
#ifndef __RYN_MEMORY__
#define __RYN_MEMORY__

#if defined(__MACH__) || defined(__APPLE__)
#define ryn_memory_Operating_System ryn_memory_Mac
#endif

#ifndef ryn_memory_Operating_System
#error Unhandled operating system.
#endif

#if ryn_memory_Operating_System == ryn_memory_Mac
#include <sys/mman.h>
#include <errno.h>
#endif


#include <stdint.h>
#define ryn_memory_u8 uint8_t
#define ryn_memory_u16 uint16_t
#define ryn_memory_u32 uint32_t
#define ryn_memory_u64 uint64_t
#define ryn_memory_s8 int8_t
#define ryn_memory_s16 int16_t
#define ryn_memory_s32 int32_t
#define ryn_memory_s64 int64_t
#define ryn_memory_b8 uint8_t
#define ryn_memory_b32 uint32_t
#define ryn_memory_f32 float
#define ryn_memory_size size_t



typedef struct
{
    ryn_memory_u64 Offset;
    ryn_memory_u64 Capacity;
    ryn_memory_u8 *Data;
    ryn_memory_u64 ParentOffset;
} ryn_memory_arena;

void *ryn_memory_AllocateVirtualMemory(ryn_memory_size Size);

ryn_memory_arena ryn_memory_CreateArena(ryn_memory_u64 Size);
void *ryn_memory_PushArena(ryn_memory_arena *Arena, ryn_memory_u64 Size);
ryn_memory_u64 ryn_memory_GetArenaFreeSpace(ryn_memory_arena *Arena);
ryn_memory_arena ryn_memory_CreateSubArena(ryn_memory_arena *Arena, ryn_memory_u64 Size);
ryn_memory_b32 ryn_memory_IsArenaUsable(ryn_memory_arena Arena);
ryn_memory_s32 ryn_memory_WriteArena(ryn_memory_arena *Arena, ryn_memory_u8 *Data, ryn_memory_u64 Size);
ryn_memory_u8 *ryn_memory_GetArenaWriteLocation(ryn_memory_arena *Arena);
void ryn_memory_FreeArena(ryn_memory_arena Arena);

#define ryn_memory_PushStruct(arena, type) \
    (ryn_memory_PushArena((arena), sizeof(type)))

/* TODO: Add the ability to compile asserts out. */
#define ryn_memory_Assert(p) ryn_memory_Assert_(p, __FILE__, __LINE__)
void ryn_memory_Assert_(ryn_memory_b32 Proposition, char *FilePath, ryn_memory_s32 LineNumber)
{
    if (!Proposition)
    {
        ryn_memory_b32 *NullPtr = 0;
        printf("Assertion failed on line %d in %s\n", LineNumber, FilePath);
        /* this should break the program... */
        Proposition = *NullPtr;
    }
}
#define ryn_memory_NotImplemented ryn_memory_Assert(0)


void ryn_memory_CopyMemory(ryn_memory_u8 *Source, ryn_memory_u8 *Destination, ryn_memory_u64 Size)
{
    /* @Speed */
    for (ryn_memory_u64 I = 0; I < Size; I++)
    {
        Destination[I] = Source[I];
    }
}

void *ryn_memory_AllocateVirtualMemory(ryn_memory_size Size)
{
    /* TODO allow setting specific address for debugging with stable pointer values */
    ryn_memory_u8 *Address = 0;
    int Protections = PROT_READ | PROT_WRITE;
    int Flags = MAP_ANON | MAP_PRIVATE;
    int FileDescriptor = 0;
    int Offset = 0;

    ryn_memory_u8 *Result = mmap(Address, Size, Protections, Flags, FileDescriptor, Offset);

    if (Result == MAP_FAILED)
    {
        char *ErrorName = 0;

        switch(errno)
        {

        case EACCES: ErrorName = "EACCES"; break;
        case EBADF: ErrorName = "EBADF"; break;
        case EINVAL: ErrorName = "EINVAL"; break;
        case ENODEV: ErrorName = "ENODEV"; break;
        case ENOMEM: ErrorName = "ENOMEM"; break;
        case ENXIO: ErrorName = "ENXIO"; break;
        case EOVERFLOW: ErrorName = "EOVERFLOW"; break;
        default: ErrorName = "<Unknown errno>"; break;
        }

        printf("Error in AllocateVirtualMemory: failed to map memory with errno = \"%s\"\n", ErrorName);
        return 0;
    }
    else
    {
        return Result;
    }
}

ryn_memory_arena ryn_memory_CreateArena(ryn_memory_u64 Size)
{
    ryn_memory_arena Arena;

    Arena.Offset = 0;
    Arena.Capacity = Size;
    Arena.Data = ryn_memory_AllocateVirtualMemory(Size);
    Arena.ParentOffset = 0;

    return Arena;
}

void *ryn_memory_PushArena(ryn_memory_arena *Arena, ryn_memory_u64 Size)
{
    ryn_memory_u8 *Result = 0;

    if ((Size + Arena->Offset) > Arena->Capacity)
    {
        printf("Error in PushArena: allocator is full\n");
    }
    else
    {
        Result = &Arena->Data[Arena->Offset];
        Arena->Offset += Size;
    }

    return Result;
}

ryn_memory_u64 ryn_memory_GetArenaFreeSpace(ryn_memory_arena *Arena)
{
    ryn_memory_Assert(Arena->Capacity >= Arena->Offset);
    ryn_memory_u64 FreeSpace = Arena->Capacity - Arena->Offset;
    return FreeSpace;
}

ryn_memory_arena ryn_memory_CreateSubArena(ryn_memory_arena *Arena, ryn_memory_u64 Size)
{
    ryn_memory_arena SubArena = {0};

    if (Size <= ryn_memory_GetArenaFreeSpace(Arena))
    {
        SubArena.Capacity = Size;
        SubArena.Data = ryn_memory_GetArenaWriteLocation(Arena);
        ryn_memory_PushArena(Arena, Size);
    }

    return SubArena;
}

inline ryn_memory_b32 ryn_memory_IsArenaUsable(ryn_memory_arena Arena)
{
    ryn_memory_b32 IsUsable = Arena.Capacity && Arena.Data;
    return IsUsable;
}

ryn_memory_s32 ryn_memory_WriteArena(ryn_memory_arena *Arena, ryn_memory_u8 *Data, ryn_memory_u64 Size)
{
    ryn_memory_s32 ErrorCode = 0;
    ryn_memory_u8 *WhereToWrite = ryn_memory_PushArena(Arena, Size);

    if (WhereToWrite)
    {
        ryn_memory_CopyMemory(Data, WhereToWrite, Size);
    }
    else
    {
        ErrorCode = 1;
    }

    return ErrorCode;
}

ryn_memory_u8 *ryn_memory_GetArenaWriteLocation(ryn_memory_arena *Arena)
{
    ryn_memory_u8 *WriteLocation = Arena->Data + Arena->Offset;
    return WriteLocation;
}

void ryn_memory_FreeArena(ryn_memory_arena Arena)
{
    munmap(Arena.Data, Arena.Capacity);
}

/* TODO: These push and pop functions have not been tested very much. We should
   write some nested use-cases to make sure push/pop work properly. */
void ryn_memory_ArenaStackPush(ryn_memory_arena *Arena)
{
    ryn_memory_u64 NewParentOffset = Arena->Offset;
    ryn_memory_WriteArena(Arena, (ryn_memory_u8 *)&Arena->ParentOffset, sizeof(Arena->ParentOffset));
    Arena->ParentOffset = NewParentOffset;
}

void ryn_memory_ArenaStackPop(ryn_memory_arena *Arena)
{
    Arena->Offset = Arena->ParentOffset;
    ryn_memory_u64 *WriteLocation = (ryn_memory_u64 *)ryn_memory_GetArenaWriteLocation(Arena);
    Arena->ParentOffset = *WriteLocation;
}

#endif /* __RYN_MEMORY__ */
