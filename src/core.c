#define ArrayCount(a) (sizeof(a)/sizeof((a)[0]))

u64 GetStringLength(u8 *String);

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


u64 GetStringLength(u8 *String)
{
    u64 Length = 0;

    for (;;)
    {
        if (String[Length] == 0)
        {
            break;
        }

        Length += 1;
    }

    return Length;
}
