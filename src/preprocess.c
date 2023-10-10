/*
  Things the preprocessor can do:
  - include text
  - documentation generation
*/

typedef enum
{
    pre_processor_command_Null,
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
} pre_processor;


b32 PreprocessFile(pre_processor PreProcessor, u8 *FilePath, u8 *OutputFilePath);
void TestPreprocessor(void);

internal pre_processor CreatePreProcessor(u8 *Bra, u8 *Ket)
{
    pre_processor PreProcessor;

    PreProcessor.Bra = Bra;
    PreProcessor.BraCount = GetStringLength(Bra);

    PreProcessor.Ket = Ket;
    PreProcessor.KetCount = GetStringLength(Ket);

    PreProcessor.CommandCount = 0;

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

b32 PreprocessFile(pre_processor PreProcessor, u8 *FilePath, u8 *OutputFilePath)
{
    buffer *Buffer = ReadFileIntoBuffer(FilePath);
    b32 Error = 0;

    if (!Buffer)
    {
        printf("Error in PreprocessFile: null buffer\n");
        Error = 1;
    }

    for (s32 I = 0; I < Buffer->Size; I++)
    {
        b32 FoundBra = CheckIfStringIsPrefix(PreProcessor.Bra, Buffer, I);

        if (FoundBra)
        {
            s32 CommandStart = I + PreProcessor.BraCount;
            for (s32 J = CommandStart; J < Buffer->Size; J++)
            {
                b32 FoundKet = CheckIfStringIsPrefix(PreProcessor.Ket, Buffer, J);

                if (FoundKet)
                {
                    for (s32 K = CommandStart; K < J; K++)
                    {
                        printf("%c", Buffer->Data[K]);
                    }
                    printf("\n");
                    break;
                }
            }
        }
    }

    WriteFile(OutputFilePath, Buffer);

    return Error;
}

void TestPreprocessor(void)
{
    u8 *GenDirectory = (u8 *)"../gen";
    u8 *LSystemInputHtml = (u8 *)"../src/l_system.html";
    u8 *LSystemOutputHtml = (u8 *)"../gen/l_system.html";

    u8 *Bra = (u8 *)"{|";
    u8 *Ket = (u8 *)"|}";

    pre_processor PreProcessor = CreatePreProcessor(Bra, Ket);
    AddPreProcessorCommand(&PreProcessor, pre_processor_command_Include, (u8 *)"include");
    AddPreProcessorCommand(&PreProcessor, pre_processor_command_Docgen, (u8 *)"docgen");

    EnsureDirectoryExists(GenDirectory);
    PreprocessFile(PreProcessor, LSystemInputHtml, LSystemOutputHtml);
}
