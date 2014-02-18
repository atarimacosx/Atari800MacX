
int AtrSpartaMount(AtrDiskInfo *info);
void AtrSpartaUnmount(AtrDiskInfo *info);
int AtrSpartaGetDir(AtrDiskInfo *info, UWORD *fileCount, ADosFileEntry *files, ULONG *freeBYtes);
int AtrSpartaChangeDir(AtrDiskInfo *info, int cdFlag, char *name);
int AtrSpartaDeleteDir(AtrDiskInfo *info, char *name);
int AtrSpartaMakeDir(AtrDiskInfo *info, char *name);
int AtrSpartaLockFile(AtrDiskInfo *info, char *name, int lock);
int AtrSpartaRenameFile(AtrDiskInfo *info, char *name, char *newname);
int AtrSpartaDeleteFile(AtrDiskInfo *info, char *name);
int AtrSpartaImportFile(AtrDiskInfo *info, char *filename, int lfConvert);
int AtrSpartaExportFile(AtrDiskInfo *info, char *nameToExport, char* outFile, int lfConvert);


