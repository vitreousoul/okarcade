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
