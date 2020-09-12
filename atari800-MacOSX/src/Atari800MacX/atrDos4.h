
int AtrDos4Mount(AtrDiskInfo *info);
void AtrDos4Unmount(AtrDiskInfo *info);
int AtrDos4GetDir(AtrDiskInfo *info, UWORD *fileCount, ADosFileEntry *files, ULONG *freeBytes);
int AtrDos4LockFile(AtrDiskInfo *info, char *name, int lock);
int AtrDos4RenameFile(AtrDiskInfo *info, char *name, char *newname);
int AtrDos4DeleteFile(AtrDiskInfo *info, char *name);
int AtrDos4ImportFile(AtrDiskInfo *info, char *filename, int lfConvert, int tabConvert);
int AtrDos4ExportFile(AtrDiskInfo *info, char *nameToExport, char* outFile, int lfConvert, int tabConvert);


