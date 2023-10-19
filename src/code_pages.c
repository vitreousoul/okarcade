void TestCodePages(void);
void GenerateCodePages(void);

void TestCodePages(void)
{
    file_list FileList = WalkDirectory((u8 *)"../src");

    for (file_list_item *CurrentItem = FileList.First; CurrentItem; CurrentItem = CurrentItem->Next)
    {
        printf("file: %s\n", CurrentItem->Name);
    }

    FreeFileList(&FileList);
}

void GenerateCodePages(void)
{

}
