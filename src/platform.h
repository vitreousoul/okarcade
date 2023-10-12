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
void CopyString(u8 *Source, u8 *Destination, s32 DestinationSize);
buffer *ReadFileIntoBuffer(u8 *FilePath);
void FreeBuffer(buffer *Buffer);
void WriteFile(u8 *FilePath, buffer *Buffer);
void EnsureDirectoryExists(u8 *DirectoryPath);
void FreeFileArray(file_array FileArray);
file_array WalkDirectory(u8 *Path);

#define LogError(s) LogError_((u8 *)s, __LINE__, __FILE__)
internal void LogError_(u8 *Message, s32 Line, char *File)
{
    printf("Error at line %d in %s: %s\n", Line, File, Message);
}

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

void CopyMemory(u8 *Source, u8 *Destination, u64 Size)
{
    for (u64 I = 0; I < Size; I++)
    {
        Destination[I] = Source[I];
    }
}

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

void FreeFileArray(file_array FileArray)
{
    free(FileArray.Files);
}

internal b32 IsRelativePathName(char *PathName)
{
    s32 Length = -1;
    while (PathName[++Length]);

    b32 SingleDot = Length == 1 && PathName[0] == '.';
    b32 DoubleDot = Length == 2 && PathName[0] == '.' && PathName[1] == '.';
    b32 IsRelative = SingleDot || DoubleDot;

    return IsRelative;
}

file_array WalkDirectory(u8 *Path)
{
    u8 ActivePaths[64][1024]; /* TODO only allow for a depth of 64 in the file tree... this should be dynamic..... */
    s32 ActivePathIndex = 0;

    s32 FileArrayCount = 64;

    file_array FileArray;
    FileArray.Count = 0;
    FileArray.Capacity = FileArrayCount;
    FileArray.Files = malloc(sizeof(file_data) * FileArrayCount);

    struct dirent *DirectoryEntry = 0;

    CopyString(Path, ActivePaths[ActivePathIndex], 1024);

    while (ActivePathIndex >= 0 && ActivePathIndex < 64)
    {
        char *CurrentPath = (char *)ActivePaths[ActivePathIndex];
        DIR *Directory = opendir(CurrentPath);
        ActivePathIndex -= 1;

        if (!Directory)
        {
            printf("Platform error opening directory \"%s\"\n", CurrentPath);
            continue;
        }

        while (1)
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
            else if (S_ISREG(Stat.st_mode))
            {
                if (FileArray.Count >= FileArray.Capacity)
                {
                    FileArrayCount *= 2;
                    FileArray.Files = realloc(FileArray.Files, sizeof(file_data) * FileArrayCount);
                }

                FileArray.Files[FileArray.Count].Name = (u8 *)DirectoryEntry->d_name;
                FileArray.Count += 1;
            }
            else if (IsRelativePathName(DirectoryEntry->d_name))
            {
                continue;
            }
            else if (S_ISDIR(Stat.st_mode))
            {
                ActivePathIndex += 1;
                Assert(ActivePathIndex < 64);

                s32 CurrentPathLength = -1;
                while (CurrentPath[++CurrentPathLength]);
                s32 DirectoryPathLength = -1;
                while (DirectoryEntry->d_name[++DirectoryPathLength ]);

                s32 TotalLength = CurrentPathLength + DirectoryPathLength + 2; /* plus two for slash between paths and null-char */

                if (TotalLength > 1024)
                {
                    printf("WalkDirectory error: working path exceeds max length of 1024 \"%s/%s\"\n", CurrentPath, DirectoryEntry->d_name);
                    continue;
                }

                u8 TempString[1024];
                sprintf((char *)TempString, "%s/%s", CurrentPath, DirectoryEntry->d_name);
                TempString[TotalLength - 1] = 0;
                CopyString(TempString, ActivePaths[ActivePathIndex], TotalLength);
            }
        }
    }

    return FileArray;
}
