#define IS_SPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')

typedef enum
{
    pre_processor_command_Undefined,
    pre_processor_command_Include,
    pre_processor_command_Docgen,
    pre_processor_command_Count,
} pre_processor_command_type;

typedef struct
{
    pre_processor_command_type Type;
    u8 *Name;
} pre_processor_command;

#define PRE_PROCESSOR_COMMAND_MAX 16
#define PRE_PROCESSOR_VARIABLE_MAX 64

typedef struct
{
    u8 *Key;
    u8 *Value;
} pre_processor_variable;

typedef struct
{
    u8 *Bra;
    s32 BraCount;
    u8 *Ket;
    s32 KetCount;
    pre_processor_command Commands[PRE_PROCESSOR_COMMAND_MAX];
    pre_processor_variable Variables[PRE_PROCESSOR_VARIABLE_MAX];
    s32 CommandCount;

    ryn_memory_arena StringAllocator;
    ryn_memory_arena OutputAllocator;
} pre_processor;

typedef enum
{
    command_result_Undefined,
    command_result_Count,
} command_result_type;

typedef struct
{

} command_result;

typedef enum
{
    blog_line_type_Paragraph,
    blog_line_type_Header,
    blog_line_type_SubHeader,
    blog_line_type_Image,
    blog_line_type_Count,
} blog_line_type;


b32 PreprocessFile(pre_processor *PreProcessor, ryn_memory_arena TempString, u8 *FilePath, u8 *OutputFilePath);
void GenerateSite(ryn_memory_arena *TempString);

u8 *BlogPageTemplateOpen =
    (u8 *)"<!doctype html>"                        \
    "<html lang=\"en-us\">"                        \
    "<head>"                                       \
    "{| include ../src/layout/head_common.html |}" \
    "<style>"                                      \
    "</style>"                                     \
    "</head>"                                      \
    "<body>"                                       \
    /* TODO: Remove hard-coded style */
    "<main style=\"padding: 1rem;\">"              \
    "{| include ../src/layout/navigation_header.html |}";

u8 *BlogPageTemplateClose =
    (u8 *)"</main>"                             \
    "</body>"                                   \
    "</html>";



internal void SkipSpace(buffer *Buffer, s32 *Index)
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

internal u8 *GetPreprocessVariable(pre_processor *PreProcessor, u8 *Key)
{
    u8 *Value = 0;

    for (s32 I = 0; I < PRE_PROCESSOR_VARIABLE_MAX; ++I)
    {
        pre_processor_variable Variable = PreProcessor->Variables[I];

        if (Variable.Key)
        {
            if (StringsEqual(Variable.Key, Key))
            {
                Value = Variable.Value;
                break;
            }
        }
        else
        {
            break;
        }
    }
    return Value;
}

internal b32 SetPreprocessVariable(pre_processor *PreProcessor, u8 *Key, u8 *Value)
{
    b32 ErrorCode = 0;
    b32 HasSet = 0;
    s32 FirstOpenIndex = -1;

    for (s32 I = 0; I < PRE_PROCESSOR_VARIABLE_MAX; ++I)
    {
        pre_processor_variable *Variable = &PreProcessor->Variables[I];

        if (Variable->Key)
        {
            if (StringsEqual(Variable->Key, Key))
            {
                Variable->Value = Value;
                HasSet = 1;
                break;
            }
        }
        else if (FirstOpenIndex < 0)
        {
            FirstOpenIndex = I;
        }
    }

    if (!HasSet && FirstOpenIndex >= 0)
    {
        PreProcessor->Variables[FirstOpenIndex].Key = Key;
        PreProcessor->Variables[FirstOpenIndex].Value = Value;
        HasSet = 1;
    }

    if (!HasSet)
    {
        ErrorCode = 1;
    }

    return ErrorCode;
}

internal pre_processor CreatePreProcessor(u8 *Bra, u8 *Ket)
{
    pre_processor PreProcessor;

    PreProcessor.Bra = Bra;
    PreProcessor.BraCount = GetStringLength(Bra);

    PreProcessor.Ket = Ket;
    PreProcessor.KetCount = GetStringLength(Ket);

    PreProcessor.CommandCount = 0;

    for (s32 I = 0; I < PRE_PROCESSOR_VARIABLE_MAX; ++I)
    {
        PreProcessor.Variables[I].Key = 0;
        PreProcessor.Variables[I].Value = 0;
    }

    { /* allocator setup */
        u64 StringAllocatorVirtualSize = Gigabytes(1);
        u64 OutputBufferVirtualSize = Gigabytes(1);
        u64 TotalVirtualSize = StringAllocatorVirtualSize + OutputBufferVirtualSize;

        PreProcessor.StringAllocator = ryn_memory_CreateArena(TotalVirtualSize);
        PreProcessor.OutputAllocator = ryn_memory_CreateSubArena(&PreProcessor.StringAllocator, OutputBufferVirtualSize);
    }

    return PreProcessor;
}

internal void AddPreProcessorCommand(pre_processor *PreProcessor, pre_processor_command_type Type, u8 *Name)
{
    Assert(PreProcessor->CommandCount < PRE_PROCESSOR_COMMAND_MAX);

    s32 I = PreProcessor->CommandCount;

    PreProcessor->Commands[I].Type = Type;
    PreProcessor->Commands[I].Name = Name;

    PreProcessor->CommandCount += 1;
}

internal b32 CheckIfStringIsPrefix(u8 *String, buffer *Buffer, s32 BufferOffset)
{
    b32 IsPrefix = 1;
    s32 StringLength = GetStringLength(String);

    if ((Buffer->Size - BufferOffset) >= StringLength)
    {
        for (s32 I = 0; I < StringLength; I++)
        {
            if (String[I] != Buffer->Data[BufferOffset + I])
            {
                IsPrefix = 0;
                break;
            }
        }
    }
    else
    {
        IsPrefix = 0;
    }

    return IsPrefix;
}

internal buffer GetCommandToken(pre_processor *PreProcessor, buffer *Buffer, s32 Offset)
{
    buffer TokenBuffer;
    TokenBuffer.Size = 0;
    TokenBuffer.Data = 0;

    SkipSpace(Buffer, &Offset);

    for (; TokenBuffer.Size < Buffer->Size - Offset; TokenBuffer.Size++)
    {
        b32 IsKet = CheckIfStringIsPrefix(PreProcessor->Ket, Buffer, TokenBuffer.Size);
        b32 IsSpace = IS_SPACE(Buffer->Data[Offset + TokenBuffer.Size]);

        if (IsKet || IsSpace)
        {
            ryn_memory_arena *StringAllocator = &PreProcessor->StringAllocator;
            TokenBuffer.Data = StringAllocator->Data + StringAllocator->Offset;

            b32 WriteError = ryn_memory_WriteArena(StringAllocator, Buffer->Data + Offset, TokenBuffer.Size + 1);

            if (!WriteError)
            {
                TokenBuffer.Data[TokenBuffer.Size] = 0;
            }
            break;
        }
    }

    return TokenBuffer;
}

internal b32 HandlePreProcessCommand(pre_processor *PreProcessor, buffer *Buffer, s32 Offset, s32 CommandSize)
{
    s32 Index = Offset;
    s32 MaxIndex = Offset + CommandSize;
    u8 *IncludeName = (u8 *)"include";
    ryn_memory_arena *OutputAllocator = &PreProcessor->OutputAllocator;
    b32 CommandWasHandled = 0;

    SkipSpace(Buffer, &Index);

    if (Index < MaxIndex && CheckIfStringIsPrefix(IncludeName, Buffer, Index))
    {
        { /* skip the include keyword */
            Index += GetStringLength(IncludeName);
            SkipSpace(Buffer, &Index);
        }

        CommandWasHandled = 1;

        buffer Token = GetCommandToken(PreProcessor, Buffer, Index);

        if (Token.Data[0] == '$')
        {
            /* we have a variable argument */
            u8 *TokenName = Token.Data + 1;
            u8 *IncludeData = GetPreprocessVariable(PreProcessor, TokenName);

            if (IncludeData)
            {
                PushString(OutputAllocator, IncludeData);
            }
            else
            {
                printf("Include variable name \"%s\"\n", Token.Data);
                LogError("while handling pre-processor \"include\", we could not load the include variable's value.");
            }
        }
        else
        {
            /* assume we are dealing with a path argument */
            buffer *Buffer = ReadFileIntoBuffer(Token.Data);

            if (Buffer)
            {
                b32 WriteError = ryn_memory_WriteArena(OutputAllocator, Buffer->Data, Buffer->Size);
                FreeBuffer(Buffer);

                if (WriteError)
                {
                    LogError("failed to write file to output");
                }
            }
            else
            {
                printf("Include file = \"%s\"\n", Token.Data);
                LogError("include file not found");
            }
        }
    }
    else
    {
        printf("PreProcessor command: \"%s\"\n", Buffer->Data);
        LogError("unknown preprocessor command");
    }

    return CommandWasHandled;
}

internal b32 PreprocessBuffer(pre_processor *PreProcessor, ryn_memory_arena *TempString, buffer *Buffer, u8 *OutputFilePath)
{
    ryn_memory_arena *OutputAllocator = &PreProcessor->OutputAllocator;

    b32 Error = 0;
    s32 WriteIndex = 0;

    if (!Buffer)
    {
        printf("Error in PreprocessFile: null buffer\n");
        Error = 1;
    }

    for (s32 I = 0; I < Buffer->Size; I++)
    {
        b32 FoundBra = CheckIfStringIsPrefix(PreProcessor->Bra, Buffer, I);
        s32 HereDocCharOffset = I + PreProcessor->BraCount;
        b32 IsHereDoc = HereDocCharOffset < Buffer->Size && Buffer->Data[HereDocCharOffset] == '#';

        if (FoundBra && IsHereDoc)
        {
            I = HereDocCharOffset + 1; /* NOTE one past the here-doc character */
            s32 Begin = I;
            /* Skip to the next space, which is the end of the heredoc label */
            while (!IS_SPACE(Buffer->Data[I]))
            {
                I += 1;
            }

            /* Set up the heredoc label, which is used to scan for the end of the heredoc. */
            s32 HereDocLabelSize = I - Begin + 1;
            u8 *HereDocLabel = TempString->Data + TempString->Offset;
            s32 WriteError = ryn_memory_WriteArena(TempString, Buffer->Data + Begin, HereDocLabelSize);

            if (WriteError)
            {
                LogError("failed to write heredoc label to allocator");
            }
            else
            {
                TempString->Data[TempString->Offset - 1] = 0;
            }

            SkipSpace(Buffer, &I);

            for (s32 J = I; J < Buffer->Size; J++)
            {
                if (CheckIfStringIsPrefix(HereDocLabel, Buffer, J))
                {
                    s32 PreDataSize = HereDocCharOffset - PreProcessor->BraCount - WriteIndex;
                    b32 WriteError = ryn_memory_WriteArena(OutputAllocator, Buffer->Data + WriteIndex, PreDataSize);

                    if (WriteError)
                    {
                        LogError("failed to write heredoc pre-data to output");
                    }

                    s32 HereDocDataBegin = Begin + HereDocLabelSize;
                    s32 HereDocDataSize = J - HereDocDataBegin;
                    u8 *DataBegin = Buffer->Data + HereDocDataBegin;

                    WriteError = ryn_memory_WriteArena(OutputAllocator, DataBegin, HereDocDataSize);

                    if (WriteError)
                    {
                        LogError("failed to write heredoc data to output");
                    }

                    I = J + HereDocLabelSize - 1; /* NOTE minus one to ignore the null terminator in the heredoc label */
                    WriteIndex = I;
                    break;
                }
            }
        }
        else if (FoundBra)
        {
            s32 CommandStart = I + PreProcessor->BraCount;

            for (s32 J = CommandStart; J < Buffer->Size; J++)
            {
                b32 FoundKet = CheckIfStringIsPrefix(PreProcessor->Ket, Buffer, J);

                if (FoundKet)
                {
                    u8 *BufferStart = Buffer->Data + WriteIndex;
                    s32 JustPastKet = J + PreProcessor->KetCount;
                    s32 Size = I - WriteIndex;

                    s32 OldI = I;
                    s32 OldWriteIndex = WriteIndex;

                    b32 WriteError = ryn_memory_WriteArena(OutputAllocator, BufferStart, Size);

                    if (WriteError)
                    {
                        LogError("failed to write to output");
                        break;
                    }

                    WriteIndex = JustPastKet;
                    I = JustPastKet;

                    b32 CommandWasHandled = HandlePreProcessCommand(PreProcessor, Buffer, CommandStart, J);

                    if (!CommandWasHandled)
                    {
                        /* We saw preprocessor brackets, but did not parse. Assume it's not a pre-processer and
                           just output the text as usual.
                        */
                        I = OldI;
                        WriteIndex = OldWriteIndex;
                    }
                    break;
                }
            }
        }
    }

    if (WriteIndex < Buffer->Size)
    {
        /* NOTE This branch handles any text after the last preprocessor command, or if
           there were not preprocessor commands.
        */
        s32 Size = Buffer->Size - WriteIndex;
        u8 *DataBegin = Buffer->Data + WriteIndex;
        b32 WriteError = ryn_memory_WriteArena(OutputAllocator, DataBegin, Size);

        if (WriteError)
        {
            LogError("failed to write to output");
        }
    }

    printf("Writing pre-processed file %s\n", OutputFilePath);
    WriteFileWithPath(OutputFilePath, OutputAllocator->Data, OutputAllocator->Offset);
    OutputAllocator->Offset = 0;

    return Error;
}

b32 PreprocessFile(pre_processor *PreProcessor, ryn_memory_arena TempString, u8 *FilePath, u8 *OutputFilePath)
{
    buffer *Buffer = ReadFileIntoBuffer(FilePath);
    b32 Error = PreprocessBuffer(PreProcessor, &TempString, Buffer, OutputFilePath);
    return Error;
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

internal b32 HandleBlogLine(ryn_memory_arena *HtmlOutput, buffer *BlogBuffer, s32 *I)
{
    b32 ShouldContinue = 0;

    while (*I < BlogBuffer->Size && BlogBuffer->Data[*I])
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

            if (BytesUntilNewline > 0)
            {
                PushString(HtmlOutput, OpenTagName);
                ryn_memory_WriteArena(HtmlOutput, StartOfText, BytesUntilNewline);
                PushString(HtmlOutput, CloseTagName);
                PushString(HtmlOutput, (u8 *)"\n");

                *I = *I + BytesUntilNewline + 1;
            }
        }
    }

    return ShouldContinue;
}

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

internal void PushNullTerminator(ryn_memory_arena *Allocator)
{
    Allocator->Data[Allocator->Offset] = 0;
    Allocator->Offset += 1;
}

internal buffer GetOutputHtmlPath(ryn_memory_arena *TempString, u8 *OldRootPath, u8 *NewRootPath, u8 *CodePagePath, b32 ExcludePathExtension, b32 AddHtmlExtension)
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

    if (ExcludePathExtension)
    {
        s32 End = CodePagePathOffset;

        while (CodePagePath[End])
        {
            if (CodePagePath[End] == '.')
            {
                break;
            }

            End += 1;
        }

        s32 Size = End - CodePagePathOffset;
        ryn_memory_WriteArena(TempString, CodePagePath + CodePagePathOffset, Size);
    }
    else
    {
        PushString(TempString, CodePagePath + CodePagePathOffset);
    }

    if (AddHtmlExtension)
    {
        PushString(TempString, Extension);
    }

    PushNullTerminator(TempString);

    Buffer.Size = TempString->Offset - BeginOffset;
    core_CopyMemory(TempString->Data + BeginOffset, Buffer.Data, Buffer.Size);

    return Buffer;
}

internal void GenerateBlogPages(ryn_memory_arena *TempString, pre_processor *PreProcessor, u8 *SiteBlogDirectory)
{
    u8 *BlogDirectory = (u8 *)"../blog";
    u8 *BlogPageTemplateFilePath = (u8 *)"../src/layout/blog.html";
    u8 *BlogListingFilePath = (u8 *)"../gen/blog_listing.html";

    ryn_memory_arena FileArena = ryn_memory_CreateArena(Gigabytes(1));
    file_list *FileList = WalkDirectory(&FileArena, BlogDirectory);
    file_list *SortedFileList = SortFileList(FileList);

    buffer File;
    buffer BlogPageTemplate;

    BlogPageTemplate.Size = platform_GetFileSize(BlogPageTemplateFilePath);
    BlogPageTemplate.Data = ryn_memory_PushSize(TempString, BlogPageTemplate.Size + 1);

    if (!BlogPageTemplate.Data)
    {
        LogError("loading blog page template file");
        return;
    }

    ReadFileIntoData(BlogPageTemplateFilePath, BlogPageTemplate.Data, BlogPageTemplate.Size);
    BlogPageTemplate.Data[BlogPageTemplate.Size] = 0; /* null terminate */

    u64 TempStringOffset = TempString->Offset;
    u64 BlogListingOffset = PreProcessor->OutputAllocator.Offset;

    /* write each blog page */
    for (file_list *CurrentFile = SortedFileList; CurrentFile; CurrentFile = CurrentFile->Next)
    {
        File.Size = platform_GetFileSize(CurrentFile->Name);
        File.Data = ryn_memory_PushSize(TempString, File.Size + 1);

        if (File.Data)
        {
            ReadFileIntoData(CurrentFile->Name, File.Data, File.Size + 1);
            File.Data[File.Size] = 0;

            u64 BlogHtmlOffset = TempString->Offset;

            PushString(TempString, BlogPageTemplateOpen);

            s32 BlogFileDataOffset = 0;
            while (HandleBlogLine(TempString, &File, &BlogFileDataOffset));

            PushString(TempString, BlogPageTemplateClose);

            buffer BlogHtmlBuffer;
            BlogHtmlBuffer.Data = TempString->Data + BlogHtmlOffset;
            BlogHtmlBuffer.Size = TempString->Offset - BlogHtmlOffset;

            buffer BlogOutputPath = GetOutputHtmlPath(TempString, BlogDirectory, SiteBlogDirectory, CurrentFile->Name, 1, 1);

            b32 Error = PreprocessBuffer(PreProcessor, TempString, &BlogHtmlBuffer, BlogOutputPath.Data);

            if (Error)
            {
                LogError("preprocessing blog template page");
            }
        }
        else
        {
            LogError("writing blog file to temp-string");
        }

        TempString->Offset = TempStringOffset;
    }

    { /* write blog listing page */
        ryn_memory_arena *OutputAllocator = &PreProcessor->OutputAllocator;
        u64 OutputOffset = PreProcessor->OutputAllocator.Offset;

        for (file_list *CurrentFile = SortedFileList; CurrentFile; CurrentFile = CurrentFile->Next)
        {
            buffer BlogOutputPath = GetOutputHtmlPath(TempString, BlogDirectory, (u8 *)"/blog", CurrentFile->Name, 1, 1);

            PushString(OutputAllocator, (u8 *)"<li><a href=\"");
            PushString(OutputAllocator, BlogOutputPath.Data);
            PushString(OutputAllocator, (u8 *)"\">");
            PushString(OutputAllocator, CurrentFile->Name);
            PushString(OutputAllocator, (u8 *)"</a></li>\n");
        }

        u8 *BlogListingData = OutputAllocator->Data + OutputOffset;
        u64 Size = OutputAllocator->Offset - BlogListingOffset;
        WriteFileWithPath(BlogListingFilePath, BlogListingData, Size);
        OutputAllocator->Offset = OutputOffset;
    }

    ryn_memory_FreeArena(FileAllocator);
}

internal buffer EscapeHtmlString(ryn_memory_arena *TempString, u8 *HtmlString, s32 Length)
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
            u8 *Data = ryn_memory_PushSize(TempString, Size);

            core_CopyMemory(BeginData, Data, Size);
            PushString(TempString, EscapeString);
            HtmlStringBegin = I + 1;
        }
    }

    s32 RemainingLength = I - HtmlStringBegin;

    if (RemainingLength)
    {
        ryn_memory_WriteArena(TempString, HtmlString + HtmlStringBegin, RemainingLength);
    }

    Buffer.Size = TempString->Offset - InitialOffset;

    return Buffer;
}

#define FILE_TREE_DEPTH_MAX 16

typedef struct
{
    u8 *Path[FILE_TREE_DEPTH_MAX];
} file_tree;

typedef struct
{
    u8 *Parts[FILE_TREE_DEPTH_MAX];
    u8 *Name;
} path_parts;

internal path_parts GetPathParts(ryn_memory_arena *TempString, u8 *Path)
{
    path_parts PathParts = {0};

    u64 Begin = 0;
    s32 PathPartIndex = 0;
    u64 I = 0;

    for (;;)
    {
        if (!Path[I])
        {
            u64 PathStringSize = I - Begin + 1; /* NOTE: Add one, and assume that Path is null-terminated. */
            u8 *WriteLocation = ryn_memory_GetArenaWriteLocation(TempString);
            s32 WriteError = ryn_memory_WriteArena(TempString, Path + Begin, PathStringSize);

            if (WriteError)
            {
                LogError("writing name to temp-string");
            }
            else
            {
                PathParts.Name = WriteLocation;
            }

            break;
        }
        else if (Path[I] == PATH_SEPARATOR)
        {
            u64 Size = I - Begin + 1;

            u8 *WriteLocation = ryn_memory_GetArenaWriteLocation(TempString);
            s32 WriteError = ryn_memory_WriteArena(TempString, Path + Begin, Size);

            if (WriteError)
            {
                LogError("writing to temp-string");
                break;
            }
            else if (PathPartIndex >= FILE_TREE_DEPTH_MAX)
            {
                printf("PathPartIndex out of max (%d/%d)", PathPartIndex, FILE_TREE_DEPTH_MAX);
                LogError("path part index max exceeded");
            }

            PathParts.Parts[PathPartIndex] = WriteLocation;
            PathPartIndex += 1;
            TempString->Data[TempString->Offset - 1] = 0;

            Begin = I + 1;
        }

        I += 1;
    }

    return PathParts;
}

internal u8 *GetFileNameFromPath(ryn_memory_arena *Allocator, u8 *Path)
{
    s32 FileNameOffset = 0;
    s32 I = 0;

    for (I = 0; Path[I]; ++I)
    {
        if (Path[I] == '/')
        {
            FileNameOffset = I + 1;
        }

        /* if (Path[I] == '.') */
        /* { */
        /*     DotOffset = I; */
        /* } */
    }

    s32 Length = I - FileNameOffset;
    Assert(Length > 0);
    u8 *FileName = ryn_memory_GetArenaWriteLocation(Allocator);
    ryn_memory_WriteArena(Allocator, Path + FileNameOffset, Length);
    PushNullTerminator(Allocator);

    return FileName;
}

void GenerateCodePages(ryn_memory_arena *FileArena, ryn_memory_arena *TempString)
{
    u8 *SourceCodePath = (u8 *)"../src";
    u8 *GenCodePagesPath = (u8 *)"../gen/code_pages";

    file_list *FileList = WalkDirectory(FileArena, SourceCodePath);
    ryn_memory_arena CodePage = ryn_memory_CreateArena(Gigabytes(1));

    file_list *SortedFileList = SortFileList(FileList);

    { /* Print out html for file-tree */
        u8 *CodePageListingPath = (u8 *)"../gen/code_page_links.html";
        file_tree Tree = {0};

        for (file_list *CurrentFile = SortedFileList; CurrentFile; CurrentFile = CurrentFile->Next)
        {
            buffer FileOutputName = GetOutputHtmlPath(TempString, SourceCodePath, 0, CurrentFile->Name, 0, 0);
            path_parts PathParts = GetPathParts(TempString, CurrentFile->Name);
            s32 LeadingSpaceCount = 0;

            PushString(&CodePage, (u8 *)"<div>");

            for (s32 I = 0; I < FILE_TREE_DEPTH_MAX; ++I)
            {
                u8 *PathPart = PathParts.Parts[I];

                if (PathPart)
                {
                    if (Tree.Path[I] && StringsEqual(PathPart, Tree.Path[I]))
                    {
                        LeadingSpaceCount += 1;
                    }
                    else
                    {
                        for (s32 J = 0; J < I - 1; ++J)
                        {
                            PushString(&CodePage, (u8 *)"<div class=\"spacer\"></div>");
                        }

                        if (I > 0)
                        {
                            PushString(&CodePage, (u8 *)"<div class=\"spacer\"></div>");
                        }

                        PushString(&CodePage, PathPart);
                        PushString(&CodePage, (u8 *)"<br>");
                        LeadingSpaceCount = I + 1;
                        /* We found a new path-part, so we overwrite old path parts.
                           We should keep any part of the path at the start where both Tree.Path
                           and PathPart match.
                        */
                        Tree.Path[I] = PathPart;
                    }
                }
                else
                {
                    break;
                }
            }

            for (s32 J = 0; J < LeadingSpaceCount - 1; ++J)
            {
                PushString(&CodePage, (u8 *)"<div class=\"spacer\"></div>");
            }

            PushString(&CodePage, (u8 *)"<div class=\"spacer\"></div>");

            PushString(&CodePage, (u8 *)"<a href=\"");
            PushString(&CodePage, FileOutputName.Data);
            PushString(&CodePage, (u8 *)".html\">");
            PushString(&CodePage, PathParts.Name);
            PushString(&CodePage, (u8 *)"</a>");

            PushString(&CodePage, (u8 *)"</div>");
        }

        WriteFileWithPath(CodePageListingPath, CodePage.Data, CodePage.Offset);
        CodePage.Offset = 0;
    }

    { /* inidividual code page docs */
        for (file_list *CurrentFile = SortedFileList; CurrentFile; CurrentFile = CurrentFile->Next)
        {
            buffer Buffer = GetOutputHtmlPath(TempString, SourceCodePath, GenCodePagesPath, CurrentFile->Name, 0, 1);
            EnsurePathDirectoriesExist(Buffer.Data);

            PushString(&CodePage, (u8 *)(
                "<!doctype html>"                                                          \
                "<html lang=\"en-us\">"                                                    \
                    "<head>"                                                               \
                        "{" "| include ../src/layout/head_common.html |" "}"               \
                        "<style>"                                                          \
                            "{" "| include ../src/layout/code_page_style.css |" "}"        \
                        "</style>"                                                         \
                    "</head>"                                                              \
                    "<body>"                                                               \
                /* TODO: move hard-coded style */
                        "<main style=\"padding: 1rem\">"                                   \
                            "{" "| include ../src/layout/navigation_header.html |" "}"     \
                                "<h2>"));

            PushString(&CodePage, CurrentFile->Name);

            PushString(&CodePage, (u8 *)(
                                "</h2>"                                                    \
                                "<pre>"                                                    \
                                    "{" "|" "#HERE" "DOC "));


            buffer *CodePageBuffer = ReadFileIntoBuffer(CurrentFile->Name);
            buffer EscapedHtmlBuffer = EscapeHtmlString(TempString, CodePageBuffer->Data, CodePageBuffer->Size);

            u8 *CodePageData = ryn_memory_PushSize(&CodePage, EscapedHtmlBuffer.Size);
            core_CopyMemory(EscapedHtmlBuffer.Data, CodePageData, EscapedHtmlBuffer.Size);

            PushString(&CodePage, (u8 *)"HERE" "DOC</pre></main></body></html>");

            WriteFileWithPath(Buffer.Data, CodePage.Data, CodePage.Offset);
            FreeBuffer(CodePageBuffer);

            CodePage.Offset = 0;
            TempString->Offset = 0;
        }

    }

    ryn_memory_FreeArena(FileAllocator);
}

void GenerateSite(ryn_memory_arena *FileArena, ryn_memory_arena *TempString)
{
    u8 *GenDirectory       = (u8 *)"../gen";
    u8 *CodePagesDirectory = (u8 *)"../gen/code_pages";

    u8 *AssetsDirectory = (u8 *)"../assets";

    u8 *SiteDirectory       = (u8 *)"../site";
    u8 *SiteBlogDirectory   = (u8 *)"../site/blog";
    u8 *SiteAssetsDirectory = (u8 *)"../site/assets";

    u8 *IndexIn  = (u8 *)"../src/layout/index.html";
    u8 *IndexOut = (u8 *)"../site/index.html";

    u8 *CodeIn  = (u8 *)"../src/layout/code.html";
    u8 *CodeOut = (u8 *)"../site/code.html";

    u8 *BlogIn  = (u8 *)"../src/layout/blog.html";
    u8 *BlogOut = (u8 *)"../site/blog/index.html";

    u8 *LSystemIn  = (u8 *)"../src/layout/l_system.html";
    u8 *LSystemOut = (u8 *)"../gen/l_system.html";

    u8 *ScubaIn  = (u8 *)"../src/layout/scuba.html";
    u8 *ScubaOut = (u8 *)"../gen/scuba.html";

    u8 *EstudiosoIn  = (u8 *)"../src/layout/estudioso.html";
    u8 *EstudiosoOut = (u8 *)"../gen/estudioso.html";

    u8 *Bra = (u8 *)"{|";
    u8 *Ket = (u8 *)"|}";

    EnsureDirectoryExists(GenDirectory);
    EnsureDirectoryExists(CodePagesDirectory);
    EnsureDirectoryExists(AssetsDirectory);
    EnsureDirectoryExists(SiteDirectory);
    EnsureDirectoryExists(SiteBlogDirectory);
    EnsureDirectoryExists(SiteAssetsDirectory);

    { /* Copy some ../assets into ../site/assets. */
        /* TODO: Put asset mappings into some kind of data structure and loop thoough it? */
        u64 OldAllocatorOffset = TempString->Offset;
        u64 FileSize = ReadFileIntoAllocator(TempString, (u8 *)"../assets/scuba.png");
        WriteFileWithPath((u8 *)"../site/assets/scuba.png", TempString->Data + OldAllocatorOffset, FileSize);
        TempString->Offset = OldAllocatorOffset;
    }

    pre_processor PreProcessor = CreatePreProcessor(Bra, Ket);
    AddPreProcessorCommand(&PreProcessor, pre_processor_command_Include, (u8 *)"include");
    AddPreProcessorCommand(&PreProcessor, pre_processor_command_Docgen, (u8 *)"docgen");

    GenerateCodePages(TempString);

    {
        file_list *FileList = WalkDirectory(CodePagesDirectory);
        file_list *SortedFileList = SortFileList(FileList);

        for (file_list *CurrentFile = SortedFileList; CurrentFile; CurrentFile = CurrentFile->Next)
        {
            buffer OutputHtmlPath = GetOutputHtmlPath(TempString, CodePagesDirectory, SiteDirectory, CurrentFile->Name, 0, 0);
            EnsurePathDirectoriesExist(OutputHtmlPath.Data);

            u8 FileName[256];
            {
                s32 FileNameLength = GetStringLength(CurrentFile->Name);
                s32 Offset = 0;

                /* NOTE: Check if path begins with relative-up-path "../" and ignore if it is there. */
                if (FileNameLength >= 3)
                {
                    u8 *Name = CurrentFile->Name;
                    if (Name[0] == '.' && Name[1] == '.' && Name[2] == '/')
                    {
                        Offset = 3;
                    }
                }

                u32 DotCount = 0;
                s32 I;
                for (I = 0; I < 256; ++I)
                {
                    u8 Char = OutputHtmlPath.Data[I + Offset];

                    if ((I + Offset) >= FileNameLength)
                    {
                        break;
                    }

                    if (Char == '.')
                    {
                        DotCount += 1;

                        if (DotCount == 2)
                        {
                            break;
                        }
                    }

                    FileName[I] = Char;
                }
                FileName[I] = 0;
            }

            SetPreprocessVariable(&PreProcessor, (u8 *)"FileName", FileName);

            PreprocessFile(&PreProcessor, *TempString, CurrentFile->Name, OutputHtmlPath.Data);
        }

        ryn_memory_FreeArena(FileAllocator);
    }

    TempString->Offset = 0;

    GenerateBlogPages(TempString, &PreProcessor, SiteBlogDirectory);

    PreprocessFile(&PreProcessor, *TempString, IndexIn, IndexOut);
    PreprocessFile(&PreProcessor, *TempString, CodeIn, CodeOut);
    PreprocessFile(&PreProcessor, *TempString, BlogIn, BlogOut);
    PreprocessFile(&PreProcessor, *TempString, LSystemIn, LSystemOut);
    PreprocessFile(&PreProcessor, *TempString, ScubaIn, ScubaOut);
    PreprocessFile(&PreProcessor, *TempString, EstudiosoIn, EstudiosoOut);

}
