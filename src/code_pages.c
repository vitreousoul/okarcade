void GenerateCodePages(void);

void GenerateCodePages(void)
{
    linear_allocator FileAllocator = WalkDirectory((u8 *)"../src");
    file_list *FileList = (file_list *)FileAllocator.Data;

    for (; FileList; FileList = FileList->Next)
    {
        printf("%s\n", FileList->Name);
    }

    FreeLinearAllocator(FileAllocator);
}
