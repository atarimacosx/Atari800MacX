/* atrXE.c -  
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
#include "atrXE.h"
#include "atrErr.h"
#include "sys/stat.h"

#define XE_DIR_ENTRY_SIZE 49
#define XE_VTOC_BLOCK     4
#define XE_ROOT_DIR_BLOCK 5

#define XE_LOCKED       0x02
#define XE_SUBDIR		0x01

typedef struct XE_DIRENT
{
	UBYTE flags;
	UBYTE atariName[11];
	UWORD numberOfBlocks;
	UBYTE bytesInLastBlock;
	UBYTE fileNumber;
	UBYTE reserved1;
	UWORD volNumber;
	UWORD blockMapList[12];
	UWORD createdTime;
	UWORD modifiedTime;
	UBYTE reserved3[2];
    ULONG dirEntryOffset;
} XE_DIRENT;

typedef struct XEDirEntry
{
    ULONG dirEntryOffset;
    struct XEDirEntry *pNext;
    struct XEDirEntry *pPrev;
    char filename[ 256 ];
	UWORD numberOfBlocks;
	UBYTE bytesInLastBlock;
	UBYTE fileNumber;
	UBYTE flags;
	UWORD blockMapList[12];
	ULONG size; //Size of file
	UBYTE createdDay;
	UBYTE createdMonth;
	UBYTE createdYear;
	UBYTE modifiedDay;
	UBYTE modifiedMonth;
	UBYTE modifiedYear;
} XEDirEntry;

typedef struct XEDir
{
	struct XEDir *pNext;
	struct XEDir *pPrev;
	char dirname[256];
	UWORD  dirStartBlock;
} XEDir;

typedef struct atrXEDiskInfo {
    XEDirEntry  *pCurrDir;
    XEDir	   *pDirList;

    UWORD volSeqNumber;
    UWORD nextFileNumber;

    UBYTE vtocMap[256];

    UWORD blockMap[1440];
    UWORD blockMapCurr;
    UWORD blockMapCount;

    UBYTE *dirBuffer;
    ULONG dirBufferLen;
	} AtrXEDiskInfo;

static int AtrXEReadDirIntoMem(AtrDiskInfo *info, UWORD dirBlock);
static int AtrXEWriteDirFromMem(AtrDiskInfo *info, UWORD dirBlock);
static void AtrXEFreeDirFromMem(AtrDiskInfo *info);
static int AtrXEReadDir(AtrDiskInfo *info, UWORD dirBlock);
static int AtrXEReadRootDir(AtrDiskInfo *info);
static int AtrXEReadCurrentDir(AtrDiskInfo *info);
static UWORD AtrXECurrentDirStartBlock(AtrDiskInfo *info);
static int AtrXEReadBlockMap(AtrDiskInfo *info, UWORD *startBlock);
static int AtrXEWriteNewBlockMap(AtrDiskInfo *info, UBYTE *blockMap, 
                                UWORD blockCount, UWORD mapBlockCount, 
                                UWORD fileNumber, UWORD *blockMapList);
static UWORD AtrXEGetNextBlockFromMap(AtrDiskInfo *info);
static XEDirEntry *AtrXEFindDirEntryByName(AtrDiskInfo *info, char *name);
static UWORD AtrXEGetFreeBlock(AtrDiskInfo *info);
static int AtrXEReadVtoc(AtrDiskInfo *info, UWORD *freeBlocks);
static int AtrXEWriteVtoc(AtrDiskInfo *info);
static int AtrXEIsBlockUsed(AtrDiskInfo *info, UWORD block);
static void AtrXEMarkBlockUsed(AtrDiskInfo *info, UWORD block, int used);
static void AtrXEDeleteDirList(AtrDiskInfo *info);
static void AtrXEDeleteDirEntryList(AtrDiskInfo *info);
static XEDirEntry *AtrXECreateDirEntry( XE_DIRENT* pDirEntry);
static int AtrXEReadBlock(AtrDiskInfo *info, int block, UBYTE * buffer);
static int AtrXEWriteBlock(AtrDiskInfo *info, int block, UBYTE * buffer);


int AtrXEMount(AtrDiskInfo *info)
{
	AtrXEDiskInfo *dinfo;

	info->atr_dosinfo = (void *) calloc(1, sizeof(AtrXEDiskInfo));
	if (info->atr_dosinfo == NULL)
		return(ADOS_MEM_ERR);
	dinfo = (AtrXEDiskInfo *) info->atr_dosinfo;

    return(AtrXEReadRootDir(info));
}

void AtrXEUnmount(AtrDiskInfo *info)
{
    AtrXEDeleteDirList(info);
    AtrXEDeleteDirEntryList(info);
	free(info->atr_dosinfo);
}

int AtrXEGetDir(AtrDiskInfo *info, UWORD *fileCount, ADosFileEntry *files, 
                ULONG *freeBytes)
{
    XEDirEntry  *pCurr = NULL;
    ADosFileEntry *pFileEntry = files;
    UWORD count = 0;
    char name[15];
    UWORD freeBlocks = 0;
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;

    *fileCount = 0;
    pCurr = dinfo->pCurrDir;
    count = 0;

    AtrXEReadVtoc(info,&freeBlocks);

    while (pCurr) 
        {
        if (pCurr->flags != 0 && (pCurr->flags & DIRE_DELETED) 
                        != DIRE_DELETED) {
        	strcpy(name, pCurr->filename);
            Host2ADos(name,pFileEntry->aname);
            pFileEntry->flags = 0;
            if (pCurr->flags & XE_LOCKED)
                pFileEntry->flags |= DIRE_LOCKED;
            if (pCurr->flags & XE_SUBDIR)
                pFileEntry->flags |= DIRE_SUBDIR;
            pFileEntry->bytes = pCurr->size;
            pFileEntry->createdDay = pCurr->createdDay;
            pFileEntry->createdMonth = pCurr->createdMonth;
            pFileEntry->createdYear = pCurr->createdYear;
            pFileEntry->day = pCurr->modifiedDay;
            pFileEntry->month = pCurr->modifiedMonth;
            pFileEntry->year = pCurr->modifiedYear;
            pFileEntry->hour = 0;
            pFileEntry->minute = 0;
            count++;
            pFileEntry++;
            }
        pCurr = pCurr->pNext;
        }

    *freeBytes = freeBlocks * 256;
    
    *fileCount = count;

    return(FALSE);
}

int AtrXEChangeDir(AtrDiskInfo *info, int cdFlag, char *name)
{
    XEDirEntry  *pDirEntry = NULL;
    UBYTE *pTmp = NULL;
    XEDir *pNew;
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;

	if (cdFlag == CD_UP) {
        if (dinfo->pDirList == NULL) 
            return(FALSE);
		
        if (dinfo->pDirList->pPrev == NULL) {
			free(dinfo->pDirList);
			dinfo->pDirList = NULL;
			return(AtrXEReadRootDir(info));
			}
		else {
			dinfo->pDirList = dinfo->pDirList->pPrev;
			free(dinfo->pDirList->pNext);
			dinfo->pDirList->pNext = NULL;
			return(AtrXEReadDir(info, dinfo->pDirList->dirStartBlock));
			}
		}
	else if (cdFlag == CD_ROOT) {
		return(AtrXEReadRootDir(info));
		}
	else {
        if ((pDirEntry = AtrXEFindDirEntryByName(info, name)) == NULL) {
        	return ADOS_FILE_NOT_FOUND;
        	}
    	
        if ((pDirEntry->flags & XE_SUBDIR) != XE_SUBDIR)
    		return ADOS_NOT_A_DIRECTORY;
    	
        pNew = (XEDir *) malloc(sizeof(XEDir));
    	
        if (pNew == NULL)
    		return(ADOS_MEM_ERR);
        
        pTmp += 3;
		pNew->dirStartBlock = pDirEntry->blockMapList[0];
		
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
		return(AtrXEReadDir(info,dinfo->pDirList->dirStartBlock));
		}
    return FALSE;
}

int AtrXEDeleteDir(AtrDiskInfo *info, char *name)
{
    XEDirEntry  *pDirEntry = NULL;
    UBYTE blockBuff[256];
    UWORD block;
    UWORD dirStartBlock;
    UBYTE *buffPtr;
    ULONG dirSize;
    UWORD freeBlocks;
    int stat;
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;

    if ((pDirEntry = AtrXEFindDirEntryByName(info, name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }

    if ((pDirEntry->flags & XE_LOCKED) == XE_LOCKED) {
        return ADOS_FILE_LOCKED;
        }

  	if ((pDirEntry->flags & XE_SUBDIR) == 0)
   		return ADOS_NOT_A_DIRECTORY;

    if(AtrXEReadDirIntoMem(info,pDirEntry->blockMapList[0]))
        return(ADOS_DIR_READ_ERR);

    buffPtr = dinfo->dirBuffer;
    dirSize = dinfo->dirBufferLen;

    while (*buffPtr && (dirSize >= XE_DIR_ENTRY_SIZE)) 
        {
		if ((*buffPtr & DIRE_DELETED) == 0) {
 		    AtrXEFreeDirFromMem(info);
 		    return(ADOS_DIR_NOT_EMPTY);
            }
		
        buffPtr += XE_DIR_ENTRY_SIZE;
        dirSize -= XE_DIR_ENTRY_SIZE;
        }
    AtrXEFreeDirFromMem(info);
    if (AtrXEReadVtoc(info, &freeBlocks))
        return ADOS_VTOC_READ_ERR;
    
    block = pDirEntry->blockMapList[0];
    
    do {
        if (AtrXEReadBlock(info,block,blockBuff))
            return(ADOS_DIR_READ_ERR);
        AtrXEMarkBlockUsed(info,block,FALSE);
        block = blockBuff[248] + (blockBuff[249] << 8);
    } while (block);
	
    if (AtrXEWriteVtoc(info)) 
        return(ADOS_VTOC_WRITE_ERR);
    
    if (dinfo->pDirList == NULL) 
        dirStartBlock = XE_ROOT_DIR_BLOCK;
    else
        dirStartBlock = dinfo->pDirList->dirStartBlock;
    
    if(AtrXEReadDirIntoMem(info, dirStartBlock))
        return(ADOS_DIR_READ_ERR);
        
    dinfo->dirBuffer[pDirEntry->dirEntryOffset] |= DIRE_DELETED;

    if(AtrXEWriteDirFromMem(info,dirStartBlock))
        return(ADOS_DIR_WRITE_ERR);
    
    stat = AtrXEReadCurrentDir(info);
    if (stat)
    	return(stat);

    return FALSE;
}

int AtrXEMakeDir(AtrDiskInfo *info, char *name)
{
    XEDirEntry  *pDirEntry = NULL;
    UBYTE blockBuff[0x100];
    UWORD block;
    int i, stat;
    time_t currTime;
    struct tm *localTime;
    ULONG currentDirLen;
    int dirEntryIndex = 0;
    UWORD dirStartBlock;
    UWORD freeBlocks;
    UWORD timeWord;
    UBYTE year;
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;

    for (i=0;i<strlen(name);i++) 
        {
        if (islower(name[i]))
            name[i] -= 32;
        }

    if ((pDirEntry = AtrXEFindDirEntryByName(info, name)) != NULL) 
            return ADOS_DUPLICATE_NAME;
 
    if (AtrXEReadVtoc(info, &freeBlocks)) {
        return(ADOS_VTOC_READ_ERR);
        }
    
    if (freeBlocks < 1) {
        return(ADOS_DISK_FULL);
        }
        
    block = AtrXEGetFreeBlock(info);
    AtrXEMarkBlockUsed(info,block,TRUE);
   
	if (dinfo->pDirList == NULL) 
        dirStartBlock = XE_ROOT_DIR_BLOCK;
    else
        dirStartBlock = dinfo->pDirList->dirStartBlock;
    
    /* Fill in blank directory block */
	memset(blockBuff, 0, 255);
	blockBuff[255] = 0xff;
	blockBuff[252] = dinfo->volSeqNumber & 0xFF;
	blockBuff[253] = dinfo->volSeqNumber >> 8;
	blockBuff[250] = dinfo->nextFileNumber & 0xFF;
	blockBuff[251] = dinfo->nextFileNumber >> 8;
		
    if (AtrXEWriteBlock(info,block, blockBuff)) {
        return(ADOS_FILE_WRITE_ERR);
        }
   
    if(AtrXEReadDirIntoMem(info,dirStartBlock))
        return(ADOS_DIR_READ_ERR);

    currentDirLen = dinfo->dirBufferLen;
     
    for (i=0;i < currentDirLen; i+= XE_DIR_ENTRY_SIZE) 
        {
        if ((dinfo->dirBuffer[i] & DIRE_DELETED) ||
             (dinfo->dirBuffer[i] == 0)) 
            {
            dirEntryIndex = i;
            break;
            }
        }
    
    /* No directory entries free, need to add one to end */
    if (i >= currentDirLen) 
        {
        /* Need to add a block to the directory */
        if ((dinfo->dirBuffer = 
             realloc(dinfo->dirBuffer, dinfo->dirBufferLen+256)) == NULL)
            return(ADOS_MEM_ERR);
            
        dinfo->dirBufferLen += 5*XE_DIR_ENTRY_SIZE;

        dirEntryIndex = currentDirLen;
        }
    
    dinfo->dirBuffer[dirEntryIndex] = DIRE_IN_USE | XE_SUBDIR;
    Host2ADos(name,&dinfo->dirBuffer[dirEntryIndex+1]);
    dinfo->dirBuffer[dirEntryIndex+12] = 0;
    dinfo->dirBuffer[dirEntryIndex+13] = 0;
    dinfo->dirBuffer[dirEntryIndex+14] = 0;
    dinfo->dirBuffer[dirEntryIndex+15] = dinfo->nextFileNumber & 0xff;
    dinfo->dirBuffer[dirEntryIndex+16] = dinfo->nextFileNumber >> 8;
    dinfo->nextFileNumber++;
    dinfo->dirBuffer[dirEntryIndex+17] = dinfo->volSeqNumber & 0xFF;
    dinfo->dirBuffer[dirEntryIndex+18] = dinfo->volSeqNumber >> 8;
    dinfo->dirBuffer[dirEntryIndex+19] = block & 0xFF;
    dinfo->dirBuffer[dirEntryIndex+20] = block >> 8;
    
    time(&currTime);
    localTime = localtime(&currTime);
    if (localTime->tm_year > 99) 
        year = localTime->tm_year - 100;
    else
        year = localTime->tm_year;
    timeWord = (year << 9) +
               ((localTime->tm_mon+1) << 5) +
               (localTime->tm_mday);
    dinfo->dirBuffer[dirEntryIndex+43] = timeWord >> 8;
    dinfo->dirBuffer[dirEntryIndex+44] = timeWord & 0xFF;
    dinfo->dirBuffer[dirEntryIndex+45] = timeWord >> 8;
    dinfo->dirBuffer[dirEntryIndex+46] = timeWord & 0xFF;
    
    if(AtrXEWriteDirFromMem(info,dirStartBlock))
        return(ADOS_DIR_WRITE_ERR);
    
    AtrXEFreeDirFromMem(info);
    
    AtrXEWriteVtoc(info);
        
    stat = AtrXEReadCurrentDir(info);
    if (stat)
    	return(stat);

    return FALSE;
}

int AtrXELockFile(AtrDiskInfo *info, char *name, int lock)
{
    XEDirEntry  *pDirEntry = NULL;
    UWORD dirStartBlock;
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;

    if ((pDirEntry = AtrXEFindDirEntryByName(info,name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }
        
    if (dinfo->pDirList == NULL) 
        dirStartBlock = XE_ROOT_DIR_BLOCK;
    else
        dirStartBlock = dinfo->pDirList->dirStartBlock;
    
    if(AtrXEReadDirIntoMem(info,dirStartBlock))
        return(ADOS_DIR_READ_ERR);
        
    if (lock) { 
        dinfo->dirBuffer[pDirEntry->dirEntryOffset] |= XE_LOCKED;
        pDirEntry->flags |= XE_LOCKED;
        }
    else {
        dinfo->dirBuffer[pDirEntry->dirEntryOffset] &= ~XE_LOCKED;
        pDirEntry->flags &= ~XE_LOCKED;
    }
    
    if(AtrXEWriteDirFromMem(info,dirStartBlock))
        return(ADOS_DIR_WRITE_ERR);

    AtrXEFreeDirFromMem(info);

    return FALSE;
}

int AtrXERenameFile(AtrDiskInfo *info, char *name, char *newname)
{
    XEDirEntry  *pDirEntry = NULL;
    UWORD dirStartBlock;
	UBYTE dosName[13];
    int stat;
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;

    if ((pDirEntry = AtrXEFindDirEntryByName(info,name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }

    if ((pDirEntry->flags & XE_LOCKED) == XE_LOCKED) {
        return ADOS_FILE_LOCKED;
        }

	Host2ADos(newname, dosName);
	ADos2Host(newname, dosName);

    if (AtrXEFindDirEntryByName(info,newname) != NULL) {
		return ADOS_DUPLICATE_NAME;
		}
        
    if (dinfo->pDirList == NULL) 
        dirStartBlock = XE_ROOT_DIR_BLOCK;
    else
        dirStartBlock = dinfo->pDirList->dirStartBlock;
    
    if(AtrXEReadDirIntoMem(info,dirStartBlock))
        return(ADOS_DIR_READ_ERR);
        
    Host2ADos(newname,&dinfo->dirBuffer[pDirEntry->dirEntryOffset + 1]);
    
    if(AtrXEWriteDirFromMem(info, dirStartBlock)) {
		AtrXEFreeDirFromMem(info);
        return(ADOS_DIR_WRITE_ERR);
		}

	AtrXEFreeDirFromMem(info);
    
    stat = AtrXEReadCurrentDir(info);
    if (stat)
    	return stat;

    return FALSE;
}

int AtrXEDeleteFile(AtrDiskInfo *info, char *name)
{
    XEDirEntry  *pDirEntry = NULL;
    UWORD block;
    UWORD dirStartBlock;
    UWORD freeBlocks;
    int stat;
    int i;
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;

    if ((pDirEntry = AtrXEFindDirEntryByName(info, name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }

    if ((pDirEntry->flags & XE_LOCKED) == XE_LOCKED) {
        return ADOS_FILE_LOCKED;
        }

  	if (pDirEntry->flags & XE_SUBDIR)
   		return ADOS_FILE_NOT_FOUND;

    if (AtrXEReadVtoc(info, &freeBlocks))
        return ADOS_VTOC_READ_ERR;
    
    if (AtrXEReadBlockMap(info, pDirEntry->blockMapList))
        return ADOS_FILE_READ_ERR;
	
    while( (block = AtrXEGetNextBlockFromMap(info)) )
	{
        AtrXEMarkBlockUsed(info,block, FALSE);
	}
	
	for (i=0;i<12;i++)
	{
       if (pDirEntry->blockMapList[i])
           AtrXEMarkBlockUsed(info,pDirEntry->blockMapList[i],FALSE);
	}
	
    if (AtrXEWriteVtoc(info)) 
        return(ADOS_VTOC_WRITE_ERR);
    
    if (dinfo->pDirList == NULL) 
        dirStartBlock = XE_ROOT_DIR_BLOCK;
    else
        dirStartBlock = dinfo->pDirList->dirStartBlock;
    
    if(AtrXEReadDirIntoMem(info,dirStartBlock))
        return(ADOS_DIR_READ_ERR);
        
    dinfo->dirBuffer[pDirEntry->dirEntryOffset] |= DIRE_DELETED;

    if(AtrXEWriteDirFromMem(info, dirStartBlock))
        return(ADOS_DIR_WRITE_ERR);

    AtrXEFreeDirFromMem(info);
    
    stat = AtrXEReadCurrentDir(info);
    if (stat)
    	return(stat);

    return(FALSE);
}

int AtrXEImportFile(AtrDiskInfo *info, char *filename, int lfConvert, int tabConvert)
{
    FILE *inFile;
    ULONG file_length, total_file_length;
    UWORD numBlocksNeeded, numMapBlocksNeeded;
    UBYTE numToWrite;
    UBYTE blockBuff[0x100];
    XEDirEntry  *pDirEntry;
    UWORD block;
    char *slash;
    int stat;
	UBYTE dosName[12];
	int i;
    UBYTE *mapBuffer, *mapBufferCurrent;
    time_t currTime;
    struct tm *localTime;
    ULONG currentDirLen;
    int dirEntryIndex = 0;
    UWORD dirStartBlock;
    UWORD firstMapBlock;
    UWORD freeBlocks;
    UWORD year;
    UWORD timeWord;
    UWORD blockNumber = 0;
    UWORD blockMapList[12];
    struct stat filestats;
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;

    inFile = fopen( filename, "rb");

    if (inFile == NULL)
        {
        return(ADOS_HOST_FILE_NOT_FOUND);
        }

    fstat(fileno(inFile),&filestats);

    if ((slash = strrchr(filename,'/')) != NULL)
        {
        filename = slash + 1;
        }

	Host2ADos(filename, dosName);
	ADos2Host(filename, dosName);

    if ((pDirEntry = AtrXEFindDirEntryByName(info,filename)) != NULL) {

    	if ((pDirEntry->flags & XE_SUBDIR) == XE_SUBDIR)
            {
            return ADOS_FILE_IS_A_DIR;
            }
    	
        if (AtrXEDeleteFile(info,filename)) {
            fclose(inFile);
            return ADOS_DELETE_FILE_ERR;
            }
        }

    fseek(inFile, 0L, SEEK_END);
    file_length = ftell(inFile);
    fseek(inFile, 0L, SEEK_SET);
    total_file_length = file_length;

    if (file_length == 0) 
        numBlocksNeeded = 1;
    else if ((file_length % 250 ) == 0) 
        numBlocksNeeded = file_length / 250 ;
    else
        numBlocksNeeded = file_length / 250  + 1;

    if ((numBlocksNeeded % 124) == 0)
        numMapBlocksNeeded = numBlocksNeeded / 124;
    else
        numMapBlocksNeeded = numBlocksNeeded / 124 + 1;

    if (AtrXEReadVtoc(info, &freeBlocks)) {
        fclose(inFile);
        return(ADOS_VTOC_READ_ERR);
        }
    
    if ((numBlocksNeeded + numMapBlocksNeeded + 1) > freeBlocks) {
        fclose(inFile);
        return(ADOS_DISK_FULL);
    }

    mapBuffer = (UBYTE *) calloc(numMapBlocksNeeded, 256);
    if (mapBuffer == NULL) 
        return ADOS_MEM_ERR;

    mapBufferCurrent = mapBuffer;
    
    do {
        if (file_length >= 250 ) 
            numToWrite = 250;
        else
            numToWrite = file_length;
       
        block = AtrXEGetFreeBlock(info);
        AtrXEMarkBlockUsed(info,block,TRUE);
        *mapBufferCurrent++ = block & 0xFF;
        *mapBufferCurrent++ = block >> 8;

        if (fread(blockBuff,1,numToWrite,inFile) != numToWrite)
            {
            fclose(inFile);
            return(ADOS_HOST_READ_ERR);
            }

        if (lfConvert)
            HostLFToAtari(blockBuff, numToWrite);
            
        if (tabConvert)
            HostTabToAtari(blockBuff, numToWrite);
            
		blockBuff[250] = dinfo->nextFileNumber & 0xFF;
		blockBuff[251] = dinfo->nextFileNumber >> 8;
		blockBuff[252] = dinfo->volSeqNumber & 0xFF;
		blockBuff[253] = dinfo->volSeqNumber >> 8;
		blockBuff[254] = blockNumber & 0xff;
		blockBuff[255] = blockNumber >> 8;
        
        if (AtrXEWriteBlock(info,block, blockBuff))
            {
            fclose(inFile);
            return(ADOS_FILE_WRITE_ERR);
            }
        
        file_length -= numToWrite;
        blockNumber++;
    } while (file_length > 0);
    
    if ((firstMapBlock = 
         AtrXEWriteNewBlockMap(info,mapBuffer, numBlocksNeeded, 
                              numMapBlocksNeeded, dinfo->nextFileNumber,
                              blockMapList)))
        return ADOS_FILE_WRITE_ERR;

    if (dinfo->pDirList == NULL) 
        dirStartBlock = XE_ROOT_DIR_BLOCK;
    else
        dirStartBlock = dinfo->pDirList->dirStartBlock;
    
    if(AtrXEReadDirIntoMem(info,dirStartBlock))
        return(ADOS_DIR_READ_ERR);

    currentDirLen = dinfo->dirBufferLen;
    for (i=0;i < currentDirLen; i+= XE_DIR_ENTRY_SIZE) 
        {
        if ((dinfo->dirBuffer[i] & DIRE_DELETED) ||
             (dinfo->dirBuffer[i] == 0)) 
            {
            dirEntryIndex = i;
            break;
            }
        }
    
    /* No directory entries free, need to add one to end */
    if (i >= currentDirLen) 
        {
        /* Need to add a block to the directory */
        if ((dinfo->dirBuffer = 
              realloc(dinfo->dirBuffer, dinfo->dirBufferLen+256)) == NULL)
            return(ADOS_MEM_ERR);
            
        dinfo->dirBufferLen += 5*XE_DIR_ENTRY_SIZE;

        dirEntryIndex = currentDirLen;
        }
    dinfo->dirBuffer[dirEntryIndex] = DIRE_IN_USE;
    Host2ADos(filename,&dinfo->dirBuffer[dirEntryIndex+1]);
    dinfo->dirBuffer[dirEntryIndex+12] = (numBlocksNeeded-1) & 0xff;
    dinfo->dirBuffer[dirEntryIndex+13] = (numBlocksNeeded-1) >> 8;
    dinfo->dirBuffer[dirEntryIndex+14] = total_file_length % 250;
    dinfo->dirBuffer[dirEntryIndex+15] = dinfo->nextFileNumber & 0xff;
    dinfo->dirBuffer[dirEntryIndex+16] = dinfo->nextFileNumber >> 8;
    dinfo->nextFileNumber++;
    dinfo->dirBuffer[dirEntryIndex+16] = 0;
    dinfo->dirBuffer[dirEntryIndex+17] = dinfo->volSeqNumber & 0xFF;
    dinfo->dirBuffer[dirEntryIndex+18] = dinfo->volSeqNumber >> 8;
    for (i=0;i<12;i++)
    {
        dinfo->dirBuffer[dirEntryIndex+19+2*i] = blockMapList[i] & 0xFF;
        dinfo->dirBuffer[dirEntryIndex+20+2*i] = blockMapList[i] >> 8;
    }
    
    time(&currTime);
    localTime = localtime(&currTime);
    if (localTime->tm_year > 99) 
        year = localTime->tm_year - 100;
    else
        year = localTime->tm_year;
    timeWord = (year << 9) +
               ((localTime->tm_mon+1) << 5) +
               (localTime->tm_mday);
    dinfo->dirBuffer[dirEntryIndex+45] = timeWord >> 8;
    dinfo->dirBuffer[dirEntryIndex+46] = timeWord & 0xFF;
	
    if (localTime->tm_year > 99) 
        year = localTime->tm_year - 100;
    else
        year = localTime->tm_year;
    timeWord = (year << 9) +
               ((localTime->tm_mon+1) << 5) +
               (localTime->tm_mday);
    dinfo->dirBuffer[dirEntryIndex+43] = timeWord >> 8;
    dinfo->dirBuffer[dirEntryIndex+44] = timeWord & 0xFF;    
    
    if(AtrXEWriteDirFromMem(info,dirStartBlock))
        return(ADOS_DIR_WRITE_ERR);

    AtrXEFreeDirFromMem(info);
    
    AtrXEWriteVtoc(info);
    
    stat = AtrXEReadCurrentDir(info);
    if (stat)
    	return(stat);

    return FALSE;
}

int AtrXEExportFile(AtrDiskInfo *info, char *nameToExport, char* outFile, 
                    int lfConvert, int tabConvert)
{
	XEDirEntry* pDirEntry;
	UBYTE blockBuff[ 0x0100 ];
    ULONG bytes;
	UWORD block, bytesToWrite;
	FILE *output = NULL;

    if ((pDirEntry = AtrXEFindDirEntryByName(info,nameToExport)) == NULL) {
        return(ADOS_FILE_NOT_FOUND);
        }
	
    bytes = pDirEntry->bytesInLastBlock +
            pDirEntry->numberOfBlocks * 250;

    if (AtrXEReadBlockMap(info,pDirEntry->blockMapList))
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
        if (bytes > 250) 
            bytesToWrite = 250;
        else
            bytesToWrite = bytes;

        block = AtrXEGetNextBlockFromMap(info);

        if ( AtrXEReadBlock(info,block, blockBuff))
		    {
			return ADOS_FILE_READ_ERR;
		    }
			
        if (lfConvert)
            AtariLFToHost(blockBuff, 250);
                    
        if (tabConvert)
            AtariTabToHost(blockBuff, 250);
                    
		if (fwrite( blockBuff, 1, bytesToWrite ,output) != bytesToWrite)
            {
            fclose(output);
            return ADOS_HOST_WRITE_ERR;
            }

		bytes -= bytesToWrite;
	}

	fclose( output );

	return FALSE;
}

static int AtrXEReadDirIntoMem(AtrDiskInfo *info, UWORD dirBlock)
{
    UWORD block;
    UBYTE *buffPtr;
    ULONG currentOffset;
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;
    
    dinfo->dirBufferLen = 0;
    
    dinfo->dirBuffer = (UBYTE *) calloc(1,256);
    if (dinfo->dirBuffer == NULL)
       return(ADOS_MEM_ERR);

    buffPtr = dinfo->dirBuffer;
    
    block = dirBlock;
    
    do
        {
        if (dinfo->dirBufferLen != 0) {
            currentOffset = buffPtr - dinfo->dirBuffer;
            dinfo->dirBuffer = 
                realloc(dinfo->dirBuffer, dinfo->dirBufferLen+256);
            if (dinfo->dirBuffer == NULL) 
               return(ADOS_MEM_ERR);
            buffPtr = dinfo->dirBuffer + currentOffset;
            } 
        if (AtrXEReadBlock(info,block,buffPtr)) {
            free(dinfo->dirBuffer);
            dinfo->dirBufferLen = 0;
            return ADOS_DIR_READ_ERR;
            }
        block = buffPtr[248] + (buffPtr[249] << 8);
        buffPtr += 5*XE_DIR_ENTRY_SIZE;
        dinfo->dirBufferLen += 5*XE_DIR_ENTRY_SIZE;
        } while (block);
    
    return FALSE;
}

static int AtrXEWriteDirFromMem(AtrDiskInfo *info, UWORD dirBlock)
{
    UWORD block, newBlock, lastBlock;
    UBYTE currBlockSeq;
    UBYTE *buffPtr;
    UBYTE blockBuff[256];
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;

    buffPtr = dinfo->dirBuffer;
    
    block = dirBlock;        
	
    do
        {
        if (AtrXEReadBlock(info,block,blockBuff)) {
            AtrXEFreeDirFromMem(info);
            return ADOS_DIR_READ_ERR;
            }
        memcpy(blockBuff, buffPtr, 5*XE_DIR_ENTRY_SIZE); 
        if (AtrXEWriteBlock(info,block,blockBuff)) {
            AtrXEFreeDirFromMem(info);
            return ADOS_DIR_WRITE_ERR;
            }
        lastBlock = block;
        block = blockBuff[248] + (blockBuff[249] << 8);
        buffPtr += 5*XE_DIR_ENTRY_SIZE;
        dinfo->dirBufferLen -= 5*XE_DIR_ENTRY_SIZE;
        } while (block);
    
    if (dinfo->dirBufferLen != 0) {
        currBlockSeq = blockBuff[254];
        newBlock = AtrXEGetFreeBlock(info);
        AtrXEMarkBlockUsed(info,newBlock,TRUE);
       	blockBuff[248] = newBlock & 0xFF;
       	blockBuff[249] = newBlock >> 8;
        if (AtrXEWriteBlock(info,lastBlock,blockBuff)) {
           AtrXEFreeDirFromMem(info);
           return ADOS_DIR_WRITE_ERR;
           }
      	memset(blockBuff, 0, 256);
       	memcpy(blockBuff, buffPtr, 5*XE_DIR_ENTRY_SIZE);
       	blockBuff[254] = currBlockSeq+1;
        if (AtrXEWriteBlock(info,newBlock,blockBuff)) {
            AtrXEFreeDirFromMem(info);
            return ADOS_DIR_READ_ERR;
            }
        }

    return FALSE;
}

static void AtrXEFreeDirFromMem(AtrDiskInfo *info)
{
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;

    if (dinfo->dirBuffer) 
        free(dinfo->dirBuffer);
    dinfo->dirBuffer = NULL;
    dinfo->dirBufferLen = 0;
}

static int AtrXEReadDir(AtrDiskInfo *info, UWORD dirBlock)
{
    XEDirEntry  *pEntry;
	XEDirEntry  *pPrev = NULL;
	XE_DIRENT dirEntry;
    UBYTE *buffPtr;
    UWORD offset;
    int i;
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;
    
    if (dinfo->pCurrDir) 
        AtrXEDeleteDirEntryList(info);
    
    if (AtrXEReadDirIntoMem(info,dirBlock)) {
        return ADOS_DIR_READ_ERR;
        }

    buffPtr = dinfo->dirBuffer;
    offset = 0;
    while (*buffPtr ) 
        {
		dirEntry.flags = buffPtr[0];
        memcpy(dirEntry.atariName, &buffPtr[1],11);
        dirEntry.numberOfBlocks = buffPtr[12] + (buffPtr[13] << 8);
        dirEntry.bytesInLastBlock = buffPtr[14];
        dirEntry.fileNumber = buffPtr[15];
        dirEntry.volNumber = buffPtr[17] + (buffPtr[18] << 8);
        for (i=0;i<12;i++)
        {
           dirEntry.blockMapList[i] = buffPtr[19+2*i] + (buffPtr[20+2*i] << 8);
        }
        dirEntry.createdTime = buffPtr[44] + (buffPtr[43] << 8);
        dirEntry.modifiedTime = buffPtr[46] + (buffPtr[45] << 8);
        dirEntry.dirEntryOffset = offset;
		
        pEntry = AtrXECreateDirEntry( &dirEntry);
		if (pEntry == NULL)
			{
			AtrXEDeleteDirEntryList(info);
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
		
        buffPtr += XE_DIR_ENTRY_SIZE;
        offset += XE_DIR_ENTRY_SIZE;
        }

    AtrXEFreeDirFromMem(info);

	return FALSE;
}

static int AtrXEReadRootDir(AtrDiskInfo *info)
{	
	AtrXEDeleteDirList(info);
	return(AtrXEReadDir(info, XE_ROOT_DIR_BLOCK));
}

static int AtrXEReadCurrentDir(AtrDiskInfo *info)
{
    return(AtrXEReadDir(info,AtrXECurrentDirStartBlock(info)));
}

static UWORD AtrXECurrentDirStartBlock(AtrDiskInfo *info)
{
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;

    if (dinfo->pDirList) 
        return(dinfo->pDirList->dirStartBlock);
    else
        return(XE_ROOT_DIR_BLOCK);
}

static int AtrXEReadBlockMap(AtrDiskInfo *info, UWORD *blockMapList)
{
    UBYTE blockBuff[0x100];
    UBYTE *mapPtr;
    UWORD nextBlock;
    UWORD block;
    int i;
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;

    if (AtrXEReadBlock(info,*blockMapList++, blockBuff)) 
        return(ADOS_FILE_READ_ERR);

    dinfo->blockMapCurr = 0;
    dinfo->blockMapCount = 0;

    do {
        mapPtr = blockBuff;
        for (i=0;i<125;i++) 
            {
            block =  *mapPtr + ((*(mapPtr+1)) << 8);
            
            if (block) {
                dinfo->blockMap[dinfo->blockMapCurr++] = 
                    *mapPtr + ((*(mapPtr+1)) << 8);
                dinfo->blockMapCount++;
                mapPtr += 2;
                }
            else
                break;
            }
            
        nextBlock = *blockMapList;

        if (nextBlock) 
            if (AtrXEReadBlock(info,*blockMapList++, blockBuff)) 
                return(ADOS_FILE_READ_ERR);
    } while (nextBlock);
    
    dinfo->blockMapCurr = 0;

    return(FALSE);
}
     
static int AtrXEWriteNewBlockMap(AtrDiskInfo *info, UBYTE *blockMap, 
                                UWORD blockCount, UWORD mapBlockCount, 
                                UWORD fileNumber, UWORD *blockMapList)
{
    UBYTE blockBuff[0x100];
    UBYTE *mapPtr;
    UWORD bytesToXfer;
    int i;
    UBYTE mapBlockNumber = 0;
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;

    for (i=0;i<mapBlockCount;i++) {
        blockMapList[i] = AtrXEGetFreeBlock(info);
        AtrXEMarkBlockUsed(info,blockMapList[i],TRUE);
    }
    for (i=mapBlockCount;i<12;i++)
        blockMapList[i] = 0;

    mapPtr = blockMap;
    
    while (blockCount) 
        {
        blockBuff[255] = 0x80 | (mapBlockNumber*2);  
        blockBuff[254] = mapBlockNumber*2;
        blockBuff[253] = dinfo->volSeqNumber >> 8;
        blockBuff[252] = dinfo->volSeqNumber & 0xFF;
        blockBuff[251] = fileNumber >> 8;
        blockBuff[250] = fileNumber & 0xFF;
         
        if (blockCount >= 125 ) 
            bytesToXfer = 125*2;
        else
            bytesToXfer = blockCount * 2;

        memcpy(blockBuff, mapPtr, 125*2);
        mapPtr += 125*2;

        if (blockCount < 125) 
            memset(&blockBuff[bytesToXfer], 0, 
                   (125*2) - bytesToXfer); 
        
        if (AtrXEWriteBlock(info,blockMapList[mapBlockNumber], blockBuff)) {
            return(ADOS_FILE_WRITE_ERR);
            }
            
        blockCount -= bytesToXfer/2;
        mapBlockNumber++;
        }
        
    if (mapBlockCount<12)
    {
    }
    
    return(0);
}

static UWORD AtrXEGetNextBlockFromMap(AtrDiskInfo *info)
{
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;

    if (dinfo->blockMapCurr < dinfo->blockMapCount) 
        return(dinfo->blockMap[dinfo->blockMapCurr++]);
    else
        return 0;
}

static XEDirEntry *AtrXEFindDirEntryByName(AtrDiskInfo *info, char *name)
{
    XEDirEntry  *pCurr = NULL;
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;

    pCurr = dinfo->pCurrDir;

    while (pCurr) 
        {
        if ((strcmp(name,pCurr->filename) == 0) &&
            ((pCurr->flags & DIRE_DELETED) != DIRE_DELETED))
            return(pCurr);
        pCurr = pCurr->pNext;
        }
    return(NULL);
}

static UWORD AtrXEGetFreeBlock(AtrDiskInfo *info)
{
    UWORD i;
	UWORD blockCount;

	blockCount = AtrSectorCount(info);
	if (AtrSectorSize(info) == 128)
        blockCount /= 2;

    for (i=1;blockCount+1;i++)
        {
        if (!AtrXEIsBlockUsed(info,i) )
            {
            return i;
            }
        }

    return  0;
}

static int AtrXEReadVtoc(AtrDiskInfo *info, UWORD *freeBlocks)
{
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;

    if(AtrXEReadBlock(info,XE_VTOC_BLOCK,dinfo->vtocMap))
        return(ADOS_DISK_READ_ERR);
    
    dinfo->volSeqNumber = dinfo->vtocMap[8] + (dinfo->vtocMap[9] << 8);
    dinfo->nextFileNumber = dinfo->vtocMap[6] + (dinfo->vtocMap[7] << 8) + 1;

    *freeBlocks = dinfo->vtocMap[4] + (dinfo->vtocMap[5] << 8);    

    return FALSE;
}

static int AtrXEWriteVtoc(AtrDiskInfo *info)
{
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;
	UWORD blockCount;
	UWORD freeBlocks;
	int i;

	blockCount = AtrSectorCount(info);
	if (AtrSectorSize(info) == 128)
        blockCount /= 2;

    freeBlocks = 0;
    for (i=1;i<blockCount+1;i++)
        if (!AtrXEIsBlockUsed(info,i))
            freeBlocks++;
    dinfo->vtocMap[4] = freeBlocks & 0xFF;
    dinfo->vtocMap[5] = freeBlocks >> 8;
    
    dinfo->vtocMap[6] = (dinfo->nextFileNumber-1) & 0xff;
    dinfo->vtocMap[7] = (dinfo->nextFileNumber-1) >> 8;
    
    if(AtrXEWriteBlock(info,XE_VTOC_BLOCK,dinfo->vtocMap))
        return(ADOS_DISK_WRITE_ERR);
    
    return FALSE;

}

static int AtrXEIsBlockUsed(AtrDiskInfo *info, UWORD block)
{
    UWORD entry;
    UBYTE mask;
    UBYTE maskTable[] = {128, 64, 32, 16, 8, 4, 2, 1};
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;

    entry = (block-1) / 8;
    mask = maskTable[ (block-1) & 7];
    return ( !(dinfo->vtocMap[entry + 10] & mask) );
}

static void AtrXEMarkBlockUsed(AtrDiskInfo *info, UWORD block, int used)
{
    UWORD entry;
    UBYTE mask;
    UBYTE maskTable[] = {128, 64, 32, 16, 8, 4, 2, 1};
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;

    entry = (block-1) / 8;
    mask = maskTable[ (block-1) & 7];

    if ( used != FALSE )
        dinfo->vtocMap[entry + 10] &= (~mask);
    else
        dinfo->vtocMap[entry + 10] |= mask;
}

static void AtrXEDeleteDirList(AtrDiskInfo *info)
{
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;
	XEDir* pCurr = dinfo->pDirList;
	XEDir* pPrev;

	while( pCurr )
	{
		pPrev = pCurr->pPrev;
		free(pCurr);

		pCurr = pPrev;
	}

    dinfo->pDirList = NULL;
}

static void AtrXEDeleteDirEntryList(AtrDiskInfo *info)
{
	AtrXEDiskInfo *dinfo = (AtrXEDiskInfo *)info->atr_dosinfo;
	XEDirEntry* pCurr = dinfo->pCurrDir;
	XEDirEntry* pNext;

	while( pCurr )
	{
		pNext = pCurr->pNext;
		free(pCurr);

		pCurr = pNext;
	}

    dinfo->pCurrDir = NULL;
}

static XEDirEntry *AtrXECreateDirEntry( XE_DIRENT* pDirEntry)
{
	XEDirEntry *pEntry;
	int i;
    
    pEntry = (XEDirEntry *) malloc(sizeof(XEDirEntry));
	
    if ( pEntry == NULL )
	{
		return NULL;
	}
    pEntry->dirEntryOffset = pDirEntry->dirEntryOffset;
    
    ADos2Host( pEntry->filename, pDirEntry->atariName );
	pEntry->flags = pDirEntry->flags;
	pEntry->numberOfBlocks = pDirEntry->numberOfBlocks;
	pEntry->bytesInLastBlock = pDirEntry->bytesInLastBlock;
	pEntry->size = pEntry->numberOfBlocks * 256 + pEntry->bytesInLastBlock;
	pEntry->fileNumber = pDirEntry->fileNumber;
	for (i=0;i<12;i++)
	   pEntry->blockMapList[i] = pDirEntry->blockMapList[i];
	pEntry->createdDay = pDirEntry->createdTime & 0x1F;
	pEntry->createdMonth = (pDirEntry->createdTime & 0x01E0) >> 5;
	pEntry->createdYear = (pDirEntry->createdTime & 0xFE00) >> 9;
	pEntry->modifiedDay = pDirEntry->modifiedTime & 0x1F;
	pEntry->modifiedMonth = (pDirEntry->modifiedTime & 0x01E0) >> 5;
	pEntry->modifiedYear = (pDirEntry->modifiedTime & 0xFE00) >> 9;
	
    return pEntry;
}

static int AtrXEReadBlock(AtrDiskInfo *info, int block, UBYTE * buffer)
{
    int status;
    
    if (AtrSectorSize(info) == 256)
        return(AtrReadSector(info, block, buffer));
    else {
        status = AtrReadSector(info, block*2, buffer);
        if (status)
            return(status);
        return(AtrReadSector(info, block*2+1, buffer+128));
        }
}

static int AtrXEWriteBlock(AtrDiskInfo *info, int block, UBYTE * buffer)
{
    int status;
    
    if (AtrSectorSize(info) == 256)
        return(AtrWriteSector(info, block, buffer));
    else {
        status = AtrWriteSector(info, block*2, buffer);
        if (status)
            return(status);
        return(AtrWriteSector(info, block*2+1, buffer+128));
        }
}


