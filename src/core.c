#define ArrayCount(a) (sizeof(a)/sizeof((a)[0]))

u64 GetStringLength(u8 *String);
b32 StringsEqual(char *StringA, char *StringB);

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

b32 StringsEqual(char *StringA, char *StringB)
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
