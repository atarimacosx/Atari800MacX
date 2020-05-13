/* atrMyDos.c -  
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
#include "atrMyDos.h"
#include "atrErr.h"

#define VTOC	        360
#define ROOT_DIR        361

typedef struct MYDOS_DIRENT
{
	UBYTE flags;
	UWORD secCount; //Number of sectors in file
	UWORD secStart; //First sector in file
	UBYTE atariName[11];
} MYDOS_DIRENT;

typedef struct MyDosDirEntry
{
    struct MyDosDirEntry *pNext;
    struct MyDosDirEntry *pPrev;
    char filename[ 256 ];
    UWORD fileNumber;
	UBYTE flags;
	UWORD secCount; //Number of sectors in file
	UWORD secStart; //First sector in file
} MyDosDirEntry;

typedef struct MyDosDir
{
	struct MyDosDir *pNext;
	struct MyDosDir *pPrev;
	char dirname[256];
	UWORD  dirStartSector;
} MyDosDir;

typedef struct atrMyDosDiskInfo {
	MyDosDirEntry   *pCurrDir;
	MyDosDir		*pDirList;
	UBYTE			vtocMap[256*34];
	} AtrMyDosDiskInfo;

static int AtrMyDosReadDir(AtrDiskInfo *info, UWORD dirSector);
static int AtrMyDosReadRootDir(AtrDiskInfo *info);
static int AtrMyDosReadCurrentDir(AtrDiskInfo *info);
static UWORD AtrMyDosCurrentDirStartSector(AtrDiskInfo *info);
static MyDosDirEntry *AtrMyDosFindDirEntryByName(AtrDiskInfo *info, char *name);
static UWORD AtrMyDosGetFreeSector(AtrDiskInfo *info);
static UWORD AtrMyDosGetFreeSectorBlock(AtrDiskInfo *info, int count);
static int AtrMyDosReadVtoc(AtrDiskInfo *info, UWORD *freeSectors);
static int AtrMyDosWriteVtoc(AtrDiskInfo *info);
static int AtrMyDosIsSectorUsed(AtrDiskInfo *info, UWORD sector);
static void AtrMyDosMarkSectorUsed(AtrDiskInfo *info, UWORD sector, int used);
static MyDosDirEntry *AtrMyDosFindFreeDirEntry(AtrDiskInfo *info);
static void AtrMyDosDeleteDirList(AtrDiskInfo *info);
static void AtrMyDosDeleteDirEntryList(AtrDiskInfo *info);
static MyDosDirEntry *AtrMyDosCreateDirEntry(MYDOS_DIRENT* pDirEntry, UWORD entry );

int AtrMyDosMount(AtrDiskInfo *info)
{
	info->atr_dosinfo = (void *) calloc(1, sizeof(AtrMyDosDiskInfo));
	if (info->atr_dosinfo == NULL)
		return(ADOS_MEM_ERR);
	else
		return(AtrMyDosReadRootDir(info));
}

void AtrMyDosUnmount(AtrDiskInfo *info)
{
    AtrMyDosDeleteDirList(info);
    AtrMyDosDeleteDirEntryList(info);
	free(info->atr_dosinfo);
}

int AtrMyDosGetDir(AtrDiskInfo *info, UWORD *fileCount, ADosFileEntry *files, 
                   ULONG *freeBytes)
{
    MyDosDirEntry  *pCurr = NULL;
    ADosFileEntry *pFileEntry = files;
    UWORD count = 0;
    char name[15];
    UWORD freeSectors;
	AtrMyDosDiskInfo *dinfo = (AtrMyDosDiskInfo *)info->atr_dosinfo;

    *fileCount = 0;
    pCurr = dinfo->pCurrDir;
    count = 0;
	
    while (pCurr) 
        {
        if (pCurr->flags != 0 && (pCurr->flags & DIRE_DELETED) != DIRE_DELETED) {
        	strcpy(name, pCurr->filename);
            Host2ADos(name,pFileEntry->aname);
            pFileEntry->flags = pCurr->flags;
            pFileEntry->sectors = pCurr->secCount;
            count++;
            pFileEntry++;
            }
        pCurr = pCurr->pNext;
        }

    if (AtrMyDosReadVtoc(info, &freeSectors))
        return(ADOS_VTOC_READ_ERR);

    *fileCount = count;
    *freeBytes = freeSectors * AtrSectorSize(info);

    return(FALSE);
}

int AtrMyDosChangeDir(AtrDiskInfo *info, int cdFlag, char *name)
{
    MyDosDirEntry  *pDirEntry = NULL;
    UBYTE secBuff[0x100];
    UBYTE *pTmp;
    MyDosDir *pNew;
	AtrMyDosDiskInfo *dinfo = (AtrMyDosDiskInfo *)info->atr_dosinfo;

	if (cdFlag == CD_UP) {
        if (dinfo->pDirList == NULL) 
            return(FALSE);
		
        if (dinfo->pDirList->pPrev == NULL) {
			free(dinfo->pDirList);
			dinfo->pDirList = NULL;
			return(AtrMyDosReadRootDir(info));
			}
		else {
			dinfo->pDirList = dinfo->pDirList->pPrev;
			free(dinfo->pDirList->pNext);
			dinfo->pDirList->pNext = NULL;
			return(AtrMyDosReadDir(info,dinfo->pDirList->dirStartSector));
			}
		}
	else if (cdFlag == CD_ROOT) {
		return(AtrMyDosReadRootDir(info));
		}
	else {
        if ((pDirEntry = AtrMyDosFindDirEntryByName(info,name)) == NULL) {
        	return ADOS_FILE_NOT_FOUND;
        	}
    	
        if ( AtrReadSector(info, AtrMyDosCurrentDirStartSector(info) +
    						( pDirEntry->fileNumber / 8 ), secBuff) )
        	{
        	return ADOS_DIR_READ_ERR;
        	}
    	
        pTmp = secBuff + ( pDirEntry->fileNumber % 8) * 16;
    	
        if ((*pTmp & DIRE_SUBDIR) == 0)
    		return ADOS_NOT_A_DIRECTORY;
    	
        pNew = (MyDosDir *) malloc(sizeof(MyDosDir));
    	
        if (pNew == NULL)
    		return(ADOS_MEM_ERR);
        
        pTmp += 3;
		pNew->dirStartSector = *pTmp + (*(pTmp+1) << 8);
		
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
			pNew->pNext = NULL;
			pNew->pPrev = NULL;
			strcpy(pNew->dirname,":");
			strcat(pNew->dirname,name);
			}
		return(AtrMyDosReadDir(info,dinfo->pDirList->dirStartSector));
		}

    return FALSE;
}

int AtrMyDosDeleteDir(AtrDiskInfo *info, char *name)
{
    MyDosDirEntry  *pDirEntry = NULL;
    UBYTE secBuff[0x100],dirBuff[0x100];
    UWORD sector, startSector;
    UBYTE *pTmp;
    UWORD free;
    int i, stat;

    if ((pDirEntry = AtrMyDosFindDirEntryByName(info,name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }
    
    if ( AtrReadSector(info, AtrMyDosCurrentDirStartSector(info) + 
    					( pDirEntry->fileNumber / 8 ), secBuff) )
        {
        return ADOS_DIR_READ_ERR;
        }

    pTmp = secBuff + ( pDirEntry->fileNumber % 8) * 16;
 
  	if ((*pTmp & DIRE_SUBDIR) == 0)
   		return ADOS_NOT_A_DIRECTORY;
    
	startSector = pTmp[3] + (pTmp[4] << 8);
	
    for (sector=startSector;sector<startSector+8;sector++) {
    	if ( AtrReadSector(info, sector, dirBuff)) {
        	return ADOS_DIR_READ_ERR;
        	}
        for (i=0;i<8;i++) {
        	if (dirBuff[i*8] !=0 &&
        		((dirBuff[i*8] & DIRE_DELETED) != DIRE_DELETED)) {
        		return ADOS_DIR_NOT_EMPTY;
        		}
        	}
		}
		
    if (AtrMyDosReadVtoc(info,&free))
        return ADOS_VTOC_READ_ERR;
    	
	for (sector=startSector;sector<startSector+8;sector++) {
		AtrMyDosMarkSectorUsed(info,sector,FALSE);
		}
	
    if (AtrMyDosWriteVtoc(info))
        return ADOS_VTOC_WRITE_ERR;		

	*pTmp = DIRE_DELETED;

    if ( AtrWriteSector(info, AtrMyDosCurrentDirStartSector(info) + 
    					( pDirEntry->fileNumber / 8 ), secBuff) )
        {
        return ADOS_DIR_WRITE_ERR;
        }

    stat = AtrMyDosReadCurrentDir(info);
    if (stat)
    	return(stat);
    return FALSE;
}

int AtrMyDosMakeDir(AtrDiskInfo *info, char *name)
{
    MyDosDirEntry  *pDirEntry = NULL;
    UBYTE secBuff[0x100];
    UWORD sector, startingSector;
    UBYTE *pTmp;
    UWORD freeSectors;
    int stat;

    if ((pDirEntry = AtrMyDosFindDirEntryByName(info,name)) != NULL) 
        return ADOS_DUPLICATE_NAME;

    if (AtrMyDosReadVtoc(info,&freeSectors)) {
        return(ADOS_VTOC_READ_ERR);
        }
    
    if ((startingSector = AtrMyDosGetFreeSectorBlock(info,8)) == 0) {
        return(ADOS_DISK_FULL);
        }

    if ((pDirEntry = AtrMyDosFindFreeDirEntry(info)) == NULL) {
        return(ADOS_DIR_FULL);
        }
    
    memset(secBuff,0,0x100);
    for (sector=startingSector;sector<startingSector+8;sector++) {
        AtrWriteSector(info,sector, secBuff);
        AtrMyDosMarkSectorUsed(info,sector,TRUE);
        }

    if (AtrMyDosWriteVtoc(info)) {
        return(ADOS_VTOC_WRITE_ERR);
        }
    
    if ( AtrReadSector(info, AtrMyDosCurrentDirStartSector(info) + 
    					( pDirEntry->fileNumber / 8 ), secBuff) )
    {
        return ADOS_DIR_READ_ERR;
    }
    
    pTmp = secBuff + ( pDirEntry->fileNumber % 8) * 16;
    pTmp[0] = DIRE_SUBDIR;
    pTmp[1] = 8;
    pTmp[2] = 0;
    pTmp[3] = startingSector & 0xff;
    pTmp[4] = startingSector >> 8;

    Host2ADos(name,&pTmp[5]);

    if ( AtrWriteSector(info, AtrMyDosCurrentDirStartSector(info) + 
    					( pDirEntry->fileNumber / 8 ), secBuff) )
    {
        return ADOS_DIR_WRITE_ERR;
    }
    
    stat = AtrMyDosReadCurrentDir(info);
    if (stat)
    	return(stat);
    return FALSE;
}

int AtrMyDosLockFile(AtrDiskInfo *info, char *name, int lock)
{
    MyDosDirEntry  *pDirEntry = NULL;
    UBYTE secBuff[0x100];
    UBYTE *pTmp;

    if ((pDirEntry = AtrMyDosFindDirEntryByName(info,name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }

    if ( AtrReadSector(info, AtrMyDosCurrentDirStartSector(info) + 
    					( pDirEntry->fileNumber / 8 ), secBuff) )
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

    if ( AtrWriteSector(info, AtrMyDosCurrentDirStartSector(info)
    					 + ( pDirEntry->fileNumber / 8 ), secBuff) )
        {
        return ADOS_DIR_WRITE_ERR;
        }

    return FALSE;
}

int AtrMyDosRenameFile(AtrDiskInfo *info, char *name, char *newname)
{
    MyDosDirEntry  *pDirEntry = NULL;
    UBYTE secBuff[0x100];
	UBYTE dosName[13];
    UBYTE *pTmp;
    int stat;

    if ((pDirEntry = AtrMyDosFindDirEntryByName(info,name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }

    if ((pDirEntry->flags & DIRE_LOCKED) == DIRE_LOCKED) {
        return ADOS_FILE_LOCKED;
        }

	Host2ADos(newname, dosName);
	ADos2Host(newname, dosName);

    if (AtrMyDosFindDirEntryByName(info,newname) != NULL) {
		return ADOS_DUPLICATE_NAME;
		}

    if ( AtrReadSector(info, AtrMyDosCurrentDirStartSector(info) + 
    					( pDirEntry->fileNumber / 8 ), secBuff) )
        {
        return ADOS_DIR_READ_ERR;
        }

    pTmp = secBuff + ( pDirEntry->fileNumber % 8) * 16;

    Host2ADos(newname,&pTmp[5]);

    if ( AtrWriteSector(info, AtrMyDosCurrentDirStartSector(info) + 
    					( pDirEntry->fileNumber / 8 ), secBuff) )
        {
        return ADOS_DIR_WRITE_ERR;
        }

    stat = AtrMyDosReadCurrentDir(info);
    if (stat)
    	return stat;

    return FALSE;
}

int AtrMyDosDeleteFile(AtrDiskInfo *info, char *name)
{
    MyDosDirEntry  *pDirEntry = NULL;
    UBYTE secBuff[0x100];
    UWORD count, free, sector, sectorSize;
    UBYTE *pTmp;
    int stat;

    if ((pDirEntry = AtrMyDosFindDirEntryByName(info,name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }

    if ((pDirEntry->flags & DIRE_LOCKED) == DIRE_LOCKED) {
        return ADOS_FILE_LOCKED;
        }

  	if (pDirEntry->flags & DIRE_SUBDIR)
   		return ADOS_FILE_NOT_FOUND;

    if (AtrMyDosReadVtoc(info,&free))
        return ADOS_VTOC_READ_ERR;
    
    count = pDirEntry->secCount;
    sector = pDirEntry->secStart;
    sectorSize = AtrSectorSize(info);
    	
    secBuff[ sectorSize - 1 ] = 0;

	while( count )
	{
		if ( sector < 1 || (sector > AtrSectorCount(info)))
		{
			return ADOS_FILE_CORRUPTED;
		}

		if ( ( secBuff[ sectorSize - 1 ] & 0x80 ) && ( sectorSize == 0x80 ) )
		{
			return ADOS_FILE_CORRUPTED;
		}

		if ( AtrReadSector(info,sector, secBuff))
		{
			return ADOS_FILE_READ_ERR;
		}

        AtrMyDosMarkSectorUsed(info,sector,FALSE);
		
    	if ( AtrSectorCount(info) <= 943 ) 
             sector = secBuff[ sectorSize - 2 ] + 
                ( 0x03 & secBuff[ sectorSize - 3 ] ) * 0x100;
        else
             sector = secBuff[ sectorSize - 2 ] + 
                ( 0xff & secBuff[ sectorSize - 3 ] ) * 0x100;
		
    	if ( AtrSectorCount(info) <= 943 ) {
            if ( ( secBuff[ sectorSize-3 ] >> 2 ) != pDirEntry->fileNumber )
                {
			    return ADOS_FILE_CORRUPTED;
		        }
	        }

		count--;

	}

    if (AtrMyDosWriteVtoc(info)) 
        return(ADOS_VTOC_WRITE_ERR);
    
    if ( AtrReadSector(info, AtrMyDosCurrentDirStartSector(info) + 
    					( pDirEntry->fileNumber / 8 ), secBuff) )
        {
        return ADOS_DIR_READ_ERR;
        }

    pTmp = secBuff + ( pDirEntry->fileNumber % 8) * 16;

    *pTmp |= DIRE_DELETED;

    if ( AtrWriteSector(info, AtrMyDosCurrentDirStartSector(info) + 
    					( pDirEntry->fileNumber / 8 ), secBuff) )
        {
        return ADOS_DIR_WRITE_ERR;
        }
    
    stat = AtrMyDosReadCurrentDir(info);
    if (stat)
    	return(stat);

    return(FALSE);
}

int AtrMyDosImportFile(AtrDiskInfo *info, char *filename, int lfConvert)
{
    FILE *inFile;
    int file_length;
    UWORD freeSectors;
    UWORD numSectorsNeeded;
    UBYTE numToWrite;
    MyDosDirEntry *pEntry;
    UBYTE *pTmp;
    UBYTE secBuff[0x100];
    MyDosDirEntry  *pDirEntry;
    UWORD starting_sector = 0, last_sector = 0, curr_sector;
    int first_sector;
    char *slash;
    int stat;
	UBYTE dosName[12];
	UWORD sectorSize = AtrSectorSize(info);
    
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

    if ((pDirEntry = AtrMyDosFindDirEntryByName(info,filename)) != NULL) {
    
    	if ((pDirEntry->flags & DIRE_SUBDIR) == DIRE_SUBDIR)
            {
            return ADOS_FILE_IS_A_DIR;
            }
    	
        if (AtrMyDosDeleteFile(info,filename)) {
            fclose(inFile);
            return ADOS_DELETE_FILE_ERR;
            }
        }

    fseek(inFile, 0L, SEEK_END);
    file_length = ftell(inFile);
    fseek(inFile, 0L, SEEK_SET);

    if (file_length == 0) 
        numSectorsNeeded = 1;
    else if ((file_length % (sectorSize - 3)) == 0) 
        numSectorsNeeded = file_length / (sectorSize - 3);
    else
        numSectorsNeeded = file_length / (sectorSize - 3) + 1;

    if (AtrMyDosReadVtoc(info,&freeSectors)) {
        fclose(inFile);
        return(ADOS_VTOC_READ_ERR);
        }
    
    if (numSectorsNeeded > freeSectors) {
        fclose(inFile);
        return(ADOS_DISK_FULL);
    }

    if ((pEntry = AtrMyDosFindFreeDirEntry(info)) == NULL) {
        fclose(inFile);
        return(ADOS_DIR_FULL);
    }
    
    first_sector = TRUE;
    do {
        if (file_length >= (sectorSize - 3)) 
            numToWrite = (sectorSize - 3);
        else
            numToWrite = file_length;
       
        if (first_sector) {
            last_sector = AtrMyDosGetFreeSector(info);
            AtrMyDosMarkSectorUsed(info,last_sector,TRUE);
            starting_sector = last_sector;
            if (fread(secBuff,1,numToWrite,inFile) != numToWrite)
                {
                fclose(inFile);
                return(ADOS_HOST_READ_ERR);
                }
           if (lfConvert)
              HostLFToAtari(secBuff, numToWrite);
           secBuff[sectorSize - 1] = numToWrite;
            first_sector = 0;
            }
        else {
            curr_sector = AtrMyDosGetFreeSector(info);
            AtrMyDosMarkSectorUsed(info,curr_sector,TRUE);
            
		if ( AtrSectorCount(info) > 943 ) {
            secBuff[sectorSize - 3]  =  ((curr_sector & 0xFF00) >> 8);
			}
		else {
            secBuff[sectorSize - 3]  =  ((curr_sector & 0x300) >> 8) |
                                        (((UBYTE)pEntry->fileNumber) << 2);
			}
        secBuff[sectorSize - 2] = curr_sector & 0xFF;
            
        if (AtrWriteSector(info,last_sector, secBuff))
            {
            fclose(inFile);
            return(ADOS_FILE_WRITE_ERR);
            }
        if (fread(secBuff,1,numToWrite,inFile) != numToWrite)
            {
            fclose(inFile);
            return(ADOS_HOST_READ_ERR);
            }
		if (lfConvert)
			HostLFToAtari(secBuff, numToWrite);

        secBuff[sectorSize - 1] = numToWrite;
        last_sector = curr_sector;
        }
        file_length -= numToWrite;
    } while (file_length > 0);

    if ( AtrSectorCount(info) <= 943 )
        secBuff[sectorSize - 3]  = (((UBYTE)pEntry->fileNumber) << 2);
    else
        secBuff[sectorSize - 3]  = 0;
    secBuff[sectorSize - 2] = 0;
    AtrWriteSector(info,last_sector, secBuff);
    AtrMyDosWriteVtoc(info);
    
    if ( AtrReadSector(info, AtrMyDosCurrentDirStartSector(info) + 
    					( pEntry->fileNumber / 8 ), secBuff) )
    {
        return ADOS_DIR_READ_ERR;
    }
    pTmp = secBuff + ( pEntry->fileNumber % 8) * 16;
    pTmp[0] = DIRE_IN_USE | DIRE_DOS2CREATE | DIRE_MYDOSCREATE;
    pTmp[1] = numSectorsNeeded & 0xff;
    pTmp[2] = numSectorsNeeded >> 8;
    pTmp[3] = starting_sector & 0xff;
    pTmp[4] = starting_sector >> 8;
    Host2ADos(filename,&pTmp[5]);

    if ( AtrWriteSector(info, AtrMyDosCurrentDirStartSector(info) + 
    					( pEntry->fileNumber / 8 ), secBuff) )
    {
        return ADOS_DIR_WRITE_ERR;
    }

    stat = AtrMyDosReadCurrentDir(info);
    if (stat)
    	return(stat);
    return FALSE;
}

int AtrMyDosExportFile(AtrDiskInfo *info, char *nameToExport, char* outFile, int lfConvert)
{
	MyDosDirEntry* pDirEntry;
	UBYTE secBuff[ 0x0100 ];
	UWORD sector,count,sectorSize,fileNumber;
	FILE *output = NULL;

    if ((pDirEntry = AtrMyDosFindDirEntryByName(info,nameToExport)) == NULL) {
        return(ADOS_FILE_NOT_FOUND);
        }
	
    sector = pDirEntry->secStart;
	count = pDirEntry->secCount;
	sectorSize = AtrSectorSize(info);
	fileNumber = pDirEntry->fileNumber;

	if ( outFile )
	{
		output = fopen(outFile, "wb+");

		if ( output == NULL)
		{
			return ADOS_HOST_CREATE_ERR;
		}
	}

	secBuff[ sectorSize - 1 ] = 0;

	while( count )
	{
		if ( sector < 1 || (sector > AtrSectorCount(info)))
		{
			return ADOS_FILE_CORRUPTED;
		}

		if ( ( secBuff[ sectorSize - 1 ] & 0x80 ) && ( sectorSize == 0x80 ) )
		{
			return ADOS_FILE_CORRUPTED;
		}

		if ( AtrReadSector(info,sector, secBuff))
		{
			return ADOS_FILE_READ_ERR;
		}
		
		if ( AtrSectorCount(info) > 943 )
			sector = secBuff[ sectorSize - 2 ] + (
                      (UWORD)secBuff[ sectorSize - 3 ] & 0xff ) * 0x100;
		else
			sector = secBuff[ sectorSize - 2 ] + 
                      ( 0x03 & (UWORD)secBuff[ sectorSize - 3 ] ) * 0x100;

		if (lfConvert)
			AtariLFToHost(secBuff, secBuff[ sectorSize - 1 ]);
			
		if (fwrite( secBuff, 1, secBuff[ sectorSize - 1 ] ,output) != 
               secBuff[ sectorSize - 1 ])
            {
            fclose(output);
            return ADOS_HOST_WRITE_ERR;
            }

		count--;
	}

	if ( pDirEntry->secCount )
	{
		if ( ! ( secBuff[ sectorSize - 1 ] & 128 ) && 
               ( sectorSize == 128 ) && sector)
		{
			return ADOS_FILE_CORRUPTED;
		}
	}

	fclose( output );

	return FALSE;
}

static int AtrMyDosReadDir(AtrDiskInfo *info, UWORD dirSector)
{
    UBYTE secBuff[ 0x100 ];
    UBYTE *pTmp;
    MyDosDirEntry  *pEntry;
	MyDosDirEntry  *pPrev = NULL;
	MYDOS_DIRENT dirEntry;
	UWORD entry = 0;
	AtrMyDosDiskInfo *dinfo = (AtrMyDosDiskInfo *)info->atr_dosinfo;

    if (dinfo->pCurrDir) 
        AtrMyDosDeleteDirEntryList(info);

    do
	{
        if ( AtrReadSector(info, dirSector + ( entry / 8 ), secBuff) )
		{
			return ADOS_DIR_READ_ERR;
		}

		pTmp = secBuff + ( entry % 8) * 16;
		dirEntry.flags = *pTmp++;
		dirEntry.secCount = *pTmp + (*(pTmp+1) << 8);
        pTmp += 2;
		dirEntry.secStart = *pTmp + (*(pTmp+1) << 8);
        pTmp += 2;
		memcpy( dirEntry.atariName, pTmp, 11 );

		pEntry = AtrMyDosCreateDirEntry(&dirEntry, entry );
		if (pEntry == NULL)
			{
			AtrMyDosDeleteDirEntryList(info);
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
				pEntry->pPrev = NULL;
                pEntry->pNext = NULL;
				pPrev = pEntry;
			}

		}

		entry++;

	} while( entry < 64 );

	return FALSE;
}

static int AtrMyDosReadRootDir(AtrDiskInfo *info)
{	
	AtrMyDosDeleteDirList(info);
	return(AtrMyDosReadDir(info,ROOT_DIR));
}

static int AtrMyDosReadCurrentDir(AtrDiskInfo *info)
{
    return(AtrMyDosReadDir(info,AtrMyDosCurrentDirStartSector(info)));
}

static UWORD AtrMyDosCurrentDirStartSector(AtrDiskInfo *info)
{
	AtrMyDosDiskInfo *dinfo = (AtrMyDosDiskInfo *)info->atr_dosinfo;

    if (dinfo->pDirList) 
        return(dinfo->pDirList->dirStartSector);
    else
        return(ROOT_DIR);
}

static MyDosDirEntry *AtrMyDosFindDirEntryByName(AtrDiskInfo *info, char *name)
{
	AtrMyDosDiskInfo *dinfo = (AtrMyDosDiskInfo *)info->atr_dosinfo;
    MyDosDirEntry  *pCurr = NULL;
    
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

static UWORD AtrMyDosGetFreeSector(AtrDiskInfo *info)
{
    UWORD i;

    for (i=1;i<=AtrSectorCount(info)+1;i++)
        {
        if (!AtrMyDosIsSectorUsed(info,i) )
            {
            return i;
            }
        }
    return  0;
}

static UWORD AtrMyDosGetFreeSectorBlock(AtrDiskInfo *info, int count)
{
    UWORD i,j;
    int blockFree;

    for (i=1;i<=AtrSectorCount(info)+1-count;i++)
        {
        if (!AtrMyDosIsSectorUsed(info,i) )
            {
            blockFree = TRUE;
            for (j=i;j<i+count;j++) {
                if (AtrMyDosIsSectorUsed(info,j)) {
                    blockFree=FALSE;
                    }
                }
            if (blockFree)
                return i;
            }
        }
    return  0;
}

static int AtrMyDosReadVtoc(AtrDiskInfo *info, UWORD *freeSectors)
{
    UWORD sector, vtocCount;
    UBYTE secBuf[256];
    int i,j,stat,sectorSize;
	AtrMyDosDiskInfo *dinfo = (AtrMyDosDiskInfo *)info->atr_dosinfo;

    memset(dinfo->vtocMap,0,sizeof(dinfo->vtocMap));
    j = 0;
    
    stat = AtrReadSector(info, VTOC, secBuf);
    if (stat)
    	return stat;
    
    sectorSize = AtrSectorSize(info);
    	
    if ( sectorSize == 128 )
        vtocCount = ((UWORD)secBuf[0])*2-4;
    else
        vtocCount = ((UWORD)secBuf[0])-2;

    if (vtocCount > 33)
    	vtocCount = 33;
    if (vtocCount == 0) 
        vtocCount = 1;
    
    for(sector=VTOC;sector>VTOC-vtocCount;sector--)
        {
        stat = AtrReadSector(info, sector, secBuf);
        if ( stat )
            return stat;
        if ( sector == VTOC )
            {
            *freeSectors = secBuf[3] | ((UWORD)(secBuf[4])<<8);
            }
        for( i=((sector==VTOC)?10:0); i<sectorSize; i++ )
            dinfo->vtocMap[j++] = secBuf[i];
        }
    
    return FALSE;
}

static int AtrMyDosWriteVtoc(AtrDiskInfo *info)
{
    UWORD sector,sectorSize,vtocCount;
    int i,j=0, stat;
    unsigned char secBuf[256];
    UWORD freeSectors;
	AtrMyDosDiskInfo *dinfo = (AtrMyDosDiskInfo *)info->atr_dosinfo;
    
    stat = AtrReadSector(info, VTOC, secBuf);
    if (stat)
    	return stat;
    
    sectorSize = AtrSectorSize(info);
    	
    if ( sectorSize == 128 )
        vtocCount = ((UWORD)secBuf[0])*2-4;
    else
        vtocCount = ((UWORD)secBuf[0])-2;
        
    if (vtocCount > 33)
    	vtocCount = 33;
    if (vtocCount == 0) 
        vtocCount = 1;
        
    for(sector=VTOC;sector>VTOC-vtocCount;sector--)
        {
        stat = AtrReadSector(info, sector, secBuf);
        if ( stat )
            return stat;
        for( i=((sector==VTOC)?10:0); i<sectorSize; i++ )
            secBuf[i] = dinfo->vtocMap[j++];
        if ( sector == VTOC )
            {
        	freeSectors = 0;
        	for (i=0;i<(AtrSectorCount(info)+1);i++) 
            	{
            	if (!AtrMyDosIsSectorUsed(info,i)) 
                	freeSectors++;
            	}
            secBuf[3] = (UWORD)(freeSectors&255);
            secBuf[4] = (UWORD)(freeSectors>>8);
            }
        stat = AtrWriteSector(info, sector, secBuf);
        if ( stat )
            return stat;
        }
    
    return FALSE;

}

static int AtrMyDosIsSectorUsed(AtrDiskInfo *info, UWORD sector)
{
    UWORD entry;
    UBYTE mask;
    UBYTE maskTable[] = {128, 64, 32, 16, 8, 4, 2, 1};
	AtrMyDosDiskInfo *dinfo = (AtrMyDosDiskInfo *)info->atr_dosinfo;

    entry = sector / 8;
    mask = maskTable[ sector & 7];
    return ( !(dinfo->vtocMap[entry] & mask) );
}

static void AtrMyDosMarkSectorUsed(AtrDiskInfo *info, UWORD sector, int used)
{
    UWORD entry;
    UBYTE mask;
    UBYTE maskTable[] = {128, 64, 32, 16, 8, 4, 2, 1};
	AtrMyDosDiskInfo *dinfo = (AtrMyDosDiskInfo *)info->atr_dosinfo;

    entry = sector / 8;
    mask = maskTable[ sector & 7];

    if ( used != FALSE )
        dinfo->vtocMap[entry] &= (~mask);
    else
        dinfo->vtocMap[entry] |= mask;
}


static MyDosDirEntry *AtrMyDosFindFreeDirEntry(AtrDiskInfo *info)
{
	AtrMyDosDiskInfo *dinfo = (AtrMyDosDiskInfo *)info->atr_dosinfo;
    MyDosDirEntry *pCurr = dinfo->pCurrDir;

    while(pCurr->flags != 0 && (pCurr->flags & DIRE_DELETED) != DIRE_DELETED) {
        if (pCurr->pNext == NULL) 
            return NULL;
        pCurr = pCurr->pNext;
    }

    return pCurr;

}

static void AtrMyDosDeleteDirList(AtrDiskInfo *info)
{
	AtrMyDosDiskInfo *dinfo = (AtrMyDosDiskInfo *)info->atr_dosinfo;
	MyDosDir* pCurr = dinfo->pDirList;
	MyDosDir* pPrev;

	while( pCurr )
	{
		pPrev = pCurr->pPrev;
		free(pCurr);

		pCurr = pPrev;
	}

    dinfo->pDirList = NULL;
}

static void AtrMyDosDeleteDirEntryList(AtrDiskInfo *info)
{
	AtrMyDosDiskInfo *dinfo = (AtrMyDosDiskInfo *)info->atr_dosinfo;
	MyDosDirEntry* pCurr = dinfo->pCurrDir;
	MyDosDirEntry* pNext;

	while( pCurr )
	{
		pNext = pCurr->pNext;
		free(pCurr);

		pCurr = pNext;
	}

    dinfo->pCurrDir = NULL;
}

static MyDosDirEntry *AtrMyDosCreateDirEntry( MYDOS_DIRENT* pDirEntry, 
                                              UWORD entry )
{
	MyDosDirEntry *pEntry;
    
    pEntry = (MyDosDirEntry *) malloc(sizeof(MyDosDirEntry));

	if ( !pEntry )
	{
		return NULL;
	}

    ADos2Host( pEntry->filename, pDirEntry->atariName );

	pEntry->fileNumber = entry;
	pEntry->flags = pDirEntry->flags;
	pEntry->secStart = pDirEntry->secStart;
	pEntry->secCount = pDirEntry->secCount;

	return pEntry;
}

