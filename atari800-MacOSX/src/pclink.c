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

#include <stdlib.h>
#include "pclink.h"
#include "console.h"
#include "cio.h"
#include "cpu.h"
#include "kerneldb.h"
#include "uirender.h"
#include "debuggerlog.h"

ATDebuggerLogChannel g_ATLCPCLink(false, false, "PCLINK", "PCLink activity");

uint8 ATTranslateWin32ErrorToSIOError(uint32 err);

///////////////////////////////////////////////////////////////////////////

typedef struct diskInfo {
    uint8   InfoVersion;
    uint8   RootDirLo;
    uint8   RootDirHi;
    uint8   SectorCountLo;
    uint8   SectorCountHi;
    uint8   SectorsFreeLo;
    uint8   SectorsFreeHi;
    uint8   VTOCSectorCount;
    uint8   VTOCSectorStartLo;
    uint8   VTOCSectorStartHi;
    uint8   NextFileSectorLo;
    uint8   NextFileSectorHi;
    uint8   NextDirSectorLo;
    uint8   NextDirSectorHi;
    uint8   VolumeLabel[8];
    uint8   TrackCount;
    uint8   BytesPerSectorCode;
    uint8   Version;
    uint8   BytesPerSectorLo;
    uint8   BytesPerSectorHi;
    uint8   MapSectorCountLo;
    uint8   MapSectorCountHi;
    uint8   SectorsPerCluster;
    uint8   NoSeq;
    uint8   NoRnd;
    uint8   BootLo;
    uint8   BootHi;
    uint8   WriteProtectFlag;
    uint8   Pad[29];
} DiskInfo;

typdef struct dirEntry {
    enum : uint8 {
        Flag_OpenForWrite  = 0x80,
        Flag_Directory     = 0x20,
        Flag_Deleted       = 0x10,
        Flag_InUse         = 0x08,
        Flag_Archive       = 0x04,
        Flag_Hidden        = 0x02,
        Flag_Locked        = 0x01
    };

    enum : uint8 {
        AttrMask_NoSubDir      = 0x80,
        AttrMask_NoArchived    = 0x40,
        AttrMask_NoHidden      = 0x20,
        AttrMask_NoLocked      = 0x10,
        AttrMask_OnlySubDir    = 0x08,
        AttrMask_OnlyArchived  = 0x04,
        AttrMask_OnlyHidden    = 0x02,
        AttrMask_OnlyLocked    = 0x01,
    };

    uint8   Flags;
    uint8   SectorMapLo;
    uint8   SectorMapHi;
    uint8   LengthLo;
    uint8   LengthMid;
    uint8   LengthHi;
    uint8   Name[11];
    uint8   Day;
    uint8   Month;
    uint8   Year;
    uint8   Hour;
    uint8   Min;
    uint8   Sec;
} DirEntry;

enum {
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

int Dir_Entry_Test_Attr_Filter(DirEntry *entry, uint8 attrFilter) const {
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

int Dir_Entry_Compare(DirEntry *x, DirEntry *y) {
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

void Dir_Entry_Decode_Date(const uint8 tsdata[6], struct tm* fileExpTime) {
    fileExpTime->tm_mday = tsdata[0];
    fileExpTime->tm_mon = tsdata[1];
    fileExpTime->tm_year = tsdata[2] < 50 ? 2000 + tsdata[2] : 1900 + tsdata[2];
    fileExpTime->tm_hour = tsdata[3];
    fileExpTime->tm_min = tsdata[4];
    fileExpTime->tm_sec = tsdata[5];
 }

typedef struct fileName {
    uint8   Name[11];
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
    return (memcmp(name->Name, second->Name, 11) == 0)
}

int File_Name_Parse_From_Net(FileName *name, const uint8 *fn) {
    uint8 fill = 0;

    for(int i=0; i<11; ++i) {
        if (i == 8)
            fill = 0;

        if (fill)
            name->Name[i] = fill;
        else {
            uint8 c = fn[i];

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

bool File_Name_Parse_From_Native(FileName *name, const char *fn) {
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
            name->Name[i++] = (uint8)(c - 0x20);
        else if ((c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9') || c == L'_')
            mName[i++] = (uint8)c;
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

int File_Name_Wild_Match(FileName *name, File_Name *fn)  
{
    for(int i=0; i<11; ++i) {
        const uint8 c = name->Name[i];
        const uint8 d = fn->mName[i];

        if (c != d && c != '?')
            return FALSE;
    }

    return TRUE;
}

void File_Name_Wild_Merge(FileName *name, const File_Name *fn)
{
    for(int i=0; i<11; ++i) {
        const uint8 d = fn->Name[i];

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

uint8 TranslateErrnoToSIOError(uint32 err) {
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
        case ELOOP:
        case EMFILE:
        case ENAMETOOLONG:
        case ENFILE:
        case ELOOP:
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
        case ERROR_FILE_NOT_FOUND:
            return CIOStatFileNotFound;

        case ERROR_PATH_NOT_FOUND:
            return CIOStatPathNotFound;

        case ERROR_FILE_EXISTS:
        case ERROR_ALREADY_EXISTS:
            return CIOStatFileExists;

        case ERROR_DISK_FULL:
            return CIOStatDiskFull;

        case ERROR_DIR_NOT_EMPTY:
            return CIOStatDirNotEmpty;

        case ERROR_ACCESS_DENIED:
            return CIOStatAccessDenied;

        case ERROR_SHARING_VIOLATION:
            return CIOStatFileLocked;

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
    uint32   Pos;
    uint32   Length;

    DirEntry *dirEnts;

    DirEntry *dirEnt;
    FileName *FnextPattern;
    uint8    FnextAttrFilter;

    File     *File;
} FileHandle;

FileHandle *File_Handle_Alloc()
{
    FileHandle *hndl = (FileHandle *) malloc(sizoef(FileHandle));
    memset(hndl, 0, sizeof(FileHandle));
    hndl->dirEnts = vector_create();
    return hndl;
}

void File_Handle_Free(FileHandle *hndl)
{
    vector_free(hndl->dirEnts);
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

void File_Handle_Set_Dir_Ent(FileHandle *hndl, const DirEnt* dirEnt)
{
    hndl->DirEnt = dirEnt;
}

void File_Handle_Add_Dir_Ent(FileHandle *hndl, DirEnt* dirEnt) {
    vector_add(&hndl->DirEnts, dirEnt);
}

uint8 File_Handle_Open_File(FileHandle *hndl, const char *nativePath, 
                            FileOpenType openType, int allowRead, 
                            int allowWrite, int append) 
{
    FILE *file;

    int exists = access(nativePath, F_OK);

    hndl->WasCreated = FALSE;

    switch (openType) {
        case FILE_READ:
            file = fopen(nativePath, "r");
            break
        case FILE_WRITE:
            break
            file = fopen(nativePath, "w");
        case FILE_APPEND:
            file = fopen(nativePath, "a");
            break
        case FILE_UPDATE:
            file = fopen(nativePath, "r+");
            break
    }

    if (file) {
        hndl->Open = TRUE;
        hndl->IsDirectory = FALSE;
        hndl->AllowRead = allowRead;
        hndl->AllowWrite = allowWrite;
        hndl->WasCreated = !exists;

        sint64 len = hndl->File.size();
        if (len > 0xffffff)
            hndl->Length = 0xffffff;
        else
            hndl->Length = (uint32)len;

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
                                   uint8 attrFilter)
{
    qsort(hndl->DirEnts,
          vector_size(hdnl->DirEnts),
          sizeof(DirEntry),
          Dir_Ent_Compare 
         );
    hndl->Open = TRUE;
    hndl->IsDirectory = TRUE;
    hndl->Length = 23 * ((uint32)cvector_size(hndl->DirEnts) + 1);
    hndl->Pos = 23;
    hndl->AllowRead = TRUE;
    hndl->AllowWrite = FALSE;

    memset(hndl->DirEnt, 0, sizeof(DirEnt));
    hndl->DirEnt->Flags = Flag_InUse | Flag_Directory;
    hndl->DirEnt->LengthLo = (uint8)hndl->Length;
    hndl->DirEnt->LengthMid = (uint8)(hndl->Length >> 8);
    hndl->DirEnt->LengthHi = (uint8)(hndl->Length >> 16);
    memcpy(hndl->DirEnt->Name, dirName->Name, 11);

    vector_insert(&hndl->DirEnts, 0, hndl->DirEnt);

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

uint8 File_Handle_Seek(FileHandle *hndl, uint32 pos) {
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

uint8 File_Handle_Read(FileHandle *hndl, void *dst, uint32 len, uint32 *actual) {
    *actual = 0;

    if (!hndl->Open)
        return CIOStatNotOpen;

    if (!hndl->AllowRead)
        return CIOStatWriteOnly;

    long act = 0;

    if (hndl->Pos < hndl->Length) {
        if (hndl->IsDirectory) {
            uint32 dirIndex = hndl->Pos / 23;
            uint32 offset = hndl->Pos % 23;
            uint32 left = hndl->Length - hndl->Pos;

            if (left > len)
                left = len;
        
            while(left > 0) {
                const uint8 *src = (const uint8 *)&mDirEnts[dirIndex];
                uint32 tc = 23 - offset;
                if (tc > left)
                    tc = left;

                memcpy((char *)dst + act, src + offset, tc);
                left -= tc;
                act += tc;
                offset = 0;
                ++dirIndex;
            }
        } else {
            uint32 tc = len;

            if (hndl->Length - hndl->Pos < tc)
                tc = hndl->Length - hndl->Pos;

            act = fread(dst, 1, tc, hndl->File);
            if (act < 0)
                return CIOStatFatalDiskIO;
        }
    }

    *actual = (uint32)act;

    printf("Read at pos %d/%d, len %d, actual %d\n", mPos, mLength, len, *actual);

    hndl->Pos += *actual;

    if (actual < len) {
        memset((char *)dst + *actual, 0, len - *actual);
        return CIOStatTruncRecord;
    }

    return CIOStatSuccess;
}

uint8 File_Handle_Write(FileHandle *hndl, const void *dst, uint32 len) {
    if (!hndl->Open)
        return CIOStatNotOpen;

    if (!hndl->AllowWrite)
        return CIOStatReadOnly;

    uint32 tc = len;
    uint32 actual = 0;

    if (hndl->Pos < 0xffffff) {
        if (0xffffff - hndl->Pos < tc)
            tc = 0xffffff - mPos;

        actual = fwrite(dst, 1, tc, hndl->File);

        if (actual != tc) {
            fseek(hdnl->File, hndl->Pos, SEEK_SET);
            return CIOStatFatalDiskIO;
        }
    }

    printf("Write at pos %d/%d, len %d, actual %d\n", hndl->Pos, hndl->Length, len, actual);

    hndl->Pos += actual;
    if (hndl->Pos > hndl->Length)
        hndl->Length = hndl->Pos;

    return actual != len ? CIOStatDiskFull : CIOStatSuccess;
}

void File_Handle_Set_Timestamp(FileHandle *hndl, const uint8 *tsdata) {
    struct timeval fileTime[2];
    struct tm fileExpTime;

    Dir_Entry_Decode_Date(tsdata, &fileExpTime);

    fileTime[0].tv_sec = mktime(&fileExpTime);
    fileTime[0].tv_usec = 0;
    fileTime[1].tv_sec = fileTime[0].tv_sec;
    fileTime[1].tv_usec = fileTime[0].tv_usec;
    utimes (hndl->nativePath, fileTime);
}

bool File_Handle_Get_Next_Dir_Ent(FileHandle *hndl, DirEntry *dirEnt) {
    uint32 actual;

    while(File_Handle_Read(hndl, dirEnt, 23, &actual) == CIOStatSuccess && actual >= 23) {
        File_Name *name = File_Name_Alloc();
        File_Name_Parse_From_Net(name, dirEnt->Name);

        if (File_Name_Wild_Match(name, hndl->FnextPattern) && Dir_Entry_Test_Attr_Filter(hndl->dirEnt, hndl->FnextAttrFilter))
            return TRUE;
    }

    return FALSE;
}

typedef struct parameterBuffer {
        uint8   Function;  // function number
        uint8   Handle;    // file handle
        uint8   F[6];
        uint8   Mode;      // file open mode
        uint8   Attr1;
        uint8   Attr2;
        uint8   Name1[12];
        uint8   Name2[12];
        uint8   Path[65];
    } ParameterBuffer;

enum Command {
    CommandNone,
    CommandGetHiSpeedIndex,
    CommandStatus,
    CommandPut,
    CommandRead
};

typedef struct linkDevice {
    SIOManager *SIOMgr;
    DeviceIndicatorManager *UIRenderer;

    char    *BasePathNative;
    bool    ReadOnly = FALSE;
    bool    SetTimestamps = FALSE;

    vdfunction<void(const void *, uint32)> mpReceiveFn;
    vdfunction<void()> mpFenceFn;

    uint8   StatusFlags = 0;
    uint8   StatusError = 0;
    uint8   StatusLengthLo = 0;
    uint8   StatusLengthHi = 0;

    Command Command = CommandNone;
    uint32  CommandPhase = 0;
    uint8   CommandAux1 = 0;
    uint8   CommandAux2 = 0;

    char    *CurDir;

    ParameterBuffer ParBuf;

    FileHandle FileHandles[15];

    uint8   TransferBuffer[65536];
} LinkDevice;

///////////////////////////////////////////////////////////////////////////

class ATPCLinkDevice final : public IATPCLinkDevice
            , public ATDevice
            , public IATDeviceSIO
            , public IATDeviceIndicators
            , public IATDeviceDiagnostics
{
    ATPCLinkDevice(const ATPCLinkDevice&) = delete;
    ATPCLinkDevice& operator=(const ATPCLinkDevice&) = delete;
public:
    ATPCLinkDevice();
    ~ATPCLinkDevice();

    void *AsInterface(uint32 id) override;

    bool IsReadOnly() { return mbReadOnly; }
    void SetReadOnly(bool readOnly);

    const wchar_t *GetBasePath() { return mBasePathNative.c_str(); }
    void SetBasePath(const wchar_t *basePath);

public:
    void GetDeviceInfo(ATDeviceInfo& info) override;
    void GetSettings(ATPropertySet& settings) override;
    bool SetSettings(const ATPropertySet& settings) override;
    void Shutdown() override;
    void ColdReset() override;

public:
    void InitIndicators(IATDeviceIndicatorManager *uir) override;

public:
    void InitSIO(IATDeviceSIOManager *mgr) override;
    CmdResponse OnSerialBeginCommand(const ATDeviceSIOCommand& cmd) override;
    void OnSerialAbortCommand() override;
    void OnSerialReceiveComplete(uint32 id, const void *data, uint32 len, bool checksumOK) override;
    void OnSerialFence(uint32 id) override;
    CmdResponse OnSerialAccelCommand(const ATDeviceSIORequest& request) override;

public:
    void DumpStatus(ATConsoleOutput& output) override;

protected:

    void AbortCommand();
    void BeginCommand(Command cmd);
    void AdvanceCommand();
    void FinishCommand();

    bool OnPut();
    bool OnRead();

    bool CheckValidFileHandle(bool setError);
    bool IsDirEntIncluded(const ATPCLinkDirEnt& dirEnt) const;
    bool ResolvePath(bool allowDir, VDStringA& resultPath);
    bool ResolveNativePath(bool allowDir, VDStringW& resultPath);
    bool ResolveNativePath(VDStringW& resultPath, const VDStringA& netPath);
    void OnReadActivity();
    void OnWriteActivity();

};

LinkDevice *Link_Device_Alloc() {
    LinkDevice *dev;

    dev = malloc(sizof(LinkDevice));
    memset(dev, 0, sizeof(LinkDevice));
    return dev;
}

void Link_Device_Free(LinkDevice *dev) {
    free(dev);
}

void Link_Device_Set_Read_Only(LinkDevice *dev, int readOnly) {
    dev->ReadOnly = readOnly;
}

void Link_Device_Set_Base_Path(LinkDevice *dev, const char *basePath) {
    dev->BasePathNative = basePath;
}

void Link_Device_Set_Settings(LinkDevice *dev, int setTimestamps, int readOnly, char *basePath) {
    dev->SetTimestamps = SetTimestamps;
    Link_Device_Set_Read_Only(dev, readOnly);
    Link_Device_Set_Base_Path(basePath);
}

void Link_Device_Shutdown(LinkDevice *dev) {
    Link_Device_Abort_Command(dev);

    UIRenderer = NULL;

    SIOMgr = NULL;
}

void Link_Device_Cold_Reset(LinkDevice *dev) {
    for (i = 0; i < 15; i++)
        File_Handle_Close(dev->FileHandles[i]);
}

IATDeviceSIO::CmdResponse ATPCLinkDevice::OnSerialBeginCommand(const ATDeviceSIOCommand& cmd) {
    if (cmd.mDevice != 0x6F)
        return kCmdResponse_NotHandled;

    if (mBasePathNative.empty())
        return kCmdResponse_NotHandled;

    if (!cmd.mbStandardRate) {
        if (cmd.mCyclesPerBit < 30 || cmd.mCyclesPerBit > 34)
            return kCmdResponse_NotHandled;
    }

    const uint8 commandId = cmd.mCommand & 0x7f;

    mCommandAux1 = cmd.mAUX[0];
    mCommandAux2 = cmd.mAUX[1];

    Command command = kCommandNone;

    if (commandId == 0x53)          // status
        command = kCommandStatus;
    else if (commandId == 0x50)     // put
        command = kCommandPut;
    else if (commandId == 0x52)     // read
        command = kCommandRead;
    else if (commandId == 0x3F)
        command = kCommandGetHiSpeedIndex;
    else {
        g_ATLCPCLink("Unsupported command $%02x\n", cmd);
        return kCmdResponse_Fail_NAK;
    }

    mpSIOMgr->BeginCommand();

    // High-speed via bit 7 uses 38400 baud.
    // High-speed via HS command frame uses 52Kbaud. Currently we use US Doubler timings.
    if (cmd.mCommand & 0x80)
        mpSIOMgr->SetTransferRate(45, 450);
    else if (!cmd.mbStandardRate)
        mpSIOMgr->SetTransferRate(34, 394);

    mpSIOMgr->SendACK();

    BeginCommand(command);
    return kCmdResponse_Start;
}

void ATPCLinkDevice::OnSerialAbortCommand() {
    AbortCommand();
}

void ATPCLinkDevice::OnSerialReceiveComplete(uint32 id, const void *data, uint32 len, bool checksumOK) {
    mpReceiveFn(data, len);
}

void ATPCLinkDevice::OnSerialFence(uint32 id) {
    mpFenceFn();
}

IATDeviceSIO::CmdResponse ATPCLinkDevice::OnSerialAccelCommand(const ATDeviceSIORequest& request) {
    return OnSerialBeginCommand(request);
}

void ATPCLinkDevice::BeginCommand(Command cmd) {
    mCommand = cmd;
    mCommandPhase = 0;

    AdvanceCommand();
}

void ATPCLinkDevice::AbortCommand(LinkDevice *dev) {
    if (dev->Command) {
        dev->Command = CommandNone;
        dev->CommandPhase = 0;
    }
}

void ATPCLinkDevice::AdvanceCommand() {
    switch(mCommand) {
        case kCommandGetHiSpeedIndex:
            g_ATLCPCLink("Sending high-speed index\n");
            mpSIOMgr->SendComplete();
            {
                uint8 hsindex = 9;
                mpSIOMgr->SendData(&hsindex, 1, true);
            }
            mpSIOMgr->EndCommand();
            break;

        case kCommandStatus:
            g_ATLCPCLink("Sending status: Flags=$%02x, Error=%3d, Length=%02x%02x\n", mStatusFlags, mStatusError, mStatusLengthHi, mStatusLengthLo);
            mpSIOMgr->SendComplete();
            {
                const uint8 data[4] = {
                    mStatusFlags,
                    mStatusError,
                    mStatusLengthLo,
                    mStatusLengthHi
                };

                mpSIOMgr->SendData(data, 4, true);
            }
            mpSIOMgr->EndCommand();
            break;

        case kCommandPut:
            mpReceiveFn = [this](const void *src, uint32 len) {
                memcpy(&mParBuf, src, std::min<uint32>(len, sizeof(mParBuf)));
            };
            mpSIOMgr->ReceiveData(0, mCommandAux1 ? mCommandAux1 : 256, true);
            mpFenceFn = [this]() {
                if (OnPut())
                    mpSIOMgr->SendComplete();
                else
                    mpSIOMgr->SendError();
                mpSIOMgr->EndCommand();
            };
            mpSIOMgr->InsertFence(0);
            break;

        case kCommandRead:
            // fwrite ($01) is special
            if (mParBuf.mFunction == 0x01) {
                mpReceiveFn = [this](const void *src, uint32 len) {
                    memcpy(mTransferBuffer, src, len);
                };
                mpSIOMgr->ReceiveData(0, mParBuf.mF[0] + ((uint32)mParBuf.mF[1] << 8), true);
                mpSIOMgr->InsertFence(0);
                mpFenceFn = [this]() {
                    OnRead();
                    mpSIOMgr->EndCommand();
                };
                mpSIOMgr->SendComplete();
            } else {
                mpSIOMgr->SendComplete();
                mpFenceFn = [this]() {
                    OnRead();
                    mpSIOMgr->EndCommand();
                };
                mpSIOMgr->InsertFence(0);
            }

            break;
    }
}

void Link_Device_Finish_Command() {
    Link_Device_Abort_Command();
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
    char srcNativePath[FILENAME_MAX];
    DIR * dirStream;
    DirEntry dirEnt;
    FileHandle* fh;
    FileName dirName;
    FileName dstpat;
    FileName entryName;
    FileName fn;
    FileName fn2;
    FileName pattern;
    FileName srcpat;
    int openDir;
    int setTimestamp;
    int matched;
    mode_t newmode;
    sint64 slen;
    size_t fnlen;
    size_t extlen;
    struct dirent *ep;
    struct stat file_stats;
    struct timeval fileTime[2];
    struct tm fileExpTime;
    uint32 bufLen;
    uint32 len;
    uint32 pos;

    switch(dev->ParBuf.Function) {
        case 0:     // fread
            bufLen = dev->ParBuf.F[0] + 256*dev->ParBuf.F[1];
            printf("Received fread($%02x,%d) command.\n", dev->ParBuf.Handle, bufLen);

            if (Link_Device_Check_Valid_File_Handle(TRUE)) {
                fh = &FileHandles[dev->ParBuf.Handle - 1];

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

                dev->StatusLengthLo = (uint8)bufLen;
                dev->StatusLengthHi = (uint8)(bufLen >> 8);
                dev->StatusError = bufLen ? CIOStatSuccess : CIOStatEndOfFile;
                }
            return TRUE;

        case 1:     // fwrite
            if (dev->ReadOnly) {
                dev->StatusError = CIOStatReadOnly;
                return TRUE;
            }

            bufLen = dev->ParBuf.F[0] + 256*dev->ParBuf.m1];
            printf("Received fwrite($%02x,%d) command.\n", dev->ParBuf.Handle, bufLen);

            if (Link_Device_Check_Valid_File_Handle(TRUE)) {
                fh = &FileHandles[dev->ParBuf.Handle - 1];

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

                dev->StatusLengthLo = (uint8)bufLen;
                dev->StatusLengthHi = (uint8)(bufLen >> 8);
                dev->StatusError = bufLen ? CIOStatSuccess : CIOStatDiskFull;
                }
            return TRUE;

        case 2:     // fseek
            uint32 pos = dev->ParBuf.F[0] + ((uint32)dev->ParBuf.F[1] << 8) + ((uint32)dev->ParBuf.F[2] << 16);
            printf("Received fseek($%02x,%d) command.\n", mParBuf.mHandle, pos);

            if (Link_Device_Check_Valid_File_Handle(TRUE)) {
                fh = &FileHandles[dev->ParBuf.Handle - 1];

                if (!File_Handle_Is_Open(fh)) {
                    dev->StatusError = CIOStatNotOpen;
                    return TRUE;
                }

                dev->StatusError = fFile_Handle_Seek(fh, pos);
                }
            return TRUE;

        case 3:     // ftell
            printf("Received ftell($%02x) command.\n", dev->ParBuf.Handle);

            if (!Link_Device_Check_Valid_File_Handle(TRUE))
                return TRUE;

            fh = &FileHandles[dev->ParBuf.Handle - 1];

            if (!File_Handle_Is_Open(fh)) {
                dev->StatusError = CIOStatNotOpen;
                return TRUE;
            }

            dev->StatusError = CIOStatSuccess;
            return TRUE;

        case 4:     // flen
            if (!Link_Device_Check_Valid_File_Handle(TRUE))
                return TRUE;

            dev->StatusError = CIOStatSuccess;
            return TRUE;

        case 5:     // reserved
            dev->StatusError = CIOStatNotSupported;
            return TRUE;

        case 6:     // fnext
            printf("Received fnext($%02x) command.\n", dev->ParBuf.Handle);
            if (!Link_Device_Check_Valid_File_Handle(TRUE))
                return TRUE;

            dev->StatusError = CIOStatSuccess;
            return TRUE;

        case 7:     // fclose
            printf("Received close($%02x) command.\n", dev->ParBuf.Handle);
            if (Link_Device_Check_Valid_File_Handle(FALSE))
                File_Handle_Close(&FileHandles[dev->ParBuf.Handle - 1]);

            dev->StatusError = CIOStatSuccess;
            return TRUE;

        case 8:     // init
            printf("Received init command.\n");
            for(size_t i = 0; i < sizeof(dev->FileHandles)/sizeof(dev->FileHandles[0]); ++i)
                File_Handle_Close(&FileHandles[i]);

            dev->StatusFlags = 0;
            dev->StatusError = CIOStatSuccess;
            dev->StatusLengthLo = 0;
            dev->StatusLengthHi = 0x6F;

            dev->CurDir.clear();
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

                if (!File_Handle_Is_Open(dev->FileHandles[dev->ParBuf.Handle - 1]))
                    break;

                ++dev->ParBuf.Handle;
            }

            fh = dev->FileHandles[dev->ParBuf.Handle - 1];

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
            while(ep = readdir(dirStream)) {
                if (!File_Name_Parse_From_Native(fn, ep->d_name));
                    continue;

                // We can't filter at this point for a directory, because the byte stream
                // needs to reflect all files while the FNEXT output shouldn't. Therefore,
                // we need to cache the pattern with the file handle instead.
                if (!openDir && !File_Name_Wild_Match(pattern, fn))
                    continue;

                if ((stat(ep->d_name, &file_stats)) == -1) {
                    //TBD handle error;
                }
                slen = file_stats->st_size;

                if (slen > 0xFFFFFF)
                    slen = 0xFFFFFF;

                Dir_Entry_Set_Flags_From_Attributes(&dirEnt,
                    file_stats.st_mode);

                if (ep->d_type == D_DIR) {
                    // skip blasted . and .. dirs
                    if ((strcmp(ep->d_name,".") == 0) ||
                        (strcmp(ep->d_name,"..") == 0))
                        continue;
                }

                if (!Link_Device_Is_Dir_Ent_Included(dev, &dirEnt))
                    continue;

                dirEnt.SectorMapLo = 0;
                dirEnt.SectorMapHi = 0;
                dirEnt.LengthLo = (uint8)slen;
                dirEnt.LengthMid = (uint8)((uint32)slen >> 8);
                dirEnt.LengthHi = (uint8)((uint32)slen >> 16);
                memcpy(dirEnt.Name, fn.Name, sizeof(dirEnt.Name));
                Dir_Entry_Set_Date(&dirEnt, ep->st_mtime);

                if (!openDir) {
                    matched = TRUE;
                    nativeFilePath = it.GetFullPath();
                    break;
                }

                File_Handle_Add_Dir_Ent(&fh, &dirEnt);
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

                File_Handle_Open_As_Directory(&fh, &dirName, pattern, dev->ParBuf.Attr1);

                StatusError = CIOStatSuccess;
            } else {
                if (!matched) {
                    // cannot create file with a wildcard
                    if (!(dev->ParBuf.Mode & 4) && File_Name_Is_Wild(&pattern)) {
                        dev->StatusError = CIOStatIllegalWild;
                        return TRUE;
                    }

                    dev->nativeFilePath = nativePath;
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
                                dev->StatusError = File_Handle_Open_File(&fh, nativeFilePath,
                                    FILE_READ, TRUE, FALSE, FALSE);
                            }
                            break;

                        case 8:     // write
                            dev->StatusError = File_Handle_Open_File(&fh, nativeFilePath,
                                FILE_WRITE, FALSE, TRUE, FALSE);
                            dev->setTimestamp = TRUE;
                            break;

                        case 9:     // append
                            dev->StatusError = File_Handle_Open_File(&fh, nativeFilePath,
                                FILE_APPEND, FALSE, TRUE, TRUE);

                            if (File_Handle_Was_Created(&fh))
                                dev->setTimestamp = true;
                            break;

                        case 12:    // update
                            if (!matched)
                                dev->StatusError = CIOStatFileNotFound;
                            else {
                                dev->StatusError = File_Handle_Open_File(&fh, nativeFilePath,
                                    FILE_UPDATE, TRUE, TRUE, FALSE);
                            }
                            break;

                        default:
                            dev->StatusError = CIOStatInvalidCmd;
                            break;
                    }

                    if (dev->SetTimestamps && dev->setTimestamp && File_Handle_Is_Open(&fh)) {
                        File_Handle_Set_Timestamp(&fh, dev->ParBuf.F);
                    }
                }

                File_Handle_Set_Dir_Ent(&fh, &dirEnt);
            }
            return TRUE;

        case 11:    // rename
            printf("Received rename() command.\n");

            if (dev->ReadOnly) {
                dev->StatusError = CIOStatReadOnly;
                return TRUE;
            }

            if (!File_Name_Parse_From_Net(&srcpat, dev->ParBuf.Name1)
                || !File_Name_Parse_From_Net(&dstpat, dev->ParBuf.mName2))
            {
                dev->StatusError = CIOStatFileNameErr;
                return TRUE;
            }

            if ((dirStream = opendir(path)) == NULL) {
                // TBD Handle Error
            }

            matched = FALSE;
            while(ep = readdir(dirStream)) {
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
                File_Name_Append_Native(&fn, &srcNativePath);

                strcpy(dstNativePath, path);
                File_Name_Append_Native(&fn2, &dstNativePath);

                if (rename(srcNativePath, dstNativePath))
                    {
                    dev->statusError = TranslateErrnoToSIOError(errno);
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

            if (!Link_Device_Resolve_Native_Path(dev, FALSE, resultPath))
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
            while(ep = readdir(dirStream)) {
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
                    dev->statusError = TranslateErrnoToSIOError(errno);
                    return TRUE;
                    }
                matched = TRUE;
            }

            if (!matched) {
                dev->StatusError = CIOStatFileNotFound;
                return TRUE;
                }
            } else {
                File_Name_Append_Native(&fname, &resultPath);

                if ((stat(resultPath, &file_stats)) == -1) {
                    dev->StatusError = CIOStatFileNotFound;
                    return TRUE;
                }

                Dir_Ent_Set_Flags_From_Attributes(&dirEnt, file_stats.st_mode);

                if (Link_Device_Is_Dir_Ent_Included(dev, &dirEnt))
                    if (remove(resultPath)) {
                        dev->statusError = TranslateErrnoToSIOError(errno);
                        return TRUE;
                    }
                else {
                    dev->StatusError = CIOStatFileNotFound;
                    return TRUE;
                }

            dev->StatusError = CIOStatSuccess;
            }
            return TRUE;

        case 13:    // chmod
            print("Received chmod() command.\n");

            if (dev->ReadOnly) {
                dev->StatusError = CIOStatReadOnly;
                return TRUE;
            }

            if (!Link_Device_ResolveNativePath(dev, FALSE, path))
                return TRUE;

            if (!File_Name_Parse_From_Net(&srcpat, dev->ParBuf.Name1)) {
                dev->StatusError = CIOStatFileNameErr;
                return TRUE;
            }

            if ((dirStream = opendir(resultPath)) == NULL) {
                // TBD Handle Error
            }

            matched = FALSE;
            while(ep = readdir(dirStream)) {
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
                    dev->statusError = TranslateErrnoToSIOError(errno);
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

            if (!Link_Device_Resolve_Native_Path(dev, FALSE, resultPath))
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
                dev->statusError = TranslateErrnoToSIOError(errno);
                return TRUE;
            }

            if (dev->SetTimestamps) {
                Dir_Entry_Decode_Date(dev->ParBuf.F, &fileExpTime);

                fileTime[0].tv_sec = mktime(&fileExpTime);
                fileTime[0].tv_usec = 0;
                fileTime[1].tv_sec = fileTime[0].tv_sec;
                fileTime[1].tv_usec = fileTime[0].tv_usec;
                if (utimes (resultPath, fileTime)) {
                    dev->statusError = TranslateErrnoToSIOError(errno);
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

            if (!Link_Device_ResolveNativePath(dev, FALSE, resultPath))
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
                dev->statusError = TranslateErrnoToSIOError(errno);
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

            if (((stat(nativePath, &file_stats)) == -1) {
                dev->statusError = TranslateErrnoToSIOError(errno);
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
            devv->StatusError = CIOStatSuccess;
            return TRUE;
    }

    printf("Unsupported put for function $%02x\n", dev->ParBuf.Function);
    dev->StatusError = CIOStatNotSupported;
    return TRUE;
}

bool ATPCLinkDevice::OnRead() {
    switch(mParBuf.mFunction) {
        case 0:     // fread
            OnReadActivity();
            {
                uint32 blocklen = mStatusLengthLo + ((uint32)mStatusLengthHi << 8);
                if (CheckValidFileHandle(true)) {
                    ATPCLinkFileHandle& fh = mFileHandles[mParBuf.mHandle - 1];
                    uint32 actual = 0;

                    mStatusError = fh.Read(mTransferBuffer, blocklen, actual);

                    mStatusLengthLo = (uint8)actual;
                    mStatusLengthHi = (uint8)(actual >> 8);
                }

                mpSIOMgr->SendData(mTransferBuffer, blocklen, true);
            }
            return true;

        case 1:     // fwrite
            if (mbReadOnly) {
                mStatusError = ATCIOSymbols::CIOStatReadOnly;
                return true;
            }

            OnWriteActivity();
            {
                uint32 blocklen = mStatusLengthLo + ((uint32)mStatusLengthHi << 8);
                if (CheckValidFileHandle(true)) {
                    ATPCLinkFileHandle& fh = mFileHandles[mParBuf.mHandle - 1];

                    mStatusError = fh.Write(mTransferBuffer, blocklen);
                }
            }
            return true;

        case 3:     // ftell
            if (CheckValidFileHandle(true)) {
                ATPCLinkFileHandle& fh = mFileHandles[mParBuf.mHandle - 1];

                if (!fh.IsOpen()) {
                    mStatusError = ATCIOSymbols::CIOStatNotOpen;
                    return true;
                }

                const uint32 len = fh.GetPosition();
                mTransferBuffer[0] = (uint8)len;
                mTransferBuffer[1] = (uint8)(len >> 8);
                mTransferBuffer[2] = (uint8)(len >> 16);
                mStatusError = ATCIOSymbols::CIOStatSuccess;
            }
            mpSIOMgr->SendData(mTransferBuffer, 3, true);
            return true;

        case 4:     // flen
            memset(mTransferBuffer, 0, 3);
            if (CheckValidFileHandle(true)) {
                ATPCLinkFileHandle& fh = mFileHandles[mParBuf.mHandle - 1];

                if (!fh.IsOpen())
                    mStatusError = ATCIOSymbols::CIOStatNotOpen;
                else {
                    uint32 len = fh.GetLength();

                    mTransferBuffer[0] = (uint8)len;
                    mTransferBuffer[1] = (uint8)(len >> 8);
                    mTransferBuffer[2] = (uint8)(len >> 16);
                }
            }
            mpSIOMgr->SendData(mTransferBuffer, 3, true);
            return true;

        case 5:     // reserved
            mStatusError = ATCIOSymbols::CIOStatNotSupported;
            return true;

        case 6:     // fnext
            OnReadActivity();
            memset(mTransferBuffer, 0, sizeof(ATPCLinkDirEnt) + 1);

            if (CheckValidFileHandle(true)) {
                ATPCLinkFileHandle& fh = mFileHandles[mParBuf.mHandle - 1];

                if (!fh.IsDir()) {
                    mStatusError = ATCIOSymbols::CIOStatBadParameter;
                } else {
                    ATPCLinkDirEnt dirEnt = {0};

                    if (!fh.GetNextDirEnt(dirEnt))
                        mStatusError = ATCIOSymbols::CIOStatEndOfFile;
                    else
                        mStatusError = ATCIOSymbols::CIOStatSuccess;

                    memcpy(mTransferBuffer + 1, &dirEnt, sizeof(ATPCLinkDirEnt));
                }
            }

            mTransferBuffer[0] = mStatusError;
            mpSIOMgr->SendData(mTransferBuffer, sizeof(ATPCLinkDirEnt) + 1, true);
            return true;

        case 7:     // fclose
        case 8:     // init
            mStatusError = ATCIOSymbols::CIOStatNotSupported;
            return true;

        case 9:     // open
        case 10:    // ffirst
            mTransferBuffer[0] = mParBuf.mHandle;
            memset(mTransferBuffer + 1, 0, sizeof(ATPCLinkDirEnt));

            if (CheckValidFileHandle(true)) {
                ATPCLinkFileHandle& fh = mFileHandles[mParBuf.mHandle - 1];

                const ATPCLinkDirEnt dirEnt = fh.GetDirEnt();

                mStatusError = ATCIOSymbols::CIOStatSuccess;

                memcpy(mTransferBuffer + 1, &dirEnt, sizeof(ATPCLinkDirEnt));
            }
            mpSIOMgr->SendData(mTransferBuffer, sizeof(ATPCLinkDirEnt) + 1, true);
            return true;

        case 11:    // rename
        case 12:    // remove
        case 13:    // chmod
        case 14:    // mkdir
        case 15:    // rmdir
        case 16:    // chdir
            mStatusError = ATCIOSymbols::CIOStatNotSupported;
            return true;

        case 17:    // getcwd
            {
                memset(mTransferBuffer, 0, 65);
                strncpy((char *)mTransferBuffer, mCurDir.c_str(), 64);
                mpSIOMgr->SendData(mTransferBuffer, 64, true);
            }
            return true;

        case 18:    // setboot
            mStatusError = ATCIOSymbols::CIOStatNotSupported;
            return true;

        case 19:    // getdfree
            {
                ATPCLinkDiskInfo diskInfo = {};

                VDASSERTCT(sizeof(diskInfo) == 64);

                diskInfo.mInfoVersion = 0x21;
                diskInfo.mSectorCountLo = 0xff;
                diskInfo.mSectorCountHi = 0xff;
                diskInfo.mSectorsFreeLo = 0xff;
                diskInfo.mSectorsFreeHi = 0xff;
                diskInfo.mBytesPerSectorCode = 1;
                diskInfo.mVersion = 0x80;
                diskInfo.mBytesPerSectorHi = 2;
                diskInfo.mSectorsPerCluster = 1;
                memcpy(diskInfo.mVolumeLabel, "PCLink  ", 8);

                memcpy(mTransferBuffer, &diskInfo, 64);
                mpSIOMgr->SendData(mTransferBuffer, 64, true);
            }
            return true;

        case 20:    // chvol
            mStatusError = ATCIOSymbols::CIOStatNotSupported;
            return true;
    }

    return false;
}

bool Link_Device_Check_Valid_File_Handle(LinkDevice *dev, bool setError) {
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

int Link_Device_Resolve_Path(int allowDir, char *resultPath) {
    const uint8 *s = dev->ParBuf.Path;

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
            uint8 c = resultPath[len-1];

            resultPath[len-1] = 0;

            if (c == '\\')
                break;
        }
    }

    // parse out remaining components

    int inext = false;
    int fnchars = 0;
    int extchars = 0;

    while(uint8 c = *s++) {
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

                    while(!resultPath.empty()) {
                        int len = strlen(resultPath);
                        uint8 c = resultPath[len-1];

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

            strcat(resultPath, ".";
            inext = TRUE;
            continue;
        }

        if (!fnchars)
            strcat(resultPath, "\\");

        if ((uint8)(c - 'a') < 26)
            c &= ~0x20;

        if (c != '_' && (uint8)(c - '0') >= 10 && (uint8)(c - 'A') >= 26) {
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

        strncat(resultPath, &c, 1);
    }

    if (inext && !extchars && !allowDir) {
        dev->StatusError = CIOStatFileNameErr;
        return FALSE;
    }

    // strip off trailing separator if present
    if (!resultPath.empty() && resultPath.back() == '\\')
        resultPath.pop_back();

    return TRUE;
}

int Link_Device_Resolve_Native_Path(int allowDir, char *resultPath) {
    VDStringA netPath;

    if (!ResolvePath(allowDir, netPath))
        return false;

    return ResolveNativePath(resultPath, netPath);
}

bool ATPCLinkDevice::ResolveNativePath(VDStringW& resultPath, const VDStringA& netPath) {

    // translate path
    resultPath = mBasePathNative;

    for(VDStringA::const_iterator it(netPath.begin()), itEnd(netPath.end());
        it != itEnd;
        ++it)
    {
        char c = *it;

        if (c >= 'A' && c <= 'Z')
            c &= ~0x20;

        resultPath += (wchar_t)c;   
    }

    // ensure trailing separator
    if (!resultPath.empty() && resultPath.back() != L'\\')
        resultPath += L'\\';

    return true;
}
