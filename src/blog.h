#define IS_SPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')

void SkipSpace(buffer *Buffer, s32 *Index);
void GenerateBlogPages(linear_allocator *TempString, u8 *GenDirectory);

typedef enum
{
    blog_line_type_Paragraph,
    blog_line_type_Header,
    blog_line_type_SubHeader,
    blog_line_type_Image,
    blog_line_type_Count,
} blog_line_type;

void SkipSpace(buffer *Buffer, s32 *Index)
{
    while (*Index < Buffer->Size)
    {
        if (!IS_SPACE(Buffer->Data[*Index]))
        {
            break;
        }
        else
        {
            *Index += 1;
        }
    }
}

internal s32 StringMatchLength(u8 *StringA, u8 *StringB, u64 MaxBytes)
{    u32 I = 0;

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

global_variable buffer BlogLineTypes[blog_line_type_Count] = {
    [blog_line_type_Paragraph]   = (buffer){0,(u8 *)""},
    [blog_line_type_Header]      = (buffer){3,(u8 *)"==="},
    [blog_line_type_SubHeader]   = (buffer){3,(u8 *)"---"},
    [blog_line_type_Image]       = (buffer){6,(u8 *)"#image"},
};

internal blog_line_type GetBlogLineType(u8 *BlogData, u64 MaxBytes)
{
    blog_line_type Type = blog_line_type_Paragraph;

    buffer Header = BlogLineTypes[blog_line_type_Header];
    buffer SubHeader = BlogLineTypes[blog_line_type_SubHeader];
    buffer Image = BlogLineTypes[blog_line_type_Image];

    if (StringMatchLength(BlogData, Header.Data, MaxBytes) == Header.Size)
    {
        Type = blog_line_type_Header;
    }
    else if (StringMatchLength(BlogData, SubHeader.Data, MaxBytes) == SubHeader.Size)
    {
        Type = blog_line_type_SubHeader;
    }
    else if (StringMatchLength(BlogData, Image.Data, MaxBytes) == Image.Size)
    {
        Type = blog_line_type_Image;
    }

    return Type;
}

internal s32 GetBytesUntilNewline(u8 *Bytes, s32 MaxBytes)
{
    s32 BytesUntilNewline = -1;

    for (s32 I = 0; I < MaxBytes; ++I)
    {
        if (Bytes[I] == '\n')
        {
            BytesUntilNewline = I;
            break;
        }
    }

    return BytesUntilNewline;
}

internal b32 HandleBlogLine(linear_allocator *HtmlOutput, buffer *BlogBuffer, s32 *I)
{
    b32 ShouldContinue = 0;

    while (*I < BlogBuffer->Size)
    {
        SkipSpace(BlogBuffer, I);
        u8 *BlogOffsetData = BlogBuffer->Data + *I;

        blog_line_type BlogLineType = GetBlogLineType(BlogOffsetData, BlogBuffer->Size);
        b32 ShouldHandleTextTag = 0;
        u8 *OpenTagName = (u8 *)"<div>";
        u8 *CloseTagName = (u8 *)"</div>";

        switch (BlogLineType)
        {
        case blog_line_type_Header:
        {
            ShouldHandleTextTag = 1;
            OpenTagName = (u8 *)"<h1>";
            CloseTagName = (u8 *)"</h1>";
        } break;
        case blog_line_type_SubHeader: {
            ShouldHandleTextTag = 1;
            OpenTagName = (u8 *)"<h2>";
            CloseTagName = (u8 *)"</h2>";
        } break;
        case blog_line_type_Paragraph: {
            ShouldHandleTextTag = 1;
            OpenTagName = (u8 *)"<p>";
            CloseTagName = (u8 *)"</p>";
        } break;
        case blog_line_type_Image: {
            ShouldHandleTextTag = 1;
            OpenTagName = (u8 *)"<img src=\"";
            CloseTagName = (u8 *)"\" />";
        } break;
        default: break;
        }

        if (ShouldHandleTextTag)
        {
            *I += BlogLineTypes[BlogLineType].Size;
            SkipSpace(BlogBuffer, I);
            u8 *StartOfText = BlogBuffer->Data + *I;
            s32 BytesUntilNewline = GetBytesUntilNewline(StartOfText, BlogBuffer->Size);

            PushString(HtmlOutput, OpenTagName);
            WriteLinearAllocator(HtmlOutput, StartOfText, BytesUntilNewline);
            PushString(HtmlOutput, CloseTagName);
            PushString(HtmlOutput, (u8 *)"\n");

            *I = *I + BytesUntilNewline + 1;
        }
    }

    return ShouldContinue;
}

void GenerateBlogPages(linear_allocator *TempString, u8 *GenDirectory)
{
    u8 *BlogDirectory = (u8 *)"../blog";

    linear_allocator FileAllocator = WalkDirectory(BlogDirectory);
    file_list *FileList = (file_list *)FileAllocator.Data;
    file_list *SortedFileList = SortFileList(FileList);

    buffer File;

    for (file_list *CurrentFile = SortedFileList; CurrentFile; CurrentFile = CurrentFile->Next)
    {
        File.Size = GetFileSize(CurrentFile->Name);
        File.Data = PushLinearAllocator(TempString, File.Size + 1);

        if (File.Data)
        {
            ReadFileIntoData(CurrentFile->Name, File.Data, File.Size);
            File.Data[File.Size] = 0;

            u8 *HtmlPageData = GetLinearAllocatorWriteLocation(TempString);

            s32 BlogFileDataOffset = 0;
            while (HandleBlogLine(TempString, &File, &BlogFileDataOffset));
            WriteLinearAllocator(TempString, (u8 *)"\0", 1);

            printf("\n\nblog file:\n%s\n", File.Data);
            printf("\n\nhtml file:\n%s\n", HtmlPageData);
        }
        else
        {
            LogError("writing blog file to temp-string");
        }

        TempString->Offset = 0;
    }

    FreeLinearAllocator(FileAllocator);
}
