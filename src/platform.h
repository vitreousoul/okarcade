#include <sys/stat.h>

typedef struct
{
    s32 Size;
    u8 *Data;
} buffer;

internal buffer *ReadFileIntoBuffer(char *FilePath)
{
    struct stat StatResult;
    int StatError = stat(FilePath, &StatResult);

    if (StatError)
    {
        printf("ReadFile stat error\n");
        return 0;
    }

    buffer *Buffer = malloc(sizeof(buffer));
    FILE *file = fopen(FilePath, "rb");

    Buffer->Size = StatResult.st_size;
    Buffer->Data = malloc(Buffer->Size + 1);
    fread(Buffer->Data, 1, Buffer->Size, file);
    Buffer->Data[Buffer->Size] = 0; // null terminate

    fclose(file);

    return Buffer;
}
