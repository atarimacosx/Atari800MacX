
int AtrXEMount(AtrDiskInfo *info);
void AtrXEUnmount(AtrDiskInfo *info);
int AtrXEGetDir(AtrDiskInfo *info, UWORD *fileCount, ADosFileEntry *files, ULONG *freeBytes);
int AtrXEChangeDir(AtrDiskInfo *info, int cdFlag, char *name);
int AtrXEDeleteDir(AtrDiskInfo *info, char *name);
int AtrXEMakeDir(AtrDiskInfo *info, char *name);
int AtrXELockFile(AtrDiskInfo *info, char *name, int lock);
int AtrXERenameFile(AtrDiskInfo *info, char *name, char *newname);
int AtrXEDeleteFile(AtrDiskInfo *info, char *name);
int AtrXEImportFile(AtrDiskInfo *info, char *filename, int lfConvert, int tabConvert);
int AtrXEExportFile(AtrDiskInfo *info, char *nameToExport, char* outFile, int lfConvert, int tabConvert);


