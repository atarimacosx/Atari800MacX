/* atrMount.c -  
 *  Part of the atrEditLib Library
 *  Mark Grebe <atarimacosx@gmail.com>
 *  
 * Based on code from:
 *    Atari800 Emulator (atari800.sourceforge.net)
 *    Adir v0.67 (c) 1999-2001 Jindrich Kubec <kubecj@asw.cz>
 *    Atr8fs v0.1  http://www.rho-sigma.de/atari8bit/fs.html
 *
 * Copyright (C) 2004 Mark Grebe
 *
 * atrEditLib Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * atrEditLib Library is distributed in the hope 
 * that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari Disk Image File Editor Library; if not, write to the 
 * Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* atrMount.c - Main API  for atrEditLib
 *
 * Description:  This file provides the user callable functions which allow
 *  mounting/unmounting of ATR disk images, and manipulation of these images
 *  either by sector or by file.  Any ATR image is editable by this library
 *  on a sector basis.  However, to edit files, the image must be formated
 *  with a recognized DOS.  The following DOSes are supported:
 *     Atari DOS 1.0
 *     Atari DOS 2.x (and compatibles - Smart Dos,MachDos,DOS XL)
 *     TopDOS 3.0
 *     BiboDOS
 *     Atari DOS 3.0
 *     Atari DOS 4.0
 *     Atari DOS XE
 *     MyDOS
 *     SpartaDos 2 and up (and compatibles - BWDos)
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "afile.h"
#include "atari.h"
#include "atrUtil.h"
#include "atrMount.h"
#include "atrDos2.h"
#include "atrDos3.h"
#include "atrDos4.h"
#include "atrXE.h"
#include "atrMyDos.h"
#include "atrSparta.h"
#include "atrErr.h"

/*
   =================
   ATR Info
   =================
 */
 
#define	AFILE_ATR_MAGIC1	0x96
#define	AFILE_ATR_MAGIC2	0x02

#define BOOT_SECTORS_LOGICAL	0
#define BOOT_SECTORS_PHYSICAL	1
#define BOOT_SECTORS_SIO2PC		2

#define DOS4_26_FAT_SECTOR	515
#define DOS4_18_FAT_SECTOR	360
#define DOS4_BLOCK_SIZE     768

static void SizeOfSector(AtrDiskInfo *info, int sector, int *sz, ULONG * ofs);
static int AtrSeekSector(AtrDiskInfo *info, int sector);
static int AtrDiskType(AtrDiskInfo *info);

int AtrMount(const char *filename, int *dosType, 
             int *readWrite, int *writeProtect, AtrDiskInfo **info)
{
	struct AFILE_ATR_Header header;
	int i;
    int stat;
    
    *info = (AtrDiskInfo *) calloc(1, sizeof(AtrDiskInfo));
    if (*info == NULL)
    	return ADOS_MEM_ERR;

    *dosType = DOS_UNKNOWN;
    (*info)->atr_dostype = DOS_UNKNOWN;

    (*info)->atr_file = fopen(filename, "rb+");
    if (!(*info)->atr_file) {
        *readWrite = FALSE;
        (*info)->atr_file = fopen(filename, "rb");
        }
    else
        *readWrite = TRUE;    
    
	if ((*info)->atr_file) {
		ULONG file_length;

		fseek((*info)->atr_file, 0L, SEEK_END);
		file_length = ftell((*info)->atr_file);
		fseek((*info)->atr_file, 0L, SEEK_SET);

		if (fread(&header, 1, sizeof(struct AFILE_ATR_Header), (*info)->atr_file) < 
              sizeof(struct AFILE_ATR_Header)) {
			fclose((*info)->atr_file);
			return ADOS_DISK_READ_ERR;
            }
		}

	(*info)->atr_boot_sectors_type = BOOT_SECTORS_LOGICAL;

	if ((header.magic1 == AFILE_ATR_MAGIC1) && (header.magic2 == AFILE_ATR_MAGIC2)) {

		if (header.writeprotect)
			*writeProtect = 1;
		else
			*writeProtect = 0;

		(*info)->atr_sectorsize = header.secsizehi << 8 |
			header.secsizelo;

		/* ATR header contains length in 16-byte chunks */
		/* First compute number of 128-byte chunks */
		(*info)->atr_sectorcount = (header.hiseccounthi << 24 |
			header.hiseccountlo << 16 |
			header.seccounthi << 8 |
			header.seccountlo) >> 3;

		/* Fix if double density */
		if ((*info)->atr_sectorsize == 256) {
			if ((*info)->atr_sectorcount & 1)
				(*info)->atr_sectorcount += 3;   /* logical (128-byte) boot sectors */
			else {	
                /* 256-byte boot sectors */
                /* check if physical or SIO2PC */
				/* Physical if there's a non-zero byte in bytes 0x190-0x30f 
                    of the ATR file */
				UBYTE buffer[0x180];
				fseek((*info)->atr_file, 0x190L, SEEK_SET);
				fread(buffer, 1, 0x180, (*info)->atr_file);
				(*info)->atr_boot_sectors_type = BOOT_SECTORS_SIO2PC;
				for (i = 0; i < 0x180; i++)
					if (buffer[i] != 0) {
						(*info)->atr_boot_sectors_type = BOOT_SECTORS_PHYSICAL;
						break;
					}
                }
            (*info)->atr_sectorcount >>= 1;
            }
		else if ((*info)->atr_sectorsize == 512) {
            (*info)->atr_sectorcount >>= 2;
			}
		
        *dosType = AtrDiskType(*info);
        if (*dosType == -1)
            return ADOS_DISK_READ_ERR;

        (*info)->atr_dostype = *dosType;

        switch((*info)->atr_dostype) {
            case DOS_UNKNOWN:
                return(ADOS_UNKNOWN_FORMAT);
            case DOS_ATARI:
            case DOS_ATARI1:
                stat = AtrDos2Mount(*info, TRUE);
                if (stat) 
                    AtrUnmount(*info);
                return(stat);
            case DOS_TOP:
            case DOS_BIBO:
                stat = AtrDos2Mount(*info, FALSE);
                if (stat) 
                    AtrUnmount(*info);
                return(stat);
            case DOS_ATARI3:
                stat = AtrDos3Mount(*info);
                if (stat) 
                    AtrUnmount(*info);
                return(stat);
            case DOS_ATARI4:
                stat = AtrDos4Mount(*info);
                if (stat) 
                    AtrUnmount(*info);
                return(stat);
            case DOS_ATARIXE:
                stat = AtrXEMount(*info);
                if (stat) 
                    AtrUnmount(*info);
                return(stat);
            case DOS_MYDOS:
                stat = AtrMyDosMount(*info);
                if (stat) 
                    AtrUnmount(*info);
                return(stat);
            case DOS_SPARTA2:
                stat = AtrSpartaMount(*info);
                if (stat) 
                    AtrUnmount(*info);
                return(stat);
            }

	    return ADOS_UNKNOWN_FORMAT;
    }
    else
        return ADOS_NOT_A_ATR_IMAGE;
}

static int AtrDiskType(AtrDiskInfo *info)
{
    UBYTE buffer[0x200];
    UBYTE buffer1[0x200];
    UWORD maxSectors;
    UWORD fat_sector = 0;
    UWORD max_free_blocks;
    UWORD sectorSize = AtrSectorSize(info);
    UWORD sectorCount = AtrSectorCount(info);

    /* Check for SpartaDOS 2 and compatibles */
    if (AtrReadSector(info,1, buffer1))
        return -1;

	/* TBD - MDG - Need to add other changes for SpartDos format 2.1 */
    if (( buffer1[7] == 0x80 ) && ((buffer1[32] == 0x20 ) ||
								   (buffer1[32] == 0x21 )))
        return(DOS_SPARTA2);
    /*  Check for AtariDos 2.0 and compatibles */
    else {
        if (AtrReadSector(info,360, buffer))
            return -1;
        maxSectors = buffer[1] + (buffer[2]<<8);
        if (sectorSize == 128 && sectorCount == 720)
        {
            if (buffer[0] == 0x01 && maxSectors == 0x02c5)
                return DOS_ATARI1;
            if (buffer[0] == 0x02 && maxSectors == 0x02c3)
                return DOS_ATARI;
            if (buffer[0] == 0x02 && maxSectors == 0x02c4)
                return DOS_MYDOS;
            if (buffer[0] == 0x03 && maxSectors == 0x02c3)
                return DOS_TOP;
            if (buffer[0] == 0x00 && maxSectors == 0x02c3)
                return DOS_BIBO;
        }
        else if (sectorSize == 128 && (sectorCount == 1040 || sectorCount ==1024))
        {
            if (buffer[0] == 0x02 && maxSectors == 0x03f2)
                return DOS_ATARI;
            if (buffer[0] == 0x03 && maxSectors == 0x0403)
                return DOS_MYDOS;
            if (buffer[0] == 0x03 && maxSectors == 0x03a3)
                return DOS_TOP;
            if (buffer[0] == 0xff && maxSectors == 0x03f3)
                return DOS_BIBO;
        }
        else if (sectorSize == 256 && sectorCount == 720)
        {
            if (buffer[0] == 0x02 && maxSectors == 0x02c3)
                return DOS_ATARI;
            if (buffer[0] == 0x03 && maxSectors == 0x02c3)
                return DOS_TOP;
            if (buffer[0] == 0x02 && maxSectors == 0x02c4)
                return DOS_MYDOS;
            if (buffer[0] == 0x00 && maxSectors == 0x02c3)
                return DOS_BIBO;
        }
        else if (sectorSize == 256 && sectorCount == 1440)
        {
            if (buffer[0] == 0x03 && maxSectors == 0x0594)
                return DOS_MYDOS;
            if (buffer[0] == 0x83 && maxSectors == 0x0593)
                return DOS_TOP;
            if (buffer[0] == 0x03 && maxSectors == 0x0593)
                return DOS_BIBO;
        }
        else if ( (buffer[0] > 0) && (buffer[0] < 36)  && 
               (sectorCount > 1040)) 
            return(DOS_MYDOS);
        
        /*  Check for AtariDos 3.0 */        
        if (AtrReadSector(info,16, buffer))
            return -1;
        if (sectorSize == 128 && sectorCount == 720) {
            if ((buffer[15] == 0xA5) && (buffer[14] == 0x57))
                return(DOS_ATARI3);
            }
        if (sectorSize == 128 && sectorCount == 1040) {
            if ((buffer[15] == 0xA5) && (buffer[14] == 0x7F))
                return(DOS_ATARI3);
            }
        
        /*  Check for AtariDos 4.0 */                         
        if (sectorSize == 128) {
            if (sectorCount == 720) 
		        fat_sector = DOS4_18_FAT_SECTOR;
		    else if (sectorCount == 1040)
		        fat_sector = DOS4_26_FAT_SECTOR;
		    else
		        return DOS_UNKNOWN;
	        }
        else if (sectorSize == 256) {
            if (sectorCount == 720 || sectorCount == 1440)
		        fat_sector = DOS4_18_FAT_SECTOR;
		    else
		        return DOS_UNKNOWN;
	        }

	    /* read the possible DOS4 FAT */
        if (AtrReadSector(info,fat_sector, buffer))
            return -1;
               
		max_free_blocks = AtrSectorSize(info) * AtrSectorCount(info) / 
                              DOS4_BLOCK_SIZE;
        if (((buffer[0] == 0x52 && sectorCount != 1440)||
              (buffer[0] == 0x43 && sectorCount == 1440))&&
		    buffer[1] >= 8 &&
		    buffer[2] == 0 &&
		    buffer[3] <= max_free_blocks &&
		    buffer[4] == 0) {
		    if (sectorSize == 256 && sectorCount == 1440)
		       return DOS_UNKNOWN; /* DSDD Dos 4 disk format is broken */
            else
		       return DOS_ATARI4;
		   }
        /*  Check for AtariDos XE*/                         
        if (AtrReadSector(info,1,buffer))
              return -1;
        if (sectorSize == 128) {
            if (sectorCount == 720) {
               if (strncmp((const char *)&buffer[16],"AT810",4) == 0) {
                   return DOS_ATARIXE;
                   }
               }
            else if (sectorCount == 1040) { 
               if (strncmp((const char *)&buffer[16],"AT1050",6) == 0) {
                   return DOS_ATARIXE;
                   }
               }
           }
        else if (sectorSize == 256) {
           if (sectorCount == 720) { 
               if (strncmp((const char *)&buffer[16],"SSDD",4) == 0) {
                   return DOS_ATARIXE;
                   }
               }
           else if (sectorCount == 1440) {
               if (strncmp((const char *)&buffer[16],"XF551",5) == 0) {
                   return DOS_ATARIXE;
                   }
               }
           }
        
       }
    return(DOS_UNKNOWN);
}

void AtrUnmount(AtrDiskInfo *info)
{
    switch(info->atr_dostype) {
        case DOS_ATARI:
            AtrDos2Unmount(info);
			break;
        case DOS_ATARI3:
            AtrDos3Unmount(info);
			break;
        case DOS_ATARI4:
            AtrDos4Unmount(info);
			break;
        case DOS_ATARIXE:
            AtrXEUnmount(info);
			break;
        case DOS_MYDOS:
            AtrMyDosUnmount(info);
			break;
        case DOS_SPARTA2:
            AtrSpartaUnmount(info);
			break;
        }

    if (info->atr_file) 
        fclose(info->atr_file);
	free(info);
}

int AtrSectorSize(AtrDiskInfo *info)
{
	ULONG offset;
	int size;

	SizeOfSector(info, 4, &size, (ULONG*)&offset);
    return(size);
}

int AtrSectorNumberSize(AtrDiskInfo *info, int sector)
{
	ULONG offset;
	int size;

	SizeOfSector(info, sector, &size, (ULONG*)&offset);
    return(size);
}

int AtrSectorCount(AtrDiskInfo *info)
{
    if (info->atr_file) 
        return info->atr_sectorcount;
    else
        return 0;
}

/* Unit counts from zero up */
int AtrReadSector(AtrDiskInfo *info, int sector, UBYTE * buffer)
{
	int size;

    if (info->atr_file) {
    	if (sector > 0 && sector <= info->atr_sectorcount) {
    		size = AtrSeekSector(info, sector);
            fread(buffer, 1, size, info->atr_file);
            return FALSE;
            }
        }

	return ADOS_DISK_READ_ERR;
}

int AtrWriteSector(AtrDiskInfo *info, int sector, UBYTE * buffer)
{
	int size;

	if (info->atr_file) {
		if (sector > 0 && sector <= info->atr_sectorcount) {
			size = AtrSeekSector(info, sector);
			fwrite(buffer, 1, size, info->atr_file);
			return FALSE;
        }
	}
	
    return ADOS_DISK_WRITE_ERR;
}

int AtrSetWriteProtect(AtrDiskInfo *info, int writeProtect)
{
	struct AFILE_ATR_Header header;

	fseek(info->atr_file, 0L, SEEK_SET);

	if (fread(&header, 1, sizeof(struct AFILE_ATR_Header), info->atr_file) < 
             sizeof(struct AFILE_ATR_Header)) 
		return ADOS_DISK_READ_ERR;

    if (writeProtect)
		header.writeprotect = 1;
	else
		header.writeprotect = 0;

	fseek(info->atr_file, 0L, SEEK_SET);

	if (fwrite(&header, 1, sizeof(struct AFILE_ATR_Header), info->atr_file) < 
             sizeof(struct AFILE_ATR_Header)) 
		return ADOS_DISK_WRITE_ERR;

	return(FALSE);
}

int AtrGetDir(AtrDiskInfo *info, UWORD *fileCount, ADosFileEntry *files, 
              ULONG *freeBytes)
{
    switch (info->atr_dostype) {
        case DOS_ATARI:
        case DOS_ATARI1:
        case DOS_TOP:
        case DOS_BIBO:
            return(AtrDos2GetDir(info, fileCount, files, freeBytes));
        case DOS_ATARI3:
            return(AtrDos3GetDir(info, fileCount, files, freeBytes));
        case DOS_ATARI4:
            return(AtrDos4GetDir(info, fileCount, files, freeBytes));
        case DOS_ATARIXE:
            return(AtrXEGetDir(info, fileCount, files, freeBytes));
        case DOS_MYDOS:
            return(AtrMyDosGetDir(info, fileCount, files, freeBytes));
        case DOS_SPARTA2:
            return(AtrSpartaGetDir(info, fileCount, files, freeBytes));
        }
    return(ADOS_UNKNOWN_FORMAT);
}

int AtrChangeDir(AtrDiskInfo *info, int cdFlag, char *name)
{
    switch (info->atr_dostype) {
        case DOS_ATARI:
        case DOS_ATARI1:
        case DOS_TOP:
        case DOS_BIBO:
            return(ADOS_FUNC_NOT_SUPPORTED);
        case DOS_ATARIXE:
            return(AtrXEChangeDir(info, cdFlag, name));
        case DOS_MYDOS:
            return(AtrMyDosChangeDir(info, cdFlag, name));
        case DOS_SPARTA2:
            return(AtrSpartaChangeDir(info, cdFlag, name));
        }
    return(ADOS_UNKNOWN_FORMAT);
}

int AtrDeleteDir(AtrDiskInfo *info, char *name)
{
    switch (info->atr_dostype) {
        case DOS_ATARI:
        case DOS_ATARI1:
        case DOS_TOP:
        case DOS_BIBO:
            return(ADOS_FUNC_NOT_SUPPORTED);
        case DOS_ATARIXE:
            return(AtrXEDeleteDir(info, name));
        case DOS_MYDOS:
            return(AtrMyDosDeleteDir(info, name));
        case DOS_SPARTA2:
            return(AtrSpartaDeleteDir(info, name));
        }
    return(ADOS_UNKNOWN_FORMAT);
}

int AtrMakeDir(AtrDiskInfo *info, char *name)
{
    switch (info->atr_dostype) {
        case DOS_ATARI:
        case DOS_ATARI1:
        case DOS_TOP:
        case DOS_BIBO:
            return(ADOS_FUNC_NOT_SUPPORTED);
        case DOS_ATARIXE:
            return(AtrXEMakeDir(info,name));
        case DOS_MYDOS:
            return(AtrMyDosMakeDir(info,name));
        case DOS_SPARTA2:
            return(AtrSpartaMakeDir(info,name));
        }
    return(ADOS_UNKNOWN_FORMAT);
}

int AtrLockFile(AtrDiskInfo *info, char *name, int lock)
{
    switch (info->atr_dostype) {
        case DOS_ATARI:
        case DOS_ATARI1:
        case DOS_TOP:
        case DOS_BIBO:
            return(AtrDos2LockFile(info,name,lock));
        case DOS_ATARI3:
            return(AtrDos3LockFile(info,name,lock));
        case DOS_ATARI4:
            return(AtrDos4LockFile(info,name,lock));
        case DOS_ATARIXE:
            return(AtrXELockFile(info,name,lock));
        case DOS_MYDOS:
            return(AtrMyDosLockFile(info,name,lock));
        case DOS_SPARTA2:
            return(AtrSpartaLockFile(info,name,lock));
        }
    return(ADOS_UNKNOWN_FORMAT);
}

int AtrRenameFile(AtrDiskInfo *info, char *name, char *newname)
{
    switch (info->atr_dostype) {
        case DOS_ATARI:
        case DOS_ATARI1:
        case DOS_TOP:
        case DOS_BIBO:
            return(AtrDos2RenameFile(info, name,newname));
        case DOS_ATARI3:
            return(AtrDos3RenameFile(info, name,newname));
        case DOS_ATARI4:
            return(AtrDos4RenameFile(info, name,newname));
        case DOS_ATARIXE:
            return(AtrXERenameFile(info, name,newname));
        case DOS_MYDOS:
            return(AtrMyDosRenameFile(info, name,newname));
        case DOS_SPARTA2:
            return(AtrSpartaRenameFile(info, name,newname));
        }
    return(ADOS_UNKNOWN_FORMAT);
}

int AtrDeleteFile(AtrDiskInfo *info, char *name)
{
    switch (info->atr_dostype) {
        case DOS_ATARI:
        case DOS_ATARI1:
        case DOS_TOP:
        case DOS_BIBO:
            return(AtrDos2DeleteFile(info,name));
        case DOS_ATARI3:
            return(AtrDos3DeleteFile(info,name));
        case DOS_ATARI4:
            return(AtrDos4DeleteFile(info,name));
        case DOS_ATARIXE:
            return(AtrXEDeleteFile(info,name));
        case DOS_MYDOS:
            return(AtrMyDosDeleteFile(info,name));
        case DOS_SPARTA2:
            return(AtrSpartaDeleteFile(info,name));
        }
    return(ADOS_UNKNOWN_FORMAT);
}

int AtrImportFile(AtrDiskInfo *info, char *filename, int lfConvert)
{
    switch (info->atr_dostype) {
        case DOS_ATARI:
        case DOS_ATARI1:
        case DOS_TOP:
        case DOS_BIBO:
            return(AtrDos2ImportFile(info, filename, lfConvert));
        case DOS_ATARI3:
            return(AtrDos3ImportFile(info, filename, lfConvert));
        case DOS_ATARI4:
            return(AtrDos4ImportFile(info, filename, lfConvert));
        case DOS_ATARIXE:
            return(AtrXEImportFile(info, filename, lfConvert));
        case DOS_MYDOS:
            return(AtrMyDosImportFile(info, filename, lfConvert));
        case DOS_SPARTA2:
            return(AtrSpartaImportFile(info, filename, lfConvert));
        }
    return(ADOS_UNKNOWN_FORMAT);
}

int AtrExportFile(AtrDiskInfo *info, char *nameToExport, char* outFile, int lfConvert)
{
    switch (info->atr_dostype) {
        case DOS_ATARI:
        case DOS_ATARI1:
        case DOS_TOP:
        case DOS_BIBO:
            return(AtrDos2ExportFile(info, nameToExport,outFile, lfConvert));
        case DOS_ATARI3:
            return(AtrDos3ExportFile(info, nameToExport,outFile, lfConvert));
        case DOS_ATARI4:
            return(AtrDos4ExportFile(info, nameToExport,outFile, lfConvert));
        case DOS_ATARIXE:
            return(AtrXEExportFile(info, nameToExport,outFile, lfConvert));
        case DOS_MYDOS:
            return(AtrMyDosExportFile(info, nameToExport,outFile, lfConvert));
        case DOS_SPARTA2:
            return(AtrSpartaExportFile(info, nameToExport,outFile, lfConvert));
        }
    return(ADOS_UNKNOWN_FORMAT);
}

static void SizeOfSector(AtrDiskInfo *info, int sector, int *sz, ULONG * ofs)
{
	int size;
	ULONG offset;

	if (info->atr_sectorsize == 512) {
		size = 512;
		offset = sizeof(struct AFILE_ATR_Header) + (sector -1) * size;
	}
	else if (sector < 4) {
		/* special case for first three sectors in ATR and XFD image */
		size = 128;
		offset = sizeof(struct AFILE_ATR_Header) + 
            (sector - 1) * 
            (info->atr_boot_sectors_type == BOOT_SECTORS_PHYSICAL ? 256 : 128);
	}
	else {
		size = info->atr_sectorsize;
		offset = sizeof(struct AFILE_ATR_Header) +  
            (info->atr_boot_sectors_type == BOOT_SECTORS_LOGICAL ? 0x180 : 0x300) + 
            (sector - 4) * size;
	}

	if (sz)
		*sz = size;

	if (ofs)
		*ofs = offset;
}

static int AtrSeekSector(AtrDiskInfo *info, int sector)
{
	ULONG offset;
	int size;

	SizeOfSector(info, sector, &size, (ULONG*)&offset);
	fseek(info->atr_file, 0L, SEEK_END);
	if (offset > ftell(info->atr_file)) {
		}
	else
		fseek(info->atr_file, offset, SEEK_SET);

	return size;
}



