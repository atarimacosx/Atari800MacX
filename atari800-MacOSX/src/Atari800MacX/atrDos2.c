/* atrDos2.c -  
 *  Part of the Atari Disk Image File Editor Library
 *  Mark Grebe <atarimac@kc.rr.com>
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
#include "atrDos2.h"
#include "atrErr.h"

#define VTOC_1          360
#define VTOC_2          1024
#define ROOT_DIR        361

typedef struct DOS2_DIRENT
{
	UBYTE flags;
	UWORD secCount; //Number of sectors in file
	UWORD secStart; //First sector in file
	UBYTE atariName[11];
} DOS2_DIRENT;

typedef struct Dos2DirEntry
{
    struct Dos2DirEntry *pNext;
    struct Dos2DirEntry *pPrev;
    char filename[ 256 ];
    UWORD fileNumber;
	UBYTE flags;
	UWORD secCount; //Number of sectors in file
	UWORD secStart; //First sector in file
} Dos2DirEntry;

typedef struct atrDos2DiskInfo {
    Dos2DirEntry  *pRoot;
	int            useFileNumbers;
    UBYTE vtocMap[128*2];
	} AtrDos2DiskInfo;

static int AtrDos2ReadRootDir(AtrDiskInfo *info);
static Dos2DirEntry *AtrDos2FindDirEntryByName(AtrDiskInfo *info, char *name);
static UWORD AtrDos2GetFreeSector(AtrDiskInfo *info);
static int AtrDos2ReadVtoc(AtrDiskInfo *info, UWORD *freeSectors);
static int AtrDos2WriteVtoc(AtrDiskInfo *info);
static int AtrDos2IsSectorUsed(AtrDiskInfo *info,  UWORD sector);
static void AtrDos2MarkSectorUsed(AtrDiskInfo *info, UWORD sector, int used);
static Dos2DirEntry *AtrDos2FindFreeDirEntry(AtrDiskInfo *info);
static void AtrDos2DeleteDirList(AtrDiskInfo *info);
static Dos2DirEntry *AtrDos2CreateDirEntry( DOS2_DIRENT* pDirEntry, 
                                            UWORD entry );

int AtrDos2Mount(AtrDiskInfo *info, int useFileNumbers)
{
	AtrDos2DiskInfo *dinfo;
	info->atr_dosinfo = (void *) calloc(1, sizeof(AtrDos2DiskInfo));
	if (info->atr_dosinfo == NULL)
		return(ADOS_MEM_ERR);
		
	dinfo = (AtrDos2DiskInfo *)info->atr_dosinfo;
	dinfo->useFileNumbers = useFileNumbers;
	return(AtrDos2ReadRootDir(info));
}

void AtrDos2Unmount(AtrDiskInfo *info)
{
    AtrDos2DeleteDirList(info);
	free(info->atr_dosinfo);
}

int AtrDos2GetDir(AtrDiskInfo *info, UWORD *fileCount, ADosFileEntry *files, 
                  ULONG *freeBytes)
{
    Dos2DirEntry  *pCurr = NULL;
    ADosFileEntry *pFileEntry = files;
    UWORD count = 0;
    char name[15];
    UWORD freeSectors;
	AtrDos2DiskInfo *dinfo = (AtrDos2DiskInfo *)info->atr_dosinfo;

    *fileCount = 0;
    pCurr = dinfo->pRoot;
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

    if (AtrDos2ReadVtoc(info,&freeSectors))
        return(TRUE);

    *fileCount = count;
    *freeBytes = freeSectors * AtrSectorSize(info);

    return(FALSE);
}

int AtrDos2LockFile(AtrDiskInfo *info, char *name, int lock)
{
    Dos2DirEntry  *pDirEntry = NULL;
    UBYTE secBuff[0x100];
    UBYTE *pTmp;

    if ((pDirEntry = AtrDos2FindDirEntryByName(info,name)) == NULL) {
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

int AtrDos2RenameFile(AtrDiskInfo *info, char *name, char *newname)
{
    Dos2DirEntry  *pDirEntry = NULL;
    UBYTE secBuff[0x100];
	UBYTE dosName[13];
    UBYTE *pTmp;
    int stat;

    if ((pDirEntry = AtrDos2FindDirEntryByName(info,name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }

    if ((pDirEntry->flags & DIRE_LOCKED) == DIRE_LOCKED) {
        return ADOS_FILE_LOCKED;
        }
		
	Host2ADos(newname, dosName);
	ADos2Host(newname, dosName);

    if (AtrDos2FindDirEntryByName(info,newname) != NULL) {
		return ADOS_DUPLICATE_NAME;
		}

    if ( AtrReadSector(info, ROOT_DIR + ( pDirEntry->fileNumber / 8 ), secBuff) )
        {
        return ADOS_DIR_READ_ERR;
        }

    pTmp = secBuff + ( pDirEntry->fileNumber % 8) * 16;

    Host2ADos(newname,&pTmp[5]);

    if ( AtrWriteSector(info, ROOT_DIR + ( pDirEntry->fileNumber / 8 ), secBuff) )
        {
        return ADOS_DIR_WRITE_ERR;
        }

    stat = AtrDos2ReadRootDir(info);
    if (stat)
    	return stat;

    return FALSE;
}

int AtrDos2DeleteFile(AtrDiskInfo *info, char *name)
{
    Dos2DirEntry  *pDirEntry = NULL;
    UBYTE secBuff[0x100];
    UWORD count, free, sector, sectorSize;
    UBYTE *pTmp;
    int stat;

    if ((pDirEntry = AtrDos2FindDirEntryByName(info,name)) == NULL) {
        return ADOS_FILE_NOT_FOUND;
        }

    if ((pDirEntry->flags & DIRE_LOCKED) == DIRE_LOCKED) {
        return ADOS_FILE_LOCKED;
        }

    if (AtrDos2ReadVtoc(info,&free))
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
			return ADOS_FILE_CORRUPTED;
		}

        AtrDos2MarkSectorUsed(info,sector,FALSE);
		
        sector = secBuff[ sectorSize - 2 ] + 
                ( 0x03 & secBuff[ sectorSize - 3 ] ) * 0x100;
		
        if ( ( secBuff[ sectorSize-3 ] >> 2 ) != pDirEntry->fileNumber )
		{
			return ADOS_FILE_CORRUPTED;
		}

		count--;
	}

    if (AtrDos2WriteVtoc(info)) 
        return(ADOS_VTOC_WRITE_ERR);
    
    if ( AtrReadSector(info, ROOT_DIR + ( pDirEntry->fileNumber / 8 ), secBuff) )
        {
        return ADOS_DIR_READ_ERR;
        }

    pTmp = secBuff + ( pDirEntry->fileNumber % 8) * 16;

    *pTmp |= DIRE_DELETED;

    if ( AtrWriteSector(info, ROOT_DIR + ( pDirEntry->fileNumber / 8 ), secBuff) )
        {
        return ADOS_DIR_WRITE_ERR;
        }
    
    stat = AtrDos2ReadRootDir(info);
    if (stat)
    	return(stat);

    return(FALSE);
}

int AtrDos2ImportFile(AtrDiskInfo *info, char *filename, int lfConvert)
{
    FILE *inFile;
    int file_length;
    UWORD freeSectors;
    UWORD numSectorsNeeded;
    UBYTE numToWrite;
    Dos2DirEntry *pEntry;
    UBYTE *pTmp;
    UBYTE secBuff[0x100];
    Dos2DirEntry  *pDirEntry;
    UWORD starting_sector = 0, last_sector = 0, curr_sector = 0;
    int first_sector;
    char *slash;
    int stat;
	UBYTE dosName[12];
	UWORD sectorSize = AtrSectorSize(info);
 	AtrDos2DiskInfo *dinfo = (AtrDos2DiskInfo *)info->atr_dosinfo;
   
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

    if ((pDirEntry = AtrDos2FindDirEntryByName(info, filename)) != NULL) {
        if (AtrDos2DeleteFile(info,filename)) {
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

    if (AtrDos2ReadVtoc(info, &freeSectors)) {
        fclose(inFile);
        return(ADOS_VTOC_READ_ERR);
        }
    
    if (numSectorsNeeded > freeSectors) {
        fclose(inFile);
        return(ADOS_DISK_FULL);
    }

    if ((pEntry = AtrDos2FindFreeDirEntry(info)) == NULL) {
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
            last_sector = AtrDos2GetFreeSector(info);
            AtrDos2MarkSectorUsed(info,last_sector,TRUE);
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
            curr_sector = AtrDos2GetFreeSector(info);
            AtrDos2MarkSectorUsed(info,curr_sector,TRUE);
            if (dinfo->useFileNumbers)
                secBuff[sectorSize - 3]  = 
                    (pEntry->fileNumber << 2) |
                    ((curr_sector & 0x300) >> 8);
            else
                secBuff[sectorSize - 3] = 
                      (curr_sector & 0xff00) >> 8;

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


    if (dinfo->useFileNumbers)
      secBuff[sectorSize - 3]  = (pEntry->fileNumber << 2);
    else
      secBuff[sectorSize - 3]  = 0;
    secBuff[sectorSize - 2] = 0;
    AtrWriteSector(info,last_sector, secBuff);
    AtrDos2WriteVtoc(info);
    
    if ( AtrReadSector(info, ROOT_DIR + ( pEntry->fileNumber / 8 ), secBuff) )
    {
        return ADOS_DIR_READ_ERR;
    }
    pTmp = secBuff + ( pEntry->fileNumber % 8) * 16;
    pTmp[0] = DIRE_IN_USE | DIRE_DOS2CREATE;
    pTmp[1] = numSectorsNeeded & 0xff;
    pTmp[2] = numSectorsNeeded >> 8;
    pTmp[3] = starting_sector & 0xff;
    pTmp[4] = starting_sector >> 8;
    Host2ADos(filename,&pTmp[5]);

    if ( AtrWriteSector(info, ROOT_DIR + ( pEntry->fileNumber / 8 ), secBuff) )
    {
        return ADOS_DIR_WRITE_ERR;
    }

    stat = AtrDos2ReadRootDir(info);
    if (stat)
    	return(stat);
    return FALSE;
}

int AtrDos2ExportFile(AtrDiskInfo *info, char *nameToExport, char* outFile, int lfConvert)
{
	Dos2DirEntry* pDirEntry;
	UBYTE secBuff[ 0x0100 ];
	UWORD sector,count,sectorSize,fileNumber;
	FILE *output = NULL;
	AtrDos2DiskInfo *dinfo = (AtrDos2DiskInfo *)info->atr_dosinfo;

    if ((pDirEntry = AtrDos2FindDirEntryByName(info,nameToExport)) == NULL) {
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

		if (dinfo->useFileNumbers)
		{
            sector = secBuff[ sectorSize - 2 ] + 
                     ( 0x03 & secBuff[ sectorSize - 3 ] ) * 0x100;

		    if ( ( secBuff[ sectorSize-3 ] >> 2 ) != fileNumber )
		    {
			    return ADOS_FILE_CORRUPTED;
		    }
        }
        else
        {
             sector = secBuff[ sectorSize - 2 ] + 
                     ( 0xFF & secBuff[ sectorSize - 3 ] ) * 0x100;
        }
            

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

static int AtrDos2ReadRootDir(AtrDiskInfo *info)
{
    UBYTE secBuff[ 0x100 ];
    UBYTE *pTmp;
    Dos2DirEntry  *pEntry;
	Dos2DirEntry  *pPrev = NULL;
	DOS2_DIRENT dirEntry;
	UWORD entry = 0;
	AtrDos2DiskInfo *dinfo = (AtrDos2DiskInfo *)info->atr_dosinfo;

    if (dinfo->pRoot) 
        AtrDos2DeleteDirList(info);
	
    do
	{
        if ( AtrReadSector(info, ROOT_DIR + ( entry / 8 ), secBuff) )
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

		pEntry = AtrDos2CreateDirEntry( &dirEntry, entry );
		if (pEntry == NULL)
			{
			AtrDos2DeleteDirList(info);
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

static Dos2DirEntry *AtrDos2FindDirEntryByName(AtrDiskInfo *info, char *name)
{
    Dos2DirEntry  *pCurr = NULL;
	AtrDos2DiskInfo *dinfo = (AtrDos2DiskInfo *)info->atr_dosinfo;

    pCurr = dinfo->pRoot;

    while (pCurr) 
        {
        if ((strcmp(name,pCurr->filename) == 0) &&
            ((pCurr->flags & DIRE_DELETED) != DIRE_DELETED))
            return(pCurr);
        pCurr = pCurr->pNext;
        }
    return(NULL);
}

static UWORD AtrDos2GetFreeSector(AtrDiskInfo *info)
{
    UWORD i;

    for (i=1;i<=AtrSectorCount(info)+1;i++)
        {
        if (!AtrDos2IsSectorUsed(info,i) )
            {
            return i;
            }
        }
    return  0;
}

static int AtrDos2ReadVtoc(AtrDiskInfo *info, UWORD *freeSectors)
{
    UBYTE secBuf[256];
    UWORD sectorSize = AtrSectorSize(info);
    int i,j,stat;
	AtrDos2DiskInfo *dinfo = (AtrDos2DiskInfo *)info->atr_dosinfo;

    memset(dinfo->vtocMap,0,sizeof(dinfo->vtocMap));
    
    // check for DOS 2.5 Enhanced Density, if so VTOC is located at
    // sector 1024 in addition to sector 360.
    if ( AtrSectorCount(info) == 1040 )
        {
        stat = AtrReadSector(info, VTOC_2, secBuf);
        if ( stat )
            return stat;
        for( j=6,i=0; i<122; i++ )
            dinfo->vtocMap[j++] = secBuf[i];
        *freeSectors = secBuf[122] | (secBuf[123] << 8);
        stat = AtrReadSector(info, VTOC_1, secBuf);
        if ( stat )
            return stat;
        for( j=0,i=10; i<100; i++ )
            dinfo->vtocMap[j++] = secBuf[i];
        *freeSectors += secBuf[3] | ((UBYTE)(secBuf[4])<<8);
        }
    else
        {
        j = 0;
        stat = AtrReadSector(info, VTOC_1, secBuf);
        if ( stat )
            return stat;
        *freeSectors = secBuf[3] | ((UWORD)(secBuf[4])<<8);
        for( i=10; i<sectorSize; i++ )
           dinfo->vtocMap[j++] = secBuf[i];
        }
    return FALSE;
}

static int AtrDos2WriteVtoc(AtrDiskInfo *info)
{
    UWORD sectorSize = AtrSectorSize(info);
    UWORD sectorCount = AtrSectorCount(info);
    int i,j=0, stat;
    unsigned char secBuf[256];
    UWORD freeSectors;
	AtrDos2DiskInfo *dinfo = (AtrDos2DiskInfo *)info->atr_dosinfo;

    // check for DOS 2.5 Enhanced Density, if so VTOC is located at
    // sector 1024 in addition to sector 360.
    if ( sectorCount == 1040 )
        {
        stat = AtrReadSector(info, VTOC_2, secBuf);
        if ( stat )
            return stat;
        for( j=6,i=0; i<122; i++ )
            secBuf[i] = dinfo->vtocMap[j++] ;
        freeSectors = 0;
        for (i=720;i<1024;i++) 
            {
            if (!AtrDos2IsSectorUsed(info,i)) 
                freeSectors++;
            }
        secBuf[122] = freeSectors & 0xFF;
        secBuf[123] = freeSectors >> 8;
        stat = AtrWriteSector(info, VTOC_2, secBuf);
        if ( stat )
            return stat;
        stat = AtrReadSector(info, VTOC_1, secBuf);
        if ( stat )
            return stat;
        for( j=0,i=10; i<100; i++ )
            secBuf[i] = dinfo->vtocMap[j++];
        freeSectors = 0;
        for (i=0;i<720;i++) 
            {
            if (!AtrDos2IsSectorUsed(info,i)) 
                freeSectors++;
            }
        secBuf[3] = freeSectors & 0xFF;
        secBuf[4] = freeSectors >> 8;
        stat = AtrWriteSector(info, VTOC_1, secBuf);
        if ( stat )
            return stat;
        }
    else 
        {
        stat = AtrReadSector(info, VTOC_1, secBuf);
        if ( stat )
            return stat;
        for( i=10; i<sectorSize; i++ )
            secBuf[i] = dinfo->vtocMap[j++];
        freeSectors = 0;
        for (i=0;i<sectorCount;i++) 
            {
            if (!AtrDos2IsSectorUsed(info,i)) 
                freeSectors++;
            }
        secBuf[3] = (UBYTE)(freeSectors&255);
        secBuf[4] = (UBYTE)(freeSectors>>8);
        stat = AtrWriteSector(info, VTOC_1, secBuf);
        if ( stat )
            return stat;
        }
    return 0;

}

static int AtrDos2IsSectorUsed(AtrDiskInfo *info, UWORD sector)
{
    UWORD entry;
    UBYTE mask;
    UBYTE maskTable[] = {128, 64, 32, 16, 8, 4, 2, 1};
	AtrDos2DiskInfo *dinfo = (AtrDos2DiskInfo *)info->atr_dosinfo;

    entry = sector / 8;
    mask = maskTable[ sector & 7];
    return ( !(dinfo->vtocMap[entry] & mask) );
}

static void AtrDos2MarkSectorUsed(AtrDiskInfo *info, UWORD sector, int used)
{
    UWORD entry;
    UBYTE mask;
    UBYTE maskTable[] = {128, 64, 32, 16, 8, 4, 2, 1};
	AtrDos2DiskInfo *dinfo = (AtrDos2DiskInfo *)info->atr_dosinfo;

    entry = sector / 8;
    mask = maskTable[ sector & 7];

    if ( used != FALSE )
        dinfo->vtocMap[entry] &= (~mask);
    else
        dinfo->vtocMap[entry] |= mask;
}


static Dos2DirEntry *AtrDos2FindFreeDirEntry(AtrDiskInfo *info)
{
	AtrDos2DiskInfo *dinfo = (AtrDos2DiskInfo *)info->atr_dosinfo;
    Dos2DirEntry *pCurr = dinfo->pRoot;

    while(pCurr->flags != 0 && (pCurr->flags & DIRE_DELETED) != DIRE_DELETED) {
        if (pCurr->pNext == NULL) 
            return NULL;
        pCurr = pCurr->pNext;
    }

    return pCurr;

}

static void AtrDos2DeleteDirList(AtrDiskInfo *info)
{
	AtrDos2DiskInfo *dinfo = (AtrDos2DiskInfo *)info->atr_dosinfo;
	Dos2DirEntry* pCurr = dinfo->pRoot;
	Dos2DirEntry* pNext;

	while( pCurr )
	{
		pNext = pCurr->pNext;
		free(pCurr);

		pCurr = pNext;
	}

    dinfo->pRoot = NULL;
}

static Dos2DirEntry *AtrDos2CreateDirEntry( DOS2_DIRENT* pDirEntry, 
                                            UWORD entry )
{
	Dos2DirEntry *pEntry;
    
    pEntry = (Dos2DirEntry *) malloc(sizeof(Dos2DirEntry));

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

