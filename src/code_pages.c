void TestCodePages(void);

void TestCodePages(void)
{
    file_array FileArray = WalkDirectory((u8 *)"../src");

    for (s32 I = 0; I < FileArray.Count; I++)
    {
        printf("file: %s\n", FileArray.Files[I].Name);
    }

    FreeFileArray(FileArray);
}
