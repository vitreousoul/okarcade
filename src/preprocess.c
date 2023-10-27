/*
  Things the preprocessor can do:
  - include text
  - documentation generation
*/

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

typedef struct
{
    u8 *Bra;
    s32 BraCount;
    u8 *Ket;
    s32 KetCount;
    pre_processor_command Commands[PRE_PROCESSOR_COMMAND_MAX];
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

b32 PreprocessFile(pre_processor *PreProcessor, linear_allocator TempString, u8 *FilePath, u8 *OutputFilePath);
void TestPreprocessor(void);

internal pre_processor CreatePreProcessor(u8 *Bra, u8 *Ket)
{
    pre_processor PreProcessor;

    PreProcessor.Bra = Bra;
    PreProcessor.BraCount = GetStringLength(Bra);

    PreProcessor.Ket = Ket;
    PreProcessor.KetCount = GetStringLength(Ket);

    PreProcessor.CommandCount = 0;

    { /* allocator setup */
        u64 StringAllocatorVirtualSize = Gigabytes(1);
        u64 OutputBufferVirtualSize = Gigabytes(1);
        u64 TotalVirtualSize = StringAllocatorVirtualSize + OutputBufferVirtualSize;

        PreProcessor.StringAllocator = CreateLinearAllocator(TotalVirtualSize);
        PreProcessor.StringAllocator.Capacity = StringAllocatorVirtualSize;

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
            TokenBuffer.Data = PushLinearAllocator(&PreProcessor->StringAllocator, TokenBuffer.Size + 1);
            TokenBuffer.Data[TokenBuffer.Size] = 0;
            CopyMemory(Buffer->Data + Offset, TokenBuffer.Data, TokenBuffer.Size);
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
        CommandWasHandled = 1;
        Index += GetStringLength(IncludeName);
        SkipSpace(Buffer, &Index);

        buffer Token = GetCommandToken(PreProcessor, Buffer, Index);
        buffer *Buffer = ReadFileIntoBuffer(Token.Data);

        if (Buffer)
        {
            u8 *WhereToWrite = PushLinearAllocator(OutputAllocator, Buffer->Size);
            if (WhereToWrite)
            {
                CopyMemory(Buffer->Data, WhereToWrite, Buffer->Size);
                FreeBuffer(Buffer);
            }
            else
            {
                LogError("failed to write to output\n");
            }
        }
        else
        {
            LogError("include file not found");
        }
    }

    return CommandWasHandled;
}

b32 PreprocessFile(pre_processor *PreProcessor, linear_allocator TempString, u8 *FilePath, u8 *OutputFilePath)
{
    linear_allocator *OutputAllocator = &PreProcessor->OutputAllocator;
    buffer *Buffer = ReadFileIntoBuffer(FilePath);

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
            u8 *HereDocLabel = TempString.Data + TempString.Offset;
            PushLinearAllocator(&TempString, HereDocLabelSize);
            CopyMemory(Buffer->Data + Begin, HereDocLabel, HereDocLabelSize);
            TempString.Data[TempString.Offset - 1] = 0;

            SkipSpace(Buffer, &I);

            for (s32 J = I; J < Buffer->Size; J++)
            {
                if (CheckIfStringIsPrefix(HereDocLabel, Buffer, J))
                {
                    s32 PreDataSize = HereDocCharOffset - PreProcessor->BraCount - WriteIndex;
                    u8 *DataPrecedingHereDoc = PushLinearAllocator(OutputAllocator, PreDataSize);

                    if (DataPrecedingHereDoc)
                    {
                        u8 *BufferStart = Buffer->Data + WriteIndex;
                        CopyMemory(BufferStart, DataPrecedingHereDoc, PreDataSize);
                    }
                    else
                    {
                        LogError("failed to write heredoc pre-data to output");
                    }

                    s32 HereDocDataBegin = Begin + HereDocLabelSize - 1;
                    s32 HereDocDataSize = J - HereDocDataBegin;
                    u8 *HereDocData = PushLinearAllocator(OutputAllocator, HereDocDataSize);

                    if (HereDocData)
                    {
                        u8 *BufferStart = Buffer->Data + HereDocDataBegin;
                        CopyMemory(BufferStart, HereDocData, HereDocDataSize);
                    }
                    else
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

                    u8 *WhereToWrite = PushLinearAllocator(OutputAllocator, Size);

                    if (WhereToWrite)
                    {
                        CopyMemory(BufferStart, WhereToWrite, Size);
                    }
                    else
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
        u8 *OutputAllocatorStart = OutputAllocator->Data + OutputAllocator->Offset;
        s32 Size = Buffer->Size - WriteIndex;
        u8 *WhereToWrite = PushLinearAllocator(OutputAllocator, Size);

        if (WhereToWrite)
        {
            CopyMemory(Buffer->Data + WriteIndex, OutputAllocatorStart, Size);
        }
        else
        {
            LogError("failed to write to output");
        }
    }

    printf("Writing pre-processed file %s\n", OutputFilePath);
    WriteFile(OutputFilePath, OutputAllocator->Data, OutputAllocator->Offset);
    OutputAllocator->Offset = 0;

    return Error;
}

void TestPreprocessor(void)
{
    u8 *GenDirectory = (u8 *)"../gen";
    u8 *CodePagesDirectory = (u8 *)"../gen/code_pages";
    u8 *SiteDirectory = (u8 *)"../site";

    u8 *IndexIn = (u8 *)"../src/layout/index.html";
    u8 *IndexOut = (u8 *)"../site/index.html";

    u8 *CodeIn = (u8 *)"../src/layout/code.html";
    u8 *CodeOut = (u8 *)"../site/code.html";

    u8 *LSystemIn = (u8 *)"../src/layout/l_system.html";
    u8 *LSystemOut = (u8 *)"../gen/l_system.html";

    u8 *Bra = (u8 *)"{|";
    u8 *Ket = (u8 *)"|}";

    linear_allocator TempString = CreateLinearAllocator(Gigabytes(1));

    GenerateCodePages(&TempString);

    EnsureDirectoryExists(GenDirectory);
    EnsureDirectoryExists(CodePagesDirectory);
    EnsureDirectoryExists(SiteDirectory);

    pre_processor PreProcessor = CreatePreProcessor(Bra, Ket);
    AddPreProcessorCommand(&PreProcessor, pre_processor_command_Include, (u8 *)"include");
    AddPreProcessorCommand(&PreProcessor, pre_processor_command_Docgen, (u8 *)"docgen");

    linear_allocator FileAllocator = WalkDirectory(CodePagesDirectory);
    file_list *FileList = (file_list *)FileAllocator.Data;
    file_list *SortedFileList = SortFileList(FileList);

    for (file_list *CurrentFile = SortedFileList; CurrentFile; CurrentFile = CurrentFile->Next)
    {
        buffer Buffer = GetOutputHtmlPath(&TempString, CodePagesDirectory, SiteDirectory, CurrentFile->Name, 0);
        EnsurePathDirectoriesExist(Buffer.Data);
        PreprocessFile(&PreProcessor, TempString, CurrentFile->Name, Buffer.Data);
    }

    TempString.Offset = 0;

    PreprocessFile(&PreProcessor, TempString, IndexIn, IndexOut);
    PreprocessFile(&PreProcessor, TempString, CodeIn, CodeOut);
    PreprocessFile(&PreProcessor, TempString, LSystemIn, LSystemOut);

    FreeLinearAllocator(FileAllocator);
}
