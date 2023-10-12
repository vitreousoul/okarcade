#include <sys/stat.h>
#include <dirent.h>

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

typedef struct
{
    s32 Count;
    s32 Capacity;
    file_data *Files;
} file_array;


void *AllocateMemory(u64 Size);
void FreeMemory(void *Ref);
void CopyMemory(u8 *Source, u8 *Destination, u64 Size);
buffer *ReadFileIntoBuffer(u8 *FilePath);
void FreeBuffer(buffer *Buffer);
void WriteFile(u8 *FilePath, buffer *Buffer);
void EnsureDirectoryExists(u8 *DirectoryPath);
file_array WalkDirectory(u8 *Path);

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

void CopyMemory(u8 *Source, u8 *Destination, u64 Size)
{
    for (u64 I = 0; I < Size; I++)
    {
        Destination[I] = Source[I];
    }
}

buffer *ReadFileIntoBuffer(u8 *FilePath)
{
    struct stat StatResult;
    int StatError = stat((char *)FilePath, &StatResult);

    if (StatError)
    {
        printf("ReadFile stat error\n");
        return 0;
    }

    buffer *Buffer = AllocateMemory(sizeof(buffer));
    FILE *file = fopen((char *)FilePath, "rb");

    Buffer->Size = StatResult.st_size;
    Buffer->Data = AllocateMemory(Buffer->Size + 1);
    fread(Buffer->Data, 1, Buffer->Size, file);
    Buffer->Data[Buffer->Size] = 0; // null terminate

    fclose(file);

    return Buffer;
}

void FreeBuffer(buffer *Buffer)
{
    FreeMemory(Buffer->Data);
    FreeMemory(Buffer);
}

void WriteFile(u8 *FilePath, buffer *Buffer)
{
    FILE *File = fopen((char *)FilePath, "wb");

    fwrite(Buffer->Data, 1, Buffer->Size, File);
    fclose(File);
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

file_array WalkDirectory(u8 *Path)
{
    s32 FileArraySize = 64;

    file_array FileArray;
    FileArray.Count = 0;
    FileArray.Capacity = 0;
    FileArray.Files = malloc(sizeof(file_data) * FileArraySize);

    struct dirent *DirectoryEntry = 0;

    DIR *Directory = opendir((char *)Path);

    if (!Directory)
    {
        printf("Platform error opening directory \"%s\"", Path);
    }

    for (;;)
    {
        struct stat Stat;

        DirectoryEntry = readdir(Directory);

        if (!DirectoryEntry)
        {
            break;
        }
        else if (fstatat(dirfd(Directory), DirectoryEntry->d_name, &Stat, 0) < 0)
        {
            printf("fstatat error in directory \"%s\" with child \"%s\"\n", Path, DirectoryEntry->d_name);
        }
        else
        {
            /* TODO if file, push to files array */
            /* TODO if directory, recurse/push-to-stack */
            printf("%s\n", DirectoryEntry->d_name);
        }
    }

    return FileArray;
}
