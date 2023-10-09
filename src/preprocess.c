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


void PreprocessFile(char *FilePath, char *OutputFilePath);
void TestPreprocessor(void);

void PreprocessFile(char *FilePath, char *OutputFilePath)
{
    buffer *Buffer = ReadFileIntoBuffer(FilePath);

    if (!Buffer)
    {
        printf("Error in PreprocessFile: null buffer\n");
        return;
    }
}

void TestPreprocessor(void)
{
    printf("testing preprocessor.....\n");
}
