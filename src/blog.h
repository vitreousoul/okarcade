/* #define IsLowerCase(c) ((c) >= 'a' && (c) <= 'z') */
/* #define IsUpperCase(c) ((c) >= 'A' && (c) <= 'Z') */
/* #define IsAlpha(c) (IsLowerCase(c) && IsUpperCase(c)) */

void GenerateBlogPages(linear_allocator *TempString);

typedef enum
{
    blog_line_type_Paragraph,
    blog_line_type_Header,
    blog_line_type_SubHeader,
    blog_line_type_Image,
} blog_line_type;

internal s32 StringMatchLength(u8 *StringA, u8 *StringB, u64 MaxBytes)
{
    u32 I = 0;

    if (StringA && StringB)
    {
        while (I < MaxBytes && StringA[I] && StringB[I])
        {
            if (StringA[I] == StringB[I])
            {
                I += 1;
            }
            else
            {
                break;
            }
        }
    }

    return I;
}

internal blog_line_type GetBlogLineType(u8 *BlogData, u64 MaxBytes)
{
    blog_line_type Type = blog_line_type_Paragraph;

    if (StringMatchLength(BlogData, (u8 *)"===", MaxBytes) == 3)
    {
        Type = blog_line_type_Header;
    }
    else if (StringMatchLength(BlogData, (u8 *)"---", MaxBytes) == 3)
    {
        Type = blog_line_type_SubHeader;
    }
    else if (StringMatchLength(BlogData, (u8 *)"#image", MaxBytes) == 6)
    {
        Type = blog_line_type_Image;
    }

    return Type;
}

internal void GenerateBlogPage(linear_allocator *TempString, u8 *FileName)
{
    u64 BlogFileSize = GetFileSize(FileName) + 1;
    u8 *BlogFileData = PushLinearAllocator(TempString, BlogFileSize);
    u64 FreeSpace = GetLinearAllocatorFreeSpace(TempString);

    if (BlogFileData)
    {
        u64 FileSize = ReadFileIntoData(FileName, TempString->Data, BlogFileSize);
        TempString->Data[FileSize] = 0;
        u64 BlogFileDataOffset = 0;
        b32 IsStartOfLine = 1;

        while (BlogFileDataOffset < FileSize)
        {
            u8 *BlogOffsetData = BlogFileData + BlogFileDataOffset;
            u64 MaxBytes = FreeSpace - BlogFileDataOffset;

            if (IsStartOfLine)
            {
                blog_line_type BlogLineType = GetBlogLineType(BlogOffsetData, MaxBytes);
                printf("blog line type %d\n", BlogLineType);
                while (BlogFileData[BlogFileDataOffset++] != '\n');
            }
            else if (BlogFileData[BlogFileDataOffset] == '\n')
            {
                IsStartOfLine = 1;
                BlogFileDataOffset += 1;
            }
            else
            {
                IsStartOfLine = 0;
                BlogFileDataOffset += 1;
            }
        }

        u8 *HtmlPageData = GetLinearAllocatorWriteLocation(TempString);

        printf("\n\nblog file:\n%s\n", TempString->Data);
    }
    else
    {
        LogError("writing blog file to temp-string");
    }
}

void GenerateBlogPages(linear_allocator *TempString)
{
    u8 *BlogDirectory = (u8 *)"../blog";

    linear_allocator FileAllocator = WalkDirectory(BlogDirectory);
    file_list *FileList = (file_list *)FileAllocator.Data;
    file_list *SortedFileList = SortFileList(FileList);

    for (file_list *CurrentFile = SortedFileList; CurrentFile; CurrentFile = CurrentFile->Next)
    {
        GenerateBlogPage(TempString, CurrentFile->Name);
        TempString->Offset = 0;
    }

    FreeLinearAllocator(FileAllocator);
}
