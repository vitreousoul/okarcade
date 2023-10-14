void TestCodePages(void);

void TestCodePages(void)
{
    u8 *VirtualBytes = AllocateVirtualMemory(4 * 1024 * 1024);

    if (!VirtualBytes)
    {
        printf("Error in TestCodePages while allocating virtual memory\n");
        return;
    }

    u8 FullPath[1024] = {0};
    u64 FileSize = ReadFileIntoData((u8 *)"../src/main.c", VirtualBytes, 1024 * 1024);

    file_array FileArray = WalkDirectory((u8 *)"../src");

    for (s32 I = 0; I < FileArray.Count; I++)
    {
        printf("file: %s\n", FileArray.Files[I].Name);
    }

    FreeFileArray(FileArray);
}
