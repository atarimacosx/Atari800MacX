
int AtrDos3Mount(AtrDiskInfo *info);
void AtrDos3Unmount(AtrDiskInfo *info);
int AtrDos3GetDir(AtrDiskInfo *info, UWORD *fileCount, ADosFileEntry *files, ULONG *freeBytes);
int AtrDos3LockFile(AtrDiskInfo *info, char *name, int lock);
int AtrDos3RenameFile(AtrDiskInfo *info, char *name, char *newname);
int AtrDos3DeleteFile(AtrDiskInfo *info, char *name);
int AtrDos3ImportFile(AtrDiskInfo *info, char *filename, int lfConvert);
int AtrDos3ExportFile(AtrDiskInfo *info, char *nameToExport, char* outFile, int lfConvert);


