#if defined(__MACH__) || defined(__APPLE__)
#define ryn_memory_Mac 1
#define ryn_memory_Operating_System 1
#elif defined(_WIN32)
#define ryn_memory_Windows 1
#define ryn_memory_Operating_System 1
#endif

#ifndef ryn_memory_Operating_System
#error Unhandled operating system.
#endif

#if ryn_memory_Windows

#elif ryn_memory_Mac
#include <sys/stat.h>
#include <dirent.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fts.h>
#endif



#define PATH_SEPARATOR '/'

#include "../lib/ryn_memory.h"
#include "../lib/ryn_string.h"

typedef struct file_list file_list;
struct file_list
{
    file_list *Next;
    ryn_string Name;
};

typedef struct
{
    s32 Year;
    s32 Month;
    s32 Day;
} date;

typedef struct
{
    FILE *File;
} file;

void *AllocateMemory(u64 Size);
void FreeMemory(void *Ref);

void GetResourceUsage(void);

date GetDate(void);

buffer *ReadFileIntoBuffer(u8 *FilePath);
u64 platform_GetFileSize(u8 *FilePath);
u64 ReadFileIntoData(u8 *FilePath, u8 *Bytes, u64 MaxBytes);
u64 ReadFileIntoAllocator(ryn_memory_arena *Arena, u8 *FilePath);
void FreeBuffer(buffer *Buffer);

file platform_OpenFile(u8 *FilePath);
void CloseFile(file File);
void platform_WriteFile(file File, u8 *Data, u64 Size);

void WriteFileWithPath(u8 *FilePath, u8 *Data, size Size);
void WriteFileFromBuffer(u8 *FilePath, buffer *Buffer);
void EnsureDirectoryExists(u8 *DirectoryPath);
void EnsurePathDirectoriesExist(u8 *Path);

file_list *WalkDirectory(ryn_memory_arena *Arena, u8 *Path);

void *AllocateMemory(u64 Size)
{
    /* just use malloc for now... */
    return malloc(Size);
}

void FreeMemory(void *Ref)
{
    /* just use free for now... */
    free(Ref);
}


#if ryn_memory_Windows
void GetResourceUsage(void)
{
    printf("TODO: Implement GetResourceUsage for Windows.\n");
}
#elif ryn_memory_Mac
void GetResourceUsage(void)
{
    struct rusage ResourceUsageInfo;
    s32 Code = getrusage(RUSAGE_SELF, &ResourceUsageInfo);

    if (Code == 0)
    {
        printf("Page reclaims %ld\n", ResourceUsageInfo.ru_minflt);
        printf("Page faults %ld\n", ResourceUsageInfo.ru_majflt);
    }
    else
    {
        printf("GetResourceUsage error %d\n", Code);
    }
}
#endif

date GetDate(void)
{
    date Date;

    time_t Time;
    time(&Time);
    struct tm *LocalTime = localtime(&Time);

    Date.Year = LocalTime->tm_year + 1900;
    Date.Month = LocalTime->tm_mon + 1;
    Date.Day = LocalTime->tm_mday;

    return Date;
}

#define PushString(a, s) PushString_((a), (s), GetStringLength(s))
internal u8 *PushString_(ryn_memory_arena *Arena, u8 *String, s32 StringLength)
{
    u8 *WhereToWrite = ryn_memory_PushSize(Arena, StringLength);
    core_CopyMemory(String, WhereToWrite, StringLength);
    return WhereToWrite;
}

#if ryn_memory_Windows
buffer* ReadFileIntoBuffer(u8* FilePath)
{
    Assert(0);
    printf("TODO: Implement GetResourceUsage for Windows.\n");
    buffer* Buffer = AllocateMemory(sizeof(buffer));
    return Buffer;
}
#elif ryn_memory_Mac
buffer *ReadFileIntoBuffer(u8 *FilePath)
{
    struct stat StatResult;
    int StatError = stat((char *)FilePath, &StatResult);

    if (StatError)
    {
        printf("ReadFileIntoBuffer stat error\n");
        return 0;
    }

    buffer *Buffer = AllocateMemory(sizeof(buffer));
    FILE *File = fopen((char *)FilePath, "rb");

    Buffer->Size = StatResult.st_size;
    Buffer->Data = AllocateMemory(Buffer->Size + 1);
    fread(Buffer->Data, 1, Buffer->Size, File);
    Buffer->Data[Buffer->Size] = 0; // null terminate

    fclose(File);

    return Buffer;
}
#endif

#if ryn_memory_Windows
u64 platform_GetFileSize(u8* FilePath)
{
    /* TODO: We should probably change the way we check a file's size, by assuming you have already opened the file, and then you just pass it in. This wasn't a big deal using CRT but it awkward on Windows... */
    u64 Size = 0;
    HANDLE Handle = CreateFileA(
        FilePath,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0
    );

    if (INVALID_HANDLE_VALUE != Handle)
    {
        Size = GetFileSize(Handle, 0);
        CloseHandle(Handle);
    }

    return Size;
}
#elif ryn_memory_Mac
u64 platform_GetFileSize(u8 *FilePath)
{
    u64 FileSize;
    struct stat StatResult;
    int StatError = stat((char *)FilePath, &StatResult);

    if (StatError)
    {
        printf("platform_GetFileSize stat error %d\n", StatError);
        return 0;
    }

    FileSize = StatResult.st_size;

    return FileSize;
}
#endif

u64 ReadFileIntoData(u8 *FilePath, u8 *Bytes, u64 MaxBytes)
{
    u64 FileSize = platform_GetFileSize(FilePath);

    if (!FileSize)
    {
        printf("Error in ReadFileIntoData getting file size\n");
        return 0;
    }
    else if (!FileSize || FileSize > MaxBytes)
    {
        printf("Error in ReadFileIntoData: file size exceeds max-bytes\n");
        return 0;
    }

    FILE *File = fopen((char *)FilePath, "rb");

    fread(Bytes, 1, FileSize, File);
    fclose(File);

    return FileSize;
}

u64 ReadFileIntoAllocator(ryn_memory_arena *Allocator, u8 *FilePath)
{
    u64 BytesWritten = 0;
    u64 FileSize = platform_GetFileSize(FilePath) + 1; /* NOTE: Plus 1 for null-terminator. */
    u64 AllocatorSpace = Allocator->Capacity - Allocator->Offset;

    if (!FileSize)
    {
        printf("Error in ReadFileIntoAllocator getting file size\n");
    }
    else if (FileSize <= AllocatorSpace)
    {
        FILE *File = fopen((char *)FilePath, "rb");
        u8 *Data = Allocator->Data + Allocator->Offset;

        fread(Data, 1, FileSize, File);
        fclose(File);

        Data[FileSize - 1] = 0; /* Null-terminate just to be safe... */
        BytesWritten = FileSize;
        Allocator->Offset += BytesWritten;
    }

    return BytesWritten;
}

void FreeBuffer(buffer *Buffer)
{
    FreeMemory(Buffer->Data);
    FreeMemory(Buffer);
}

void WriteFileWithPath(u8 *FilePath, u8 *Data, size Size)
{
    FILE *File = fopen((char *)FilePath, "wb");

    if (File)
    {
        fwrite(Data, 1, Size, File);
        fclose(File);
    }
    else
    {
        printf("Error in WriteFileWithPath: trying to open file \"%s\"\n", FilePath);
    }
}

file platform_OpenFile(u8 *FilePath)
{
    file File;
    File.File = fopen((char *)FilePath, "wb");

    if (!File.File)
    {
        printf("Error in platform_OpenFile: trying to open file \"%s\"\n", FilePath);
    }

    return File;
}

void CloseFile(file File)
{
    fclose(File.File);
}

void platform_WriteFile(file File, u8 *Data, u64 Size)
{
    fwrite(Data, 1, Size, File.File);
}

void WriteFileFromBuffer(u8 *FilePath, buffer *Buffer)
{
    WriteFileWithPath(FilePath, Buffer->Data, Buffer->Size);
}

#if ryn_memory_Windows
void EnsureDirectoryExists(u8* DirectoryPath)
{
    Assert(0);
}
#elif ryn_memory_Mac
void EnsureDirectoryExists(u8 *DirectoryPath)
{
    struct stat StatResult;
    int StatError = stat((char *)DirectoryPath, &StatResult);

    if (StatError)
    {
        printf("Making directory \"%s\"\n", DirectoryPath);
        s32 Mode755 = S_IRWXU | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP;
        mkdir((char *)DirectoryPath, Mode755);
    }
}
#endif

void EnsurePathDirectoriesExist(u8 *Path)
{
    u8 TempBuffer[1024];
    s32 PathLength = GetStringLength(Path);

    for (s32 I = 0; I < PathLength; I++)
    {
        if (Path[I] == '/')
        {
            if (I + 1 > 1024)
            {
                printf("Error in EnsurePathDirectoriesExist: sub-path exceeds temp buffer\n");
                continue;
            }

            core_CopyMemory(Path, TempBuffer, I);
            TempBuffer[I] = 0;

            EnsureDirectoryExists(TempBuffer);
        }
    }
}

internal b32 IsWalkableFile(u8 *FileName)
{
    b32 IsWalkable = 1;
    char *IgnoredNames[] = {".DS_Store"};

    u32 I = 0;
    u32 NameOffset = 0;
    while (FileName[I])
    {
        if (FileName[I] == PATH_SEPARATOR)
        {
            NameOffset = I + 1;
        }

        I += 1;
    }

    for (I = 0; I < ArrayCount(IgnoredNames); ++I)
    {
        u8 *OffsetFileName = FileName + NameOffset;
        if (StringsEqual((u8 *)IgnoredNames[I], OffsetFileName))
        {
            IsWalkable = 0;
            break;
        }
    }

    return IsWalkable;
}

#if ryn_memory_Windows
file_list *WalkDirectory(ryn_memory_arena* Arena, u8* Path)
{
    Assert(0);
    file_list* FileList = 0;
    return FileList;
}
#elif ryn_memory_Mac
file_list *WalkDirectory(ryn_memory_arena *Arena, u8 *Path)
{
    /* NOTE: we call the variable "Paths", but it's only every inteded to contain 1 path. */
    char *Paths[] = { (char *)Path, 0 };
    u32 FtsFlags = FTS_PHYSICAL | FTS_NOCHDIR | FTS_XDEV;
    FTS *Fts = fts_open(Paths, FtsFlags, 0);
    file_list *PreviousFileItem = (file_list *)(Arena->Data + Arena->Offset);
    file_list *Result = PreviousFileItem;

    if (Fts)
    {
        FTSENT *p;
        while ((p = fts_read(Fts)) != 0)
        {
            switch (p->fts_info)
            {
            case FTS_F:
            {
                u8 *FilePath = (u8 *)p->fts_path;

                if (IsWalkableFile(FilePath))
                {
                    b32 IsFirstAllocation = Arena->Offset == 0;
                    file_list *FileItem = ryn_memory_PushZeroStruct(Arena, file_list);
                    s32 FilePathLength = GetStringLength(FilePath) + 1;
                    FileItem->Name.Bytes = PushString_(Arena, FilePath, FilePathLength);
                    FileItem->Name.Bytes[FilePathLength-1] = 0;
                    FileItem->Name.Size = FilePathLength;

                    if (IsFirstAllocation)
                    {
                        FileItem->Next = 0;
                    }
                    else
                    {
                        PreviousFileItem->Next = FileItem;
                    }

                    PreviousFileItem = FileItem;
                }
            } break;
            case FTS_DP:
            {
                /* directory */
            } break;
            case FTS_SL:
            case FTS_SLNONE:
            {
                /* symbolic link */
            } break;
            }
        }

        fts_close(Fts);
    }
    else
    {
        printf("Error in WalkDirectory: error calling fts_open\n");
    }

    return Result;
}
#endif