#include <sys/stat.h>

typedef struct
{
    s32 Size;
    u8 *Data;
} buffer;

void *AllocateMemory(u64 Size);
void CopyMemory(u8 *Source, u8 *Destination, u64 Size);
buffer *ReadFileIntoBuffer(u8 *FilePath);
void WriteFile(u8 *FilePath, buffer *Buffer);
void EnsureDirectoryExists(u8 *DirectoryPath);

void *AllocateMemory(u64 Size)
{
    /* just use malloc for now... */
    return malloc(Size);
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

    buffer *Buffer = malloc(sizeof(buffer));
    FILE *file = fopen((char *)FilePath, "rb");

    Buffer->Size = StatResult.st_size;
    Buffer->Data = malloc(Buffer->Size + 1);
    fread(Buffer->Data, 1, Buffer->Size, file);
    Buffer->Data[Buffer->Size] = 0; // null terminate

    fclose(file);

    return Buffer;
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
