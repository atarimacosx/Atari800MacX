#include "stdio.h"

typedef struct atrDiskInfo {
	FILE *atr_file;
	int atr_sectorcount;
	int atr_sectorsize;
	int atr_boot_sectors_type;
	int atr_dostype;
	void *atr_dosinfo;
	} AtrDiskInfo;


int AtrMount(const char *filename, int *dosType, int *readWrite, int *writeProtect, AtrDiskInfo **info);
void AtrUnmount(AtrDiskInfo *info);
int AtrSetWriteProtect(AtrDiskInfo *info, int writeProtect);
int AtrSectorSize(AtrDiskInfo *info);
int AtrSectorNumberSize(AtrDiskInfo *info, int sector);
int AtrSectorCount(AtrDiskInfo *info);
int AtrReadSector(AtrDiskInfo *info, int sector, UBYTE * buffer);
int AtrWriteSector(AtrDiskInfo *info, int sector, UBYTE * buffer);

int AtrGetDir(AtrDiskInfo *info, UWORD *fileCount, ADosFileEntry *files, ULONG *freeBytes);
int AtrChangeDir(AtrDiskInfo *info, int cdFlag, char *name);
int AtrDeleteDir(AtrDiskInfo *info, char *name);
int AtrMakeDir(AtrDiskInfo *info, char *name);
int AtrLockFile(AtrDiskInfo *info, char *name, int lock);
int AtrRenameFile(AtrDiskInfo *info, char *name, char *newname);
int AtrDeleteFile(AtrDiskInfo *info, char *name);
int AtrImportFile(AtrDiskInfo *info, char *filename, int lfConvert, int tabConvert);
int AtrExportFile(AtrDiskInfo *info, char *nameToExport, char* outFile, int lfConvert, int tabConvert);


