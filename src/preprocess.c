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

    linear_allocator StringAllocator;
    linear_allocator OutputAllocator;
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


b32 PreprocessFile(pre_processor *PreProcessor, linear_allocator TempString, u8 *FilePath, u8 *OutputFilePath);
void GenerateSite(linear_allocator *TempString);


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

internal u8 *GetPreprocessVariable(pre_processor *Preprocessor, u8 *Key)
{
    u8 *Value = 0;

    for (s32 I = 0; I < PRE_PROCESSOR_VARIABLE_MAX; ++I)
    {
        pre_processor_variable Variable = Preprocessor->Variables[I];

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

internal b32 SetPreprocessVariable(pre_processor *Preprocessor, u8 *Key, u8 *Value)
{
    b32 ErrorCode = 0;
    b32 HasSet = 0;
    s32 FirstOpenIndex = -1;

    for (s32 I = 0; I < PRE_PROCESSOR_VARIABLE_MAX; ++I)
    {
        pre_processor_variable *Variable = &Preprocessor->Variables[I];

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
        Preprocessor->Variables[FirstOpenIndex].Key = Key;
        Preprocessor->Variables[FirstOpenIndex].Value = Value;
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

        PreProcessor.StringAllocator = CreateLinearAllocator(TotalVirtualSize);
        PreProcessor.StringAllocator.Capacity = StringAllocatorVirtualSize;

        /* TODO Do _not_ call CreateLinearAllocator twice here, just set one of the
           allocator's data to the first allocator's data plus an offset.
        */
        PreProcessor.OutputAllocator = CreateLinearAllocator(Gigabytes(1));
        PreProcessor.OutputAllocator.Capacity = OutputBufferVirtualSize;
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
            linear_allocator *StringAllocator = &PreProcessor->StringAllocator;
            TokenBuffer.Data = StringAllocator->Data + StringAllocator->Offset;

            b32 WriteError = WriteLinearAllocator(StringAllocator, Buffer->Data + Offset, TokenBuffer.Size + 1);

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
    linear_allocator *OutputAllocator = &PreProcessor->OutputAllocator;
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
                b32 WriteError = WriteLinearAllocator(OutputAllocator, Buffer->Data, Buffer->Size);
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
        printf("Preprocessor command: \"%s\"\n", Buffer->Data);
        LogError("unknown preprocessor command");
    }

    return CommandWasHandled;
}

internal b32 PreprocessBuffer(pre_processor *PreProcessor, linear_allocator *TempString, buffer *Buffer, u8 *OutputFilePath)
{
    linear_allocator *OutputAllocator = &PreProcessor->OutputAllocator;

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
            s32 WriteError = WriteLinearAllocator(TempString, Buffer->Data + Begin, HereDocLabelSize);

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
                    b32 WriteError = WriteLinearAllocator(OutputAllocator, Buffer->Data + WriteIndex, PreDataSize);

                    if (WriteError)
                    {
                        LogError("failed to write heredoc pre-data to output");
                    }

                    s32 HereDocDataBegin = Begin + HereDocLabelSize - 1;
                    s32 HereDocDataSize = J - HereDocDataBegin;
                    u8 *DataBegin = Buffer->Data + HereDocDataBegin;

                    WriteError = WriteLinearAllocator(OutputAllocator, DataBegin, HereDocDataSize);

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

                    b32 WriteError = WriteLinearAllocator(OutputAllocator, BufferStart, Size);

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
        b32 WriteError = WriteLinearAllocator(OutputAllocator, DataBegin, Size);

        if (WriteError)
        {
            LogError("failed to write to output");
        }
    }

    printf("Writing pre-processed file %s\n", OutputFilePath);
    WriteFile(OutputFilePath, OutputAllocator->Data, OutputAllocator->Offset);
    OutputAllocator->Offset = 0;

    return Error;
}

b32 PreprocessFile(pre_processor *PreProcessor, linear_allocator TempString, u8 *FilePath, u8 *OutputFilePath)
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

internal void GenerateBlogPages(linear_allocator *TempString, pre_processor *Preprocessor, u8 *SiteBlogDirectory)
{
    u8 *BlogDirectory = (u8 *)"../blog";
    u8 *BlogPageTemplateFilePath = (u8 *)"../src/layout/blog.html";
    u8 *BlogListingFilePath = (u8 *)"../gen/blog_listing.html";

    linear_allocator FileAllocator = WalkDirectory(BlogDirectory);
    file_list *SortedFileList = SortFileList((file_list *)FileAllocator.Data);

    buffer File;
    buffer BlogPageTemplate;

    BlogPageTemplate.Size = GetFileSize(BlogPageTemplateFilePath);
    BlogPageTemplate.Data = PushLinearAllocator(TempString, BlogPageTemplate.Size + 1);

    if (!BlogPageTemplate.Data)
    {
        LogError("loading blog page template file");
        return;
    }

    ReadFileIntoData(BlogPageTemplateFilePath, BlogPageTemplate.Data, BlogPageTemplate.Size);
    BlogPageTemplate.Data[BlogPageTemplate.Size] = 0; /* null terminate */

    u64 TempStringOffset = TempString->Offset;

    u64 BlogListingOffset = Preprocessor->OutputAllocator.Offset;

    /* write each blog page */
    for (file_list *CurrentFile = SortedFileList; CurrentFile; CurrentFile = CurrentFile->Next)
    {
        File.Size = GetFileSize(CurrentFile->Name);
        File.Data = PushLinearAllocator(TempString, File.Size + 1);

        if (File.Data)
        {
            ReadFileIntoData(CurrentFile->Name, File.Data, File.Size + 1);
            File.Data[File.Size] = 0;

            u64 BlogHtmlOffset = TempString->Offset;

            s32 BlogFileDataOffset = 0;
            while (HandleBlogLine(TempString, &File, &BlogFileDataOffset));
            WriteLinearAllocator(TempString, (u8 *)"\0", 1);

            buffer BlogOutputPath = GetOutputHtmlPath(TempString, BlogDirectory, SiteBlogDirectory, CurrentFile->Name, 1, 1);

            u8 *BlogHtmlData = TempString->Data + BlogHtmlOffset;
            SetPreprocessVariable(Preprocessor, (u8 *)"BlogFile", BlogHtmlData);

            b32 Error = PreprocessBuffer(Preprocessor, TempString, &BlogPageTemplate, BlogOutputPath.Data);

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
        linear_allocator *OutputAllocator = &Preprocessor->OutputAllocator;
        u64 OutputOffset = Preprocessor->OutputAllocator.Offset;

        for (file_list *CurrentFile = SortedFileList; CurrentFile; CurrentFile = CurrentFile->Next)
        {
            PushString(OutputAllocator, (u8 *)"<li><a href=\"");
            PushString(OutputAllocator, (u8 *)"#");
            PushString(OutputAllocator, (u8 *)"\">");
            PushString(OutputAllocator, CurrentFile->Name);
            PushString(OutputAllocator, (u8 *)"</a></li>\n");
        }

        u8 *BlogListingData = OutputAllocator->Data + OutputOffset;
        u64 Size = OutputAllocator->Offset - BlogListingOffset;
        WriteFile(BlogListingFilePath, BlogListingData, Size);
        OutputAllocator->Offset = OutputOffset;
    }

    FreeLinearAllocator(FileAllocator);
}

void GenerateSite(linear_allocator *TempString)
{
    u8 *GenDirectory =       (u8 *)"../gen";
    u8 *CodePagesDirectory = (u8 *)"../gen/code_pages";

    u8 *SiteDirectory =     (u8 *)"../site";
    u8 *SiteBlogDirectory = (u8 *)"../site/blog";

    u8 *IndexIn =  (u8 *)"../src/layout/index.html";
    u8 *IndexOut = (u8 *)"../site/index.html";

    u8 *CodeIn =  (u8 *)"../src/layout/code.html";
    u8 *CodeOut = (u8 *)"../site/code.html";

    u8 *BlogIn =  (u8 *)"../src/layout/blog.html";
    u8 *BlogOut = (u8 *)"../site/blog/index.html";

    u8 *LSystemIn =  (u8 *)"../src/layout/l_system.html";
    u8 *LSystemOut = (u8 *)"../gen/l_system.html";

    u8 *Bra = (u8 *)"{|";
    u8 *Ket = (u8 *)"|}";

    GenerateCodePages(TempString);

    EnsureDirectoryExists(GenDirectory);
    EnsureDirectoryExists(CodePagesDirectory);
    EnsureDirectoryExists(SiteDirectory);
    EnsureDirectoryExists(SiteBlogDirectory);

    pre_processor PreProcessor = CreatePreProcessor(Bra, Ket);
    AddPreProcessorCommand(&PreProcessor, pre_processor_command_Include, (u8 *)"include");
    AddPreProcessorCommand(&PreProcessor, pre_processor_command_Docgen, (u8 *)"docgen");

    linear_allocator FileAllocator = WalkDirectory(CodePagesDirectory);
    file_list *FileList = (file_list *)FileAllocator.Data;
    file_list *SortedFileList = SortFileList(FileList);

    for (file_list *CurrentFile = SortedFileList; CurrentFile; CurrentFile = CurrentFile->Next)
    {
        buffer Buffer = GetOutputHtmlPath(TempString, CodePagesDirectory, SiteDirectory, CurrentFile->Name, 0, 0);
        EnsurePathDirectoriesExist(Buffer.Data);
        PreprocessFile(&PreProcessor, *TempString, CurrentFile->Name, Buffer.Data);
    }

    TempString->Offset = 0;

    GenerateBlogPages(TempString, &PreProcessor, SiteBlogDirectory);

    PreprocessFile(&PreProcessor, *TempString, IndexIn, IndexOut);
    PreprocessFile(&PreProcessor, *TempString, CodeIn, CodeOut);
    PreprocessFile(&PreProcessor, *TempString, BlogIn, BlogOut);
    PreprocessFile(&PreProcessor, *TempString, LSystemIn, LSystemOut);


    FreeLinearAllocator(FileAllocator);
}
