/* Copyright 1997,1998 Ken Siders */

#ifndef ATR_H_INCLUDED

#define ATR_H_INCLUDED

/* constants */
#define ATR_HEADER_SIZE 16
/* Image type */
#define ATR_IMAGE 0
#define DCM_IMAGE 1
#define XFD_IMAGE 2

/* dos compatability types */
#define DOS_UNKNOWN 0
#define DOS_ATARI 1
#define DOS_MYDOS 2
#define DOS_SPARTA1 3
#define DOS_SPARTA2 6
#define DOS_LJK 4
#define DOS_KBOOT 5

#define MAX_ATARI_FILES 64

/* disk format types */
#define FORMAT_SD 1
#define FORMAT_ED 2
#define FORMAT_DD 3
#define FORMAT_SD_CUSTOM 4
#define FORMAT_DD_CUSTOM 5
#define FORMAT_DEFAULT 6

/* open modes */
#define ATARI_OPEN_READ 4
#define ATARI_OPEN_WRITE 8
#define ATARI_OPEN_DIR 6

/* insert/extract mode */
#define BINARY_MODE 0
#define TEXT_MODE 1

/* these will be in variables later */
#define FIRST_ATARIDOS_DIR_SECTOR 361;
#define lastDirSector 368
#define vtocSector 360
#define vtocSector2 0
#define dirEntrySize 16
#define dirEntriesPerSector 8

#define DELETED_FLAG    0x80
#define INUSE_FLAG      0x40
#define LOCKED_FLAG     0x20
#define OPENOUTPUT_FLAG 0x01
#define INUSE25_FLAG    0x03
#define DOS2_FLAG       0x02
#define SUBDIR_FLAG     0x10

#define SPARTA_DELETED_FLAG 0x10
#define SPARTA_INUSE_FLAG 0x08
#define SPARTA_LOCKED_FLAG 0x01
#define SPARTA_SUBDIR_FLAG 0x20

@interface AtrFile
    {
@private
    unsigned short firstFreeSector;
    unsigned short vtocCount;
    unsigned char vtocMap[128*34];
    unsigned short freeSectors;
    unsigned short totalSectors;
    FILE *atrIn;
    int verbose;
    int lastAtariError;
    unsigned long  imageSize;
    unsigned short secSize;
    unsigned long crc;
    unsigned short sectorCount;
    unsigned char flags;
    unsigned char writeProtect;
    unsigned char authenticated;
    unsigned short currentSector;
    unsigned char dosType;
    unsigned char imageType;
    unsigned char sectorBuffer[512];
    }
    -int maxNoDirectoryEntries;
    -unsigned short firstDirSector;
    -void AdjustFreeSectors:(long) amt;
    -int Format:int setDosType:(int) flag;
    -void ResetFirstFreeSector;
    -unsigned short GetFreeSector;
    -int ReadVtoc;
    -int WriteVtoc;
    -int MarkSectorUsed:(unsigned short) sector: (int) used;
    -unsigned short GetVtocSectorCount;
    -unsigned short GetUsedSectorCount;
    -int IsSectorUsed:(unsigned short) sector;
    -unsigned short GetFreeSectors;
    -int GetDosType;
    -int Open:char *file;
    -int Create:char * file:(unsigned short) sectorSize:(unsigned short) sectorCount;
    -int Close;
    -int ReadSector:(unsigned short) sector: (unsigned char) *buffer;
    -int WriteSector:(unsigned short) sector: (unsigned char) *buffer;
    -int GetInfo: (unsigned short *)sectorSize:(unsigned short *)sectorCount:(unsigned char *)protect;
    -int ExtractExe: (CString *)fname;
    -int GetImageType;
    -int SetVerbose:(int) verb;
    -int ReadDirectory:(int) entry:(char *) name: (char *)ext:(int) *status:(unsigned short *)sectors:
                       (unsigned short *)startSector;
@end
#if 0
@interface AtariFile
   {
@private
        AtrFile *atr;
        unsigned short sectorSize;
        unsigned short numberOfSectors;  /* no. of sectors in ATR */
        unsigned long fileSize ; /* not used yet */
        unsigned char openFlag;
        unsigned char  eofFlag;
        short currentOffset;
        short bytesData;
        short sectorLinkOffset;
        unsigned char sectorBuffer[256];
        int attrib;        /* internal */
        int dirSector;     /* internal */
        int dirEntry;      /* internal */
        char pattern[32];     /* internal */
        char *atrFileName; /* internal */
        unsigned char protFlag;   // atr write protected?
        unsigned short currentSector;
        int PatternMatch( char *pattern, char *fileName,
            char *extender);
@public
    unsigned short startSector;
    unsigned short sectorCount;
    unsigned char locked;
    int dosType;
    short fileNo;
    char fileName[13];
    int flag;    /* from dos */
    }
    int Rename( CAtrFile &atrFile, char *oldName, char *newName);
    long FileSize( CAtrFile &atr, char *fileName );
    int Insert( CAtrFile &atrFile, char *fileName, char *atariFileName,
                int mode);
    long Write( char *buffer, long bytes );
    unsigned short Delete( CAtrFile &atrFile, char *fileName);
    int FindFirst( CAtrFile &atr, unsigned int attrib , char *pattern);
    int FindNext(void );
    int Open( CAtrFile &atr, char *fileName, unsigned char mode);
    long Read( char *buffer, long bytes);
    int Close( void );
    int Eof( void )
    int Extract( CAtrFile &atrFile, char *fileName,
                 char *dosPath );
@end
#endif
#endif


