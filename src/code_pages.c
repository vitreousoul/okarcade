void GenerateCodePages(linear_allocator *TempString);

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

internal u8 *CodePageBegin(void)
{
    /* NOTE we split up the here-doc tag into HERE and DOC, because otherwise it breaks
       the preprocessor. even in this comment we have to separate the tag by "and". I don't
       know if this should be considered a bug, but it is at least an interesting aspect of
       using code to introspect its own source code.
    */
    return (u8 *)(
        "<!doctype html>"                                                          \
        "<html lang=\"en-us\">"                                                    \
            "<head>"                                                               \
                "{" "| include ../src/layout/head_common.html |" "}"               \
                "<style>"                                                          \
                    "{" "| include ../src/layout/code_page_style.css |" "}"        \
                "</style>"                                                         \
            "</head>"                                                              \
            "<body>"                                                               \
                "{" "| include ../src/layout/code_page_header.html |" "}"          \
                "<pre>"                                                            \
                    "{" "|" "#HERE" "DOC \n");
}

internal u8 *CodePageEnd(void)
{
    return (u8 *)"HERE" "DOC</pre></body></html>";
}

internal void PushNullTerminator(linear_allocator *Allocator)
{
    Allocator->Data[Allocator->Offset] = 0;
    Allocator->Offset += 1;
}

internal buffer GetOutputHtmlPath(linear_allocator *TempString, u8 *OldRootPath, u8 *NewRootPath, u8 *CodePagePath, b32 AddHtmlExtension)
{
    u8 *Extension = (u8 *)".html";

    buffer Buffer;
    Buffer.Size = 0;
    Buffer.Data = TempString->Data + TempString->Offset;

    s32 BeginOffset = TempString->Offset;
    s32 I = 0;

    /* We want to replace the source code path prefix "../src" with the "../gen" prefix.
       This loop increments an offset until the source/code paths don't match.
     */
    for (;;)
    {
        b32 SomethingIsNull = !(OldRootPath[I] && CodePagePath[I]);
        b32 CharsNotEqual = OldRootPath[I] != CodePagePath[I];

        if (SomethingIsNull || CharsNotEqual)
        {
            break;
        }

        ++I;
    }

    s32 CodePagePathOffset = I;

    if (NewRootPath)
    {
        PushString(TempString, NewRootPath);
    }

    PushString(TempString, CodePagePath + CodePagePathOffset);

    if (AddHtmlExtension)
    {
        PushString(TempString, Extension);
    }

    PushNullTerminator(TempString);

    /* printf("html path: \"%s\"\n", TempString->Data + BeginOffset); */

    Buffer.Size = TempString->Offset - BeginOffset;
    CopyMemory(TempString->Data + BeginOffset, Buffer.Data, Buffer.Size);

    return Buffer;
}

internal buffer EscapeHtmlString(linear_allocator *TempString, u8 *HtmlString, s32 Length)
{
    s32 HtmlStringBegin = 0;
    s32 InitialOffset = TempString->Offset;
    s32 I;

    buffer Buffer;
    Buffer.Size = 0;
    Buffer.Data = TempString->Data + TempString->Offset;

    for (I = 0; I < Length; I++)
    {
        u8 *EscapeString = 0;

        switch(HtmlString[I])
        {
        case '<': EscapeString = (u8 *)"&lt;"; break;
        case '>': EscapeString = (u8 *)"&gt;"; break;
        default:
            break;
        }

        if (EscapeString)
        {
            u8 *BeginData = HtmlString + HtmlStringBegin;
            s32 Size = I - HtmlStringBegin;
            u8 *Data = PushLinearAllocator(TempString, Size);
            CopyMemory(BeginData, Data, Size);
            PushString(TempString, EscapeString);
            HtmlStringBegin = I + 1;
        }
    }

    s32 RemainingLength = I - HtmlStringBegin;

    if (RemainingLength)
    {
        u8 *Data = PushLinearAllocator(TempString, RemainingLength);
        CopyMemory(HtmlString + HtmlStringBegin, Data, RemainingLength);
    }

    Buffer.Size = TempString->Offset - InitialOffset;

    return Buffer;
}

void GenerateCodePages(linear_allocator *TempString)
{
    u8 *SourceCodePath = (u8 *)"../src";
    u8 *GenCodePagesPath = (u8 *)"../gen/code_pages";

    linear_allocator FileAllocator = WalkDirectory(SourceCodePath);
    linear_allocator CodePage = CreateLinearAllocator(Gigabytes(1));

    file_list *FileList = (file_list *)FileAllocator.Data;
    file_list *SortedFileList = SortFileList(FileList);

    { /* code page links doc */
        u8 *CodePageListingPath = (u8 *)"../gen/code_page_links.html";
        for (file_list *CurrentFile = SortedFileList; CurrentFile; CurrentFile = CurrentFile->Next)
        {
            buffer FileOutputName = GetOutputHtmlPath(TempString, SourceCodePath, 0, CurrentFile->Name, 0);

            PushString(&CodePage, (u8 *)"<p><a href=\"");
            PushString(&CodePage, FileOutputName.Data);
            PushString(&CodePage, (u8 *)".html\">");
            PushString(&CodePage, CurrentFile->Name);
            PushString(&CodePage, (u8 *)"</a></p>");
        }

        WriteFile(CodePageListingPath, CodePage.Data, CodePage.Offset);
        CodePage.Offset = 0;
    }

    { /* inidividual code page docs */
        for (file_list *CurrentFile = SortedFileList; CurrentFile; CurrentFile = CurrentFile->Next)
        {
            buffer Buffer = GetOutputHtmlPath(TempString, SourceCodePath, GenCodePagesPath, CurrentFile->Name, 1);
            EnsurePathDirectoriesExist(Buffer.Data);

            PushString(&CodePage, CodePageBegin());

            buffer *CodePageBuffer = ReadFileIntoBuffer(CurrentFile->Name);
            buffer EscapedHtmlBuffer = EscapeHtmlString(TempString, CodePageBuffer->Data, CodePageBuffer->Size);

            u8 *CodePageData = PushLinearAllocator(&CodePage, EscapedHtmlBuffer.Size);
            CopyMemory(EscapedHtmlBuffer.Data, CodePageData, EscapedHtmlBuffer.Size);

            PushString(&CodePage, CodePageEnd());

            WriteFile(Buffer.Data, CodePage.Data, CodePage.Offset);
            FreeBuffer(CodePageBuffer);

            CodePage.Offset = 0;
            TempString->Offset = 0;
        }

    }

    FreeLinearAllocator(FileAllocator);
}
