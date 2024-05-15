#include <sys/stat.h>
#include <dirent.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fts.h>

#define PATH_SEPARATOR '/'

typedef struct
{
    u8 *Name;
    buffer *Buffer;
} file_data;

struct file_list;

struct file_list
{
    struct file_list *Next;
    u8 Name[];
};

typedef struct file_list file_list;

typedef struct
{
    u64 Offset;
    u64 Capacity;
    u8 *Data;
    u64 ParentOffset;
} arena;

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
void *AllocateVirtualMemory(size Size);

void GetResourceUsage(void);

arena CreateArena(u64 Size);
b32 IsArenaUsable(arena Arena);
void *PushArena(arena *Arena, u64 Size);
s32 WriteArena(arena *Arena, u8 *Data, u64 Size);
u8 *GetArenaWriteLocation(arena *Arena);
u64 GetArenaFreeSpace(arena *Arena);
void FreeArena(arena Arena);

date GetDate(void);

buffer *ReadFileIntoBuffer(u8 *FilePath);
u64 GetFileSize(u8 *FilePath);
u64 ReadFileIntoData(u8 *FilePath, u8 *Bytes, u64 MaxBytes);
u64 ReadFileIntoAllocator(arena *Arena, u8 *FilePath);
void FreeBuffer(buffer *Buffer);

file OpenFile(u8 *FilePath);
void CloseFile(file File);
void WriteFile(file File, u8 *Data, u64 Size);

void WriteFileWithPath(u8 *FilePath, u8 *Data, size Size);
void WriteFileFromBuffer(u8 *FilePath, buffer *Buffer);
void EnsureDirectoryExists(u8 *DirectoryPath);
void EnsurePathDirectoriesExist(u8 *Path);

arena WalkDirectory(u8 *Path);

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

void *AllocateVirtualMemory(size Size)
{
    /* TODO allow setting specific address for debugging with stable pointer values */
    u8 *Address = 0;
    int Protections = PROT_READ | PROT_WRITE;
    int Flags = MAP_ANON | MAP_PRIVATE;
    int FileDescriptor = 0;
    int Offset = 0;

    u8 *Result = mmap(Address, Size, Protections, Flags, FileDescriptor, Offset);

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

/*| v Arena Allocator v |*/
arena CreateArena(u64 Size)
{
    arena Arena;

    Arena.Offset = 0;
    Arena.Capacity = Size;
    Arena.Data = AllocateVirtualMemory(Size);
    Arena.ParentOffset = 0;

    return Arena;
}

void *PushArena(arena *Arena, u64 Size)
{
    u8 *Result = 0;

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

u64 GetArenaFreeSpace(arena *Arena)
{
    Assert(Arena->Capacity >= Arena->Offset);
    u64 FreeSpace = Arena->Capacity - Arena->Offset;
    return FreeSpace;
}

arena CreateSubArena(arena *Arena, u64 Size)
{
    arena SubArena = {0};

    if (Size <= GetArenaFreeSpace(Arena))
    {
        SubArena.Capacity = Size;
        SubArena.Data = GetArenaWriteLocation(Arena);
        PushArena(Arena, Size);
    }

    return SubArena;
}

inline b32 IsArenaUsable(arena Arena)
{
    b32 IsUsable = Arena.Capacity && Arena.Data;
    return IsUsable;
}

s32 WriteArena(arena *Arena, u8 *Data, u64 Size)
{
    s32 ErrorCode = 0;

    u8 *WhereToWrite = PushArena(Arena, Size);

    if (WhereToWrite)
    {
        CopyMemory(Data, WhereToWrite, Size);
    }
    else
    {
        ErrorCode = 1;
    }

    return ErrorCode;
}

u8 *GetArenaWriteLocation(arena *Arena)
{
    u8 *WriteLocation = Arena->Data + Arena->Offset;
    return WriteLocation;
}

void FreeArena(arena Arena)
{
    munmap(Arena.Data, Arena.Capacity);
}

void ArenaStackPush(arena *Arena)
{
    u64 NewParentOffset = Arena->Offset;
    WriteArena(Arena, (u8 *)&Arena->ParentOffset, sizeof(Arena->ParentOffset));
    Arena->ParentOffset = NewParentOffset;
}

void ArenaStackPop(arena *Arena)
{
    Arena->Offset = Arena->ParentOffset;
    u64 *WriteLocation = (u64 *)GetArenaWriteLocation(Arena);
    Arena->ParentOffset = *WriteLocation;
}
/*| ^ Arena Allocator ^ |*/

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
internal u8 *PushString_(arena *Arena, u8 *String, s32 StringLength)
{
    u8 *WhereToWrite = PushArena(Arena, StringLength);
    CopyMemory(String, WhereToWrite, StringLength);
    return WhereToWrite;
}

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

u64 GetFileSize(u8 *FilePath)
{
    u64 FileSize;
    struct stat StatResult;
    int StatError = stat((char *)FilePath, &StatResult);

    if (StatError)
    {
        printf("GetFileSize stat error %d\n", StatError);
        return 0;
    }

    FileSize = StatResult.st_size;

    return FileSize;
}

u64 ReadFileIntoData(u8 *FilePath, u8 *Bytes, u64 MaxBytes)
{
    u64 FileSize = GetFileSize(FilePath);

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

u64 ReadFileIntoAllocator(arena *Allocator, u8 *FilePath)
{
    u64 BytesWritten = 0;
    u64 FileSize = GetFileSize(FilePath);
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

        BytesWritten = FileSize;
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

file OpenFile(u8 *FilePath)
{
    file File;
    File.File = fopen((char *)FilePath, "wb");

    if (!File.File)
    {
        printf("Error in WriteFile: trying to open file \"%s\"\n", FilePath);
    }

    return File;
}

void CloseFile(file File)
{
    fclose(File.File);
}

void WriteFile(file File, u8 *Data, u64 Size)
{
    fwrite(Data, 1, Size, File.File);
}

void WriteFileFromBuffer(u8 *FilePath, buffer *Buffer)
{
    WriteFileWithPath(FilePath, Buffer->Data, Buffer->Size);
}

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

            CopyMemory(Path, TempBuffer, I);
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

/* TODO: pass in allocator, to give caller more control */
arena WalkDirectory(u8 *Path)
{
    arena Allocator = CreateArena(Gigabytes(1));
    /* NOTE: we call the variable "Paths", but it's only every inteded to contain 1 path. */
    char *Paths[] = { (char *)Path, 0 };
    u32 FtsFlags = FTS_PHYSICAL | FTS_NOCHDIR | FTS_XDEV;
    FTS *Fts = fts_open(Paths, FtsFlags, 0);
    file_list *PreviousFileItem = (file_list *)(Allocator.Data + Allocator.Offset);

    if (!Fts)
    {
        printf("Error in WalkDirectory: error calling fts_open\n");
        return Allocator;
    }
    else
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
                    b32 IsFirstAllocation = Allocator.Offset == 0;
                    s32 FilePathLength = strlen((char *)FilePath);
                    size FileItemSize = sizeof(file_list) + FilePathLength + 1;
                    file_list *FileItem = PushArena(&Allocator, FileItemSize);

                    CopyMemory(FilePath, FileItem->Name, FilePathLength);
                    FileItem->Name[FileItemSize] = 0;

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

    return Allocator;
}
