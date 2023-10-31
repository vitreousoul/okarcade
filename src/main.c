#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "core.c"
#include "platform.h"
#include "code_pages.c"
#include "preprocess.c"

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


internal command_line_args ParseCommandLineArgs(s32 ArgCount, char **Args)
{
    command_line_args CommandLineArgs = {0};
    u8 *PreProcessName = (u8 *)"preprocess";

    if (ArgCount < 2 || ArgCount > 2)
    {
        printf("Expecting a single argument to be passed... we really should print out usage here...\n");
    }
    else
    {
        u8 *FirstArg = (u8 *)Args[1];

        if (StringsEqual(FirstArg, PreProcessName))
        {
            CommandLineArgs.Type = command_line_arg_type_Preprocess;
        }
        else
        {
            printf("Command line parse error: unexpected command \"%s\"\n", FirstArg);
        }
    }

    return CommandLineArgs;
}

int main(s32 ArgCount, char **Args)
{
    GetResourceUsage();

    int Result = 0;
    command_line_args CommandLineArgs = ParseCommandLineArgs(ArgCount, Args);

    switch(CommandLineArgs.Type)
    {
    case command_line_arg_type_Preprocess:
        TestPreprocessor();
        break;
    default:
        printf("Un-handled command line arg type: %d\n", CommandLineArgs.Type);
        break;
    }

    GetResourceUsage();

    return Result;
}
