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

b32 PreprocessFile(pre_processor *PreProcessor, u8 *FilePath, u8 *OutputFilePath);
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

internal void HandlePreProcessCommand(pre_processor *PreProcessor, buffer *Buffer, s32 Offset, s32 CommandSize)
{
    s32 Index = Offset;
    s32 MaxIndex = Offset + CommandSize;
    u8 *IncludeName = (u8 *)"include";
    linear_allocator *OutputAllocator = &PreProcessor->OutputAllocator;

    SkipSpace(Buffer, &Index);

    if (Index < MaxIndex && CheckIfStringIsPrefix(IncludeName, Buffer, Index))
    {
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
    else
    {
        LogError("Unknown command");
    }
}

b32 PreprocessFile(pre_processor *PreProcessor, u8 *FilePath, u8 *OutputFilePath)
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

        if (FoundBra)
        {
            s32 CommandStart = I + PreProcessor->BraCount;

            for (s32 J = CommandStart; J < Buffer->Size; J++)
            {
                b32 FoundKet = CheckIfStringIsPrefix(PreProcessor->Ket, Buffer, J);

                if (FoundKet)
                {
                    u8 *BufferStart = Buffer->Data + WriteIndex;
                    u8 *OutputAllocatorStart = OutputAllocator->Data + OutputAllocator->Offset;
                    s32 JustPastKet = J + PreProcessor->KetCount;
                    s32 Size = I - WriteIndex;

                    u8 *WhereToWrite = PushLinearAllocator(OutputAllocator, Size);

                    if (WhereToWrite)
                    {
                        CopyMemory(BufferStart, OutputAllocatorStart, Size);
                    }
                    else
                    {
                        LogError("failed to write to output");
                        break;
                    }

                    WriteIndex = JustPastKet;
                    I = JustPastKet;

                    HandlePreProcessCommand(PreProcessor, Buffer, CommandStart, J);
                    break;
                }
            }
        }
    }

    if (WriteIndex < Buffer->Size)
    {
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
    u8 *SiteDirectory = (u8 *)"../site";

    u8 *IndexIn = (u8 *)"../src/layout/index.html";
    u8 *IndexOut = (u8 *)"../site/index.html";

    u8 *CodeIn = (u8 *)"../src/layout/code.html";
    u8 *CodeOut = (u8 *)"../site/code.html";

    u8 *LSystemIn = (u8 *)"../src/layout/l_system.html";
    u8 *LSystemOut = (u8 *)"../gen/l_system.html";

    u8 *Bra = (u8 *)"{|";
    u8 *Ket = (u8 *)"|}";

    pre_processor PreProcessor = CreatePreProcessor(Bra, Ket);
    AddPreProcessorCommand(&PreProcessor, pre_processor_command_Include, (u8 *)"include");
    AddPreProcessorCommand(&PreProcessor, pre_processor_command_Docgen, (u8 *)"docgen");

    EnsureDirectoryExists(GenDirectory);
    EnsureDirectoryExists(SiteDirectory);

    PreprocessFile(&PreProcessor, IndexIn, IndexOut);
    PreprocessFile(&PreProcessor, CodeIn, CodeOut);
    PreprocessFile(&PreProcessor, LSystemIn, LSystemOut);
}
