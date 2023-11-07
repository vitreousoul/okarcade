#include <sys/stat.h>
#include <dirent.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fts.h>

typedef struct
{
    s32 Size;
    u8 *Data;
} buffer;

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
} linear_allocator;

void *AllocateMemory(u64 Size);
void FreeMemory(void *Ref);
void *AllocateVirtualMemory(size Size);

void GetResourceUsage(void);

linear_allocator CreateLinearAllocator(u64 Size);
void *PushLinearAllocator(linear_allocator *LinearAllocator, u64 Size);
s32 WriteLinearAllocator(linear_allocator *LinearAllocator, u8 *Data, u64 Size);
void FreeLinearAllocator(linear_allocator LinearAllocator);

void CopyString(u8 *Source, u8 *Destination, s32 DestinationSize);
s32 GetStringLength(u8 *String);

buffer *ReadFileIntoBuffer(u8 *FilePath);
u64 GetFileSize(u8 *FilePath);
u64 ReadFileIntoData(u8 *FilePath, u8 *Bytes, u64 MaxBytes);
void FreeBuffer(buffer *Buffer);
void WriteFile(u8 *FilePath, u8 *Data, size Size);
void WriteFileFromBuffer(u8 *FilePath, buffer *Buffer);
void EnsureDirectoryExists(u8 *DirectoryPath);
void EnsurePathDirectoriesExist(u8 *Path);

linear_allocator WalkDirectory(u8 *Path);

#define Assert(p) Assert_(p, __FILE__, __LINE__)
internal void Assert_(b32 Proposition, char *FilePath, s32 LineNumber)
{
    if (!Proposition)
    {
        b32 *NullPtr = 0;
        printf("Assertion failed on line %d in %s\n", LineNumber, FilePath);
        /* this should break the program... */
        Proposition = *NullPtr;
    }
}

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

/*| v Linear Allocator v |*/
linear_allocator CreateLinearAllocator(u64 Size)
{
    linear_allocator LinearAllocator;

    LinearAllocator.Offset = 0;
    LinearAllocator.Capacity = Size;
    LinearAllocator.Data = AllocateVirtualMemory(Size);

    return LinearAllocator;
}

void *PushLinearAllocator(linear_allocator *LinearAllocator, u64 Size)
{
    u8 *Result = 0;

    if ((Size + LinearAllocator->Offset) > LinearAllocator->Capacity)
    {
        printf("Error in PushLinearAllocator: allocator is full\n");
    }
    else
    {
        Result = &LinearAllocator->Data[LinearAllocator->Offset];
        LinearAllocator->Offset += Size;
    }

    return Result;
}


s32 WriteLinearAllocator(linear_allocator *LinearAllocator, u8 *Data, u64 Size)
{
    s32 ErrorCode = 0;

    u8 *WhereToWrite = PushLinearAllocator(LinearAllocator, Size);

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

void FreeLinearAllocator(linear_allocator LinearAllocator)
{
    munmap(LinearAllocator.Data, LinearAllocator.Capacity);
}
/*| ^ Linear Allocator ^ |*/

void CopyString(u8 *Source, u8 *Destination, s32 DestinationSize)
{
    /* NOTE we assume strings are null-terminated */
    s32 I = 0;

    if (!(Source && Destination)) return;

    for (;;)
    {
        if (I >= DestinationSize)
        {
            break;
        }

        Destination[I] = Source[I];

        if (!Source[I])
        {
            break;
        }

        I += 1;
    }
}

s32 GetStringLength(u8 *String)
{
    s32 StringLength = -1;
    while (String[++StringLength]);
    return StringLength;
}

#define PushString(a, s) PushString_((a), (s), GetStringLength(s))
internal void PushString_(linear_allocator *LinearAllocator, u8 *String, s32 StringLength)
{
    u8 *WhereToWrite = PushLinearAllocator(LinearAllocator, StringLength);
    CopyMemory(String, WhereToWrite, StringLength);
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
        printf("GetFileSize stat error\n");
        return 0;
    }

    FileSize = StatResult.st_size;

    return FileSize;
}

u64 ReadFileIntoData(u8 *FilePath, u8 *Bytes, u64 MaxBytes)
{
    u64 FileSize = GetFileSize(FilePath);

    if (FileSize > MaxBytes)
    {
        printf("Errof in ReadFileIntoData: file size exceeds max-bytes\n");
        return 0;
    }

    FILE *File = fopen((char *)FilePath, "rb");

    fread(Bytes, 1, FileSize, File);
    fclose(File);

    return FileSize;
}

void FreeBuffer(buffer *Buffer)
{
    FreeMemory(Buffer->Data);
    FreeMemory(Buffer);
}

void WriteFile(u8 *FilePath, u8 *Data, size Size)
{
    FILE *File = fopen((char *)FilePath, "wb");

    if (File)
    {
        fwrite(Data, 1, Size, File);
        fclose(File);
    }
    else
    {
        printf("Error in WriteFile: trying to open file \"%s\"\n", FilePath);
    }
}

void WriteFileFromBuffer(u8 *FilePath, buffer *Buffer)
{
    WriteFile(FilePath, Buffer->Data, Buffer->Size);
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

/* NOTE copypasta from a post about fts... */
linear_allocator WalkDirectory(u8 *Path)
{
    linear_allocator Allocator = CreateLinearAllocator(Gigabytes(1));
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
                b32 IsFirstAllocation = Allocator.Offset == 0;
                s32 FilePathLength = strlen(p->fts_path);
                size FileItemSize = sizeof(file_list) + FilePathLength + 1;
                file_list *FileItem = PushLinearAllocator(&Allocator, FileItemSize);

                CopyMemory((u8 *)p->fts_path, FileItem->Name, FilePathLength);
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
