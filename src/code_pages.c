void GenerateCodePages(void);

internal s32 CompareString(u8 *StringA, u8 *StringB)
{
    s32 Result = 0;
    s32 I = 0;

    for (;;)
    {
        if (StringA[I] < StringB[I])
        {
            Result = -1;
            break;
        }
        else if (StringB[I] < StringA[I])
        {
            Result = 1;
            break;
        }

        if (!(StringA[I] && StringB[I]))
        {
            break;
        }

        I += 1;
    }

    return Result;
}

internal file_list *SortFileList(file_list *Files)
{
    file_list *UnsortedFiles = Files->Next;
    file_list *SortedFiles = Files;
    SortedFiles->Next = 0;

    while (UnsortedFiles)
    {
        file_list *CurrentSortedFile = SortedFiles;
        file_list *PreviousSortedFile = 0;

        while (CurrentSortedFile)
        {
            s32 CompareResult = CompareString(UnsortedFiles->Name, CurrentSortedFile->Name);

            if (CompareResult < 0)
            {
                if (!PreviousSortedFile)
                {
                    /* current sorted file is head of list */
                    file_list *NextUnsortedFile = UnsortedFiles->Next;
                    UnsortedFiles->Next = CurrentSortedFile;
                    SortedFiles = UnsortedFiles;
                    UnsortedFiles = NextUnsortedFile;
                }
                else
                {
                    file_list *NextUnsortedFile = UnsortedFiles->Next;
                    PreviousSortedFile->Next = UnsortedFiles;
                    UnsortedFiles->Next = CurrentSortedFile;
                    UnsortedFiles = NextUnsortedFile;
                }

                break;
            }
            else if (!CurrentSortedFile->Next)
            {
                /* unsorted file is greater than all sorted files */
                file_list *NextUnsortedFile = UnsortedFiles->Next;
                CurrentSortedFile->Next = UnsortedFiles;
                UnsortedFiles->Next = 0;
                UnsortedFiles = NextUnsortedFile;
                break;
            }
            else
            {
                PreviousSortedFile = CurrentSortedFile;
                CurrentSortedFile = CurrentSortedFile->Next;
            }
        }


    }

    return SortedFiles;
}

internal s32 GetLengthOfPathWithoutExtension(u8 *Path)
{
    s32 LastDotIndex = 0;
    s32 I = 0;

    while (Path[I])
    {
        if (Path[I] == '.')
        {
            LastDotIndex = I;
        }

        I += 1;
    }

    return LastDotIndex + 1;
}

internal u8 *CodePageBegin(void)
{
    return (u8 *)(
        "<!doctype html>"                                       \
        "<html lang=\"en-us\">"                                 \
            "<head>"                                            \
                "{| include ../src/layout/head_common.html |}"  \
            "</head>"                                           \
            "<body>"                                            \
                "<pre>");
}

internal u8 *CodePageEnd(void)
{
    return (u8 *)"</pre></body></html>";
}

#define EXT_SIZE 5
#define PATH_BUFFER_SIZE 1024
internal void GetCodePageOutputPathName(u8 *SourceCodePath, u8 *CodePagePath, u8 *PathBuffer)
{
    u8 *GenPath = (u8 *)"../gen";
    u64 PrefixLength = GetStringLength(GenPath);

    u8 *Extension = (u8 *)".html";

    u64 CodePagePathLength = GetStringLength(CodePagePath);
    s32 I = 0;

    /* We want to replace the source code path prefix "../src" with the "../gen" prefix.
       This loop increments an offset until the source/code paths don't match.
     */
    for (;;)
    {
        b32 SomethingIsNull = !(SourceCodePath[I] && CodePagePath[I]);
        b32 CharsNotEqual = SourceCodePath[I] != CodePagePath[I];

        if (SomethingIsNull || CharsNotEqual)
        {
            break;
        }

        ++I;
    }

    s32 CodePagePathOffset = I;
    u64 CodePageSubPathLength = CodePagePathLength - CodePagePathOffset;

    if (PrefixLength + CodePageSubPathLength + EXT_SIZE + 1 > PATH_BUFFER_SIZE)
    {
        LogError("Code page path name exceeds temp buffer size\n");
        PathBuffer[0] = 0;
        return;
    }

    CopyMemory(GenPath, PathBuffer, PrefixLength);
    CopyMemory(CodePagePath + CodePagePathOffset, PathBuffer + PrefixLength, CodePageSubPathLength);
    CopyMemory(Extension, PathBuffer + PrefixLength + CodePageSubPathLength, EXT_SIZE);

    PathBuffer[PrefixLength + CodePageSubPathLength + EXT_SIZE] = 0;
}

void GenerateCodePages(void)
{
    u8 *SourceCodePath = (u8 *)"../src";

    linear_allocator FileAllocator = WalkDirectory(SourceCodePath);
    linear_allocator CodePage = CreateLinearAllocator(Gigabytes(1));

    file_list *FileList = (file_list *)FileAllocator.Data;
    file_list *SortedFileList = SortFileList(FileList);

    { /* code page links doc */
        u8 *CodePageListingPath = (u8 *)"../gen/code_page_links.html";
        for (file_list *CurrentFile = SortedFileList; CurrentFile; CurrentFile = CurrentFile->Next)
        {
            s32 PathLength = GetLengthOfPathWithoutExtension(CurrentFile->Name);

            PushString(&CodePage, (u8 *)"<p><a href=\"");
            PushString_(&CodePage, CurrentFile->Name, PathLength);
            PushString(&CodePage, (u8 *)"html\">");
            PushString(&CodePage, CurrentFile->Name);
            PushString(&CodePage, (u8 *)"</a></p>");
        }

        WriteFile(CodePageListingPath, CodePage.Data, CodePage.Offset);
        CodePage.Offset = 0;
    }

    { /* inidividual code page docs */
        for (file_list *CurrentFile = SortedFileList; CurrentFile; CurrentFile = CurrentFile->Next)
        {
            u8 CodePagePathName[PATH_BUFFER_SIZE];
            GetCodePageOutputPathName(SourceCodePath, CurrentFile->Name, (u8 *)CodePagePathName);
            EnsurePathDirectoriesExist(CodePagePathName);

            u64 CodeFileSize = GetFileSize(CurrentFile->Name);

            PushString(&CodePage, CodePageBegin());
            u8 *CodePageData = PushLinearAllocator(&CodePage, CodeFileSize);
            ReadFileIntoData(CurrentFile->Name, CodePageData, CodeFileSize);
            PushString(&CodePage, CodePageEnd());

            printf("writing file %s\n", CodePagePathName);
            WriteFile(CodePagePathName, CodePage.Data, CodePage.Offset);
            CodePage.Offset = 0;
        }

    }

    FreeLinearAllocator(FileAllocator);
}

#undef EXT_SIZE
#undef PATH_BUFFER_SIZE
