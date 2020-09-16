/* atrSparta.c -  
 *  Part of the Atari Disk Image File Editor Library
 *  Mark Grebe <atarimacosx@gmail.com>
 *  
 * Based on code from:
 *    Atari800 Emulator (atari800.sourceforge.net)
 *    Adir v0.67 (c) 1999-2001 Jindrich Kubec <kubecj@asw.cz>
 *    Atr8fs v0.1  http://www.rho-sigma.de/atari8bit/fs.html
 *
 * Copyright (C) 2004 Mark Grebe
 *
 * Atari Disk Image File Editor Library is free software; 
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atari Disk Image File Editor Library is distributed in the hope 
 * hat it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari Disk Image File Editor Library; if not, write to the 
 * Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/fcntl.h>
#include "atari.h"
#include "atrUtil.h"
#include "atrMount.h"
#include "atrSparta.h"
#include "atrErr.h"
#include "sys/stat.h"
#include <sys/time.h>

#define SPARTA_DELETED  0x10
#define SPARTA_INUSE    0x08
#define SPARTA_LOCKED   0x01
#define SPARTA_SUBDIR   0x20

#define SPARTA_DIR_ENTRY_SIZE 23

typedef struct SPARTA_DIRENT
{
    ULONG dirEntryOffset;
	UBYTE flags;
	UWORD firstSectorMap;
	ULONG size;
	UBYTE atariName[11];
	UBYTE day;
	UBYTE month;
	UBYTE year;
	UBYTE hour;
	UBYTE minute;
	UBYTE second;
} SPARTA_DIRENT;

typedef struct SpartaDirEntry
{
    struct SpartaDirEntry *pNext;
    struct SpartaDirEntry *pPrev;
    char filename[ 256 ];
    ULONG dirEntryOffset;
	UBYTE flags;
	UWORD firstSectorMap;  // Sector number of fist sector map
	ULONG size; //Size of file
	UBYTE day;
	UBYTE month;
	UBYTE year;
	UBYTE hour;
	UBYTE minute;
	UBYTE second;
} SpartaDirEntry;

typedef struct SpartaDir
{
	struct SpartaDir *pNext;
	struct SpartaDir *pPrev;
	char dirname[256];
	UWORD  dirMapStartSector;
} SpartaDir;

typedef struct atrSpartaDiskInfo {
    SpartaDirEntry  *pCurrDir;
    SpartaDir	   *pDirList;

    UWORD mainDirMap;
    UWORD totalSectors;
    UWORD diskFreeSectors;
    UBYTE bitmapSectorCount;
    UWORD firstBitmapSector;
    UWORD dataSectorSearchStart;
    UWORD dirSectorSearchStart;
    char volumeName[9];
    UBYTE numTracks;
    UWORD sectorSize;
    UBYTE majorRev;
    UBYTE volSeqNumber;
    UBYTE volRandNumber;

    UBYTE vtocMap[512*512];

    UWORD sectorMap[1024];
    UWORD sectorMapCurr;
    UWORD sectorMapCount;

    UBYTE *dirBuffer;
    ULONG dirBufferLen;
	} AtrSpartaDiskInfo;

static int AtrSpartaReadDirIntoMem(AtrDiskInfo *info, UWORD dirSector);
static int AtrSpartaWriteDirFromMem(AtrDiskInfo *info);
static void AtrSpartaFreeDirFromMem(AtrDiskInfo *info);
static int AtrSpartaReadDir(AtrDiskInfo *info, UWORD dirSector);
static int AtrSpartaReadRootDir(AtrDiskInfo *info);
static int AtrSpartaReadCurrentDir(AtrDiskInfo *info);
static UWORD AtrSpartaCurrentDirStartSector(AtrDiskInfo *info);
static int AtrReadSectorMap(AtrDiskInfo *info, UWORD startSector);
static int AtrDeleteSectorMap(AtrDiskInfo *info, UWORD startSector, ULONG dir);
static int AtrWriteSectorMap(AtrDiskInfo *info, UWORD startSector);
static int AtrWriteNewSectorMap(AtrDiskInfo *info, UBYTE *sectorMap, UWORD sectorCount, 
                                UWORD mapSectorCount, ULONG dir);
static void AtrResetSectorMap(AtrDiskInfo *info);
static UWORD AtrGetNextSectorFromMap(AtrDiskInfo *info);
static SpartaDirEntry *AtrSpartaFindDirEntryByName(AtrDiskInfo *info, char *name);
static UWORD AtrSpartaGetFreeSectorFile(AtrDiskInfo *info);
static UWORD AtrSpartaGetFreeSectorDir(AtrDiskInfo *info);
static int AtrSpartaReadVtoc(AtrDiskInfo *info);
static int AtrSpartaWriteVtoc(AtrDiskInfo *info);
static int AtrSpartaIsSectorUsed(AtrDiskInfo *info, UWORD sector);
static void AtrSpartaMarkSectorUsed(AtrDiskInfo *info, UWORD sector,
                                    int used, ULONG dir);
static void AtrSpartaDeleteDirList(AtrDiskInfo *info);
static void AtrSpartaDeleteDirEntryList(AtrDiskInfo *info);
static SpartaDirEntry *AtrSpartaCreateDirEntry( SPARTA_DIRENT* pDirEntry);

int AtrSpartaMount(AtrDiskInfo *info)
{
    UBYTE secBuff[0x200];
	AtrSpartaDiskInfo *dinfo;

	info->atr_dosinfo = (void *) calloc(1, sizeof(AtrSpartaDiskInfo));
	if (info->atr_dosinfo == NULL)
		return(ADOS_MEM_ERR);
	dinfo = (AtrSpartaDiskInfo *) info->atr_dosinfo;

    if (AtrReadSector(info, 1,secBuff))
        return(ADOS_DISK_READ_ERR);

    dinfo->mainDirMap = secBuff[9] + (secBuff[10] << 8);
    dinfo->totalSectors = secBuff[11] + (secBuff[12] << 8);
    dinfo->diskFreeSectors = secBuff[13] + (secBuff[14] << 8);
    dinfo->bitmapSectorCount = secBuff[15];
    dinfo->firstBitmapSector = secBuff[16] + (secBuff[17] << 8);
    dinfo->dataSectorSearchStart = secBuff[18] + (secBuff[19] << 8);
    dinfo->dirSectorSearchStart = secBuff[20] + (secBuff[21] << 8);
    memcpy(dinfo->volumeName,&secBuff[22],8);
    dinfo->numTracks = secBuff[30];
    if (secBuff[31] == 1) 
        dinfo->sectorSize = 512;
    else if (secBuff[31] == 0)
        dinfo->sectorSize = 256;
    else 
        dinfo->sectorSize = 128;
    dinfo->majorRev = secBuff[32];
    dinfo->volSeqNumber = secBuff[38];
    dinfo->volRandNumber = secBuff[39];

    return(AtrSpartaReadRootDir(info));
}

void AtrSpartaUnmount(AtrDiskInfo *info)
{
    AtrSpartaDeleteDirList(info);
    AtrSpartaDeleteDirEntryList(info);
	free(info->atr_dosinfo);
}

int AtrSpartaGetDir(AtrDiskInfo *info, UWORD *fileCount, ADosFileEntry *files, 
                    ULONG *freeBytes)
{
    SpartaDirEntry  *pCurr = NULL;
    ADosFileEntry *pFileEntry = files;
    UWORD count = 0;
    char name[15];
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;

    *fileCount = 0;
    pCurr = dinfo->pCurrDir;
    count = 0;

    while (pCurr) 
        {
        if (pCurr->flags != 0 && (pCurr->flags & SPARTA_DELETED) 
                        != SPARTA_DELETED) {
        	strcpy(name, pCurr->filename);
            Host2ADos(name,pFileEntry->aname);
            pFileEntry->flags = 0;
            if (pCurr->flags & SPARTA_LOCKED)
                pFileEntry->flags |= DIRE_LOCKED;
            if (pCurr->flags & SPARTA_SUBDIR)
                pFileEntry->flags |= DIRE_SUBDIR;
            pFileEntry->bytes = pCurr->size;
            pFileEntry->day = pCurr->day;
            pFileEntry->month = pCurr->month;
            pFileEntry->year = pCurr->year;
            pFileEntry->hour = pCurr->hour;
            pFileEntry->minute = pCurr->minute;
            count++;
            pFileEntry++;
            }
        pCurr = pCurr->pNext;
        }

    *freeBytes = dinfo->diskFreeSectors  * AtrSectorSize(info);

    *fileCount = count;

    return(FALSE);
}

int AtrSpartaChangeDir(AtrDiskInfo *info, int cdFlag, char *name)
{
    SpartaDirEntry  *pDirEntry = NULL;
    UBYTE *pTmp = NULL;
    SpartaDir *pNew;
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;

	if (cdFlag == CD_UP) {
        if (dinfo->pDirList == NULL) 
            return(FALSE);
		
        if (dinfo->pDirList->pPrev == NULL) {
			free(dinfo->pDirList);
			dinfo->pDirList = NULL;
			return(AtrSpartaReadRootDir(info));
			}
		else {
			dinfo->pDirList = dinfo->pDirList->pPrev;
			free(dinfo->pDirList->pNext);
			dinfo->pDirList->pNext = NULL;
			return(AtrSpartaReadDir(info, dinfo->pDirList->dirMapStartSector));
			}
		}
	else if (cdFlag == CD_ROOT) {
		return(AtrSpartaReadRootDir(info));
		}
	else {
        if ((pDirEntry = AtrSpartaFindDirEntryByName(info, name)) == NULL) {
        	return ADOS_FILE_NOT_FOUND;
        	}
    	
        if ((pDirEntry->flags & SPARTA_SUBDIR) != SPARTA_SUBDIR)
    		return ADOS_NOT_A_DIRECTORY;
    	
        pNew = (SpartaDir *) malloc(sizeof(SpartaDir));
    	
        if (pNew == NULL)
    		return(ADOS_MEM_ERR);
        
        pTmp += 3;
		pNew->dirMapStartSector = pDirEntry->firstSectorMap;
		
        if (dinfo->pDirList != NULL) {
			strcpy(pNew->dirname, dinfo->pDirList->dirname);
			strcat(pNew->dirname,":");
			strcat(pNew->dirname,name);
			dinfo->pDirList->pNext = pNew;
			pNew->pPrev = dinfo->pDirList;
			pNew->pNext = NULL;
			dinfo->pDirList = pNew;
			}
		else {
			dinfo->pDirList = pNew;
			pNew->pPrev = NULL;
			pNew->pNext = NULL;
			strcpy(pNew->dirname,":");
			strcat(pNew->dirname,name);
			}
		return(AtrSpartaReadDir(info,dinfo->pDirList->dirMapStartSector));
		}

    return FALSE;
}

int AtrSpartaDeleteDir(AtrDiskInfo *info, char *name)
{
    SpartaDirEntry  *pDirEntry = NULL;
    UWORD sector;
    UWORD dirStartSector;
    UBYTE *buffPtr;
    ULONG dirSize;
    int stat;
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;

    if ((pDirEntry = AtrSpartaFindDirEntryByName(info, name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }

    if ((pDirEntry->flags & SPARTA_LOCKED) == SPARTA_LOCKED) {
        return ADOS_FILE_LOCKED;
        }

  	if ((pDirEntry->flags & SPARTA_SUBDIR) == 0)
   		return ADOS_NOT_A_DIRECTORY;

    if(AtrSpartaReadDirIntoMem(info,pDirEntry->firstSectorMap))
        return(ADOS_DIR_READ_ERR);

    buffPtr = dinfo->dirBuffer;
    dirSize = buffPtr[3] + (buffPtr[4] << 8) + (buffPtr[5] << 16);

    buffPtr += SPARTA_DIR_ENTRY_SIZE;
    while (*buffPtr && (dirSize >= SPARTA_DIR_ENTRY_SIZE)) 
        {
		if ((*buffPtr & SPARTA_DELETED) == 0) {
 		    AtrSpartaFreeDirFromMem(info);
 		    return(ADOS_DIR_NOT_EMPTY);
            }
		
        buffPtr += SPARTA_DIR_ENTRY_SIZE;
        dirSize -= SPARTA_DIR_ENTRY_SIZE;
        }
    AtrSpartaFreeDirFromMem(info);
    if (AtrSpartaReadVtoc(info))
        return ADOS_VTOC_READ_ERR;
    
    if (AtrReadSectorMap(info, pDirEntry->firstSectorMap))
        return ADOS_FILE_READ_ERR;
	
    while( (sector = AtrGetNextSectorFromMap(info)) )
	{
        AtrSpartaMarkSectorUsed(info, sector, FALSE,TRUE);
	}
	
	AtrDeleteSectorMap(info, pDirEntry->firstSectorMap,TRUE);

    dinfo->volSeqNumber++;

    if (AtrSpartaWriteVtoc(info)) 
        return(ADOS_VTOC_WRITE_ERR);
    
    if (dinfo->pDirList == NULL) 
        dirStartSector = dinfo->mainDirMap;
    else
        dirStartSector = dinfo->pDirList->dirMapStartSector;
    
    if(AtrSpartaReadDirIntoMem(info, dirStartSector))
        return(ADOS_DIR_READ_ERR);
        
    dinfo->dirBuffer[pDirEntry->dirEntryOffset] = SPARTA_DELETED;

    if(AtrSpartaWriteDirFromMem(info))
        return(ADOS_DIR_WRITE_ERR);

    AtrSpartaFreeDirFromMem(info);
   
    stat = AtrSpartaReadCurrentDir(info);
    if (stat)
    	return(stat);
    return FALSE;
}

int AtrSpartaMakeDir(AtrDiskInfo *info, char *name)
{
    SpartaDirEntry  *pDirEntry = NULL;
    UBYTE secBuff[0x200];
    UWORD sector;
    int i, stat;
    UBYTE *mapBuffer, *mapBufferCurrent;
    time_t currTime;
    struct tm *localTime;
    ULONG currentDirLen;
    int dirEntryIndex = 0;
    UWORD dirStartSector;
    UWORD firstMapSector;
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;

    for (i=0;i<strlen(name);i++) 
        {
        if (islower(name[i]))
            name[i] -= 32;
        }

    if ((pDirEntry = AtrSpartaFindDirEntryByName(info, name)) != NULL) 
            return ADOS_DUPLICATE_NAME;
 
    if (AtrSpartaReadVtoc(info)) {
        return(ADOS_VTOC_READ_ERR);
        }
    
    if (dinfo->diskFreeSectors < 2) {
        return(ADOS_DISK_FULL);
        }
        
    mapBuffer = (UBYTE *) calloc(1, dinfo->sectorSize);
    if (mapBuffer == NULL) 
        return ADOS_MEM_ERR;

    mapBufferCurrent = mapBuffer;
    
    sector = AtrSpartaGetFreeSectorDir(info);
    AtrSpartaMarkSectorUsed(info,sector,TRUE,TRUE);
    *mapBufferCurrent++ = sector & 0xFF;
    *mapBufferCurrent++ = sector >> 8;
    
	if (dinfo->pDirList == NULL) 
        dirStartSector = dinfo->mainDirMap;
    else
        dirStartSector = dinfo->pDirList->dirMapStartSector;
    
    /* Fill in blank directory entry */
	memset(secBuff, 0, dinfo->sectorSize);
	secBuff[1] = dirStartSector & 0xFF;
	secBuff[2] = dirStartSector >> 8;
	secBuff[3] = 23;
	secBuff[4] = 0;
	secBuff[5] = 0;
	Host2ADos(name,&secBuff[6]);	
	
    if (AtrWriteSector(info,sector, secBuff)) {
        return(ADOS_FILE_WRITE_ERR);
        }
   
    if ((firstMapSector = AtrWriteNewSectorMap(info,mapBuffer, 1, 1, TRUE)) == 0)
        return ADOS_FILE_WRITE_ERR;

    if(AtrSpartaReadDirIntoMem(info,dirStartSector))
        return(ADOS_DIR_READ_ERR);

    currentDirLen = dinfo->dirBuffer[3] + (dinfo->dirBuffer[4] << 8) + (dinfo->dirBuffer[5] << 16);
    
    for (i=0;i < currentDirLen; i+= SPARTA_DIR_ENTRY_SIZE) 
        {
        if (dinfo->dirBuffer[i] & SPARTA_DELETED) 
            {
            dirEntryIndex = i;
            break;
            }
        }
    
    /* No directory entries free, need to add one to end */
    if (i >= currentDirLen) 
        {
        /* Need to add a sector to the directory */
        if ((currentDirLen + SPARTA_DIR_ENTRY_SIZE) > dinfo->dirBufferLen) {
            dinfo->sectorMap[dinfo->sectorMapCount] = AtrSpartaGetFreeSectorDir(info);
            dinfo->sectorMapCount++;

            }
        dirEntryIndex = currentDirLen;
        currentDirLen += SPARTA_DIR_ENTRY_SIZE;
        }
    
    dinfo->dirBuffer[dirEntryIndex] = SPARTA_INUSE | SPARTA_SUBDIR;
    dinfo->dirBuffer[dirEntryIndex+1] = firstMapSector & 0xFF;
    dinfo->dirBuffer[dirEntryIndex+2] = firstMapSector >> 8;
    dinfo->dirBuffer[dirEntryIndex+3] = SPARTA_DIR_ENTRY_SIZE;
    dinfo->dirBuffer[dirEntryIndex+4] = SPARTA_DIR_ENTRY_SIZE >> 8;
    dinfo->dirBuffer[dirEntryIndex+5] = SPARTA_DIR_ENTRY_SIZE >> 16;
    Host2ADos(name,&dinfo->dirBuffer[dirEntryIndex+6]);
    time(&currTime);
    localTime = localtime(&currTime);
    dinfo->dirBuffer[dirEntryIndex+17] = localTime->tm_mday;
    dinfo->dirBuffer[dirEntryIndex+18] = localTime->tm_mon;
    if (localTime->tm_year > 99) 
        dinfo->dirBuffer[dirEntryIndex+19] = localTime->tm_year - 100;
    else
        dinfo->dirBuffer[dirEntryIndex+19] = localTime->tm_year;
    dinfo->dirBuffer[dirEntryIndex+20] = localTime->tm_hour;
    dinfo->dirBuffer[dirEntryIndex+21] = localTime->tm_min;
    dinfo->dirBuffer[dirEntryIndex+22] = localTime->tm_sec;
    dinfo->dirBuffer[3] = currentDirLen & 0xFF;
    dinfo->dirBuffer[4] = (currentDirLen >> 8) & 0xFF;
    dinfo->dirBuffer[5] = currentDirLen >> 16;
    
    if(AtrSpartaWriteDirFromMem(info))
        return(ADOS_DIR_WRITE_ERR);

    if (AtrWriteSectorMap(info, dirStartSector))
        return(ADOS_DIR_WRITE_ERR);
    
    AtrSpartaFreeDirFromMem(info);
    
    dinfo->volSeqNumber++;
    
    AtrSpartaWriteVtoc(info);
        
    stat = AtrSpartaReadCurrentDir(info);
    if (stat)
    	return(stat);
    return FALSE;
}

int AtrSpartaLockFile(AtrDiskInfo *info, char *name, int lock)
{
    SpartaDirEntry  *pDirEntry = NULL;
    UWORD dirStartSector;
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;

    if ((pDirEntry = AtrSpartaFindDirEntryByName(info,name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }
        
    if (dinfo->pDirList == NULL) 
        dirStartSector = dinfo->mainDirMap;
    else
        dirStartSector = dinfo->pDirList->dirMapStartSector;
    
    if(AtrSpartaReadDirIntoMem(info,dirStartSector))
        return(ADOS_DIR_READ_ERR);
        
    if (lock) { 
        dinfo->dirBuffer[pDirEntry->dirEntryOffset] |= SPARTA_LOCKED;
        pDirEntry->flags |= SPARTA_LOCKED;
        }
    else {
        dinfo->dirBuffer[pDirEntry->dirEntryOffset] &= ~SPARTA_LOCKED;
        pDirEntry->flags &= ~SPARTA_LOCKED;
    }
    
    if(AtrSpartaWriteDirFromMem(info))
        return(ADOS_DIR_WRITE_ERR);

    if (AtrSpartaReadVtoc(info)) 
        return(ADOS_VTOC_READ_ERR);

    dinfo->volSeqNumber++;

    if (AtrSpartaWriteVtoc(info)) 
        return(ADOS_VTOC_WRITE_ERR);

    AtrSpartaFreeDirFromMem(info);

    return FALSE;
}

int AtrSpartaRenameFile(AtrDiskInfo *info, char *name, char *newname)
{
    SpartaDirEntry  *pDirEntry = NULL;
    UWORD dirStartSector;
	UBYTE dosName[13];
    int stat;
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;

    if ((pDirEntry = AtrSpartaFindDirEntryByName(info,name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }

    if ((pDirEntry->flags & SPARTA_LOCKED) == SPARTA_LOCKED) {
        return ADOS_FILE_LOCKED;
        }

	Host2ADos(newname, dosName);
	ADos2Host(newname, dosName);

    if (AtrSpartaFindDirEntryByName(info,newname) != NULL) {
		return ADOS_DUPLICATE_NAME;
		}
        
    if (dinfo->pDirList == NULL) 
        dirStartSector = dinfo->mainDirMap;
    else
        dirStartSector = dinfo->pDirList->dirMapStartSector;
    
    if(AtrSpartaReadDirIntoMem(info,dirStartSector))
        return(ADOS_DIR_READ_ERR);
        
    Host2ADos(newname,&dinfo->dirBuffer[pDirEntry->dirEntryOffset + 6]);
    
    if(AtrSpartaWriteDirFromMem(info)) {
		AtrSpartaFreeDirFromMem(info);
        return(ADOS_DIR_WRITE_ERR);
		}

	AtrSpartaFreeDirFromMem(info);
		
	if (pDirEntry->flags & SPARTA_SUBDIR) {
		if(AtrSpartaReadDirIntoMem(info,pDirEntry->firstSectorMap))
			return(ADOS_DIR_READ_ERR);
		
		if (dinfo->dirBufferLen < 23) {
			AtrSpartaFreeDirFromMem(info);
			return(ADOS_DIR_WRITE_ERR);
			}
		
		Host2ADos(newname,&dinfo->dirBuffer[6]);
		
		if(AtrSpartaWriteDirFromMem(info)) {
			AtrSpartaFreeDirFromMem(info);
			return(ADOS_DIR_WRITE_ERR);
			}

		AtrSpartaFreeDirFromMem(info);
		
		}
		
    if (AtrSpartaReadVtoc(info)) 
        return(ADOS_VTOC_READ_ERR);

    dinfo->volSeqNumber++;

    if (AtrSpartaWriteVtoc(info)) 
        return(ADOS_VTOC_WRITE_ERR);
    
    stat = AtrSpartaReadCurrentDir(info);
    if (stat)
    	return stat;

    return FALSE;
}

int AtrSpartaDeleteFile(AtrDiskInfo *info, char *name)
{
    SpartaDirEntry  *pDirEntry = NULL;
    UWORD sector;
    UWORD dirStartSector;
    int stat;
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;

    if ((pDirEntry = AtrSpartaFindDirEntryByName(info, name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }

    if ((pDirEntry->flags & SPARTA_LOCKED) == SPARTA_LOCKED) {
        return ADOS_FILE_LOCKED;
        }

  	if (pDirEntry->flags & SPARTA_SUBDIR)
   		return ADOS_FILE_NOT_FOUND;

    if (AtrSpartaReadVtoc(info))
        return ADOS_VTOC_READ_ERR;
    
    if (AtrReadSectorMap(info, pDirEntry->firstSectorMap))
        return ADOS_FILE_READ_ERR;
	
    while( (sector = AtrGetNextSectorFromMap(info)) )
	{
        AtrSpartaMarkSectorUsed(info,sector, FALSE, FALSE);
	}
	
	AtrDeleteSectorMap(info,pDirEntry->firstSectorMap,FALSE);

    dinfo->volSeqNumber++;

    if (AtrSpartaWriteVtoc(info)) 
        return(ADOS_VTOC_WRITE_ERR);
    
    if (dinfo->pDirList == NULL) 
        dirStartSector = dinfo->mainDirMap;
    else
        dirStartSector = dinfo->pDirList->dirMapStartSector;
    
    if(AtrSpartaReadDirIntoMem(info,dirStartSector))
        return(ADOS_DIR_READ_ERR);
        
    dinfo->dirBuffer[pDirEntry->dirEntryOffset] = SPARTA_DELETED;

    if(AtrSpartaWriteDirFromMem(info))
        return(ADOS_DIR_WRITE_ERR);

    AtrSpartaFreeDirFromMem(info);
    
    stat = AtrSpartaReadCurrentDir(info);
    if (stat)
    	return(stat);

    return(FALSE);
}
extern int errno;
int AtrSpartaImportFile(AtrDiskInfo *info, char *filename, int lfConvert, int tabConvert)
{
    FILE *inFile;
    ULONG file_length, total_file_length;
    UWORD numSectorsNeeded, numMapSectorsNeeded;
    ULONG numToWrite;
    UBYTE secBuff[0x200];
    SpartaDirEntry  *pDirEntry;
    UWORD sector;
    char *slash;
    int stat;
	UBYTE dosName[12];
	int i;
    UBYTE *mapBuffer, *mapBufferCurrent;
    struct tm *localTime;
    ULONG currentDirLen;
    int dirEntryIndex = 0;
    UWORD dirStartSector;
    UWORD firstMapSector;
    struct stat buf;
    
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;
    
    inFile = fopen( filename, "rb");

    if (inFile == NULL)
        {
        return(ADOS_HOST_FILE_NOT_FOUND);
        }

    // Get file status to get modification time
    fstat ( fileno(inFile), &buf);

    if ((slash = strrchr(filename,'/')) != NULL)
        {
        filename = slash + 1;
        }

	Host2ADos(filename, dosName);
	ADos2Host(filename, dosName);

    if ((pDirEntry = AtrSpartaFindDirEntryByName(info,filename)) != NULL) {

    	if ((pDirEntry->flags & SPARTA_SUBDIR) == SPARTA_SUBDIR)
            {
            return ADOS_FILE_IS_A_DIR;
            }
    	
        if (AtrSpartaDeleteFile(info,filename)) {
            fclose(inFile);
            return ADOS_DELETE_FILE_ERR;
            }
        }

    fseek(inFile, 0L, SEEK_END);
    file_length = ftell(inFile);
    fseek(inFile, 0L, SEEK_SET);
    total_file_length = file_length;
	
	if (file_length > 0xFFFFFF) {
		fclose(inFile);
		return(ADOS_FILE_WRITE_ERR);
		}

    if (file_length == 0) 
        numSectorsNeeded = 1;
    else if ((file_length % dinfo->sectorSize ) == 0) 
        numSectorsNeeded = file_length / dinfo->sectorSize ;
    else
        numSectorsNeeded = file_length / dinfo->sectorSize  + 1;

    if ((numSectorsNeeded % ((dinfo->sectorSize - 4) / 2)) == 0)
        numMapSectorsNeeded = numSectorsNeeded / ((dinfo->sectorSize - 4) / 2);
    else
        numMapSectorsNeeded = numSectorsNeeded / ((dinfo->sectorSize - 4) / 2) + 1;

    if (AtrSpartaReadVtoc(info)) {
        fclose(inFile);
        return(ADOS_VTOC_READ_ERR);
        }

    if ((numSectorsNeeded + numMapSectorsNeeded + 1) > dinfo->diskFreeSectors) {
        fclose(inFile);
        return(ADOS_DISK_FULL);
    }

    mapBuffer = (UBYTE *) calloc(numMapSectorsNeeded, dinfo->sectorSize);
    if (mapBuffer == NULL) 
        return ADOS_MEM_ERR;

    mapBufferCurrent = mapBuffer;

    do {
        if (file_length >= dinfo->sectorSize ) 
            numToWrite = dinfo->sectorSize;
        else
            numToWrite = file_length;

        sector = AtrSpartaGetFreeSectorFile(info);
        AtrSpartaMarkSectorUsed(info,sector,TRUE,FALSE);
        *mapBufferCurrent++ = sector & 0xFF;
        *mapBufferCurrent++ = sector >> 8;

        if (fread(secBuff,1,numToWrite,inFile) != numToWrite)
            {
            fclose(inFile);
            return(ADOS_HOST_READ_ERR);
            }

        if (lfConvert)
            HostLFToAtari(secBuff, numToWrite);
        
        if (tabConvert)
            HostTabToAtari(secBuff, numToWrite);
        
        if (AtrWriteSector(info,sector, secBuff))
            {
            fclose(inFile);
            return(ADOS_FILE_WRITE_ERR);
            }

        file_length -= numToWrite;
    } while (file_length > 0);

    if ((firstMapSector = 
         AtrWriteNewSectorMap(info,mapBuffer, numSectorsNeeded, numMapSectorsNeeded, FALSE))
         == 0)
        return ADOS_FILE_WRITE_ERR;

    if (dinfo->pDirList == NULL) 
        dirStartSector = dinfo->mainDirMap;
    else
        dirStartSector = dinfo->pDirList->dirMapStartSector;

    if(AtrSpartaReadDirIntoMem(info,dirStartSector))
        return(ADOS_DIR_READ_ERR);

    currentDirLen = dinfo->dirBuffer[3] + (dinfo->dirBuffer[4] << 8) + (dinfo->dirBuffer[5] << 16);
    
    for (i=0;i < currentDirLen; i+= SPARTA_DIR_ENTRY_SIZE) 

        {
        if (dinfo->dirBuffer[i] & SPARTA_DELETED) 
            {
            dirEntryIndex = i;
            break;
            }
        }
    
    /* No directory entries free, need to add one to end */
    if (i >= currentDirLen) 
        {
        /* Need to add a sector to the directory */
        if ((currentDirLen + SPARTA_DIR_ENTRY_SIZE) > dinfo->dirBufferLen) {
            dinfo->sectorMap[dinfo->sectorMapCount] = AtrSpartaGetFreeSectorDir(info);
            dinfo->sectorMapCount++;

            }
        dirEntryIndex = currentDirLen;
        currentDirLen += SPARTA_DIR_ENTRY_SIZE;
        }
    
    dinfo->dirBuffer[dirEntryIndex] = SPARTA_INUSE;
    dinfo->dirBuffer[dirEntryIndex+1] = firstMapSector & 0xFF;
    dinfo->dirBuffer[dirEntryIndex+2] = firstMapSector >> 8;
    dinfo->dirBuffer[dirEntryIndex+3] = total_file_length & 0x0000ff;
    dinfo->dirBuffer[dirEntryIndex+4] = (total_file_length & 0x00ff00) >> 8;
    dinfo->dirBuffer[dirEntryIndex+5] = (total_file_length & 0xff0000) >> 16;
    Host2ADos(filename,&dinfo->dirBuffer[dirEntryIndex+6]);



    localTime = localtime(&buf.st_mtime);
    dinfo->dirBuffer[dirEntryIndex+17] = localTime->tm_mday;
    dinfo->dirBuffer[dirEntryIndex+18] = localTime->tm_mon+1;
    if (localTime->tm_year > 99) 
        dinfo->dirBuffer[dirEntryIndex+19] = localTime->tm_year - 100;
    else
        dinfo->dirBuffer[dirEntryIndex+19] = localTime->tm_year;
    dinfo->dirBuffer[dirEntryIndex+20] = localTime->tm_hour;
    dinfo->dirBuffer[dirEntryIndex+21] = localTime->tm_min;
    dinfo->dirBuffer[dirEntryIndex+22] = localTime->tm_sec;
    dinfo->dirBuffer[3] = currentDirLen & 0xFF;
    dinfo->dirBuffer[4] = (currentDirLen >> 8) & 0xFF;
    dinfo->dirBuffer[5] = currentDirLen >> 16;
    
    if(AtrSpartaWriteDirFromMem(info))
        return(ADOS_DIR_WRITE_ERR);

    if (AtrWriteSectorMap(info,dirStartSector))
        return(ADOS_DIR_WRITE_ERR);
    
    AtrSpartaFreeDirFromMem(info);
    
    dinfo->volSeqNumber++;
    
    AtrSpartaWriteVtoc(info);
    
    stat = AtrSpartaReadCurrentDir(info);
    if (stat)
    	return(stat);
    
    return FALSE;
}

int AtrSpartaExportFile(AtrDiskInfo *info, char *nameToExport, char* outFile, int lfConvert, int tabConvert)
{
	SpartaDirEntry* pDirEntry;
	UBYTE secBuff[ 0x0200 ];
    ULONG bytes;
	UWORD sector, bytesToWrite;
	FILE *output = NULL;
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;
    struct timeval fileTime[2];
    struct tm fileExpTime;

    if ((pDirEntry = AtrSpartaFindDirEntryByName(info,nameToExport)) == NULL) {
        return(ADOS_FILE_NOT_FOUND);
        }
	
    sector = pDirEntry->firstSectorMap;
    bytes = pDirEntry->size;
    
    if (AtrReadSectorMap(info,sector))
        return ADOS_FILE_READ_ERR;
	
    if ( outFile )
	{
		output = fopen(outFile, "wb+");

		if ( output == NULL)
		{
			return ADOS_HOST_CREATE_ERR;
		}
	}
	
    while( bytes )
	{
        if (bytes > dinfo->sectorSize) 
            bytesToWrite = dinfo->sectorSize;
        else
            bytesToWrite = bytes;

        sector = AtrGetNextSectorFromMap(info);
        
        if ( AtrReadSector(info,sector, secBuff))
		    {
			return ADOS_FILE_READ_ERR;
		    }
			
        if (lfConvert)
            AtariLFToHost(secBuff, bytesToWrite);
                    
        if (tabConvert)
            AtariTabToHost(secBuff, bytesToWrite);
                    
		if (fwrite( secBuff, 1, bytesToWrite ,output) != bytesToWrite)
            {
            fclose(output);
            return ADOS_HOST_WRITE_ERR;
            }

		bytes -= bytesToWrite;
	}
    fileExpTime.tm_mday = pDirEntry->day;
    fileExpTime.tm_mon = pDirEntry->month - 1;
    fileExpTime.tm_year = pDirEntry->year + 100;
    fileExpTime.tm_hour = pDirEntry->hour;
    fileExpTime.tm_min = pDirEntry->minute;
    fileExpTime.tm_sec = pDirEntry->second;
    fileTime[0].tv_sec = mktime(&fileExpTime);
    fileTime[0].tv_usec = 0;
    fileTime[1].tv_sec = fileTime[0].tv_sec;
    fileTime[1].tv_usec = fileTime[0].tv_usec;

	fclose( output );

    utimes (outFile, fileTime);

	return FALSE;
}

static int AtrSpartaReadDirIntoMem(AtrDiskInfo *info, UWORD dirSector)
{
    UWORD sector;
    UBYTE *buffPtr;
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;
    
    if (AtrReadSectorMap(info, dirSector))
        return ADOS_DIR_READ_ERR;
    
    dinfo->dirBufferLen = dinfo->sectorMapCount * dinfo->sectorSize;
    
    dinfo->dirBuffer = (UBYTE *) calloc(1,dinfo->dirBufferLen + dinfo->sectorSize);

    buffPtr = dinfo->dirBuffer;
    
    while((sector = AtrGetNextSectorFromMap(info)))
        {
        if (AtrReadSector(info,sector,buffPtr)) {
            free(dinfo->dirBuffer);
            dinfo->dirBufferLen = 0;
            return ADOS_DIR_READ_ERR;
            }
        buffPtr += dinfo->sectorSize;
        } 
    
    return FALSE;
}

static int AtrSpartaWriteDirFromMem(AtrDiskInfo *info)
{
    UWORD sector;
    UBYTE *buffPtr;
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;
    
    AtrResetSectorMap(info);

    buffPtr = dinfo->dirBuffer;

    while((sector = AtrGetNextSectorFromMap(info)))
        {
        if (AtrWriteSector(info,sector,buffPtr)) {
            free(dinfo->dirBuffer);
            dinfo->dirBufferLen = 0;
            return ADOS_DIR_WRITE_ERR;
            }
        buffPtr += dinfo->sectorSize;
        } 

    return FALSE;
}

static void AtrSpartaFreeDirFromMem(AtrDiskInfo *info)
{
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;

    if (dinfo->dirBuffer) 
        free(dinfo->dirBuffer);
    dinfo->dirBuffer = NULL;
}

static int AtrSpartaReadDir(AtrDiskInfo *info, UWORD dirSector)
{
    SpartaDirEntry  *pEntry;
	SpartaDirEntry  *pPrev = NULL;
	SPARTA_DIRENT dirEntry;
    UBYTE *buffPtr;
    ULONG dirSize;
    ULONG offset = 0;
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;
    
    if (dinfo->pCurrDir) 
        AtrSpartaDeleteDirEntryList(info);
    
    if (AtrSpartaReadDirIntoMem(info,dirSector)) {
        return ADOS_DIR_READ_ERR;
        }

    buffPtr = dinfo->dirBuffer;
    dirSize = buffPtr[3] + (buffPtr[4] << 8) + (buffPtr[5] << 16);

    buffPtr += SPARTA_DIR_ENTRY_SIZE;
    offset += SPARTA_DIR_ENTRY_SIZE;
    while (*buffPtr && (dirSize >= SPARTA_DIR_ENTRY_SIZE)) 
        {
        dirEntry.dirEntryOffset = offset;
		dirEntry.flags = buffPtr[0];

        dirEntry.firstSectorMap = buffPtr[1] + (buffPtr[2] << 8);
        dirEntry.size = buffPtr[3] + (buffPtr[4] << 8) + (buffPtr[5] << 16);
        memcpy(dirEntry.atariName, &buffPtr[6],11);
        dirEntry.day = buffPtr[17];
        dirEntry.month = buffPtr[18];
        dirEntry.year = buffPtr[19];
        dirEntry.hour = buffPtr[20];
        dirEntry.minute = buffPtr[21];
        dirEntry.second = buffPtr[22];
		
        pEntry = AtrSpartaCreateDirEntry( &dirEntry);
		if (pEntry == NULL)
			{
			AtrSpartaDeleteDirEntryList(info);
			return ADOS_MEM_ERR;
			}
		
        if ( pEntry )
		    {
			if ( dinfo->pCurrDir )
			    {
				pPrev->pNext = pEntry;
				pEntry->pPrev = pPrev;
                pEntry->pNext = NULL;
				pPrev = pEntry;
			    }
			else
			    {
				dinfo->pCurrDir = pEntry;
				pPrev = pEntry;
				pEntry->pPrev = NULL;
                pEntry->pNext = NULL;
			    }

		    }
		
        buffPtr += SPARTA_DIR_ENTRY_SIZE;
        offset += SPARTA_DIR_ENTRY_SIZE;
        dirSize -= SPARTA_DIR_ENTRY_SIZE;
        }

    AtrSpartaFreeDirFromMem(info);

	return FALSE;
}

static int AtrSpartaReadRootDir(AtrDiskInfo *info)
{	
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;

	AtrSpartaDeleteDirList(info);
	return(AtrSpartaReadDir(info, dinfo->mainDirMap));
}

static int AtrSpartaReadCurrentDir(AtrDiskInfo *info)
{
    return(AtrSpartaReadDir(info,AtrSpartaCurrentDirStartSector(info)));
}

static UWORD AtrSpartaCurrentDirStartSector(AtrDiskInfo *info)
{
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;

    if (dinfo->pDirList) 
        return(dinfo->pDirList->dirMapStartSector);
    else
        return(dinfo->mainDirMap);
}

static int AtrReadSectorMap(AtrDiskInfo *info, UWORD startSector)
{
    UBYTE secBuff[0x200];
    UBYTE *mapPtr;
    UWORD nextSector;
    UWORD sector;
    int i;
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;
    UWORD secPtrsPerSec = (dinfo->sectorSize - 4)/2;
    
    if (AtrReadSector(info,startSector, secBuff)) 
        return(ADOS_FILE_READ_ERR);
    
    dinfo->sectorMapCurr = 0;
    dinfo->sectorMapCount = 0;

    do {
        mapPtr = &secBuff[4];
        for (i=0;i<secPtrsPerSec;i++) 
            {
            sector =  *mapPtr + ((*(mapPtr+1)) << 8);
            
            if (sector) {
                dinfo->sectorMap[dinfo->sectorMapCurr++] = *mapPtr + ((*(mapPtr+1)) << 8);
                dinfo->sectorMapCount++;
                mapPtr += 2;
                }
            else
                break;
            }

        nextSector = secBuff[0] + (secBuff[1] << 8);
        
        if (nextSector) 
            if (AtrReadSector(info,nextSector, secBuff)) 
                return(ADOS_FILE_READ_ERR);
    
    } while (nextSector);
    
    dinfo->sectorMapCurr = 0;
    return(FALSE);
}

static int AtrDeleteSectorMap(AtrDiskInfo *info, UWORD startSector, ULONG dir)
{
    UBYTE secBuff[0x200];
    UWORD nextSector;
   
    if (AtrReadSector(info,startSector, secBuff)) 
        return(ADOS_FILE_READ_ERR);
    
    nextSector = startSector;

    do {
        AtrSpartaMarkSectorUsed(info,nextSector,FALSE, dir);
        nextSector = secBuff[0] + (secBuff[1] << 8);
        
        if (nextSector) 
            if (AtrReadSector(info,nextSector, secBuff)) 
                return(ADOS_FILE_READ_ERR);
    
    } while (nextSector);
    
     return(FALSE);
}
     
static int AtrWriteSectorMap(AtrDiskInfo *info, UWORD startSector)
{
    UBYTE secBuff[0x200];
    UBYTE *mapPtr;
    UWORD nextSector, lastSector;
    int i;
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;
    UWORD secPtrsPerSec = (dinfo->sectorSize - 4)/2;
    
    if (AtrReadSector(info,startSector, secBuff)) 
        return(ADOS_FILE_READ_ERR);

    dinfo->sectorMapCurr = 0;

    lastSector = startSector;
    mapPtr = &secBuff[4];

    while(dinfo->sectorMapCurr < dinfo->sectorMapCount) 
        {
        for (i=0;i<secPtrsPerSec;i++) {
            if (dinfo->sectorMapCurr < dinfo->sectorMapCount) 
                {
                *mapPtr++ = dinfo->sectorMap[dinfo->sectorMapCurr] & 0xFF;
                *mapPtr++ = dinfo->sectorMap[dinfo->sectorMapCurr++] >> 8;
                }
            else
                {
                *mapPtr++ = 0;
                *mapPtr++ = 0;

                }
            }

        nextSector = secBuff[0] + (secBuff[1] << 8);
        if (nextSector == 0) 
            {
            if (dinfo->sectorMapCurr >= dinfo->sectorMapCount) {
                if (AtrWriteSector(info,lastSector, secBuff))
                    return(ADOS_FILE_WRITE_ERR);
                }
            else {
                nextSector = AtrSpartaGetFreeSectorDir(info);
                AtrSpartaMarkSectorUsed(info,nextSector,TRUE,TRUE);
                secBuff[0] = nextSector & 0xFF;
                secBuff[1] = nextSector >> 8;
                if (AtrWriteSector(info,lastSector, secBuff))
                    return(ADOS_FILE_WRITE_ERR);
                secBuff[0] = 0;
                secBuff[1] = 0;
                secBuff[2] = lastSector & 0xFF;
                secBuff[3] = lastSector >> 8;
                lastSector = nextSector;
                mapPtr = &secBuff[4];
                }
            }
        else {
            if (AtrWriteSector(info,lastSector, secBuff))
                return(ADOS_FILE_WRITE_ERR);
            if (AtrReadSector(info,nextSector, secBuff)) 
                return(ADOS_FILE_READ_ERR);
            lastSector = nextSector;
            mapPtr = &secBuff[4];
            }

        }

    dinfo->sectorMapCurr = 0;
    return(FALSE);
}
     
static int AtrWriteNewSectorMap(AtrDiskInfo *info, UBYTE *sectorMap, UWORD sectorCount, 
                                UWORD mapSectorCount, ULONG dir)
{
    UBYTE secBuff[0x200];
    UBYTE *mapPtr;
    UWORD *sectorArray;
    UWORD secPtrsPerSec;
    UWORD bytesToXfer;
    UWORD firstMapSector;
    int i;
    int mapSectorNumber = 0;
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;
    
    sectorArray = (UWORD *) calloc(mapSectorCount, sizeof(UWORD));
    if (sectorArray == NULL)
        return 0;
    
    for (i=0;i<mapSectorCount;i++) {
        sectorArray[i] = AtrSpartaGetFreeSectorFile(info);
        AtrSpartaMarkSectorUsed(info,sectorArray[i],TRUE,dir);
    }
    firstMapSector = sectorArray[0];

    secPtrsPerSec = (dinfo->sectorSize - 4)/2;

    mapPtr = sectorMap;
    
    while (sectorCount) 
        {
        if (mapSectorNumber == 0) {
            secBuff[2] = 0;
            secBuff[3] = 0;
            }
        else
            {
            secBuff[2] = sectorArray[mapSectorNumber-1] & 0xFF;
            secBuff[3] = sectorArray[mapSectorNumber-1] >> 8;
            }   
        
        if (mapSectorNumber == (mapSectorCount - 1)) {
            secBuff[0] = 0;
            secBuff[1] = 0;
            }
        else
            {
            secBuff[0] = sectorArray[mapSectorNumber+1] & 0xFF;
            secBuff[1] = sectorArray[mapSectorNumber+1] >> 8;
            } 

        if (sectorCount >= secPtrsPerSec ) 
            bytesToXfer = secPtrsPerSec*2;
        else
            bytesToXfer = sectorCount * 2;

        memcpy(&secBuff[4], mapPtr, secPtrsPerSec*2);
        mapPtr += secPtrsPerSec*2;

        if (sectorCount < secPtrsPerSec) 
            memset(&secBuff[4+bytesToXfer], 0, 
                   (secPtrsPerSec*2) - bytesToXfer); 
        
        if (AtrWriteSector(info,sectorArray[mapSectorNumber], secBuff)) {
            free(sectorArray);
            return(0);
            }
        
        sectorCount -= bytesToXfer/2;
        mapSectorNumber++;
        }
    
    free(sectorArray);
    return(firstMapSector);
}

static void AtrResetSectorMap(AtrDiskInfo *info)
{
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;

    dinfo->sectorMapCurr = 0;
}

static UWORD AtrGetNextSectorFromMap(AtrDiskInfo *info)
{
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;

    if (dinfo->sectorMapCurr < dinfo->sectorMapCount) 
        return(dinfo->sectorMap[dinfo->sectorMapCurr++]);
    else
        return 0;
}

static SpartaDirEntry *AtrSpartaFindDirEntryByName(AtrDiskInfo *info, char *name)
{
    SpartaDirEntry  *pCurr = NULL;
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;

    pCurr = dinfo->pCurrDir;

    while (pCurr) 
        {
        if ((strcmp(name,pCurr->filename) == 0) &&
            ((pCurr->flags & SPARTA_DELETED) != SPARTA_DELETED))
            return(pCurr);
        pCurr = pCurr->pNext;
        }
    return(NULL);
}

static UWORD AtrSpartaGetFreeSectorFile(AtrDiskInfo *info)
{
    UWORD i;
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;

    for (i=dinfo->dataSectorSearchStart;i<=AtrSectorCount(info)+1;i++)
        {
        if (!AtrSpartaIsSectorUsed(info,i) )
            return i;
        }
    return  AtrSpartaGetFreeSectorDir(info);
}

static UWORD AtrSpartaGetFreeSectorDir(AtrDiskInfo *info)
{
    UWORD i;
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;

    for (i=dinfo->dirSectorSearchStart;i<=AtrSectorCount(info)+1;i++)
        {
        if (!AtrSpartaIsSectorUsed(info,i) )
            return i;
         }
    for (i=0;i<dinfo->dirSectorSearchStart;i++)
        {
        if (!AtrSpartaIsSectorUsed(info,i) )
             return i;
        }
    return  0;
}

static int AtrSpartaReadVtoc(AtrDiskInfo *info)
{
    UWORD sector;
    UBYTE secBuf[0x200];
    int i,j,stat;
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;

    memset(dinfo->vtocMap,0,sizeof(dinfo->vtocMap));
    j = 0;
    
    for(sector=dinfo->firstBitmapSector;
         sector < dinfo->firstBitmapSector + dinfo->bitmapSectorCount;
         sector++)
        {
        stat = AtrReadSector(info, sector, secBuf);
        if ( stat )
            return stat;
        for( i=0; i<dinfo->sectorSize; i++ )
            dinfo->vtocMap[j++] = secBuf[i];
        }
    
    return FALSE;
}

static int AtrSpartaWriteVtoc(AtrDiskInfo *info)
{
    UWORD sector;
    int i,j=0, stat;
    UBYTE secBuf[0x200];
    UWORD freeSectors;
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;
    
    for(sector=dinfo->firstBitmapSector;
         sector < dinfo->firstBitmapSector + dinfo->bitmapSectorCount;
         sector++)
        {
        for( i=0; i<dinfo->sectorSize; i++ ) {
            secBuf[i] = dinfo->vtocMap[j++];
        }
        
        stat = AtrWriteSector(info, sector, secBuf);
        if ( stat )
            return stat;
        }
    
    freeSectors = 0;
    for (i=0;i<(dinfo->totalSectors+1);i++) 
       {
       if (!AtrSpartaIsSectorUsed(info,i)) 
          freeSectors++;
       }

    stat = AtrReadSector(info, 1, secBuf);
    if ( stat )
        return stat;

    secBuf[13] = (UWORD)(freeSectors&255);
    secBuf[14] = (UWORD)(freeSectors>>8);
    secBuf[38] = dinfo->volSeqNumber;
    secBuf[18] = dinfo->dataSectorSearchStart & 0xFF;
    secBuf[19] = dinfo->dataSectorSearchStart >> 8;
    secBuf[20] = dinfo->dirSectorSearchStart & 0xFF;
    secBuf[21] = dinfo->dirSectorSearchStart >> 8;
      
    dinfo->diskFreeSectors = freeSectors;
    
    stat = AtrWriteSector(info, 1, secBuf);
    if ( stat )
        return stat;

    return FALSE;

}

static int AtrSpartaIsSectorUsed(AtrDiskInfo *info, UWORD sector)
{
    UWORD entry;
    UBYTE mask;
    UBYTE maskTable[] = {128, 64, 32, 16, 8, 4, 2, 1};
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;

    entry = sector / 8;
    mask = maskTable[ sector & 7];
    return ( !(dinfo->vtocMap[entry] & mask) );
}

static void AtrSpartaMarkSectorUsed(AtrDiskInfo *info, UWORD sector, 
                                     int used, ULONG dir)
{
    UWORD entry;
    UBYTE mask;
    UBYTE maskTable[] = {128, 64, 32, 16, 8, 4, 2, 1};
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;

    entry = sector / 8;
    mask = maskTable[ sector & 7];

    if ( used != FALSE ) {
        dinfo->vtocMap[entry] &= (~mask);
        if (dir)
           dinfo->dirSectorSearchStart = sector;
        else
           dinfo->dataSectorSearchStart = sector;
       }
    else {
        dinfo->vtocMap[entry] |= mask;
        if (dir) {
            if (sector < dinfo->dirSectorSearchStart)
                dinfo->dirSectorSearchStart = sector;
            }
        else {
            if (sector < dinfo->dataSectorSearchStart)
                dinfo->dataSectorSearchStart = sector;
            }
        }
}

static void AtrSpartaDeleteDirList(AtrDiskInfo *info)
{
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;
	SpartaDir* pCurr = dinfo->pDirList;
	SpartaDir* pPrev;

	while( pCurr )
	{
		pPrev = pCurr->pPrev;
		free(pCurr);

		pCurr = pPrev;
	}

    dinfo->pDirList = NULL;
}

static void AtrSpartaDeleteDirEntryList(AtrDiskInfo *info)
{
	AtrSpartaDiskInfo *dinfo = (AtrSpartaDiskInfo *)info->atr_dosinfo;
	SpartaDirEntry* pCurr = dinfo->pCurrDir;
	SpartaDirEntry* pNext;

	while( pCurr )
	{
		pNext = pCurr->pNext;
		free(pCurr);

		pCurr = pNext;
	}

    dinfo->pCurrDir = NULL;
}

static SpartaDirEntry *AtrSpartaCreateDirEntry( SPARTA_DIRENT* pDirEntry)
{
	SpartaDirEntry *pEntry;
    
    pEntry = (SpartaDirEntry *) malloc(sizeof(SpartaDirEntry));
	
    if ( pEntry == NULL )
	{
		return NULL;
	}
    pEntry->dirEntryOffset = pDirEntry->dirEntryOffset;
    
    ADos2Host( pEntry->filename, pDirEntry->atariName );

	pEntry->flags = pDirEntry->flags;
	pEntry->firstSectorMap = pDirEntry->firstSectorMap;
	pEntry->size = pDirEntry->size;
	pEntry->day = pDirEntry->day;
	pEntry->month = pDirEntry->month;
	pEntry->year = pDirEntry->year;
	pEntry->hour = pDirEntry->hour;
	pEntry->minute = pDirEntry->minute;
	pEntry->second = pDirEntry->second;
	
    return pEntry;
}

