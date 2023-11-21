#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "types.h"
#include "core.c"

#include "ryn_prof.h"

#include "platform.h"
#include "code_pages.c"
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

internal void GenerateGameAssets(linear_allocator TempString)
{
    u8 *PigeonAssetPath = (u8 *)"../assets/pigeon.png";
    u8 *PigeonDataPath = (u8 *)"../gen/pigeon.h";

    u64 FileSize = GetFileSize(PigeonAssetPath);
    u8 *PigeonData = PushLinearAllocator(&TempString, FileSize);

    if (PigeonData)
    {
        ReadFileIntoData(PigeonAssetPath, PigeonData, FileSize);
        u8 *PigeonOutput = GetLinearAllocatorWriteLocation(&TempString);
        u64 BeginningOffset = TempString.Offset;
        u8 HexData[16] = {};

        PushString(&TempString, (u8 *)"u8 PigeonAssetData[] = {");

        for (u64 I = 0; I < FileSize; ++I)
        {
            if (I % 16 == 0)
            {
                PushString(&TempString, (u8 *)"\n    ");
            }
            sprintf((char *)HexData, "0x%02x,", PigeonData[I]);
            WriteLinearAllocator(&TempString, HexData, 5);
        }

        PushString(&TempString, (u8 *)"\n};\0");

        u64 FileSize = TempString.Offset - BeginningOffset;
        WriteFile(PigeonDataPath, PigeonOutput, FileSize);
    }
    else
    {
        LogError("pushing TempString allocator");
    }
}

int main(s32 ArgCount, char **Args)
{
    GetResourceUsage();

    ryn_BeginProfile();
    ryn_BEGIN_TIMED_BLOCK(timed_block_Main);

    int Result = 0;
    command_line_arg_type CommandLineArgType = ParseCommandLineArgs(ArgCount, Args);

    linear_allocator TempString = CreateLinearAllocator(Gigabytes(1));

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
