void GenerateBlogPages(linear_allocator *Allocator);

void GenerateBlogPages(linear_allocator *Allocator)
{
    u8 *BlogDirectory = (u8 *)"../blog";

    linear_allocator FileAllocator = WalkDirectory(BlogDirectory);
    file_list *FileList = (file_list *)FileAllocator.Data;
    file_list *SortedFileList = SortFileList(FileList);

    for (file_list *CurrentFile = SortedFileList; CurrentFile; CurrentFile = CurrentFile->Next)
    {
        u8 *WriteLocation = GetLinearAllocatorWriteLocation(Allocator);
        u64 FreeSpace = GetLinearAllocatorFreeSpace(Allocator);

        u64 FileSize = ReadFileIntoData(CurrentFile->Name, Allocator->Data, FreeSpace);
        Allocator->Data[FileSize] = 0;

        printf("\n\nblog file:\n%s\n", Allocator->Data);

        Allocator->Offset = 0;
    }


    FreeLinearAllocator(FileAllocator);
}
