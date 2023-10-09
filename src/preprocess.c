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
    u8 *Bra;
    u8 *Ket;
} pre_processor;


b32 PreprocessFile(u8 *FilePath, u8 *OutputFilePath);
void TestPreprocessor(void);


b32 PreprocessFile(u8 *FilePath, u8 *OutputFilePath)
{
    buffer *Buffer = ReadFileIntoBuffer(FilePath);
    b32 Error = 0;

    if (!Buffer)
    {
        printf("Error in PreprocessFile: null buffer\n");
        Error = 1;
    }

    /* TODO process data */
    WriteFile(OutputFilePath, Buffer);

    return Error;
}

void TestPreprocessor(void)
{
    EnsureDirectoryExists((u8 *)"../gen");
    PreprocessFile((u8 *)"../src/l_system.html", (u8 *)"../gen/l_system.html");
}
