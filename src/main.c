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

int main(s32 ArgCount, char **Args)
{
    GetResourceUsage();

    ryn_BeginProfile();
    ryn_BEGIN_TIMED_BLOCK(timed_block_Main);

    int Result = 0;
    command_line_arg_type CommandLineArgType = ParseCommandLineArgs(ArgCount, Args);

    switch(CommandLineArgType)
    {
    case command_line_arg_type_Preprocess:
    {
        linear_allocator TempString = CreateLinearAllocator(Gigabytes(1));
        GenerateSite(&TempString);
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
