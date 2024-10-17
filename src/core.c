typedef struct
{
    s32 Size;
    u8 *Data;
} buffer;

#define ArrayCount(a) (sizeof(a)/sizeof((a)[0]))

/*
b32 StringsEqual(u8 *StringA, u8 *StringB);
void SetMemory(u8 *Source, u8 Value, u64 Size);
void CopyMemory(u8 *Source, u8 *Destination, u64 Size);

void CopyString(u8 *Source, u8 *Destination, s32 DestinationSize);

s32 GetStringLength(u8 *String);
*/

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

#define NotImplemented Assert(0)

b32 StringsEqual(u8 *StringA, u8 *StringB)
{
    s32 AreEqual = 1;
    s32 Index = 0;

    for(;;)
    {
        u8 CharA = StringA[Index];
        u8 CharB = StringB[Index];

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

void SetMemory(u8 *Source, u8 Value, u64 Size)
{
    for (u64 I = 0; I < Size; I++)
    {
        Source[I] = Value;
    }
}

internal void core_CopyMemory(u8 *Source, u8 *Destination, u64 Size)
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

s32 GetStringLength(u8 *String)
{
    s32 StringLength = -1;
    while (String && String[++StringLength]);
    return StringLength;
}
