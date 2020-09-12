
int AtrMyDosMount(AtrDiskInfo *info);
void AtrMyDosUnmount(AtrDiskInfo *info);
int AtrMyDosGetDir(AtrDiskInfo *info, UWORD *fileCount, ADosFileEntry *files, ULONG *freeBytes);
int AtrMyDosChangeDir(AtrDiskInfo *info, int cdFlag, char *name);
int AtrMyDosDeleteDir(AtrDiskInfo *info, char *name);
int AtrMyDosMakeDir(AtrDiskInfo *info, char *name);
int AtrMyDosLockFile(AtrDiskInfo *info, char *name, int lock);
int AtrMyDosRenameFile(AtrDiskInfo *info, char *name, char *newname);
int AtrMyDosDeleteFile(AtrDiskInfo *info, char *name);
int AtrMyDosImportFile(AtrDiskInfo *info, char *filename, int lfConvert, int tabConvert);
int AtrMyDosExportFile(AtrDiskInfo *info, char *nameToExport, char* outFile, int lfConvert, int tabConvert);


