#define ArrayCount(a) (sizeof(a)/sizeof((a)[0]))

b32 StringsEqual(u8 *StringA, u8 *StringB);
void SetMemory(u8 *Source, u8 Value, u64 Size);
void CopyMemory(u8 *Source, u8 *Destination, u64 Size);

#define LogError(s) LogError_((u8 *)s, __LINE__, __FILE__)
internal void LogError_(u8 *Message, s32 Line, char *File)
{
    printf("Error at line %d in %s: %s\n", Line, File, Message);
}

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

void CopyMemory(u8 *Source, u8 *Destination, u64 Size)
{
    for (u64 I = 0; I < Size; I++)
    {
        Destination[I] = Source[I];
    }
}
