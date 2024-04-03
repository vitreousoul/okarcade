/*
    TODO: Update palette for website!!!!
    TODO: Improve the colors for code pages! The green background is pretty rough :(
    TODO: Remove the empty line at the start of all code-pages (in the pre element in the code-page html)
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "types.h"
#include "core.c"

#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_image.h"

#include "ryn_prof.h"

#include "platform.h"
#include "preprocess.c"

typedef enum
{
    timed_block_UNDEFINED,
    timed_block_Main,
    timed_block_Count,
} timed_block;

typedef enum
{
    command_line_arg_type_Undefined,
    command_line_arg_type_Preprocess,
    command_line_arg_type_GameAssets,
    command_line_arg_type_Count,
} command_line_arg_type;

typedef struct
{
    b32 Valid;
    command_line_arg_type Type;
} command_line_args;

typedef struct
{
    command_line_arg_type Type;
    u8 *Name;
} command_line_command;

global_variable command_line_command CommandLineCommands[] = {
    {command_line_arg_type_Preprocess,(u8 *)"preprocess"},
    {command_line_arg_type_GameAssets,(u8 *)"game_assets"},
};

internal command_line_arg_type ParseCommandLineArgs(s32 ArgCount, char **Args)
{
    command_line_arg_type CommandLineArgs = 0;
    s32 CommandCount = ArrayCount(CommandLineCommands);

    if (ArgCount < 2 || ArgCount > 2)
    {
        printf("Expecting a single argument to be passed... we really should print out usage here...\n");
    }
    else
    {
        u8 *FirstArg = (u8 *)Args[1];

        for (s32 I = 0; I < CommandCount; ++I)
        {
            command_line_command Command = CommandLineCommands[I];

            if (StringsEqual(FirstArg, Command.Name))
            {
                CommandLineArgs = Command.Type;
            }
        }

        if (CommandLineArgs == 0)
        {
            printf("Command line parse error: unexpected command \"%s\"\n", FirstArg);
        }
    }

    return CommandLineArgs;
}

internal void GenerateByteArray(arena *TempArena, u8 *DestinationPath, u8 *ByteArrayName, u8 *ByteArray, size Size)
{
    /* TODO: Compress the data. */
    u8 HexData[16];

    file File = OpenFile(DestinationPath);

    if (!File.File)
    {
        printf("Error in GenerateByteArray: Destination file not found '%s'\n", DestinationPath);
        return;
    }

    ArenaStackPush(TempArena);
    arena ChunkArena = CreateSubArena(TempArena, 1024);
    PushString(&ChunkArena, (u8 *)"u8 ");
    PushString(&ChunkArena, ByteArrayName);
    PushString(&ChunkArena, (u8 *)"[] = {");

    for (u64 I = 0; I < Size; ++I)
    {
        b32 IsLastByte = I == Size - 1;

        char *FormatString = IsLastByte ? "0x%02x" : "0x%02x,";

        b32 IsEndOfLine = I != 0 && I % 15 == 0;

        sprintf((char *)HexData, FormatString, ByteArray[I]);

        u64 StringLength = GetStringLength(HexData);
        if (IsEndOfLine)
        {
            StringLength += 1;
        }

        if (StringLength > GetArenaFreeSpace(&ChunkArena))
        {
            WriteFile(File, ChunkArena.Data, ChunkArena.Offset);
            ChunkArena.Offset = 0;
        }

        PushString(&ChunkArena, HexData);

        if (IsEndOfLine)
        {
            PushString(&ChunkArena, (u8 *)"\n");
        }
    }

    if (ChunkArena.Offset > 0)
    {
        WriteFile(File, ChunkArena.Data, ChunkArena.Offset);
    }

    u8 *FileEndString = (u8 *)"};\0";
    WriteFile(File, FileEndString, GetStringLength(FileEndString));
    ArenaStackPop(TempArena);
}

internal void GenerateSoundData(arena *TempArena)
{
    u8 *SfxCorrectSourcePath = (u8 *)"../assets/sfx_correct.wav";
    u8 *SfxCorrectDestinationPath = (u8 *)"../gen/sfx_correct.h";
    u64 BeginningOffset = TempArena->Offset;

    buffer *Buffer = ReadFileIntoBuffer(SfxCorrectSourcePath);

    if (Buffer && Buffer->Data)
    {
        u8 *ArrayName = (u8 *)"SfxCorrectData";
        GenerateByteArray(TempArena, SfxCorrectDestinationPath, ArrayName, Buffer->Data, Buffer->Size);
    }
    else
    {
        printf("Error in GenerateSoundData: File not found '%s'\n", SfxCorrectSourcePath);
    }
}

internal void GenerateFontData(arena TempString)
{
    /* TODO: Use GenerateByteArray */
    u8 HexData[16] = {};
    u64 BeginningOffset = TempString.Offset;

    u8 *FontFilePath = (u8 *)"../assets/Roboto-Regular.ttf";
    u8 *FontDataPath = (u8 *)"../gen/roboto_regular.h";

    u8 *DataOutput = GetArenaWriteLocation(&TempString);
    buffer *Buffer = ReadFileIntoBuffer(FontFilePath);

    PushString(&TempString, (u8 *)"u8 FontData[] = {");

    for (s32 I = 0; I < Buffer->Size; ++I)
    {
        b32 IsLastByte = I == Buffer->Size - 1;

        char *FormatString = IsLastByte ? "0x%02x" : "0x%02x,";

        sprintf((char *)HexData, FormatString, Buffer->Data[I]);
        PushString(&TempString, HexData);

        if (I != 0 && I % 15 == 0)
        {
            PushString(&TempString, (u8 *)"\n");
        }
    }
    PushString(&TempString, (u8 *)"};\0");

    u64 FileSize = TempString.Offset - BeginningOffset;
    WriteFileWithPath(FontDataPath, DataOutput, FileSize);

    TempString.Offset = BeginningOffset;
}

internal void GenerateGameAssets(arena TempString)
{
    /* NOTE: for now GenerateGameAssets just generates assets for scuba. */
    u8 *ScubaAssetPath = (u8 *)"../assets/scuba.png";
    u8 *ScubaDataPath = (u8 *)"../gen/scuba.h";

    int TextureWidth, TextureHeight, BitsPerChannel;
    u8 *TextureData = stbi_load((char *)ScubaAssetPath, &TextureWidth, &TextureHeight, &BitsPerChannel, 0);

    if (!TextureData)
    {
        LogError("loading scuba texture");
    }
    else
    {
        /* TODO: compress the asset data!!!!!! */
        u8 *ScubaOutput = GetArenaWriteLocation(&TempString);
        u64 BeginningOffset = TempString.Offset;
        u8 HexData[16] = {};

        PushString(&TempString, (u8 *)"/* NOTE: The first two u32 values in ScubaAssetData are for image-width and image-height respectively. */\n");
        PushString(&TempString, (u8 *)"u32 ScubaAssetData[] = {    \n");

        { /* write texture width/height to data */
            sprintf((char *)HexData, "0x%08x,", TextureWidth);
            PushString(&TempString, HexData);

            sprintf((char *)HexData, "0x%08x,", TextureHeight);
            PushString(&TempString, HexData);
        }

        for (s32 Y = 0; Y < TextureHeight * 4; ++Y)
        {
            for (s32 X = 0; X < TextureWidth; X += 4)
            {
                s32 PixelIndex = Y * TextureWidth + X;
                b32 IsLastPixel = PixelIndex - 1 == TextureWidth * TextureHeight;

                u32 R = TextureData[PixelIndex];
                u32 G = TextureData[PixelIndex + 1];
                u32 B = TextureData[PixelIndex + 2];
                u32 A = TextureData[PixelIndex + 3];

                u32 Pixel = (R << 24) | (G << 16) | (B << 8) | A;
                char *FormatString = IsLastPixel ? "0x%08x" : "0x%08x,";
                sprintf((char *)HexData, FormatString, Pixel);

                PushString(&TempString, HexData);
            }

            PushString(&TempString, (u8 *)"\n    ");
        }

        PushString(&TempString, (u8 *)"\n};\0");

        u64 FileSize = TempString.Offset - BeginningOffset;
        WriteFileWithPath(ScubaDataPath, ScubaOutput, FileSize);
    }

    GenerateFontData(TempString);
    GenerateSoundData(&TempString);
}

int main(s32 ArgCount, char **Args)
{
    GetResourceUsage();

    ryn_BeginProfile();
    ryn_BEGIN_TIMED_BLOCK(timed_block_Main);

    int Result = 0;
    command_line_arg_type CommandLineArgType = ParseCommandLineArgs(ArgCount, Args);

    arena TempString = CreateArena(Gigabytes(1));

    switch(CommandLineArgType)
    {
    case command_line_arg_type_Preprocess:
    {
        GenerateSite(&TempString);
    } break;
    case command_line_arg_type_GameAssets:
    {
        GenerateGameAssets(TempString);
    } break;
    default:
        printf("Un-handled command line arg type: %d\n", CommandLineArgType);
        break;
    }

    ryn_END_TIMED_BLOCK(timed_block_Main);
    ryn_EndAndPrintProfile();

    GetResourceUsage();

    return Result;
}
