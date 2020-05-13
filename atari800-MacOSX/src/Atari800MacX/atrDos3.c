/* atrDos3.c -  
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
#include "atrDos3.h"
#include "atrErr.h"

#define FAT_SECTOR      24
#define ROOT_DIR        16

#define DOS_3_BLOCK_SIZE     1024
#define	FAT_RESERVED_BLOCK   0xFF
#define FAT_FREE_BLOCK       0xFE
#define FAT_END_OF_FILE      0xFD

typedef struct DOS3_DIRENT {
	UBYTE flags;		/* flags */
	UBYTE atariName[11];	/* file name, space padded */
	UBYTE numBlocks;	/* number of allocated blocks */
	UBYTE firstBlock;	/* first allocated block */
	UWORD fileLen;		/* file length */
} DOS3_DIRENT;

typedef struct Dos3DirEntry
{
    struct Dos3DirEntry *pNext;
    struct Dos3DirEntry *pPrev;
    char filename[ 256 ];
    UWORD fileNumber;
	UBYTE flags;
	UBYTE numBlocks;	/* number of allocated blocks */
	UBYTE firstBlock;	/* first allocated block */
	UWORD fileLen;		/* file length */
} Dos3DirEntry;

typedef struct atrDos3DiskInfo {
    Dos3DirEntry  *pRoot;
    UWORD fatSize;
    UBYTE fat[128*2];
	} AtrDos3DiskInfo;

static int AtrDos3ReadRootDir(AtrDiskInfo *info);
static Dos3DirEntry *AtrDos3FindDirEntryByName(AtrDiskInfo *info, char *name);
static UBYTE AtrDos3GetFreeBlock(AtrDiskInfo *info);
static int AtrDos3ReadFAT(AtrDiskInfo *info, UWORD *freeBlocks);
static int AtrDos3WriteFAT(AtrDiskInfo *info);
static void AtrDos3MarkBlockUsed(AtrDiskInfo *info, UBYTE block, UBYTE nextBlock);
static Dos3DirEntry *AtrDos3FindFreeDirEntry(AtrDiskInfo *info);
static void AtrDos3DeleteDirList(AtrDiskInfo *info);
static Dos3DirEntry *AtrDos3CreateDirEntry( DOS3_DIRENT* pDirEntry, 
                                            UWORD entry );

int AtrDos3Mount(AtrDiskInfo *info)
{
	info->atr_dosinfo = (void *) calloc(1, sizeof(AtrDos3DiskInfo));
	if (info->atr_dosinfo == NULL)
		return(ADOS_MEM_ERR);
	else
		return(AtrDos3ReadRootDir(info));
}

void AtrDos3Unmount(AtrDiskInfo *info)
{
    AtrDos3DeleteDirList(info);
	free(info->atr_dosinfo);
}

int AtrDos3GetDir(AtrDiskInfo *info, UWORD *fileCount, ADosFileEntry *files, 
                  ULONG *freeBytes)
{
    Dos3DirEntry  *pCurr = NULL;
    ADosFileEntry *pFileEntry = files;
    UWORD count = 0;
    char name[15];
    UWORD freeBlocks;
	AtrDos3DiskInfo *dinfo = (AtrDos3DiskInfo *)info->atr_dosinfo;

    *fileCount = 0;
    pCurr = dinfo->pRoot;
    count = 0;

    while (pCurr) 
        {
        if (pCurr->flags != 0 && (pCurr->flags != DIRE_DELETED)) {
        	strcpy(name, pCurr->filename);
            Host2ADos(name,pFileEntry->aname);
            pFileEntry->flags = pCurr->flags;
            pFileEntry->sectors = pCurr->numBlocks;
            count++;
            pFileEntry++;
            }
        pCurr = pCurr->pNext;
        }

    if (AtrDos3ReadFAT(info,&freeBlocks))
        return(TRUE);

    *fileCount = count;
    *freeBytes = freeBlocks * DOS_3_BLOCK_SIZE;

    return(FALSE);
}

int AtrDos3LockFile(AtrDiskInfo *info, char *name, int lock)
{
    Dos3DirEntry  *pDirEntry = NULL;
    UBYTE secBuff[0x100];
    UBYTE *pTmp;

    if ((pDirEntry = AtrDos3FindDirEntryByName(info,name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }

    if ( AtrReadSector(info, ROOT_DIR + ( pDirEntry->fileNumber / 8 ), secBuff) )
        {
        return ADOS_DIR_READ_ERR;
        }

    pTmp = secBuff + ( pDirEntry->fileNumber % 8) * 16;

    if (lock) { 
        *pTmp |= DIRE_LOCKED;
        pDirEntry->flags |= DIRE_LOCKED;
        }
    else {
        *pTmp &= ~DIRE_LOCKED;
        pDirEntry->flags &= ~DIRE_LOCKED;
    }

    if ( AtrWriteSector(info, ROOT_DIR + ( pDirEntry->fileNumber / 8 ), secBuff) )
        {
        return ADOS_DIR_WRITE_ERR;
        }

    return FALSE;
}

int AtrDos3RenameFile(AtrDiskInfo *info, char *name, char *newname)
{
    Dos3DirEntry  *pDirEntry = NULL;
    UBYTE secBuff[0x100];
	UBYTE dosName[13];
    UBYTE *pTmp;
    int stat;

    if ((pDirEntry = AtrDos3FindDirEntryByName(info,name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }

    if ((pDirEntry->flags & DIRE_LOCKED) == DIRE_LOCKED) {
        return ADOS_FILE_LOCKED;
        }
		
	Host2ADos(newname, dosName);
	ADos2Host(newname, dosName);

    if (AtrDos3FindDirEntryByName(info,newname) != NULL) {
		return ADOS_DUPLICATE_NAME;
		}

    if ( AtrReadSector(info, ROOT_DIR + ( pDirEntry->fileNumber / 8 ), secBuff) )
        {
        return ADOS_DIR_READ_ERR;
        }

    pTmp = secBuff + ( pDirEntry->fileNumber % 8) * 16;

    Host2ADos(newname,&pTmp[1]);

    if ( AtrWriteSector(info, ROOT_DIR + ( pDirEntry->fileNumber / 8 ), secBuff) )
        {
        return ADOS_DIR_WRITE_ERR;
        }

    stat = AtrDos3ReadRootDir(info);
    if (stat)
    	return stat;

    return FALSE;
}

int AtrDos3DeleteFile(AtrDiskInfo *info, char *name)
{
    Dos3DirEntry  *pDirEntry = NULL;
    UBYTE secBuff[0x100];
    UWORD count, free;
    UBYTE *pTmp,block,nextBlock;
    int stat;
	AtrDos3DiskInfo *dinfo = (AtrDos3DiskInfo *)info->atr_dosinfo;

    if ((pDirEntry = AtrDos3FindDirEntryByName(info,name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }

    if ((pDirEntry->flags & DIRE_LOCKED) == DIRE_LOCKED) {
        return ADOS_FILE_LOCKED;
        }

    if (AtrDos3ReadFAT(info,&free))
        return ADOS_VTOC_READ_ERR;
    
    count = pDirEntry->numBlocks;
    block = pDirEntry->firstBlock;

	while( count )
	{
		if ( block > dinfo->fatSize)
		{
			return ADOS_FILE_CORRUPTED;
		}

		nextBlock = dinfo->fat[block];
		
        AtrDos3MarkBlockUsed(info,block,FAT_FREE_BLOCK);
        
        if (count == 1) {
            if (nextBlock != FAT_END_OF_FILE) 
            {
			    return ADOS_FILE_CORRUPTED;
		    }
        }        
        
        block = nextBlock;
        count--;
	}	
 
    if (AtrDos3WriteFAT(info)) 
        return(ADOS_VTOC_WRITE_ERR);
    
    if ( AtrReadSector(info, ROOT_DIR + ( pDirEntry->fileNumber / 8 ), secBuff) )
        {
        return ADOS_DIR_READ_ERR;
        }

    pTmp = secBuff + ( pDirEntry->fileNumber % 8) * 16;

    *pTmp = DIRE_DELETED;

    if ( AtrWriteSector(info, ROOT_DIR + ( pDirEntry->fileNumber / 8 ), secBuff) )
        {
        return ADOS_DIR_WRITE_ERR;
        }
    
    stat = AtrDos3ReadRootDir(info);
    if (stat)
    	return(stat);

    return(FALSE);
}

int AtrDos3ImportFile(AtrDiskInfo *info, char *filename, int lfConvert)
{
    FILE *inFile;
    int file_length, total_length;
    UWORD freeBlocks;
    UWORD numBlocksNeeded;
    Dos3DirEntry *pEntry;
    UBYTE *pTmp;
    UBYTE secBuff[8*DOS_3_BLOCK_SIZE];
    Dos3DirEntry  *pDirEntry;
    UBYTE starting_block = 0, last_block = 0, curr_block;
    UWORD numToWrite;
    int first_block;
    char *slash;
    int stat;
	UBYTE dosName[12];
	int i;
    
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

    if ((pDirEntry = AtrDos3FindDirEntryByName(info, filename)) != NULL) {
        if (AtrDos3DeleteFile(info,filename)) {
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
    else if ((file_length % DOS_3_BLOCK_SIZE) == 0) 
        numBlocksNeeded = file_length / DOS_3_BLOCK_SIZE;
    else
        numBlocksNeeded = (file_length / DOS_3_BLOCK_SIZE) + 1;

    if (AtrDos3ReadFAT(info, &freeBlocks)) {
        fclose(inFile);
        return(ADOS_VTOC_READ_ERR);
        }
    
    if (numBlocksNeeded > freeBlocks) {
        fclose(inFile);
        return(ADOS_DISK_FULL);
    }

    if ((pEntry = AtrDos3FindFreeDirEntry(info)) == NULL) {
        fclose(inFile);
        return(ADOS_DIR_FULL);
    }
    
    first_block = TRUE;
    do {
        if (file_length >= DOS_3_BLOCK_SIZE) 
            numToWrite = DOS_3_BLOCK_SIZE;
        else
            numToWrite = file_length;
       
        if (first_block) {
            last_block = AtrDos3GetFreeBlock(info);
            /* have to do this temporarily so same block isn't returned 
                next time */
            AtrDos3MarkBlockUsed(info,last_block,last_block);
            starting_block = last_block;
            if (fread(secBuff,1,numToWrite,inFile) != numToWrite)
                {
                fclose(inFile);
                return(ADOS_HOST_READ_ERR);
                }
            first_block = 0;
            }
        else {
            curr_block = AtrDos3GetFreeBlock(info);
            AtrDos3MarkBlockUsed(info,last_block,curr_block);
            
            for (i=0;i<8;i++) {
                if (AtrWriteSector(info,(last_block * 8) + i + FAT_SECTOR + 1, 
                                   secBuff + (i*128)))
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
    
    AtrDos3MarkBlockUsed(info,last_block,FAT_END_OF_FILE);
    for (i=0;i<8;i++) {
        if (AtrWriteSector(info,(last_block * 8) + i + FAT_SECTOR + 1, 
                            secBuff + (i*128)))
            {
            fclose(inFile);
            return(ADOS_FILE_WRITE_ERR);
            }
        }
    AtrDos3WriteFAT(info);
    
    if ( AtrReadSector(info, ROOT_DIR + ( pEntry->fileNumber / 8 ), secBuff) )
    {
        return ADOS_DIR_READ_ERR;
    }
    pTmp = secBuff + ( pEntry->fileNumber % 8) * 16;
    pTmp[0] = DIRE_IN_USE | DIRE_DELETED;
    Host2ADos(filename,&pTmp[1]);
    pTmp[12] = numBlocksNeeded;
    pTmp[13] = starting_block;
    pTmp[14] = total_length & 0xff;
    pTmp[15] = total_length >> 8;
 
    if ( AtrWriteSector(info, ROOT_DIR + ( pEntry->fileNumber / 8 ), secBuff) )
    {
        return ADOS_DIR_WRITE_ERR;
    }

    stat = AtrDos3ReadRootDir(info);
    if (stat)
    	return(stat);

    return FALSE;
}

int AtrDos3ExportFile(AtrDiskInfo *info, char *nameToExport, char* outFile, int lfConvert)
{
	Dos3DirEntry* pDirEntry;
	UBYTE secBuff[ 8 * 128 ];
	UBYTE count;
	UWORD block,sector,bytesToWrite,fileLength;
	int i;
	FILE *output = NULL;
	AtrDos3DiskInfo *dinfo = (AtrDos3DiskInfo *)info->atr_dosinfo;

    if ((pDirEntry = AtrDos3FindDirEntryByName(info,nameToExport)) == NULL) {
        return(ADOS_FILE_NOT_FOUND);
        }
	
    block = pDirEntry->firstBlock;
	count = pDirEntry->numBlocks;
	fileLength = pDirEntry->fileLen;

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
	    if (fileLength < DOS_3_BLOCK_SIZE)
	       bytesToWrite = fileLength;
        else
           bytesToWrite = DOS_3_BLOCK_SIZE;

        sector = (block * 8) + FAT_SECTOR + 1;
        for (i=0;i<8;i++) {
            if ( AtrReadSector(info,sector+i, secBuff+(i*128)))
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
        
		if ( bytesToWrite == DOS_3_BLOCK_SIZE )
		{
			if ( block >= FAT_END_OF_FILE )
				return ADOS_FILE_CORRUPTED;
		}
		else
		{
			if ( block != FAT_END_OF_FILE )
				return ADOS_FILE_CORRUPTED;
		}


		count--;
		fileLength -= bytesToWrite;
	}

	fclose(output);
	return FALSE;
}

static int AtrDos3ReadRootDir(AtrDiskInfo *info)
{
    UBYTE secBuff[ 0x100 ];
    UBYTE *pTmp;
    Dos3DirEntry  *pEntry;
	Dos3DirEntry  *pPrev = NULL;
	DOS3_DIRENT dirEntry;
	UWORD entry = 1;
	AtrDos3DiskInfo *dinfo = (AtrDos3DiskInfo *)info->atr_dosinfo;

    if (dinfo->pRoot) 
        AtrDos3DeleteDirList(info);
        
    do
	{
        if ( AtrReadSector(info, ROOT_DIR + ( entry / 8 ), secBuff) )
		{
			return ADOS_DIR_READ_ERR;
		}

		pTmp = secBuff + ( entry % 8) * 16;
		dirEntry.flags = *pTmp++;
		memcpy( dirEntry.atariName, pTmp, 11 );
		pTmp += 11;
		dirEntry.numBlocks = *pTmp++;
		dirEntry.firstBlock = *pTmp++;
		dirEntry.fileLen = *pTmp + (*(pTmp+1) << 8);

		pEntry = AtrDos3CreateDirEntry( &dirEntry, entry );
		if (pEntry == NULL)
			{
			AtrDos3DeleteDirList(info);
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

	} while( entry < 64 );

	return FALSE;
}

static Dos3DirEntry *AtrDos3FindDirEntryByName(AtrDiskInfo *info, char *name)
{
    Dos3DirEntry  *pCurr = NULL;
	AtrDos3DiskInfo *dinfo = (AtrDos3DiskInfo *)info->atr_dosinfo;

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

static UBYTE AtrDos3GetFreeBlock(AtrDiskInfo *info)
{
    UWORD i;
	AtrDos3DiskInfo *dinfo = (AtrDos3DiskInfo *)info->atr_dosinfo;

    for (i=0;i<=dinfo->fatSize;i++)
        {
        if (dinfo->fat[i] == FAT_FREE_BLOCK)
            {
            dinfo->fat[i] = FAT_RESERVED_BLOCK;
            return i;
            }
        }
    return  0xFF;
}

static int AtrDos3ReadFAT(AtrDiskInfo *info, UWORD *freeBlocks)
{
    int i,stat;
	AtrDos3DiskInfo *dinfo = (AtrDos3DiskInfo *)info->atr_dosinfo;

	dinfo->fatSize = 
        ((AtrSectorCount(info) - FAT_SECTOR) * AtrSectorSize(info))/DOS_3_BLOCK_SIZE;
        
    memset(dinfo->fat,0,sizeof(dinfo->fat));
    
    stat = AtrReadSector(info, FAT_SECTOR, dinfo->fat);
    if ( stat )
        return stat;
        
    *freeBlocks = 0;
    for (i=0;i<dinfo->fatSize;i++)
        if (dinfo->fat[i] == FAT_FREE_BLOCK)
            *freeBlocks = *freeBlocks + 1;
 
    return FALSE;
}

static int AtrDos3WriteFAT(AtrDiskInfo *info)
{
    int stat;
    
	AtrDos3DiskInfo *dinfo = (AtrDos3DiskInfo *)info->atr_dosinfo;

    stat = AtrWriteSector(info, FAT_SECTOR, dinfo->fat);
    if ( stat )
       return stat;

    return FALSE;

}

static void AtrDos3MarkBlockUsed(AtrDiskInfo *info, UBYTE block, UBYTE nextBlock)
{
	AtrDos3DiskInfo *dinfo = (AtrDos3DiskInfo *)info->atr_dosinfo;
	
	dinfo->fat[block] = nextBlock;
}

static Dos3DirEntry *AtrDos3FindFreeDirEntry(AtrDiskInfo *info)
{
	AtrDos3DiskInfo *dinfo = (AtrDos3DiskInfo *)info->atr_dosinfo;
    Dos3DirEntry *pCurr = dinfo->pRoot;

    while(pCurr->flags != 0 && (pCurr->flags  != DIRE_DELETED)) {
        if (pCurr->pNext == NULL) 
            return NULL;
        pCurr = pCurr->pNext;
    }

    return pCurr;

}

static void AtrDos3DeleteDirList(AtrDiskInfo *info)
{
	AtrDos3DiskInfo *dinfo = (AtrDos3DiskInfo *)info->atr_dosinfo;
	Dos3DirEntry* pCurr = dinfo->pRoot;
	Dos3DirEntry* pNext;

	while( pCurr )
	{
		pNext = pCurr->pNext;
		free(pCurr);

		pCurr = pNext;
	}

    dinfo->pRoot = NULL;
}

static Dos3DirEntry *AtrDos3CreateDirEntry( DOS3_DIRENT* pDirEntry, 
                                            UWORD entry )
{
	Dos3DirEntry *pEntry;
    
    pEntry = (Dos3DirEntry *) malloc(sizeof(Dos3DirEntry));

	if ( !pEntry )
	{
		return NULL;
	}

    ADos2Host( pEntry->filename, pDirEntry->atariName );

	pEntry->fileNumber = entry;
	pEntry->flags = pDirEntry->flags;
	pEntry->numBlocks = pDirEntry->numBlocks;
	pEntry->firstBlock = pDirEntry->firstBlock;
	pEntry->fileLen = pDirEntry->fileLen;

	return pEntry;
}

