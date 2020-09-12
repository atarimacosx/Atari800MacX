
int AtrDos2Mount(AtrDiskInfo *info, int useFileNumbers);
void AtrDos2Unmount(AtrDiskInfo *info);
int AtrDos2GetDir(AtrDiskInfo *info, UWORD *fileCount, ADosFileEntry *files, ULONG *freeBytes);
int AtrDos2LockFile(AtrDiskInfo *info, char *name, int lock);
int AtrDos2RenameFile(AtrDiskInfo *info, char *name, char *newname);
int AtrDos2DeleteFile(AtrDiskInfo *info, char *name);
int AtrDos2ImportFile(AtrDiskInfo *info, char *filename, int lfConvert, int tabConvert);
int AtrDos2ExportFile(AtrDiskInfo *info, char *nameToExport, char* outFile, int lfConvert, int tabConvert);


