/* atrDos4.c -  
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
#include <ctype.h>
#include <sys/fcntl.h>
#include "atari.h"
#include "atrUtil.h"
#include "atrMount.h"
#include "atrDos4.h"
#include "atrErr.h"

#define DOS4_FIRST_BLK_NUM	8
#define DOS4_26_FAT_SECTOR	515
#define DOS4_18_FAT_SECTOR	360
#define DOS4_BLOCK_SIZE		(6 * 128)
#define FAT_FREE_BLOCK       0x00

typedef struct Dos4_DIRENT {
	UBYTE flags;		/* flags */
	UBYTE numBlocks;	/* number of allocated blocks */
	UBYTE lastUsedByte; /* last used byte in last sector */
	UBYTE firstBlock;	/* first allocated block */
	UBYTE zero;
	UBYTE atariName[11];	/* file name, space padded */
} Dos4_DIRENT;

typedef struct Dos4DirEntry
{
    struct Dos4DirEntry *pNext;
    struct Dos4DirEntry *pPrev;
    char filename[ 256 ];
    UWORD fileNumber;
	UBYTE flags;
	UBYTE numBlocks;	/* number of allocated blocks */
	UBYTE firstBlock;	/* first allocated block */
	UBYTE lastUsedByte;	/* last used byte in last sector */
} Dos4DirEntry;

typedef struct atrDos4DiskInfo {
    Dos4DirEntry  *pRoot;
    UWORD fatSector;
    UWORD rootDirSector;
    UWORD numDirSectors;
    UBYTE firstFreeBlock;
    UBYTE numFreeBlocks;
    UWORD fatSize;
    UBYTE *fat;
	} AtrDos4DiskInfo;

static int AtrDos4ReadRootDir(AtrDiskInfo *info);
static Dos4DirEntry *AtrDos4FindDirEntryByName(AtrDiskInfo *info, char *name);
static UBYTE AtrDos4GetFreeBlock(AtrDiskInfo *info);
static int AtrDos4ReadFAT(AtrDiskInfo *info, UWORD *freeBlocks);
static int AtrDos4WriteFAT(AtrDiskInfo *info);
static Dos4DirEntry *AtrDos4FindFreeDirEntry(AtrDiskInfo *info);
static void AtrDos4DeleteDirList(AtrDiskInfo *info);
static Dos4DirEntry *AtrDos4CreateDirEntry( Dos4_DIRENT* pDirEntry, 
                                            UWORD entry );

int AtrDos4Mount(AtrDiskInfo *info)
{
	AtrDos4DiskInfo *dinfo;
	UWORD sectorSize = AtrSectorSize(info);
	UWORD sectorCount = AtrSectorCount(info);
	
	info->atr_dosinfo = (void *) calloc(1, sizeof(AtrDos4DiskInfo));
	if (info->atr_dosinfo == NULL)
		return(ADOS_MEM_ERR);
	
	dinfo = (AtrDos4DiskInfo *)info->atr_dosinfo;
	dinfo->fatSector = DOS4_18_FAT_SECTOR;
	dinfo->fatSize = ((sectorCount * sectorSize) / DOS4_BLOCK_SIZE) + 8;
	dinfo->numDirSectors = (DOS4_BLOCK_SIZE * 2 - 256) / sectorSize;

	if (sectorSize == 128) {
		if (sectorCount == 720) {
			dinfo->numDirSectors = 11;
		}
		else if (sectorCount == 1040) {
			dinfo->fatSector = DOS4_26_FAT_SECTOR;
		}
	}
	else if (sectorSize == 256 && sectorCount == 1440)
			dinfo->numDirSectors = 11;
	
	dinfo->rootDirSector = dinfo->fatSector - dinfo->numDirSectors;
	dinfo->fat = calloc(1,((dinfo->fatSize + sectorSize -1)/sectorSize)*sectorSize);
	if (dinfo->fat == NULL)
        return(ADOS_MEM_ERR);
	
	return(AtrDos4ReadRootDir(info));
}

void AtrDos4Unmount(AtrDiskInfo *info)
{
	AtrDos4DiskInfo *dinfo = (AtrDos4DiskInfo *)info->atr_dosinfo;
	
    AtrDos4DeleteDirList(info);
    free(dinfo->fat);
    free(dinfo);
}

int AtrDos4GetDir(AtrDiskInfo *info, UWORD *fileCount, 
                  ADosFileEntry *files, ULONG *freeBytes)
{
    Dos4DirEntry  *pCurr = NULL;
    ADosFileEntry *pFileEntry = files;
    UWORD count = 0;
    char name[15];
    UWORD freeBlocks;
	AtrDos4DiskInfo *dinfo = (AtrDos4DiskInfo *)info->atr_dosinfo;

    *fileCount = 0;
    pCurr = dinfo->pRoot;
    count = 0;

    while (pCurr) 
        {
        if (pCurr->flags != 0 && (pCurr->flags != DIRE_DELETED)) {
        	strcpy(name, pCurr->filename);
            Host2ADos(name,pFileEntry->aname);
            pFileEntry->flags = pCurr->flags;
            pFileEntry->sectors = pCurr->numBlocks * 
                                  (DOS4_BLOCK_SIZE/AtrSectorSize(info));
            count++;
            pFileEntry++;
            }
        pCurr = pCurr->pNext;
        }

    if (AtrDos4ReadFAT(info,&freeBlocks))
        return(TRUE);

    *freeBytes = freeBlocks * DOS4_BLOCK_SIZE;

    *fileCount = count;

    return(FALSE);
}

int AtrDos4LockFile(AtrDiskInfo *info, char *name, int lock)
{
    Dos4DirEntry  *pDirEntry = NULL;
    UBYTE secBuff[0x100];
    UBYTE *pTmp;
	UWORD entriesPerSec = AtrSectorSize(info)/16;
	AtrDos4DiskInfo *dinfo = (AtrDos4DiskInfo *)info->atr_dosinfo;

    if ((pDirEntry = AtrDos4FindDirEntryByName(info,name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }

    if ( AtrReadSector(info, dinfo->rootDirSector + ( pDirEntry->fileNumber / entriesPerSec ), secBuff) )
        {
        return ADOS_DIR_READ_ERR;
        }

    pTmp = secBuff + ( pDirEntry->fileNumber % entriesPerSec) * 16;

    if (lock) { 
        *pTmp |= DIRE_LOCKED;
        pDirEntry->flags |= DIRE_LOCKED;
        }
    else {
        *pTmp &= ~DIRE_LOCKED;
        pDirEntry->flags &= ~DIRE_LOCKED;
    }

    if ( AtrWriteSector(info, dinfo->rootDirSector + ( pDirEntry->fileNumber / entriesPerSec ), secBuff) )
        {
        return ADOS_DIR_WRITE_ERR;
        }

    return FALSE;
}

int AtrDos4RenameFile(AtrDiskInfo *info, char *name, char *newname)
{
    Dos4DirEntry  *pDirEntry = NULL;
    UBYTE secBuff[0x100];
	UBYTE dosName[13];
    UBYTE *pTmp;
	UWORD entriesPerSec = AtrSectorSize(info)/16;
    int stat;
	AtrDos4DiskInfo *dinfo = (AtrDos4DiskInfo *)info->atr_dosinfo;

    if ((pDirEntry = AtrDos4FindDirEntryByName(info,name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }

    if ((pDirEntry->flags & DIRE_LOCKED) == DIRE_LOCKED) {
        return ADOS_FILE_LOCKED;
        }
		
	Host2ADos(newname, dosName);
	ADos2Host(newname, dosName);

    if (AtrDos4FindDirEntryByName(info,newname) != NULL) {
		return ADOS_DUPLICATE_NAME;
		}

    if ( AtrReadSector(info, dinfo->rootDirSector + ( pDirEntry->fileNumber / entriesPerSec ), secBuff) )
        {
        return ADOS_DIR_READ_ERR;
        }

    pTmp = secBuff + ( pDirEntry->fileNumber % entriesPerSec) * 16;

    Host2ADos(newname,&pTmp[5]);

    if ( AtrWriteSector(info, dinfo->rootDirSector + ( pDirEntry->fileNumber / entriesPerSec ), secBuff) )
        {
        return ADOS_DIR_WRITE_ERR;
        }

    stat = AtrDos4ReadRootDir(info);
    if (stat)
    	return stat;

    return FALSE;
}

int AtrDos4DeleteFile(AtrDiskInfo *info, char *name)
{
    Dos4DirEntry  *pDirEntry = NULL;
    UBYTE secBuff[0x100];
    UWORD count, free;
    UBYTE *pTmp,block,nextBlock;
    int stat;
	UWORD entriesPerSec = AtrSectorSize(info)/16;
	AtrDos4DiskInfo *dinfo = (AtrDos4DiskInfo *)info->atr_dosinfo;

    if ((pDirEntry = AtrDos4FindDirEntryByName(info,name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }

    if ((pDirEntry->flags & DIRE_LOCKED) == DIRE_LOCKED) {
        return ADOS_FILE_LOCKED;
        }

    if (AtrDos4ReadFAT(info,&free))
        return ADOS_VTOC_READ_ERR;
    
    count = pDirEntry->numBlocks - 1;
    block = pDirEntry->firstBlock;

	while( count )
	{
		if ( block > dinfo->fatSize)
		{
			return ADOS_FILE_CORRUPTED;
		}

		nextBlock = dinfo->fat[block];
		                
        block = nextBlock;
        count--;
	}
 
    dinfo->fat[block] = dinfo->firstFreeBlock;
    dinfo->firstFreeBlock = pDirEntry->firstBlock;
    dinfo->numFreeBlocks += pDirEntry->numBlocks;	
 
    if (AtrDos4WriteFAT(info)) 
        return(ADOS_VTOC_WRITE_ERR);
    
    if ( AtrReadSector(info, dinfo->rootDirSector + ( pDirEntry->fileNumber / entriesPerSec ), secBuff) )
        {
        return ADOS_DIR_READ_ERR;
        }

    pTmp = secBuff + ( pDirEntry->fileNumber % entriesPerSec) * 16;

    *pTmp = DIRE_DELETED;

    if ( AtrWriteSector(info, dinfo->rootDirSector + ( pDirEntry->fileNumber / entriesPerSec ), secBuff) )
        {
        return ADOS_DIR_WRITE_ERR;
        }
    
    stat = AtrDos4ReadRootDir(info);
    if (stat)
    	return(stat);

    return(FALSE);
}

int AtrDos4ImportFile(AtrDiskInfo *info, char *filename, int lfConvert)
{
    FILE *inFile;
    int file_length, total_length;
    UWORD freeBlocks;
    UBYTE numBlocksNeeded;
    Dos4DirEntry *pEntry;
    UBYTE *pTmp;
    UBYTE secBuff[DOS4_BLOCK_SIZE];
    Dos4DirEntry  *pDirEntry;
    UBYTE starting_block = 0, last_block = 0, curr_block;
	UBYTE last_byte;
    UWORD numToWrite, sector;
	UWORD sectorSize = AtrSectorSize(info);
    int first_block;
    char *slash;
    int stat;
	UBYTE dosName[12];
	int i;
	UWORD entriesPerSec = AtrSectorSize(info)/16;
	AtrDos4DiskInfo *dinfo = (AtrDos4DiskInfo *)info->atr_dosinfo;
    
    inFile = fopen( filename, "rb");

    if (inFile == NULL)
        {
        return(ADOS_HOST_FILE_NOT_FOUND);
        }

    if ((slash = strrchr(filename,'/')) != NULL)
        {
        filename = slash + 1;
        }
		
	Host2ADos(filename, dosName);
	ADos2Host(filename, dosName);

    if ((pDirEntry = AtrDos4FindDirEntryByName(info, filename)) != NULL) {
        if (AtrDos4DeleteFile(info,filename)) {
            fclose(inFile);
            return ADOS_DELETE_FILE_ERR;
            }
        }

    fseek(inFile, 0L, SEEK_END);
    file_length = ftell(inFile);
    fseek(inFile, 0L, SEEK_SET);
    total_length = file_length;

    if (file_length == 0) 
        numBlocksNeeded = 1;
    else if ((file_length % DOS4_BLOCK_SIZE) == 0) 
        numBlocksNeeded = file_length / DOS4_BLOCK_SIZE;
    else
        numBlocksNeeded = file_length / DOS4_BLOCK_SIZE + 1;

    if (AtrDos4ReadFAT(info, &freeBlocks)) {
        fclose(inFile);
        return(ADOS_VTOC_READ_ERR);
        }
    
    if (numBlocksNeeded > freeBlocks) {
        fclose(inFile);
        return(ADOS_DISK_FULL);
    }

    if ((pEntry = AtrDos4FindFreeDirEntry(info)) == NULL) {
        fclose(inFile);
        return(ADOS_DIR_FULL);
    }
    
    first_block = TRUE;
    do {
        if (file_length >= DOS4_BLOCK_SIZE) 
            numToWrite = DOS4_BLOCK_SIZE;
        else
            numToWrite = file_length;
       
        if (first_block) {
            last_block = AtrDos4GetFreeBlock(info);
            starting_block = last_block;
            if (fread(secBuff,1,numToWrite,inFile) != numToWrite)
                {
                fclose(inFile);
                return(ADOS_HOST_READ_ERR);
                }
            first_block = 0;
            }
        else {
            curr_block = AtrDos4GetFreeBlock(info);
			
			sector = (last_block - DOS4_FIRST_BLK_NUM) * DOS4_BLOCK_SIZE / 
				    sectorSize + 1;
            for (i=0;i<(DOS4_BLOCK_SIZE / sectorSize);i++) {
                if (AtrWriteSector(info,sector+i, secBuff + (i*sectorSize)))
                    {
                    fclose(inFile);
                    return(ADOS_FILE_WRITE_ERR);
                    }
                }
                
            if (fread(secBuff,1,numToWrite,inFile) != numToWrite)
                {
                fclose(inFile);
                return(ADOS_HOST_READ_ERR);
                }
				
			if (lfConvert)
				HostLFToAtari(secBuff, numToWrite);

             last_block = curr_block;
            }
        file_length -= numToWrite;
    } while (file_length > 0);
 
	fclose(inFile);

	sector = (last_block - DOS4_FIRST_BLK_NUM) * DOS4_BLOCK_SIZE / 
		    sectorSize + 1;
    for (i=0;i<(DOS4_BLOCK_SIZE / sectorSize);i++) {
                if (AtrWriteSector(info,sector+i, secBuff + (i*sectorSize)))
            {
            fclose(inFile);
            return(ADOS_FILE_WRITE_ERR);
            }
        }
		
	dinfo->fat[last_block] = (total_length % DOS4_BLOCK_SIZE) / sectorSize;
	last_byte = ((total_length % DOS4_BLOCK_SIZE) % sectorSize);
	if (last_byte == 0)
		last_byte = sectorSize-1;
	else
		last_byte--;
    AtrDos4WriteFAT(info);
    
    if ( AtrReadSector(info, dinfo->rootDirSector + ( pEntry->fileNumber / entriesPerSec ), secBuff) )
    {
        return ADOS_DIR_READ_ERR;
    }
    pTmp = secBuff + ( pEntry->fileNumber % entriesPerSec) * 16;
    pTmp[0] = DIRE_IN_USE ;
    Host2ADos(filename,&pTmp[5]);
    pTmp[1] = numBlocksNeeded;
	pTmp[2] = last_byte;
    pTmp[3] = starting_block;
    pTmp[4] = 0;

    if ( AtrWriteSector(info, dinfo->rootDirSector + ( pEntry->fileNumber / entriesPerSec ), secBuff) )
    {
        return ADOS_DIR_WRITE_ERR;
    }

    stat = AtrDos4ReadRootDir(info);
    if (stat)
    	return(stat);

    return FALSE;
}

int AtrDos4ExportFile(AtrDiskInfo *info, char *nameToExport, char* outFile, int lfConvert)
{
	Dos4DirEntry* pDirEntry;
	UBYTE secBuff[ 8 * 128 ];
	UWORD block,sector,count,bytesToWrite,fileLength;
	UBYTE lastBlock;
	int i;
	FILE *output = NULL;
	UWORD free;
	int status;
	UWORD sectorSize = AtrSectorSize(info);
	AtrDos4DiskInfo *dinfo = (AtrDos4DiskInfo *)info->atr_dosinfo;

    if ((pDirEntry = AtrDos4FindDirEntryByName(info,nameToExport)) == NULL) {
        return(ADOS_FILE_NOT_FOUND);
        }
	
    block = pDirEntry->firstBlock;
	count = pDirEntry->numBlocks;

    status = AtrDos4ReadFAT(info, &free);
    if (status)
        return(status);

	lastBlock = block;
	while (lastBlock >= DOS4_FIRST_BLK_NUM) {
		lastBlock = dinfo->fat[lastBlock];
		}

	fileLength = ((pDirEntry->numBlocks-1) * DOS4_BLOCK_SIZE) +
				  lastBlock * sectorSize +
	              pDirEntry->lastUsedByte + 1;

	if ( outFile )
	{
		output = fopen(outFile, "wb+");

		if ( output == NULL)
		{
			return ADOS_HOST_CREATE_ERR;
		}
	}

	while( count )
	{
	    if (fileLength < DOS4_BLOCK_SIZE)
	       bytesToWrite = fileLength;
        else
           bytesToWrite = DOS4_BLOCK_SIZE;

        sector = (block - DOS4_FIRST_BLK_NUM) * DOS4_BLOCK_SIZE / 
				    sectorSize + 1;
        for (i=0;i<(DOS4_BLOCK_SIZE / sectorSize);i++) {
            if ( AtrReadSector(info,sector+i, secBuff+(i*sectorSize)))
                {
			    return ADOS_FILE_READ_ERR;
		        }
            }
 
		if (lfConvert)
			AtariLFToHost(secBuff, bytesToWrite);
			
		if (fwrite( secBuff, 1, bytesToWrite ,output) != 
               bytesToWrite)
            {
            fclose(output);
            return ADOS_HOST_WRITE_ERR;
            }
        
        block = dinfo->fat[block];

		if ( count > 1 )
		{
			if ( block < DOS4_FIRST_BLK_NUM )
				return ADOS_FILE_CORRUPTED;
		}
		else
		{
			if ( block >= DOS4_FIRST_BLK_NUM )
				return ADOS_FILE_CORRUPTED;
		}

		count--;
		fileLength -= bytesToWrite;
	}
	
	fclose(output);
	return FALSE;
}

static int AtrDos4ReadRootDir(AtrDiskInfo *info)
{
    UBYTE secBuff[ 0x100 ];
    UBYTE *pTmp;
    Dos4DirEntry  *pEntry;
	Dos4DirEntry  *pPrev = NULL;
	Dos4_DIRENT dirEntry;
	UWORD entry = 0;
	AtrDos4DiskInfo *dinfo = (AtrDos4DiskInfo *)info->atr_dosinfo;
	UWORD entriesPerSec = AtrSectorSize(info)/16;

    if (dinfo->pRoot) 
        AtrDos4DeleteDirList(info);
        
    do
	{
        if ( AtrReadSector(info, dinfo->rootDirSector + ( entry / entriesPerSec ), secBuff) )
		{
			return ADOS_DIR_READ_ERR;
		}

		pTmp = secBuff + ( entry % entriesPerSec) * 16;
		dirEntry.flags = *pTmp++;
		dirEntry.numBlocks = *pTmp++;
		dirEntry.lastUsedByte = *pTmp++;
		dirEntry.firstBlock = *pTmp++;
		pTmp++;
 		memcpy( dirEntry.atariName, pTmp, 11 );

		pEntry = AtrDos4CreateDirEntry( &dirEntry, entry );
		if (pEntry == NULL)
			{
			AtrDos4DeleteDirList(info);
			return ADOS_MEM_ERR;
			}

		if ( pEntry )
		{
			if ( dinfo->pRoot )
			{

				pPrev->pNext = pEntry;
				pEntry->pPrev = pPrev;
                pEntry->pNext = NULL;
				pPrev = pEntry;
			}
			else
			{
				dinfo->pRoot = pEntry;
				pPrev = pEntry;
				pEntry->pPrev = NULL;
                pEntry->pNext = NULL;
			}

		}

		entry++;

	} while( entry < (dinfo->numDirSectors * entriesPerSec) );

	return FALSE;
}

static Dos4DirEntry *AtrDos4FindDirEntryByName(AtrDiskInfo *info, char *name)
{
    Dos4DirEntry  *pCurr = NULL;
	AtrDos4DiskInfo *dinfo = (AtrDos4DiskInfo *)info->atr_dosinfo;

    pCurr = dinfo->pRoot;

    while (pCurr) 
        {
        if ((strcmp(name,pCurr->filename) == 0) &&
            (pCurr->flags  != DIRE_DELETED))
            return(pCurr);
        pCurr = pCurr->pNext;
        }
    return(NULL);
}

static UBYTE AtrDos4GetFreeBlock(AtrDiskInfo *info)
{
	AtrDos4DiskInfo *dinfo = (AtrDos4DiskInfo *)info->atr_dosinfo;
	UBYTE freeBlock;
	
	freeBlock = dinfo->firstFreeBlock;
	dinfo->firstFreeBlock = dinfo->fat[dinfo->firstFreeBlock];
	dinfo->numFreeBlocks--;

    return(freeBlock);
}

static int AtrDos4ReadFAT(AtrDiskInfo *info, UWORD *freeBlocks)
{
    int i,stat;
	AtrDos4DiskInfo *dinfo = (AtrDos4DiskInfo *)info->atr_dosinfo;
	int sectorSize = AtrSectorSize(info);
	int fatSectors = (dinfo->fatSize + sectorSize - 1)/ sectorSize;

    memset(dinfo->fat,0,sectorSize*fatSectors);
    
    for (i=0;i<fatSectors;i++) {
        stat = AtrReadSector(info, dinfo->fatSector+i, dinfo->fat + sectorSize*i);
        if ( stat )
                return stat;
        }
    
    dinfo->firstFreeBlock = dinfo->fat[1];
    dinfo->numFreeBlocks = dinfo->fat[3];
    
    *freeBlocks = dinfo->numFreeBlocks;
    
    return FALSE;
}

static int AtrDos4WriteFAT(AtrDiskInfo *info)
{
    int i,stat;
	AtrDos4DiskInfo *dinfo = (AtrDos4DiskInfo *)info->atr_dosinfo;
	int sectorSize = AtrSectorSize(info);
	int fatSectors = (dinfo->fatSize + sectorSize - 1)/ sectorSize;

    dinfo->fat[1] = dinfo->firstFreeBlock;
    dinfo->fat[3] = dinfo->numFreeBlocks;

    for (i=0;i<fatSectors;i++) {
        stat = AtrWriteSector(info, dinfo->fatSector+i, dinfo->fat + sectorSize*i);
        if ( stat )
            return stat;
        }

    return FALSE;

}

static Dos4DirEntry *AtrDos4FindFreeDirEntry(AtrDiskInfo *info)
{
	AtrDos4DiskInfo *dinfo = (AtrDos4DiskInfo *)info->atr_dosinfo;
    Dos4DirEntry *pCurr = dinfo->pRoot;

    while(pCurr->flags != 0 && (pCurr->flags  != DIRE_DELETED)) {
        if (pCurr->pNext == NULL) 
            return NULL;
        pCurr = pCurr->pNext;
    }

    return pCurr;

}

static void AtrDos4DeleteDirList(AtrDiskInfo *info)
{
	AtrDos4DiskInfo *dinfo = (AtrDos4DiskInfo *)info->atr_dosinfo;
	Dos4DirEntry* pCurr = dinfo->pRoot;
	Dos4DirEntry* pNext;

	while( pCurr )
	{
		pNext = pCurr->pNext;
		free(pCurr);

		pCurr = pNext;
	}

    dinfo->pRoot = NULL;
}

static Dos4DirEntry *AtrDos4CreateDirEntry( Dos4_DIRENT* pDirEntry, 
                                            UWORD entry )
{
	Dos4DirEntry *pEntry;
    
    pEntry = (Dos4DirEntry *) malloc(sizeof(Dos4DirEntry));

	if ( !pEntry )
	{
		return NULL;
	}

    ADos2Host( pEntry->filename, pDirEntry->atariName );

	pEntry->fileNumber = entry;
	pEntry->flags = pDirEntry->flags;
	pEntry->numBlocks = pDirEntry->numBlocks;
	pEntry->firstBlock = pDirEntry->firstBlock;
	pEntry->lastUsedByte = pDirEntry->lastUsedByte;

	return pEntry;
}

