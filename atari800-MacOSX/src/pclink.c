//  Altirra - Atari 800/800XL/5200 emulator
//  Copyright (C) 2009-2011 Avery Lee
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "atari.h"
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <time.h>
#include <unistd.h>
#include "vec.h"
//#include "pclink.h"

UBYTE ATTranslateWin32ErrorToSIOError(ULONG err);

///////////////////////////////////////////////////////////////////////////

typedef struct diskInfo {
    UBYTE   InfoVersion;
    UBYTE   RootDirLo;
    UBYTE   RootDirHi;
    UBYTE   SectorCountLo;
    UBYTE   SectorCountHi;
    UBYTE   SectorsFreeLo;
    UBYTE   SectorsFreeHi;
    UBYTE   VTOCSectorCount;
    UBYTE   VTOCSectorStartLo;
    UBYTE   VTOCSectorStartHi;
    UBYTE   NextFileSectorLo;
    UBYTE   NextFileSectorHi;
    UBYTE   NextDirSectorLo;
    UBYTE   NextDirSectorHi;
    UBYTE   VolumeLabel[8];
    UBYTE   TrackCount;
    UBYTE   BytesPerSectorCode;
    UBYTE   Version;
    UBYTE   BytesPerSectorLo;
    UBYTE   BytesPerSectorHi;
    UBYTE   MapSectorCountLo;
    UBYTE   MapSectorCountHi;
    UBYTE   SectorsPerCluster;
    UBYTE   NoSeq;
    UBYTE   NoRnd;
    UBYTE   BootLo;
    UBYTE   BootHi;
    UBYTE   WriteProtectFlag;
    UBYTE   Pad[29];
} DiskInfo;

typedef struct dirEntry {
    enum : UBYTE {
        Flag_OpenForWrite  = 0x80,
        Flag_Directory     = 0x20,
        Flag_Deleted       = 0x10,
        Flag_InUse         = 0x08,
        Flag_Archive       = 0x04,
        Flag_Hidden        = 0x02,
        Flag_Locked        = 0x01
    };

    enum : UBYTE {
        AttrMask_NoSubDir      = 0x80,
        AttrMask_NoArchived    = 0x40,
        AttrMask_NoHidden      = 0x20,
        AttrMask_NoLocked      = 0x10,
        AttrMask_OnlySubDir    = 0x08,
        AttrMask_OnlyArchived  = 0x04,
        AttrMask_OnlyHidden    = 0x02,
        AttrMask_OnlyLocked    = 0x01,
    };

    UBYTE   Flags;
    UBYTE   SectorMapLo;
    UBYTE   SectorMapHi;
    UBYTE   LengthLo;
    UBYTE   LengthMid;
    UBYTE   LengthHi;
    UBYTE   Name[11];
    UBYTE   Day;
    UBYTE   Month;
    UBYTE   Year;
    UBYTE   Hour;
    UBYTE   Min;
    UBYTE   Sec;
} DirEntry;

typedef enum {
    FILE_READ,
    FILE_WRITE,
    FILE_APPEND,
    FILE_UPDATE
} FileOpenType;

DirEntry *Dir_Entry_Alloc() 
{
    DirEntry *entry = (DirEntry *) malloc(sizeof(DirEntry));
    memset(entry, 0, sizeof(DirEntry));
    return entry;
}

void Dir_Entry_Set_Flags_From_Attributes(DirEntry *entry, mode_t attr) {
    entry->Flags = Flag_InUse;

    if (!(attr & S_IWRITE))
        entry->Flags |= Flag_Locked;

    if (S_ISDIR(attr))
        entry->Flags |= Flag_Directory;
}

int Dir_Entry_Test_Attr_Filter(DirEntry *entry, UBYTE attrFilter) {
    if (!attrFilter)
        return TRUE;

    if (attrFilter & AttrMask_NoArchived) {
        if (entry->Flags & Flag_Archive)
            return FALSE;
    }

    if (attrFilter & AttrMask_NoHidden) {
        if (entry->Flags & Flag_Hidden)
            return FALSE;
    }

    if (attrFilter & AttrMask_NoLocked) {
        if (entry->Flags & Flag_Locked)
            return FALSE;
    }

    if (attrFilter & AttrMask_NoSubDir) {
        if (entry->Flags & Flag_Directory)
            return FALSE;
    }

    if (attrFilter & AttrMask_OnlyArchived) {
        if (!(entry->Flags & Flag_Archive))
            return FALSE;
    }

    if (attrFilter & AttrMask_OnlyHidden) {
        if (!(entry->Flags & Flag_Hidden))
            return FALSE;
    }

    if (attrFilter & AttrMask_OnlyLocked) {
        if (!(entry->Flags & Flag_Locked))
            return FALSE;
    }

    if (attrFilter & AttrMask_OnlySubDir) {
        if (!(entry->Flags & Flag_Directory))
            return FALSE;
    }

    return TRUE;
}

int Dir_Entry_Compare(const void *vx, const void *vy) {
    DirEntry * x = (DirEntry *) vx;
    DirEntry * y = (DirEntry *) vy;
    
    if ((x->Flags ^ y->Flags) & Flag_Directory) {
        if ((x->Flags & Flag_Directory) == 0)
            return -1;
        else
            return 1;
    }

    return memcmp(x->Name, y->Name, 11);
}

void Dir_Entry_Set_Date(DirEntry *entry, time_t date) {
    struct tm *localTime;

    localTime = localtime(&date);

    entry->Day = localTime->tm_mday;
    entry->Month = localTime->tm_mon;
    entry->Year = localTime->tm_year > 99 ? localTime->tm_year - 100 : localTime->tm_year;
    entry->Hour = localTime->tm_hour;
    entry->Min = localTime->tm_min;
    entry->Sec = localTime->tm_sec;
}

void Dir_Entry_Decode_Date(const UBYTE tsdata[6], struct tm* fileExpTime) {
    fileExpTime->tm_mday = tsdata[0];
    fileExpTime->tm_mon = tsdata[1];
    fileExpTime->tm_year = tsdata[2] < 50 ? 2000 + tsdata[2] : 1900 + tsdata[2];
    fileExpTime->tm_hour = tsdata[3];
    fileExpTime->tm_min = tsdata[4];
    fileExpTime->tm_sec = tsdata[5];
 }

typedef struct fileName {
    UBYTE   Name[11];
} FileName;

FileName *File_Name_Alloc()
{
    FileName *name = (FileName *) malloc(sizeof(FileName));
    memset(name, 0, sizeof(FileName));
    return name;
}

void File_Name_Free(FileName *name)
{
    free(name);
}

int File_Name_Is_Equal(FileName *name, FileName *second)
{
    return (memcmp(name->Name, second->Name, 11) == 0);
}

int File_Name_Parse_From_Net(FileName *name, const UBYTE *fn) {
    UBYTE fill = 0;

    for(int i=0; i<11; ++i) {
        if (i == 8)
            fill = 0;

        if (fill)
            name->Name[i] = fill;
        else {
            UBYTE c = fn[i];

            if (c == '*') {
                c = '?';
                fill = c;
            }

            if (c == ' ')
                fill = c;
            else if (c >= 'a' && c <= 'z')
                c &= ~0x20;
            else if ((c < 'A' || c > 'Z') && (c < '0' || c > '9') && c != '_' && c != '?')
                return FALSE;

            name->Name[i] = c;
        }
    }

    return TRUE;
}

int File_Name_Parse_From_Native(FileName *name, const char *fn) {
    int i = 0;
    int inext = FALSE;
    int isescaped = FALSE;

    if (*fn == '!' || *fn == '$') {
        ++fn;
        isescaped = TRUE;
    }
    char c = *fn++;
    while(c) {
        if (c == '.') {
            if (i > 8)
                return FALSE;

            while(i < 8)
                name->Name[i++] = ' ';

            inext = TRUE;
            continue;
        }

        if (inext) {
            if (i >= 11)
                return FALSE;
        } else {
            if (i >= 8)
                return FALSE;
        }

        if (c >= L'a' && c <= L'z')
            name->Name[i++] = (UBYTE)(c - 0x20);
        else if ((c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9') || c == L'_')
            name->Name[i++] = (UBYTE)c;
        else
            return FALSE;
        c = *fn++;
    }

    while(i < 11)
        name->Name[i++] = L' ';

    return TRUE;
}

void File_Name_Append_Native(FileName *name, char *s) 
{
    for(int i=0; i<11; ++i) {
        const char c = (char)name->Name[i];

        if (c == ' ') {
            if (i == 8)
                break;
            continue;
        }

        if (i == 8)
            s += '.';

        s += c;
    }
}

int File_Name_Is_Wild(FileName *name) {
    for(int i=0; i<11; ++i) {
        if (name->Name[i] == '?')
            return TRUE;
    }

    return FALSE;
}

int File_Name_Wild_Match(FileName *name, FileName *fn)
{
    for(int i=0; i<11; ++i) {
        const UBYTE c = name->Name[i];
        const UBYTE d = fn->Name[i];

        if (c != d && c != '?')
            return FALSE;
    }

    return TRUE;
}

void File_Name_Wild_Merge(FileName *name, const FileName *fn)
{
    for(int i=0; i<11; ++i) {
        const UBYTE d = fn->Name[i];

        if (d != '?')
            name->Name[i] = d;

        if (name->Name[i] == ' ') {
            if (i < 8) {
                while(i < 7)
                    name->Name[++i] = ' ';
            } else {
                while(i < 10)
                    name->Name[++i] = ' ';
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////

enum {
    CIOStatSuccess      = 0x01,
    CIOStatSuccessEOF   = 0x03, // succeeded, but at end of file (undocumented)
    CIOStatBreak        = 0x80, // break key abort
    CIOStatIOCBInUse    = 0x81, // IOCB in use
    CIOStatUnkDevice    = 0x82, // unknown device
    CIOStatWriteOnly    = 0x83, // opened for write only
    CIOStatInvalidCmd   = 0x84, // invalid command
    CIOStatNotOpen      = 0x85, // device or file not open
    CIOStatInvalidIOCB  = 0x86, // invalid IOCB number
    CIOStatReadOnly     = 0x87, // opened for read only
    CIOStatEndOfFile    = 0x88, // end of file reached
    CIOStatTruncRecord  = 0x89, // record truncated
    CIOStatTimeout      = 0x8A, // device timeout
    CIOStatNAK          = 0x8B, // device NAK
    CIOStatSerFrameErr  = 0x8C, // serial bus framing error
    CIOStatCursorRange  = 0x8D, // cursor out of range
    CIOStatSerOverrun   = 0x8E, // serial frame overrun error
    CIOStatSerChecksum  = 0x8F, // serial checksum error
    CIOStatDeviceDone   = 0x90, // device done error
    CIOStatBadScrnMode  = 0x91, // bad screen mode
    CIOStatNotSupported = 0x92, // function not supported by handler
    CIOStatOutOfMemory  = 0x93, // not enough memory
    CIOStatPathNotFound = 0x96, // [SDX] path not found
    CIOStatFileExists   = 0x97, // [SDX] file exists
    CIOStatBadParameter = 0x9C, // [SDX] bad parameter
    CIOStatDriveNumErr  = 0xA0, // disk drive # error
    CIOStatTooManyFiles = 0xA1, // too many open disk files
    CIOStatDiskFull     = 0xA2, // disk full
    CIOStatFatalDiskIO  = 0xA3, // fatal disk I/O error
    CIOStatIllegalWild  = 0xA3, // [SDX] Illegal wildcard in name
    CIOStatFileNumDiff  = 0xA4, // internal file # mismatch
    CIOStatFileNameErr  = 0xA5, // filename error
    CIOStatPointDLen    = 0xA6, // point data length error
    CIOStatFileLocked   = 0xA7, // file locked
    CIOStatDirNotEmpty  = 0xA7, // [SDX] directory not empty
    CIOStatInvDiskCmd   = 0xA8, // invalid command for disk
    CIOStatDirFull      = 0xA9, // directory full (64 files)
    CIOStatFileNotFound = 0xAA, // file not found
    CIOStatInvPoint     = 0xAB, // invalid point
    CIOStatAccessDenied = 0xB0, // [SDX] access denied
    CIOStatPathTooLong  = 0xB6, // [SDX] path too long
    CIOStatSystemError  = 0xFF, // {SDX] system error
};

UBYTE TranslateErrnoToSIOError(ULONG err) {
    switch(err) {
        case EACCES:
            return CIOStatAccessDenied;
        case EAGAIN:
        case EFAULT:
        case EINTR:
        case EIO:
        case ELOOP:
            return CIOStatSystemError;
        case EDQUOT:
            return CIOStatDiskFull;
        case EEXIST:
            return CIOStatFileExists;
        case EINVAL:
            return CIOStatInvalidCmd;
        case EISDIR:
        case EMFILE:
        case ENAMETOOLONG:
        case ENFILE:
        case ENOENT:
        case ENOSPC:
        case ENOTDIR:
        case ENXIO:
        case EOPNOTSUPP:
        case EOVERFLOW:
        case EROFS:
        case ETXTBSY:
        case EBADF:
        case EILSEQ:
        default:
            return CIOStatSystemError;
    }
}

///////////////////////////////////////////////////////////////////////////

typedef struct fileHandle {
    int      Open;
    int      AllowRead;
    int      AllowWrite;
    int      IsDirectory;
    int      WasCreated;
    ULONG    Pos;
    ULONG    Length;

    DirEntry *DirEnts;

    DirEntry *DirEnt;
    FileName *FnextPattern;
    UBYTE    FnextAttrFilter;

    FILE     *File;
} FileHandle;

FileHandle *File_Handle_Alloc()
{
    FileHandle *hndl = (FileHandle *) malloc(sizeof(FileHandle));
    memset(hndl, 0, sizeof(FileHandle));
    hndl->DirEnts = vector_create();
    return hndl;
}

void File_Handle_Free(FileHandle *hndl)
{
    vector_free(hndl->DirEnts);
    free(hndl);
}

int File_Handle_Is_Open(FileHandle *hndl)
{
    return hndl->Open;
}

int File_Handle_Is_Readable(FileHandle *hndl)
{
    return hndl->AllowRead;
}

int File_Handle_Is_Writable(FileHandle *hndl)
{
    return hndl->AllowWrite;
}

int File_Handle_Is_Dir(FileHandle *hndl)
{
    return hndl->IsDirectory;
}

int File_Handle_Get_Position(FileHandle *hndl)
{
    return hndl->Pos;
}

int File_Handle_Get_Length(FileHandle *hndl)
{
    return hndl->Length;
}

DirEntry *File_Handle_Get_Dir_Ent(FileHandle *hndl)
{
    return hndl->DirEnt;
}

void File_Handle_Set_Dir_Ent(FileHandle *hndl, DirEntry *dirEnt)
{
    hndl->DirEnt = dirEnt;
}

void File_Handle_Add_Dir_Ent(FileHandle *hndl, DirEntry *dirEnt) {
    vector_add(&hndl->DirEnts, *dirEnt);
}

UBYTE File_Handle_Open_File(FileHandle *hndl, const char *nativePath,
                            FileOpenType openType, int allowRead, 
                            int allowWrite, int append) 
{
    struct stat file_stats;
    FILE *file = NULL;

    int exists = access(nativePath, F_OK);

    hndl->WasCreated = FALSE;

    switch (openType) {
        case FILE_READ:
            file = fopen(nativePath, "r");
            break;
        case FILE_WRITE:
            break;
            file = fopen(nativePath, "w");
        case FILE_APPEND:
            file = fopen(nativePath, "a");
            break;
        case FILE_UPDATE:
            file = fopen(nativePath, "r+");
            break;
    }

    if (file) {
        hndl->Open = TRUE;
        hndl->IsDirectory = FALSE;
        hndl->AllowRead = allowRead;
        hndl->AllowWrite = allowWrite;
        hndl->WasCreated = !exists;

        if ((stat(nativePath, &file_stats)) == -1) {
            //TBD handle error;
        }

        SLONG len = file_stats.st_size;
        if (len > 0xffffff)
            hndl->Length = 0xffffff;
        else
            hndl->Length = (ULONG)len;

        hndl->Pos = 0;

        return CIOStatSuccess;
    } else {
        hndl->Open = FALSE;
        return TranslateErrnoToSIOError(errno);
    }
}

void File_Handle_Open_As_Directory(FileHandle *hndl, 
                                   FileName* dirName,
                                   FileName* pattern, 
                                   UBYTE attrFilter)
{
    qsort(hndl->DirEnts,
          vector_size(hndl->DirEnts),
          sizeof(DirEntry),
          Dir_Entry_Compare
         );
    hndl->Open = TRUE;
    hndl->IsDirectory = TRUE;
    hndl->Length = 23 * ((ULONG)cvector_size(hndl->DirEnts) + 1);
    hndl->Pos = 23;
    hndl->AllowRead = TRUE;
    hndl->AllowWrite = FALSE;

    memset(hndl->DirEnt, 0, sizeof(DirEntry));
    hndl->DirEnt->Flags = Flag_InUse | Flag_Directory;
    hndl->DirEnt->LengthLo = (UBYTE)hndl->Length;
    hndl->DirEnt->LengthMid = (UBYTE)(hndl->Length >> 8);
    hndl->DirEnt->LengthHi = (UBYTE)(hndl->Length >> 16);
    memcpy(hndl->DirEnt->Name, dirName->Name, 11);

    vector_insert(&hndl->DirEnts, 0, *hndl->DirEnt);

    hndl->FnextPattern = pattern;
    hndl->FnextAttrFilter = attrFilter;
}

void File_Handle_Close(FileHandle *hndl)
{
    fclose(hndl->File);
    hndl->Open = FALSE;
    hndl->AllowRead = FALSE;
    hndl->AllowWrite = FALSE;
    vector_free(&hndl->DirEnts);
    hndl->DirEnts = NULL;
}

UBYTE File_Handle_Seek(FileHandle *hndl, ULONG pos) {
    if (pos > hndl->Length && !hndl->AllowRead)
        return CIOStatPointDLen;

    if (!hndl->IsDirectory) {
        int stat;
        stat = fseek(hndl->File, pos, SEEK_SET);
        if (stat == 0) {
            hndl->Pos = pos;
            return CIOStatSuccess;
        } else {
            return TranslateErrnoToSIOError(errno);
        }
    }
    return 0;
}

UBYTE File_Handle_Read(FileHandle *hndl, void *dst, ULONG len, ULONG *actual) {
    *actual = 0;

    if (!hndl->Open)
        return CIOStatNotOpen;

    if (!hndl->AllowRead)
        return CIOStatWriteOnly;

    long act = 0;

    if (hndl->Pos < hndl->Length) {
        if (hndl->IsDirectory) {
            ULONG dirIndex = hndl->Pos / 23;
            ULONG offset = hndl->Pos % 23;
            ULONG left = hndl->Length - hndl->Pos;

            if (left > len)
                left = len;
        
            while(left > 0) {
                const UBYTE *src = (const UBYTE *)&hndl->DirEnts[dirIndex];
                ULONG tc = 23 - offset;
                if (tc > left)
                    tc = left;

                memcpy((char *)dst + act, src + offset, tc);
                left -= tc;
                act += tc;
                offset = 0;
                ++dirIndex;
            }
        } else {
            ULONG tc = len;

            if (hndl->Length - hndl->Pos < tc)
                tc = hndl->Length - hndl->Pos;

            act = fread(dst, 1, tc, hndl->File);
            if (act < 0)
                return CIOStatFatalDiskIO;
        }
    }

    *actual = (ULONG)act;

    printf("Read at pos %d/%d, len %d, actual %d\n", hndl->Pos, hndl->Length, len, *actual);

    hndl->Pos += *actual;

    if (*actual < len) {
        memset((char *)dst + *actual, 0, len - *actual);
        return CIOStatTruncRecord;
    }

    return CIOStatSuccess;
}

UBYTE File_Handle_Write(FileHandle *hndl, const void *dst, ULONG len) {
    if (!hndl->Open)
        return CIOStatNotOpen;

    if (!hndl->AllowWrite)
        return CIOStatReadOnly;

    ULONG tc = len;
    ULONG actual = 0;

    if (hndl->Pos < 0xffffff) {
        if (0xffffff - hndl->Pos < tc)
            tc = 0xffffff - hndl->Pos;

        actual = fwrite(dst, 1, tc, hndl->File);

        if (actual != tc) {
            fseek(hndl->File, hndl->Pos, SEEK_SET);
            return CIOStatFatalDiskIO;
        }
    }

    printf("Write at pos %d/%d, len %d, actual %d\n", hndl->Pos, hndl->Length, len, actual);

    hndl->Pos += actual;
    if (hndl->Pos > hndl->Length)
        hndl->Length = hndl->Pos;

    return actual != len ? CIOStatDiskFull : CIOStatSuccess;
}

void File_Handle_Set_Timestamp(FileHandle *hndl, const UBYTE *tsdata) {
    struct timeval fileTime[2];
    struct tm fileExpTime;

    Dir_Entry_Decode_Date(tsdata, &fileExpTime);

    fileTime[0].tv_sec = mktime(&fileExpTime);
    fileTime[0].tv_usec = 0;
    fileTime[1].tv_sec = fileTime[0].tv_sec;
    fileTime[1].tv_usec = fileTime[0].tv_usec;
    futimes (hndl->File, fileTime);
}

int File_Handle_Get_Next_Dir_Ent(FileHandle *hndl, DirEntry *dirEnt) {
    ULONG actual;

    while(File_Handle_Read(hndl, dirEnt, 23, &actual) == CIOStatSuccess && actual >= 23) {
        FileName *name = File_Name_Alloc();
        File_Name_Parse_From_Net(name, dirEnt->Name);

        if (File_Name_Wild_Match(name, hndl->FnextPattern) && Dir_Entry_Test_Attr_Filter(hndl->DirEnt, hndl->FnextAttrFilter))
            return TRUE;
    }

    return FALSE;
}

typedef struct parameterBuffer {
        UBYTE   Function;  // function number
        UBYTE   Handle;    // file handle
        UBYTE   F[6];
        UBYTE   Mode;      // file open mode
        UBYTE   Attr1;
        UBYTE   Attr2;
        UBYTE   Name1[12];
        UBYTE   Name2[12];
        UBYTE   Path[65];
    } ParameterBuffer;

typedef enum {
    CommandNone,
    CommandGetHiSpeedIndex,
    CommandStatus,
    CommandPut,
    CommandRead
} Command;

typedef struct linkDevice {
    char    *BasePathNative;
    int     ReadOnly;
    int     SetTimestamps;

    UBYTE   StatusFlags;
    UBYTE   StatusError;
    UBYTE   StatusLengthLo;
    UBYTE   StatusLengthHi;

    Command Command;
    ULONG   CommandPhase;
    UBYTE   CommandAux1;
    UBYTE   CommandAux2;

    char    *CurDir;

    ParameterBuffer ParBuf;

    FileHandle FileHandles[15];
    
    int     ExpectedBytes;
    int     ReadCommand;
    int     ParSize;

    UBYTE   TransferBuffer[65536];
} LinkDevice;

///////////////////////////////////////////////////////////////////////////
LinkDevice *Link_Device_Alloc();
void Link_Device_Free(LinkDevice *dev);
void Link_Device_Set_Read_Only(LinkDevice *dev, int readOnly);
void Link_Device_Set_Base_Path(LinkDevice *dev, char *basePath);
void Link_Device_Set_Settings(LinkDevice *dev, int setTimestamps, int readOnly, char *basePath);
void Link_Device_Shutdown(LinkDevice *dev);
void Link_Device_Cold_Reset(LinkDevice *dev);
void Link_Device_Begin_Command(LinkDevice *dev, Command cmd);
void Link_Device_Abort_Command(LinkDevice *dev);
void Link_Device_Advance_Command(LinkDevice *dev);
int  Link_Device_Check_Valid_File_Handle(LinkDevice *dev, int setError);
int  Link_Device_Is_Dir_Ent_Included(LinkDevice *dev, DirEntry *dirEnt);
int  Link_Device_Resolve_Path(LinkDevice *dev, int allowDir, char *resultPath);
int  Link_Device_Resolve_Native_Path(LinkDevice *dev, char *resultPath, const char *netPath);
int Link_Device_Resolve_Native_Path_Dir(LinkDevice *dev, int allowDir, char *resultPath);

LinkDevice *Link_Device_Alloc() {
    LinkDevice *dev;

    dev = malloc(sizeof(LinkDevice));
    memset(dev, 0, sizeof(LinkDevice));
    return dev;
}

void Link_Device_Free(LinkDevice *dev) {
    free(dev);
}

void Link_Device_Set_Read_Only(LinkDevice *dev, int readOnly) {
    dev->ReadOnly = readOnly;
}

void Link_Device_Set_Base_Path(LinkDevice *dev, char *basePath) {
    dev->BasePathNative = basePath;
}

void Link_Device_Set_Settings(LinkDevice *dev, int setTimestamps,
                              int readOnly, char *basePath) {
    dev->SetTimestamps = setTimestamps;
    Link_Device_Set_Read_Only(dev, readOnly);
    Link_Device_Set_Base_Path(dev, basePath);
}

void Link_Device_Shutdown(LinkDevice *dev) {
    Link_Device_Abort_Command(dev);
}

void Link_Device_Cold_Reset(LinkDevice *dev) {
    for (int i = 0; i < 15; i++)
        File_Handle_Close(&dev->FileHandles[i]);
}

UBYTE Link_Device_On_Serial_Begin_Command( LinkDevice *dev,
                                           UBYTE *commandFrame,
                                           int *read, int *ExpectedBytes) {
    if (commandFrame[0] != 0x6F)
        return 'N';

    if (strlen(dev->BasePathNative) == 0)
        return 'N';

    // Protocol version must be zero
    if (commandFrame[3] & 0xf0)
        return 'N';
    
    // Parameter buffer must be within parameter sizel
    dev->ParSize = commandFrame[2] ? commandFrame[2] : 256;
    if (dev->ParSize  > sizeof(ParameterBuffer))
        return 'N';
    
    dev->CommandAux1 = commandFrame[2];
    dev->CommandAux2 = commandFrame[3];

    Command command = CommandNone;

    if (commandFrame[1] == 0x53)          // status
        command = CommandStatus;
    else if (commandFrame[1] == 0x50)     // put
        command = CommandPut;
    else if (commandFrame[1] == 0x52)     // read
        command = CommandRead;
    else if (commandFrame[1] == 0x3F)
        command = CommandGetHiSpeedIndex;
    else {
        printf("Unsupported command $%02x\n", commandFrame[1]);
        return 'N';
    }

    //mpSIOMgr->SendACK();
    dev->CommandPhase = 0;

    Link_Device_Advance_Command(dev);

    *ExpectedBytes = dev->ExpectedBytes;
    *read = dev->ReadCommand;
    return 'A';
}

void Link_Device_Abort_Command(LinkDevice *dev);
void Link_Device_Advance_Command(LinkDevice *dev);
    
void Link_Device_Abort_Command(LinkDevice *dev) {
    if (dev->Command) {
        dev->Command = CommandNone;
        dev->CommandPhase = 0;
    }
}

void Link_Device_Advance_Command(LinkDevice *dev) {
    switch(dev->Command) {
        case CommandGetHiSpeedIndex:
            printf("Sending high-speed index\n");
            dev->ExpectedBytes = 1;
            dev->ReadCommand = TRUE;
            dev->TransferBuffer[0] = 9;
            break;

        case CommandStatus:
            printf("Sending status: Flags=$%02x, Error=%3d, Length=%02x%02x\n", dev->StatusFlags, dev->StatusError, dev->StatusLengthHi, dev->StatusLengthLo);
            dev->ExpectedBytes = 4;
            dev->ReadCommand = TRUE;
            dev->TransferBuffer[0] = dev->StatusFlags;
            dev->TransferBuffer[1] = dev->StatusError;
            dev->TransferBuffer[2] = dev->StatusLengthLo;
            dev->TransferBuffer[3] = dev->StatusLengthHi;
            break;

        case CommandPut:
            dev->ReadCommand = FALSE;
            dev->ExpectedBytes = dev->ParSize;
            break;

        case CommandRead:
            dev->ReadCommand = TRUE;
            break;
        
        case CommandNone:
            break;
    }
}

void Link_Device_Finish_Command(LinkDevice *dev) {
    Link_Device_Abort_Command(dev);
}

int Link_Device_On_Put(LinkDevice *dev) {
    char *end;
    char *ext;
    char *fnend;
    char *s;
    char *t;
    char fullPath[FILENAME_MAX];
    char nativeFilePath[FILENAME_MAX];
    char nativePath[FILENAME_MAX];
    char netPath[FILENAME_MAX];
    char path[FILENAME_MAX];
    char resultPath[FILENAME_MAX];
    char dstNativePath[FILENAME_MAX];
    char srcNativePath[FILENAME_MAX];
    DIR * dirStream;
    DirEntry dirEnt;
    FileHandle* fh;
    FileName dirName;
    FileName dstpat;
    FileName entryName;
    FileName fn;
    FileName fn2;
    FileName fname;
    FileName pattern;
    FileName srcpat;
    int openDir;
    int setTimestamp;
    int matched;
    mode_t newmode;
    long long slen;
    size_t fnlen;
    size_t extlen;
    struct dirent *ep;
    struct stat file_stats;
    struct timeval fileTime[2];
    struct tm fileExpTime;
    ULONG bufLen;
    ULONG len;
    ULONG pos;

    switch(dev->ParBuf.Function) {
        case 0:     // fread
            bufLen = dev->ParBuf.F[0] + 256*dev->ParBuf.F[1];
            printf("Received fread($%02x,%d) command.\n", dev->ParBuf.Handle, bufLen);

            if (Link_Device_Check_Valid_File_Handle(dev, TRUE)) {
                fh = &dev->FileHandles[dev->ParBuf.Handle - 1];

                if (!File_Handle_Is_Open(fh)) {
                    dev->StatusError = CIOStatNotOpen;
                    return TRUE;
                }

                if (!File_Handle_Is_Readable(fh)) {
                    dev->StatusError = CIOStatWriteOnly;
                    return TRUE;
                }

                pos = File_Handle_Get_Position(fh);
                len = File_Handle_Get_Length(fh);

                if (pos >= len)
                    bufLen = 0;
                else if (len - pos < bufLen)
                    bufLen = len - pos;

                dev->StatusLengthLo = (UBYTE)bufLen;
                dev->StatusLengthHi = (UBYTE)(bufLen >> 8);
                dev->StatusError = bufLen ? CIOStatSuccess : CIOStatEndOfFile;
                }
            return TRUE;

        case 1:     // fwrite
            if (dev->ReadOnly) {
                dev->StatusError = CIOStatReadOnly;
                return TRUE;
            }

            bufLen = dev->ParBuf.F[0] + 256*dev->ParBuf.F[1];
            printf("Received fwrite($%02x,%d) command.\n", dev->ParBuf.Handle, bufLen);

            if (Link_Device_Check_Valid_File_Handle(dev, TRUE)) {
                fh = &dev->FileHandles[dev->ParBuf.Handle - 1];

                if (!File_Handle_Is_Open(fh)) {
                    dev->StatusError = CIOStatNotOpen;
                    return TRUE;
                }

                if (!File_Handle_Is_Writable(fh)) {
                    dev->StatusError = CIOStatReadOnly;
                    return TRUE;
                }

                pos = File_Handle_Get_Position(fh);

                if (pos >= 0xffffff)
                    bufLen = 0;
                else if (0xffffff - pos < bufLen)
                    bufLen = 0xffffff - pos;

                dev->StatusLengthLo = (UBYTE)bufLen;
                dev->StatusLengthHi = (UBYTE)(bufLen >> 8);
                dev->StatusError = bufLen ? CIOStatSuccess : CIOStatDiskFull;
                }
            return TRUE;

        case 2:     // fseek
            pos = dev->ParBuf.F[0] + ((ULONG)dev->ParBuf.F[1] << 8) + ((ULONG)dev->ParBuf.F[2] << 16);
            printf("Received fseek($%02x,%d) command.\n", dev->ParBuf.Handle, pos);

            if (Link_Device_Check_Valid_File_Handle(dev, TRUE)) {
                fh = &dev->FileHandles[dev->ParBuf.Handle - 1];

                if (!File_Handle_Is_Open(fh)) {
                    dev->StatusError = CIOStatNotOpen;
                    return TRUE;
                }

                dev->StatusError = File_Handle_Seek(fh, pos);
                }
            return TRUE;

        case 3:     // ftell
            printf("Received ftell($%02x) command.\n", dev->ParBuf.Handle);

            if (!Link_Device_Check_Valid_File_Handle(dev, TRUE))
                return TRUE;

            fh = &dev->FileHandles[dev->ParBuf.Handle - 1];

            if (!File_Handle_Is_Open(fh)) {
                dev->StatusError = CIOStatNotOpen;
                return TRUE;
            }

            dev->StatusError = CIOStatSuccess;
            return TRUE;

        case 4:     // flen
            if (!Link_Device_Check_Valid_File_Handle(dev, TRUE))
                return TRUE;

            dev->StatusError = CIOStatSuccess;
            return TRUE;

        case 5:     // reserved
            dev->StatusError = CIOStatNotSupported;
            return TRUE;

        case 6:     // fnext
            printf("Received fnext($%02x) command.\n", dev->ParBuf.Handle);
            if (!Link_Device_Check_Valid_File_Handle(dev, TRUE))
                return TRUE;

            dev->StatusError = CIOStatSuccess;
            return TRUE;

        case 7:     // fclose
            printf("Received close($%02x) command.\n", dev->ParBuf.Handle);
            if (Link_Device_Check_Valid_File_Handle(dev, FALSE))
                File_Handle_Close(&dev->FileHandles[dev->ParBuf.Handle - 1]);

            dev->StatusError = CIOStatSuccess;
            return TRUE;

        case 8:     // init
            printf("Received init command.\n");
            for(size_t i = 0; i < sizeof(dev->FileHandles)/sizeof(dev->FileHandles[0]); ++i)
                File_Handle_Close(&dev->FileHandles[i]);

            dev->StatusFlags = 0;
            dev->StatusError = CIOStatSuccess;
            dev->StatusLengthLo = 0;
            dev->StatusLengthHi = 0x6F;

            dev->CurDir[0] = 0;
            return TRUE;

        case 9:     // fopen
        case 10:    // ffirst
            if (dev->ParBuf.Function == 9)
                printf("Received fopen() command.\n");
            else
                printf("Received ffirst() command.\n");

            dev->ParBuf.Handle = 1;

            for(;;) {
                if (dev->ParBuf.Handle > sizeof(dev->FileHandles)/sizeof(dev->FileHandles[0])) {
                    dev->StatusError = CIOStatTooManyFiles;
                    return TRUE;
                }

                if (!File_Handle_Is_Open(&dev->FileHandles[dev->ParBuf.Handle - 1]))
                    break;

                ++dev->ParBuf.Handle;
            }

            fh = &dev->FileHandles[dev->ParBuf.Handle - 1];

            if (!Link_Device_Resolve_Path(dev, dev->ParBuf.Function == 10, netPath) || 
                !Link_Device_Resolve_Native_Path(dev, nativePath, netPath))
                return TRUE;

            if (!File_Name_Parse_From_Net(&pattern, dev->ParBuf.Name1))
                return TRUE;

            openDir = dev->ParBuf.Function == 10 || (dev->ParBuf.Mode & 0x10) != 0;

            if ((dirStream = opendir(nativePath)) == NULL) {
                // TBD Handle Error
            }

            matched = FALSE;
            while((ep = readdir(dirStream))) {
                if (!File_Name_Parse_From_Native(&fn, ep->d_name));
                    continue;

                // We can't filter at this point for a directory, because the byte stream
                // needs to reflect all files while the FNEXT output shouldn't. Therefore,
                // we need to cache the pattern with the file handle instead.
                if (!openDir && !File_Name_Wild_Match(&pattern, &fn))
                    continue;

                if ((stat(ep->d_name, &file_stats)) == -1) {
                    //TBD handle error;
                }
                slen = file_stats.st_size;

                if (slen > 0xFFFFFF)
                    slen = 0xFFFFFF;

                Dir_Entry_Set_Flags_From_Attributes(&dirEnt,
                    file_stats.st_mode);

                if (ep->d_type == DT_DIR) {
                    // skip blasted . and .. dirs
                    if ((strcmp(ep->d_name,".") == 0) ||
                        (strcmp(ep->d_name,"..") == 0))
                        continue;
                }

                if (!Link_Device_Is_Dir_Ent_Included(dev, &dirEnt))
                    continue;

                dirEnt.SectorMapLo = 0;
                dirEnt.SectorMapHi = 0;
                dirEnt.LengthLo = (UBYTE)slen;
                dirEnt.LengthMid = (UBYTE)((ULONG)slen >> 8);
                dirEnt.LengthHi = (UBYTE)((ULONG)slen >> 16);
                memcpy(dirEnt.Name, fn.Name, sizeof(dirEnt.Name));
                Dir_Entry_Set_Date(&dirEnt, ep->st_mtime);

                if (!openDir) {
                    matched = TRUE;
                    nativeFilePath = it.GetFullPath();
                    break;
                }

                File_Handle_Add_Dir_Ent(fh, &dirEnt);
            }

            if (openDir) {
                // extract name from net path
                s = netPath;
                t = strrchr(s, '\\');
                if (t)
                    s = t + 1;

                end = s + strlen(s);
                ext = strchr(s, '.');
                fnend = ext ? ext : end;

                fnlen = fnend - s;
                extlen = end - ext;

                if (fnlen > 8)
                    fnlen = 8;

                if (extlen > 3)
                    extlen = 8;

                memset(dirName.Name, ' ', 11);

                if (fnlen == 0)
                    memcpy(dirName.Name, "MAIN", 4);
                else
                    memcpy(dirName.Name, s, fnlen);

                if (ext)
                    memcpy(dirName.Name + 8, ext, extlen);

                File_Handle_Open_As_Directory(fh, &dirName, &pattern, dev->ParBuf.Attr1);

                dev->StatusError = CIOStatSuccess;
            } else {
                if (!matched) {
                    // cannot create file with a wildcard
                    if (!(dev->ParBuf.Mode & 4) && File_Name_Is_Wild(&pattern)) {
                        dev->StatusError = CIOStatIllegalWild;
                        return TRUE;
                    }

                    File_Name_Append_Native(&pattern, nativeFilePath);
                }

                if ((dev->ParBuf.Mode & 8) && dev->ReadOnly) {
                    dev->StatusError = CIOStatReadOnly;
                } else {
                    setTimestamp = FALSE;

                    switch(dev->ParBuf.Mode & 15) {
                        case 4:     // read
                            if (!matched)
                                dev->StatusError = CIOStatFileNotFound;
                            else {
                                dev->StatusError = File_Handle_Open_File(fh, nativeFilePath,
                                    FILE_READ, TRUE, FALSE, FALSE);
                            }
                            break;

                        case 8:     // write
                            dev->StatusError = File_Handle_Open_File(fh, nativeFilePath,
                                FILE_WRITE, FALSE, TRUE, FALSE);
                            dev->SetTimestamps = TRUE;
                            break;

                        case 9:     // append
                            dev->StatusError = File_Handle_Open_File(fh, nativeFilePath,
                                FILE_APPEND, FALSE, TRUE, TRUE);

                            if (fh->WasCreated)
                                dev->SetTimestamps = true;
                            break;

                        case 12:    // update
                            if (!matched)
                                dev->StatusError = CIOStatFileNotFound;
                            else {
                                dev->StatusError = File_Handle_Open_File(fh, nativeFilePath,
                                    FILE_UPDATE, TRUE, TRUE, FALSE);
                            }
                            break;

                        default:
                            dev->StatusError = CIOStatInvalidCmd;
                            break;
                    }

                    if (dev->SetTimestamps && dev->SetTimestamps && File_Handle_Is_Open(fh)) {
                        File_Handle_Set_Timestamp(fh, dev->ParBuf.F);
                    }
                }

                File_Handle_Set_Dir_Ent(fh, &dirEnt);
            }
            return TRUE;

        case 11:    // rename
            printf("Received rename() command.\n");

            if (dev->ReadOnly) {
                dev->StatusError = CIOStatReadOnly;
                return TRUE;
            }

            if (!File_Name_Parse_From_Net(&srcpat, dev->ParBuf.Name1)
                || !File_Name_Parse_From_Net(&dstpat, dev->ParBuf.Name2))
            {
                dev->StatusError = CIOStatFileNameErr;
                return TRUE;
            }

            if ((dirStream = opendir(path)) == NULL) {
                // TBD Handle Error
            }

            matched = FALSE;
            while((ep = readdir(dirStream))) {
                if ((strcmp(ep->d_name,".") == 0) ||
                    (strcmp(ep->d_name,"..") == 0))
                    continue;

                if ((stat(ep->d_name, &file_stats)) == -1) {
                    //TBD handle error;
                }
                Dir_Entry_Set_Flags_From_Attributes(&dirEnt,
                    file_stats.st_mode);

                if (!Link_Device_Is_Dir_Ent_Included(dev, &dirEnt))
                    continue;

                if (!File_Name_Parse_From_Native(&fn, ep->d_name))
                    continue;

                if (!File_Name_Wild_Match(&srcpat, &fn))
                    continue;

                memcpy(fn2.Name, fn.Name, 11);
                File_Name_Wild_Merge(&fn2, &dstpat);

                if (File_Name_Is_Equal(&fn, &fn2))
                    continue;

                strcpy(srcNativePath, path);
                File_Name_Append_Native(&fn, srcNativePath);

                strcpy(dstNativePath, path);
                File_Name_Append_Native(&fn2, dstNativePath);

                if (rename(srcNativePath, dstNativePath))
                    {
                        dev->StatusError = TranslateErrnoToSIOError(errno);
                    }

                matched = TRUE;
                }

            if (matched)
                dev->StatusError = CIOStatSuccess;
            else
                dev->StatusError = CIOStatFileNotFound;
            return TRUE;

        case 12:    // remove
            printf("Received remove() command.\n");

            if (dev->ReadOnly) {
                dev->StatusError = CIOStatReadOnly;
                return TRUE;
            }

            if (!Link_Device_Resolve_Native_Path_Dir(dev, FALSE, resultPath))
                return TRUE;

            if (!File_Name_Parse_From_Net(&fname, dev->ParBuf.Name1)) {
                dev->StatusError = CIOStatFileNameErr;
                return TRUE;
            }

            if (File_Name_Is_Wild(&fname)) {
                if ((dirStream = opendir(resultPath)) == NULL) {
                    // TBD Handle Error
                }

            matched = FALSE;
                while((ep = readdir(dirStream))) {
                if ((stat(ep->d_name, &file_stats)) == -1) {
                    //TBD handle error;
                }

                if (S_ISDIR(file_stats.st_mode))
                    continue;

                Dir_Entry_Set_Flags_From_Attributes(&dirEnt,
                    file_stats.st_mode);

                if (!Link_Device_Is_Dir_Ent_Included(dev, &dirEnt))
                    continue;

                if (!File_Name_Parse_From_Native(&entryName, ep->d_name))
                    continue;

                if (!File_Name_Wild_Match(&fname, &entryName))
                    continue;

                strcpy(fullPath, resultPath);
                strcat(fullPath, "/");
                strcat(fullPath, ep->d_name);
                if (remove(fullPath))
                    {
                    dev->StatusError = TranslateErrnoToSIOError(errno);
                    return TRUE;
                    }
                matched = TRUE;
            }

            if (!matched) {
                dev->StatusError = CIOStatFileNotFound;
                return TRUE;
                }
            } else {
                File_Name_Append_Native(&fname, resultPath);

                if ((stat(resultPath, &file_stats)) == -1) {
                    dev->StatusError = CIOStatFileNotFound;
                    return TRUE;
                }

                Dir_Entry_Set_Flags_From_Attributes(&dirEnt, file_stats.st_mode);

                if (Link_Device_Is_Dir_Ent_Included(dev, &dirEnt)) {
                    if (remove(resultPath)) {
                        dev->StatusError = TranslateErrnoToSIOError(errno);
                        return TRUE;
                    }
                }
                else {
                    dev->StatusError = CIOStatFileNotFound;
                    return TRUE;
                }

            dev->StatusError = CIOStatSuccess;
            }
            return TRUE;

        case 13:    // chmod
            printf("Received chmod() command.\n");

            if (dev->ReadOnly) {
                dev->StatusError = CIOStatReadOnly;
                return TRUE;
            }

            if (!Link_Device_Resolve_Native_Path(dev, FALSE, path))
                return TRUE;

            if (!File_Name_Parse_From_Net(&srcpat, dev->ParBuf.Name1)) {
                dev->StatusError = CIOStatFileNameErr;
                return TRUE;
            }

            if ((dirStream = opendir(resultPath)) == NULL) {
                // TBD Handle Error
            }

            matched = FALSE;
            while((ep = readdir(dirStream))) {
                if ((strcmp(ep->d_name,".") == 0) ||
                    (strcmp(ep->d_name,"..") == 0))
                    continue;

                if ((stat(ep->d_name, &file_stats)) == -1) {
                    //TBD handle error;
                }

                Dir_Entry_Set_Flags_From_Attributes(&dirEnt,
                    file_stats.st_mode);

                if (!Link_Device_Is_Dir_Ent_Included(dev, &dirEnt))
                    continue;

                if (!File_Name_Parse_From_Native(&fn, ep->d_name))
                    continue;

                if (!File_Name_Wild_Match(&srcpat, &fn))
                    continue;

                strcpy(srcNativePath, path);
                File_Name_Append_Native(&fn, srcNativePath);

                newmode = sb.st_mode;

                if (dev->ParBuf.Attr2 & 0x10)
                    newmode |= S_IWUSR;

                if (dev->ParBuf.Attr2 & 0x01)
                    newmode &= ~S_IWUSR;

                if (chmod(srcNativePath, newmode)) {
                    dev->StatusError = TranslateErrnoToSIOError(errno);
                    return TRUE;
                    }

                matched = TRUE;
                }

            if (matched)
                dev->StatusError = CIOStatSuccess;
            else
                dev->StatusError = CIOStatFileNotFound;

            return TRUE;

        case 14:    // mkdir
            printf("Received mkdir() command.\n");

            if (dev->ReadOnly) {
                dev->StatusError = CIOStatReadOnly;
                return TRUE;
            }

            if (!Link_Device_Resolve_Native_Path_Dir(dev, FALSE, resultPath))
                return TRUE;

            if (!File_Name_Parse_From_Net(&fname, dev->ParBuf.Name1)) {
                dev->StatusError = CIOStatFileNameErr;
                return TRUE;
            }

            if (File_Name_Is_Wild(&fname)) {
                dev->StatusError = CIOStatIllegalWild;
                return TRUE;
            }

            File_Name_Append_Native(&fname, resultPath);

            if (mkdir(resultPath, S_IRWXU)) {
                dev->StatusError = TranslateErrnoToSIOError(errno);
                return TRUE;
            }

            if (dev->SetTimestamps) {
                Dir_Entry_Decode_Date(dev->ParBuf.F, &fileExpTime);

                fileTime[0].tv_sec = mktime(&fileExpTime);
                fileTime[0].tv_usec = 0;
                fileTime[1].tv_sec = fileTime[0].tv_sec;
                fileTime[1].tv_usec = fileTime[0].tv_usec;
                if (utimes (resultPath, fileTime)) {
                    dev->StatusError = TranslateErrnoToSIOError(errno);
                    return TRUE;
                }   
            }

            dev->StatusError = CIOStatSuccess;
            return TRUE;

        case 15:    // rmdir
            printf("Received rmdir() command.\n");

            if (dev->ReadOnly) {
                dev->StatusError = CIOStatReadOnly;
                return TRUE;
            }

            if (!Link_Device_Resolve_Native_Path(dev, FALSE, resultPath))
                return true;

            if (!File_Name_Parse_From_Net(&fname, dev->ParBuf.Name1)) {
                dev->StatusError = CIOStatFileNameErr;
                return TRUE;
            }

            if (File_Name_Is_Wild(&fname)) {
                dev->StatusError = CIOStatIllegalWild;
                return TRUE;
            }

            File_Name_Append_Native(&fname, resultPath);

            if (rmdir(resultPath)) {
                dev->StatusError = TranslateErrnoToSIOError(errno);
                return TRUE;
            }
            dev->StatusError = CIOStatSuccess;
            
            return TRUE;

        case 16:    // chdir
            printf("Received chdir() command.\n");

            if (!Link_Device_Resolve_Path(dev, TRUE, resultPath))
                return TRUE;

            if (strlen(resultPath) > 64) {
                dev->StatusError = CIOStatPathTooLong;
                return TRUE;
            }

            if (!Link_Device_Resolve_Native_Path(dev, nativePath, resultPath))
                return TRUE;

            if (((stat(nativePath, &file_stats)) == -1)) {
                dev->StatusError = TranslateErrnoToSIOError(errno);
                return TRUE;
            }

            if (!S_ISDIR(file_stats.st_mode)) {
                dev->StatusError = CIOStatPathNotFound;
                return TRUE;
            }

            dev->CurDir = resultPath;

            dev->StatusError = CIOStatSuccess;

            return TRUE;

        case 17:    // getcwd
            dev->StatusError = CIOStatSuccess;
            return TRUE;

        case 18:    // setboot
            dev->StatusError = CIOStatNotSupported;
            return TRUE;

        case 19:    // getdfree
                dev->StatusError = CIOStatSuccess;
            return TRUE;

        default:
            printf("Unsupported put for function $%02x\n", dev->ParBuf.Function);
            dev->StatusError = CIOStatNotSupported;
            return TRUE;
    }
}

int Link_Device_On_Read(LinkDevice *dev) {
    ULONG blocklen;
    DiskInfo diskInfo;
    FileHandle *fh;

    switch(dev->ParBuf.Function) {
        case 0:     // fread
            blocklen = dev->StatusLengthLo + ((ULONG)dev->StatusLengthHi << 8);
            if (Link_Device_Check_Valid_File_Handle(dev, TRUE)) {
                fh = &dev->FileHandles[dev->ParBuf.Handle - 1];
                ULONG actual = 0;

                dev->StatusError = File_Handle_Read(fh, dev->TransferBuffer, blocklen, actual);

                dev->StatusLengthLo = (UBYTE)actual;
                dev->StatusLengthHi = (UBYTE)(actual >> 8);
            }

            dev->ExpectedBytes = blocklen;
            return TRUE;

        case 1:     // fwrite
            if (dev->ReadOnly) {
                dev->StatusError = CIOStatReadOnly;
                return TRUE;
            }

            blocklen = dev->StatusLengthLo + ((ULONG)dev->StatusLengthHi << 8);
            if (Link_Device_Check_Valid_File_Handle(dev, TRUE)) {
                fh = &dev->FileHandles[dev->ParBuf.Handle - 1];

                dev->StatusError = File_Handle_Write(fh, dev->TransferBuffer, blocklen);
            }
            dev->ExpectedBytes = 0;
            return TRUE;

        case 3:     // ftell
            if (Link_Device_Check_Valid_File_Handle(dev, TRUE)) {
                fh = &dev->FileHandles[dev->ParBuf.Handle - 1];

                if (!File_Handle_Is_Open(fh)) {
                    dev->StatusError = CIOStatNotOpen;
                    return true;
                }

                const ULONG len = File_Handle_Get_Position(fh);
                dev->TransferBuffer[0] = (UBYTE)len;
                dev->TransferBuffer[1] = (UBYTE)(len >> 8);
                dev->TransferBuffer[2] = (UBYTE)(len >> 16);
                dev->StatusError = CIOStatSuccess;
            }
            dev->ExpectedBytes = 3;
            return TRUE;

        case 4:     // flen
            memset(dev->TransferBuffer, 0, 3);
            if (Link_Device_Check_Valid_File_Handle(dev, TRUE)) {
                fh = &dev->FileHandles[dev->ParBuf.Handle - 1];

                if (!File_Handle_Is_Open(fh))
                    dev->StatusError = CIOStatNotOpen;
                else {
                    ULONG len = File_Handle_Get_Length(fh);

                    dev->TransferBuffer[0] = (UBYTE)len;
                    dev->TransferBuffer[1] = (UBYTE)(len >> 8);
                    dev->TransferBuffer[2] = (UBYTE)(len >> 16);
                }
            }
            dev->ExpectedBytes = 3;
            return TRUE;

        case 5:     // reserved
            dev->ExpectedBytes = 0;
            dev->StatusError = CIOStatNotSupported;
            return TRUE;

        case 6:     // fnext
            memset(dev->TransferBuffer, 0, sizeof(DirEntry) + 1);

            if (Link_Device_Check_Valid_File_Handle(dev, TRUE)) {
                fh = &dev->FileHandles[dev->ParBuf.Handle - 1];

                if (!File_Handle_Is_Dir(fh)) {
                    dev->StatusError = CIOStatBadParameter;
                } else {
                    DirEntry dirEnt = {0};

                    if (!File_Handle_Get_Next_Dir_Ent(fh, &dirEnt))
                        dev->StatusError = CIOStatEndOfFile;
                    else
                        dev->StatusError = CIOStatSuccess;

                    memcpy(dev->TransferBuffer + 1, &dirEnt, sizeof(DirEntry));
                }
            }

            dev->TransferBuffer[0] = dev->StatusError;
            dev->ExpectedBytes = sizeof(DirEntry) + 1;
            return TRUE;

        case 7:     // fclose
        case 8:     // init
            dev->ExpectedBytes = 0;
            dev->StatusError = CIOStatNotSupported;
            return TRUE;

        case 9:     // open
        case 10:    // ffirst
            dev->TransferBuffer[0] = dev->ParBuf.Handle;
            memset(dev->TransferBuffer + 1, 0, sizeof(DirEntry));

            if (Link_Device_Check_Valid_File_Handle(dev, TRUE)) {
                fh = &dev->FileHandles[dev->ParBuf.Handle - 1];

                const DirEntry *dirEnt = File_Handle_Get_Dir_Ent(fh);

                dev->StatusError = CIOStatSuccess;

                memcpy(dev->TransferBuffer + 1, dirEnt, sizeof(DirEntry));
            }
            dev->ExpectedBytes = sizeof(DirEntry) + 1;
            return TRUE;

        case 11:    // rename
        case 12:    // remove
        case 13:    // chmod
        case 14:    // mkdir
        case 15:    // rmdir
        case 16:    // chdir
            dev->ExpectedBytes = 0;
            dev->StatusError = CIOStatNotSupported;
            return true;

        case 17:    // getcwd
            memset(dev->TransferBuffer, 0, 65);
            strncpy((char *)dev->TransferBuffer, dev->CurDir, 64);
            dev->ExpectedBytes = 64;
        return TRUE;

        case 18:    // setboot
            dev->StatusError = CIOStatNotSupported;
            dev->ExpectedBytes = 0;
            return TRUE;

        case 19:    // getdfree
            memset(&diskInfo, 0, sizeof(DiskInfo));

            diskInfo.InfoVersion = 0x21;
            diskInfo.SectorCountLo = 0xff;
            diskInfo.SectorCountHi = 0xff;
            diskInfo.SectorsFreeLo = 0xff;
            diskInfo.SectorsFreeHi = 0xff;
            diskInfo.BytesPerSectorCode = 1;
            diskInfo.Version = 0x80;
            diskInfo.BytesPerSectorHi = 2;
            diskInfo.SectorsPerCluster = 1;
            memcpy(diskInfo.VolumeLabel, "PCLink  ", 8);

            memcpy(dev->TransferBuffer, &diskInfo, 64);
            dev->ExpectedBytes = 64;
            return TRUE;

        case 20:    // chvol
            dev->StatusError = CIOStatNotSupported;
            dev->ExpectedBytes = 0;
            return TRUE;
    }

    return FALSE;
}

int Link_Device_Check_Valid_File_Handle(LinkDevice *dev, int setError) {
    if (dev->ParBuf.Handle == 0 || dev->ParBuf.Handle >= 16) {
        if (setError)
            dev->StatusError = CIOStatInvalidIOCB;

        return FALSE;
    }

    return TRUE;
}

int Link_Device_Is_Dir_Ent_Included(LinkDevice *dev, DirEntry *dirEnt) {
    return Dir_Entry_Test_Attr_Filter(dirEnt, dev->ParBuf.Attr1);
}

int Link_Device_Resolve_Path(LinkDevice *dev, int allowDir, char *resultPath) {
    const UBYTE *s = dev->ParBuf.Path;

    // check for absolute path
    if (*s == '>' || *s == '\\') {
        *resultPath = 0;
        ++s;
    } else
        resultPath = dev->CurDir;

    // check for back-up specifiers
    while(*s == '<') {
        ++s;

        while(strlen(resultPath) != 0) {
            int len = strlen(resultPath);
            UBYTE c = resultPath[len-1];

            resultPath[len-1] = 0;

            if (c == '\\')
                break;
        }
    }

    // parse out remaining components

    int inext = false;
    int fnchars = 0;
    int extchars = 0;
    UBYTE c;

    while((c = *s++)) {
        if (c == '>' || c == '\\') {
            if (inext && !extchars) {
                dev->StatusError = CIOStatFileNameErr;
                return FALSE;
            }

            if (fnchars)
                strcat(resultPath, "\\");

            inext = false;
            fnchars = 0;
            extchars = 0;
            continue;
        }

        if (c == '.') {
            if (!fnchars) {
                if (s[0] == '.' && (s[1] == 0 || s[1] == '>' || s[1] == '\\')) {
                    int len = strlen(resultPath);
                    // remove a component
                    if ((len != 0) && resultPath[len-1] == '\\')
                        resultPath[len-1] = 0;

                    while(!strlen(resultPath)) {
                        int len = strlen(resultPath);
                        UBYTE c = resultPath[len-1];

                        resultPath[len-1] = 0;

                        if (c == '\\')
                            break;
                    }

                    ++s;

                    if (!*s)
                        break;

                    ++s;
                    continue;
                }
            }

            if (inext) {
                dev->StatusError = CIOStatFileNameErr;
                return FALSE;
            }

            strcat(resultPath, ".");
            inext = TRUE;
            continue;
        }

        if (!fnchars)
            strcat(resultPath, "\\");

        if ((UBYTE)(c - 'a') < 26)
            c &= ~0x20;

        if (c != '_' && (UBYTE)(c - '0') >= 10 && (UBYTE)(c - 'A') >= 26) {
            dev->StatusError = CIOStatFileNameErr;
            return FALSE;
        }

        if (inext) {
            if (++extchars > 3) {
                dev->StatusError = CIOStatFileNameErr;
                return FALSE;
            }
        } else {
            if (++fnchars > 8) {
                dev->StatusError = CIOStatFileNameErr;
                return FALSE;
            }
        }

        strncat(resultPath, (const char *) &c, 1);
    }

    if (inext && !extchars && !allowDir) {
        dev->StatusError = CIOStatFileNameErr;
        return FALSE;
    }

    // strip off trailing separator if present
    if ((strlen(resultPath) != 0) && (resultPath[strlen(resultPath) - 1] != '/')) {
        resultPath[strlen(resultPath) - 1] = 0;
    }

    return TRUE;
}

int Link_Device_Resolve_Native_Path_Dir(LinkDevice *dev, int allowDir, char *resultPath) {
    char netPath[FILENAME_MAX];

    if (!Link_Device_Resolve_Path(dev, allowDir, netPath))
        return FALSE;

    return Link_Device_Resolve_Native_Path(dev, resultPath, netPath);
}

int Link_Device_Resolve_Native_Path(LinkDevice *dev, char *resultPath, const char *netPath) {

    // translate path
    resultPath = dev->BasePathNative;

    for (int i=0; i<strlen(netPath); i++)
    {
        char c = netPath[i];

        if (c >= 'A' && c <= 'Z')
            c &= ~0x20;

        resultPath += (wchar_t)c;   
    }

    // ensure trailing separator
    if ((strlen(resultPath) != 0) && (resultPath[strlen(resultPath) - 1] != '/')) {
        const UBYTE c = '/';
        strncat(resultPath, (const char *) &c, 1);
    }

    return TRUE;
}