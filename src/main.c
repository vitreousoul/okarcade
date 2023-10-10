#include <stdlib.h>
#include <stdio.h>

#include "../lib/raylib.h"

#include "types.h"
#include "platform.h"
#include "core.c"
#include "preprocess.c"
#include "ui.c"

int SCREEN_WIDTH = 600;
int SCREEN_HEIGHT = 400;
#define TARGET_FPS 30

#include "./l_system.c"

typedef enum
{
    command_line_arg_type_Undefined,
    command_line_arg_type_Preprocess,
    command_line_arg_type_LSystem,
    command_line_arg_type_Count,
} command_line_arg_type;

typedef struct
{
    b32 Valid;
    command_line_arg_type Type;
} command_line_args;

/* TODO StringsEqual should maybe live in a more core location */
internal b32 StringsEqual(char *StringA, char *StringB)
{
    s32 AreEqual = 1;
    s32 Index = 0;

    for(;;)
    {
        char CharA = StringA[Index];
        char CharB = StringB[Index];

        if (CharA != CharB)
        {
            AreEqual = 0;
        }

        if (CharA == 0 || CharB == 0)
        {
            break;
        }

        Index += 1;
    }
    return AreEqual;
}

internal command_line_args ParseCommandLineArgs(s32 ArgCount, char **Args)
{
    command_line_args CommandLineArgs = {0};

    if (ArgCount < 2 || ArgCount > 2)
    {
        printf("Expecting a single argument to be passed... we really should print out usage here...\n");
    }
    else
    {
        char *FirstArg = Args[1];

        if (StringsEqual(FirstArg, "preprocess"))
        {
            CommandLineArgs.Type = command_line_arg_type_Preprocess;
        }
        else if (StringsEqual(FirstArg, "lsystem"))
        {
            CommandLineArgs.Type = command_line_arg_type_LSystem;
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
    int Result = 0;
    command_line_args CommandLineArgs = ParseCommandLineArgs(ArgCount, Args);

    switch(CommandLineArgs.Type)
    {
    case command_line_arg_type_Preprocess:
    {
        TestPreprocessor();
    } break;
    case command_line_arg_type_LSystem:
    {
        RUN_LSystem();
    } break;
    default:
    {
        printf("Un-handled command line arg type: %d\n", CommandLineArgs.Type);
    } break;
    }


    return Result;
}
