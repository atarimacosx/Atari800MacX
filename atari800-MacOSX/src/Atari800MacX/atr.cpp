
/*********************************************************************
   ATR / XFD File handling library
   (C) Copyright 1997-1998 Ken Siders    

   This file can be used in any freeware or public domain software
   as long as credit is given.


History

00.000  6/16/97    Initial version
00.001  6/24/97    Fixed AtariFileSize, Added EofAtariFile and
                   ExtractAtariFile functions
00.002  6/26/97    Added SortDirectory Function
00.003  7/14/97    Fixed Double density to treat first 3 sectors as  
                   Single Density.
00.004  7/16/97    Added CreateBootAtr
00.005  8/22/97    Added ExtractExeFromBootAtr
00.006  9/04/97    Fix signature check
00.007  1/23/98    Clean up Code.  Began adding ability to read DCM's.
00.008  1/24/98    More work on reading DCM's
00.009  1/28/98    Convert to C++
*********************************************************************

To do:

1 Clean up warnings + make more portable
2 Allow opening write-protected ATRs
3 Allow more than one reference to an ATR file to be opened so more 
   than one atari file can be opened at once. (Keep a count)
4 More specific error returns
5 Implement XFD handling
6 Create documentation
7 Optimize if necessary


*********************************************************************/
//atr.c  (C)1998-1998 Ken Siders
//

#include "stdafx.h"
#include "atr.h"
#include "utility.h"

int lastAtariError = 0;
static int verbose = 0;
static unsigned short dcmSectorMap[1041] = {0};
static unsigned short curSec = 0;
static unsigned char sectorBuffer[512];


/******************************************************************
SetVerbose - Sets verbose flag, returns last value of flag
******************************************************************/

int CAtrFile::SetVerbose( int verb )
   {
   int last = verbose;
   verbose = verb;
   return(last);
   }


/******************************************************************
 OpenAtr - opens ATR file specified for read/write (if writeable).
           Returns Atr Pointer if successful, 0 if not
******************************************************************/

int CAtrFile::Open( char *file )
   {
   unsigned char hdr[16];
   int bytes,stat;
   unsigned short signature;
   int i;

   stat = 0; i=i;

   maxNoDirectoryEntries = 64;
   firstDirSector = FIRST_ATARIDOS_DIR_SECTOR;
   dosType = DOS_UNKNOWN; /* assume atari dos disk */
   if ( !IsBootAtr(file) )
       dosType = DOS_KBOOT;

   firstFreeSector = 1;  // first "possible" free sector
   if ( atrIn )
       return 1;
   atrIn = fopen(file, "rb+");
   writeProtect = 0;
   if ( !atrIn )
       {
       atrIn = fopen(file, "rb");
       if ( !atrIn )
            return 2;
       else
           writeProtect = 2;
       }
   bytes = fread(&hdr, 1, 16, atrIn);
   if ( !bytes )
       return 3;
   
   signature = (unsigned short)(hdr[0] | hdr[1] << 8);
   imageSize = 16L * (hdr[2] | hdr[3] * 256L | hdr[6] * 65536L);
   secSize = (unsigned short)(hdr[4] | hdr[5] << 8); 
   crc = hdr[7] | hdr[8] * 256L | hdr[9] * 65536L | hdr[10] *256L * 65536L ;

   sectorCount = (unsigned short)(imageSize / secSize);
   flags = hdr[15];
   writeProtect |= (flags&1);
   authenticated = (flags >> 1) & 1;
   imageType = ATR_IMAGE;
   
   // read sector 360, if first byte is 2 it is a Mydos
   // enhanced density disk
   unsigned char buffer[256];
   if ( dosType != DOS_KBOOT )
       {
       if (!ReadSector(1, buffer) )
            {
            if ( buffer[7] == 0x80 )
                {
                if (buffer[32] == 0x20 )
                    dosType = DOS_SPARTA2;
                else if ( buffer[32] == 0x11 )
                    dosType = DOS_SPARTA1;
                if ( dosType == DOS_SPARTA1 ||
                     dosType == DOS_SPARTA2 )
                     firstDirSector =
                        (unsigned short) buffer[9] |
                        ((unsigned short) buffer[10]<<8);
                }
            totalSectors = buffer[11] | (buffer[12] << 8);
            freeSectors = buffer[13] | (buffer[14] << 8);
            if (!ReadSector(firstDirSector+1, buffer) )
                {
                maxNoDirectoryEntries = buffer[3] | (buffer[4] << 8);
                maxNoDirectoryEntries /= 23;
                }
            else
                {
                maxNoDirectoryEntries = 0;
                }
           }
       else
            {
            dosType = 0;
            }
       if ( dosType != DOS_SPARTA1 && dosType != DOS_SPARTA2 )
           {
           if (!ReadSector(360, buffer) )
                {
                if ( buffer[0] == 2 )
                    dosType = DOS_ATARI;
                else if ( buffer[0] && buffer[0] < 34 )
                    dosType = DOS_MYDOS;
                else
                    dosType = DOS_UNKNOWN; 

                // needs fixed!!!! doesn't work for 2.5 ED

                //vtocCount = buffer[0] - 1;
        
                //int vtocSize;
                if ( dosType == DOS_MYDOS )
                    {
                    if ( secSize == 128 )
                        vtocCount = ((unsigned short)buffer[0])*2-4;
                    else
                        vtocCount = ((unsigned short)buffer[0])-2;
                    }
                else
                    {
                    vtocCount = 1 + (sectorCount >= 1010);
                    }

                totalSectors = buffer[1] | (buffer[2] << 8);
                freeSectors = buffer[3] | (buffer[4] << 8);
                if ( dosType == DOS_ATARI || dosType == DOS_MYDOS )
                    stat = ReadVtoc();
                // Check for Dos 2.5 Enhanced Density
                if ( dosType == DOS_ATARI && sectorCount == 1040 )
                    {
                    if (!ReadSector(1024, buffer) )
                        freeSectors += buffer[122] | (buffer[123] << 8);
                    }
                }
           else
                {
                dosType = 0;
                }
           }
       } // if (dosType != DOS_KBOOT)
   if ( stat )
       return 3;

   if ( signature == 0x296 )
      return 0;
   else
       return 2;
   }


/******************************************************************
 CloseAtr - closes ATR Disk Image doesn't destroy the object, a
            new image can then be opened with Open member function
******************************************************************/

   int CAtrFile::Close( void )
    {
    int stat;
    if ( atrIn )
        {
        stat = fflush(atrIn);
        stat = fclose(atrIn);
        atrIn = 0;
        return stat;
        }
    else
      return 1;
    }


/******************************************************************
 ReadSector - Reads specified sector from the ATR file specified
              info buffer which must be big enough for the sector
              size of teh file.  Returns number of bytes read or
              0 if error.
******************************************************************/

   int CAtrFile::ReadSector( unsigned short sector, unsigned char *buffer)
   {
   unsigned long pos;
   size_t bytes;
   
//   printf("reading sector %u\n", sector);  getchar();
                                                            
   if ( !atrIn )                                               
      {
      return 1;
      }     
   
   if ( sector > sectorCount )
      {
      return 2;
      }
  
   switch ( imageType )
      {
      case ATR_IMAGE: 
         /* calculate offset into file */                                          
         //if ( secSize > 128 && sector > 3 )
         //   pos = (unsigned long)(sector-4) * secSize + 400L;
         //else
         //   pos = (unsigned long)(sector-1) * 128L + 16;
         pos =(sector-1)*secSize+16;

         /* position file pointer at that offset */
         if ( fseek(atrIn, pos, SEEK_SET) )
            return 3;
      
         /* read the data */
         memset(buffer,0,256);
         bytes = fread(buffer, 1, (sector<4)?128:secSize, atrIn);
         if ( bytes & 127 )
            return 4;
         return 0;
       }
   return 5;
   }
//    case DCM_IMAGE:
//       curSec = sector;
//       if ( fseek(atr.atrIn, dcmSectorMap[sector+1], SEEK_SET) )
//          {
//          lastAtariError = 13;
//          return 0;
//          }
//       if ( DecodeDcmBlock(atr) )
//          {
//          lastAtariError = 22;
//          return 0;
//          }
//       if ( sector > 3 )
//          bytes = atr->secSize;
//       else
//          bytes = 128;
//        memcpy(buffer, sectorBuffer, bytes );
//       return bytes;
//    }
// }

/******************************************************************
 WriteSector - Writes specified sector from the ATR file specified
               from buffer specified.  Returns number of bytes
               written or 0 if error.  Image must be writeable.
******************************************************************/

   int CAtrFile::WriteSector( unsigned short sector, unsigned char *buffer = NULL)
   {
   unsigned long pos;
   size_t bytes;
   int mallocFlag = FALSE;

   if ( !atrIn )
      {
      return 1;
      }

    if ( buffer == NULL )
        {
        buffer = (unsigned char *)calloc(256, sizeof(char) );
        mallocFlag = TRUE;
        }

   if ( sector > sectorCount )
      {
      return 2;
      }
/* calculate offset into file */
   //if ( secSize > 128 && sector > 3 )
   //   pos = (unsigned long)(sector-4) * secSize + 400L;
   //else
   //   pos = (unsigned long)(sector-1) * 128L + 16;
   pos =(sector-1)*secSize+16;


/* set file pointer to that position */
   if ( fseek(atrIn, pos, SEEK_SET) )
       {
       if ( mallocFlag )
           free(buffer);
       return 1;
       }
/* sector # to high? */
   if ( pos + secSize > imageSize + 16 )
       {
       if ( mallocFlag )
           free(buffer);
       return 2;
       }

/* write the data */
   bytes = fwrite(buffer, 1, (sector<4)?128:secSize, atrIn);
   if ( bytes & 127 )
       {
       if ( mallocFlag )
           free(buffer);
       return 3;
       }

   if ( mallocFlag )
       free(buffer);
   
   return 0;
}

/******************************************************************
 CreateAtr - Creates an ATR file with parameters specified.  Sector
             size must be a multiple of 128 bytes.  Return 0 for
             success
******************************************************************/

   int CAtrFile::Create( char *file, unsigned short sectors,
                            unsigned short sectorSize )
   {
   FILE *fp;
   unsigned char hdr[16] = {0};
   unsigned long imageSize;
   int bytes;

   if ( atrIn )
       return 10;

secSize = sectorSize;
sectorCount = sectors;


/* sector size must be a multiple of 128 */
   if ( sectorSize & 127 || sectorSize > 256)
      return 11;

/* determine the file size for the image */

//   if ( sectors > 3)
//      imageSize = (unsigned long)(sectors-3) * sectorSize + 384;
//   else
//      imageSize = 128 * sectors;
imageSize = sectorSize * sectors;

/* create the file */
   fp = fopen(file, "wb");
   if ( !fp )
      return 12;

/* set up the ATR header */
   hdr[1] = (unsigned char)0x02;
   hdr[0] = (unsigned char)0x96;
   hdr[2] = (unsigned char)( (imageSize >> 4) & 255 );
   hdr[3] = (unsigned char)( (imageSize >> 12) & 255 ); 
   hdr[6] = (unsigned char)( imageSize >> 20 );
   hdr[4] = (unsigned char)( sectorSize & 255 );
   hdr[5] = (unsigned char)( sectorSize >> 8 );
   bytes = fwrite(hdr, 1, 16, fp);
   if ( bytes != 16 )
      return 13;

/* seek to last position needed in file - 1 */
   if ( fseek(fp, imageSize - 1 , SEEK_SET) )
      return 14;
/* write one null byte */
   if ( fputc( 0, fp ) == EOF )
      return 15;
/* close the newly created image */
   if ( fclose(fp) )
      return 16;
/* reopen image normally */

   return Open(file);
   }
   
/******************************************************************
 GetAtrInfo - returns info for an open ATR image via pointers.
              non 0 returned is error. 
******************************************************************/

   CAtrFile::GetInfo( unsigned short *sectorSize,
                      unsigned short *secCount,
                      unsigned char  *prot)
   {
   if ( !atrIn )
      return 1;
/* duh */
   *sectorSize = secSize;
   *secCount = sectorCount;
   *prot = writeProtect;
   return 0;
   }


/******************************************************************
 GetImageType - returns ATR_IMAGE or DCM_IMAGE, or -1 for error.
******************************************************************/

   int CAtrFile::GetImageType( void )
   {
   if ( !atrIn )
      return -1;
   if ( imageType == ATR_IMAGE || imageType == DCM_IMAGE )
      return imageType;
   else
      return -1;
   }

/******************************************************************
 ReadDirectory
******************************************************************/

   int CAtrFile::ReadDirectory( int entry, char *name, char *ext,
                                int *status, unsigned short *sectors,
                                unsigned short *startSector)
   {
   int stat, offset,i,j;
   unsigned short sector;
   switch ( dosType )
       {
       case DOS_SPARTA1:
       case DOS_SPARTA2:
           unsigned short dirSectorList[256];
           unsigned short bitmapSector;
           int spartaStatus;
           // read sector 1
           if ( stat = ReadSector(1, sectorBuffer) )
               return stat;
           // get bitmap sector from offset 16/17
           bitmapSector = sectorBuffer[9] |
                         (sectorBuffer[10]<<8);
           if ( stat = ReadSector(bitmapSector, sectorBuffer) )
               return stat;
           for(j=0,i=4;i<secSize;i+=2)
               {
               dirSectorList[j++] = sectorBuffer[i] |
                                   (sectorBuffer[i+1]<<8);
               if ( !dirSectorList[j-1] )
                   break;
               }

           
           sector = ((entry+1) * 23)/secSize;
           offset = ((entry+1)*23) % secSize;

           if ( stat = ReadSector(dirSectorList[sector], sectorBuffer) )
               return stat;
           if ( dirSectorList[sector+1] )
               if ( stat = ReadSector(dirSectorList[sector+1], sectorBuffer+secSize) )
                   return stat;
   
           memset(name, 0, 9);
           memset(ext, 0, 4);
           memcpy(name, (char *)&sectorBuffer[offset+6], 8);
           memcpy(ext, (char *)&sectorBuffer[offset+14], 3);
           spartaStatus = sectorBuffer[offset];
           *sectors = (unsigned short)
                      (( (unsigned long)(sectorBuffer[offset+3]) |
                         (unsigned long)(sectorBuffer[offset+4]<<8) |
                         (unsigned long)(sectorBuffer[offset+5]<<16)
                       ) / secSize );
           *startSector = (unsigned short) (sectorBuffer[offset+1] |
                          (unsigned short)  (sectorBuffer[offset+2])<<8 );

           *status = 0;
           if ( spartaStatus & SPARTA_LOCKED_FLAG )
               *status |= LOCKED_FLAG;
           if ( spartaStatus & SPARTA_INUSE_FLAG )
               *status |= INUSE_FLAG;
           if ( spartaStatus & SPARTA_DELETED_FLAG )
               *status |= DELETED_FLAG;
           if ( spartaStatus & SPARTA_SUBDIR_FLAG )
               *status |= SUBDIR_FLAG;
           if ( !sectorBuffer[offset] )
               return 1234;
           return 0;

       case DOS_ATARI:
       case DOS_MYDOS:
           sector = firstDirSector + ( entry / dirEntriesPerSector);
           offset = ( entry % dirEntriesPerSector ) * dirEntrySize;
           if ( stat = ReadSector(sector, sectorBuffer) )
               return stat;
   
           memset(name, 0, 9);
           memset(ext, 0, 4);
           memcpy(name, (char *)&sectorBuffer[offset+5], 8);
           memcpy(ext, (char *)&sectorBuffer[offset+13], 3);
           *status = sectorBuffer[offset];
           *sectors = (unsigned short)
                          (sectorBuffer[offset+1] | (unsigned short)(sectorBuffer[offset+2])<<8 );
           *startSector = (unsigned short)
                          (sectorBuffer[offset+3] | (unsigned short)(sectorBuffer[offset+4])<<8 );

           return 0;
       }
   return 1; 
   }
     





/*-----------------------------------------------------------------*/
/*   ATARI 8-bit File IO routines                                  */
/*-----------------------------------------------------------------*/

/******************************************************************
 MakeFileName - Creates a filename.ext string from a zero padded
                raw fileName and extender.  Result is stored in
                string pointed to be result.  There is no return
                value.
******************************************************************/

void MakeFileName( char *result, char *fileName, char *extender )
   {
   int i;
   
   for(i=0; i<8; i++)
      {
      if (fileName[i] == ' ' || !fileName[i] )
         break;
      *(result++) = fileName[i];
      }      
   if (*extender && *extender != ' ')
       {
       *(result++) = '.';
       for(i=0; i<3; i++)
           { 
           if (extender[i] == ' ' || !extender[i] )
               break;
           *(result++) = extender[i];
           }
       }
   *(result++) = 0;
   }


/******************************************************************
 PatternMatch - Returns 1 if fileName+extender matches pattern in
                pattern. Wildcards are the standard '?' and '*'
                as supported by all Atari Dos's.  Returns 0 if
                it does not match.
******************************************************************/
int CAtariFile::PatternMatch( char *pattern, char *fileName, char *extender)
   {
   int i=0;
   char file[13];


   MakeFileName(file, fileName, extender);

   while (*pattern && file[i] )
      {
      if ( !file[i] && *pattern )
         return 0;
      if ( file[i] && !*pattern )
         return 0;

      if ( *pattern == '*')
         {
         while ( file[i] && file[i] != '.')
            i++;
         while ( *pattern && *pattern != '.')
            pattern++;
         if ( file[i] == '.' && *pattern == '.' )
            {
            pattern++;
            i++;
            continue;
            }
         continue;
         }

      if ( *pattern == '?' && file[i] != '.' )
         {
         i++;
         pattern++;
         continue;
         }

      if ( *pattern != file[i] )
         return 0;

      i++; 
      pattern++;
      }

   if ( !*pattern && !file[i] )
      return 1;
   else
      return 0;
   }


/******************************************************************
 AtariFindFirst - Finds first match for pattern and sets struct
                  with file information.  returns 0 for success,
                  -1 if not found, other for error.  This is 
                  similiar to _dosfindfist in the DOS world.
******************************************************************/

int CAtariFile::FindFirst( CAtrFile &atrC, unsigned attrib, char *fpattern)
   {
   unsigned char buffer[256];
   int i,j;
   unsigned short sectorSize, sectorCount;
   unsigned char prot;
   int stat;

   atr = &atrC;
   strcpy(pattern, fpattern);
/* Get some info about the ATR image and save */
   if ( stat = atr->GetInfo(  &sectorSize, &sectorCount, &prot) )
      return stat;

/* look for the file in the directory, if found initilize the fileInfo
   structure with data from the directory sector */

   for( i = atr->firstDirSector, fileNo = 0; i <= lastDirSector; i++ )
      {
      if ( stat = atr->ReadSector( (unsigned short) i, buffer) )
         return stat;
      for( j=0; j< dirEntriesPerSector; j++, fileNo++ )
         {
         flag = buffer[dirEntrySize * j ];
         locked = (flag & LOCKED_FLAG)?1:0;
         if ( flag & DELETED_FLAG )
            continue; 
         if ( (flag & INUSE_FLAG || flag == INUSE25_FLAG) && PatternMatch(pattern,
             (char *)&buffer[dirEntrySize * j + 5], 
             (char *)&buffer[dirEntrySize * j + 13]) )
            {
            dirSector = i;
            dirEntry = j;
            MakeFileName( fileName, (char *)&buffer[dirEntrySize * j + 5],
                (char *)&buffer[dirEntrySize * j + 13]);
            startSector = buffer[dirEntrySize *j+3] |
                          buffer[dirEntrySize *j+4] << 8;
            sectorCount = buffer[dirEntrySize *j+1] |
                          buffer[dirEntrySize *j+2] << 8;
            dosType = atr->GetDosType();
         //   if ( numberOfSectors > 720 )
         //       dosType = DOS_MYDOS;
            return( 0 ); /* success */
            }
         }
      }
   return -1;
   }

/******************************************************************
 AtariFindNext - Returns next matching file after previous 
                 AtariFindFirst or AtariFindNext call.  The fileinfo
                 structure passed should not be altered from the
                 previous call.  Also the ATR file name and pattern
                 from the initial AtariFindFirst call must still be
                 in scope. Similiar to _dosfindnext in DOS world.
******************************************************************/

int CAtariFile::FindNext( void )
   {
   unsigned char buffer[256];
   int i,j;
   int stat;
   unsigned short sectorSize, sectorCount;
   unsigned char prot;

   return 1;
   if ( stat = atr->GetInfo( &sectorSize, &sectorCount, &prot) )
      return stat;

   i = dirSector;
   j = dirEntry;

   j++;
   if ( j >= dirEntriesPerSector )
      {
      j=0;
      i++;
      }

   for( ; i <= lastDirSector; i++ , j = 0)
      {
      if ( stat = atr->ReadSector( (unsigned short) i, buffer) )
         return stat;
      for( ; j< dirEntriesPerSector; j++, fileNo++ )
         {
         flag = buffer[dirEntrySize * j];
         locked = ( flag & LOCKED_FLAG) ? 1 : 0;
         if ( flag & DELETED_FLAG )
            continue; 
         if ( (flag & INUSE_FLAG || flag == INUSE25_FLAG) && PatternMatch(pattern,
             (char *)&buffer[dirEntrySize * j + 5], 
             (char *)&buffer[dirEntrySize * j + 13]) )
            {
            dirSector = i;
            dirEntry = j;
            MakeFileName( fileName, (char *)&buffer[dirEntrySize * j + 5],
                (char *)&buffer[dirEntrySize * j + 13]);
            startSector = buffer[dirEntrySize *j+3] |
                          buffer[dirEntrySize *j+4] << 8;
            sectorCount = buffer[dirEntrySize *j+1] |
                          buffer[dirEntrySize *j+2] << 8;
            return( 0 );
            }
         }
      }

   return -1;
   }

/******************************************************************
 OpenAtariFile - Opens file in an ATR image in the mode specified
                 (ATARI_OPEN_READ, ATARI_OPEN_WRITE, or
                 ATARI_OPEN_DIR.  Returns pointer to atari file
                 structure or 0 on error. 
******************************************************************/

int CAtariFile::Open( CAtrFile &atrFile, char *fileName, 
                      unsigned char mode)
   {
   int stat;

   atr = &atrFile;

   dosType = atrFile.GetDosType();
   if ( stat = atrFile.GetInfo(&sectorSize, &numberOfSectors,
               &protFlag) )
       {
       return stat;
       }
/* bad open mode? */
   if ( mode != ATARI_OPEN_READ && mode != ATARI_OPEN_WRITE &&
        mode != ATARI_OPEN_DIR )
      {
       return 22;
      }

/* set file parameters */
   openFlag = mode;
   eofFlag = 0;

/* is ATR write protected? (APE extension?) */
//   if ( protFlag && (mode & ATARI_OPEN_WRITE) )
//      {
//      return 24;
//      }

if ( (mode & ATARI_OPEN_WRITE) && (mode & ATARI_OPEN_READ) )
    {
    return 25;
    }

if ( (mode & ATARI_OPEN_WRITE) && !(mode & ATARI_OPEN_READ) )
    {
    int i,j;
    unsigned char buf[256];
    sectorCount = 0;
    // if the file exists already, delete it.
    if ( !FindFirst(atrFile, 0, fileName ) )
        {
        Delete(atrFile, fileName);
        //atrFile.ReadVtoc();
        }
    /* set some file info data in the structure */
    currentSector = atrFile.GetFreeSector();
    openFlag = mode;
    currentOffset = 0;
    bytesData = 0;
    if (sectorSize == 128)
        sectorLinkOffset = 125;
    else if (sectorSize == 256 )
        sectorLinkOffset = 253;
    else
        return 27;
    // create a directory entry in the first deleted or empty
    // entry we find
    int working = TRUE;
    for(dirEntry = 0,dirSector=361;working&&dirSector<=368;dirSector++)
        {
        atrFile.ReadSector(dirSector,buf);
        for(j=0;j<128;dirEntry++,j+=16)
            {
            if (buf[j]&DELETED_FLAG || !buf[j] )
                {
                dirSector--; // it will get incremented when we exit loop
                working = FALSE;
                break;
                }
            }
        }
    if ( dirSector == 369 )
        return 169; // more than 64 files
    // create the filename
    char *fileNamePtr = fileName;
    memset(&buf[j+5],' ',8+3);
    for(i=0;i<8;i++)
        {
        if (!*fileName || *fileName == '.' || *fileName == ' ')
            break;
        buf[j+i+5] = *(fileName++);
        }
    if ( *fileName == '.' )
        {
        fileName++;
        for(i=8;i<11;i++)
            {
            if (!*fileName || *fileName == ' ')
                break;
            buf[j+i+5] = *(fileName++);
            }
        }

    buf[j] = DOS2_FLAG | INUSE_FLAG; // | OPEN_OUTPUTFLAG ;
    buf[j+1] = buf[j+2] = 0; // file size
    buf[j+3] = (unsigned char)(currentSector & 255);
    buf[j+4] = (unsigned char)(currentSector >> 8);
    
    // if it is atari dos ED disk, the flag is different

    //!!!! need to add !!!!

    // Update the directory
    atrFile.WriteSector(dirSector,buf);



    return 0;    
    }
else if ( !(mode & ATARI_OPEN_WRITE) && (mode & ATARI_OPEN_READ) )
    {

    /* read directory, find start sector and number of sectors and set
       in atFile.  Initialize current sector to start sector also */

    if ( FindFirst(atrFile, 0, fileName ) )
        return 170;

    /* is the file the ATR is lcoated in write protected? */   
    if ( locked && (mode & ATARI_OPEN_WRITE) )
        return NULL;

    /* set some file info data in the structure */
    currentSector = startSector;
    openFlag = mode;
    currentOffset = 0;
    bytesData = 0;
    if (sectorSize == 128)
        sectorLinkOffset = 125;
    else if (sectorSize == 256 )
        sectorLinkOffset = 253;
    else
        return 27;

    return 0;

   }
return 111;
}


/******************************************************************
 ReadAtariFile - reads bytes bytes from the open atari file specified
   in atFile and stores them in buffer.  buffer must be big enough.
   Returns bytes actually read. 
******************************************************************/

long CAtariFile::Read( char *buffer, long bytes )
{
   int stat;
   int lastSector = 0;
   long bytesRead = 0;

   if ( !bytes || eofFlag)
      return 0;
   if ( !(openFlag & ATARI_OPEN_READ) )
      return 0;
   if ( !currentOffset )
      {
      /* read sector */
      if ( stat = atr->ReadSector( currentSector, sectorBuffer) )
         {
         return 0;
         }
      if ( sectorSize == 128 )
         bytesData = sectorBuffer[sectorLinkOffset+2] & 127;
      else
         bytesData = sectorBuffer[sectorLinkOffset+2]; 
      }
   while( bytes )
      {
      while (bytesData && bytes )  // remove -- from bytesData
         {
         *(buffer++) = sectorBuffer[currentOffset++];
         bytes--;
         bytesRead++;
         bytesData--; //fix
         }

      if ( bytes )
         {
         /* read next sector */
         currentOffset = 0;
         if (dosType == DOS_MYDOS )
            currentSector =
                (sectorBuffer[sectorLinkOffset] << 8) |
                 sectorBuffer[sectorLinkOffset+1];
         else /* assume atari dos */
            currentSector =
                ((sectorBuffer[sectorLinkOffset] & 3) << 8) |
                 (sectorBuffer[sectorLinkOffset+1]);

         if (!currentSector )
            {
            eofFlag = 1;
            return bytesRead;
            }

         if (atr->ReadSector( currentSector, sectorBuffer) )
             return 0;
         if ( sectorSize == 128 )
            bytesData = sectorBuffer[sectorLinkOffset+2] & 127;
         else
            bytesData = sectorBuffer[sectorLinkOffset+2]; 
         }

      }
   if ( !bytesData)
       currentOffset = 0;
   return bytesRead;
   }


/******************************************************************
 CloseAtariFile - Closes Atari File 
******************************************************************/

int CAtariFile::Close( void )
   {
   unsigned char buf[256];
   int offset;
   if ( !openFlag )
       return 0;
   //if ( !currentOffset )
   //    offset = 1;
   //else
   //    {
   //    }
   if ( openFlag & ATARI_OPEN_WRITE )
       {
       // clear the sector link for the current sector
       if ( dosType == DOS_MYDOS )
           sectorBuffer[sectorLinkOffset] = sectorBuffer[sectorLinkOffset+1] = 0;
       else if ( dosType == DOS_ATARI )
           {
           sectorBuffer[sectorLinkOffset] = (unsigned char)(dirEntry << 2);
           sectorBuffer[sectorLinkOffset+1] = 0;
           }
       else
            sectorBuffer[sectorLinkOffset] = sectorBuffer[sectorLinkOffset+1] = 0;
       
       if (atr->WriteSector( currentSector, sectorBuffer) )
            return 3;

       if ( currentOffset )
            {
            atr->MarkSectorUsed(currentSector, TRUE);
            atr->AdjustFreeSectors(-1);
            sectorCount++;
            }
       
       
       if ( atr->ReadSector(dirSector, buf) )
           return 1;
       offset = (dirEntry&7)*16;
       // need to improve this later!!!
       if ( dosType == DOS_MYDOS )
           buf[offset] = 0x46;
       else
        buf[offset] &= (~OPENOUTPUT_FLAG);
       buf[offset+1] = (sectorCount)&255;
       buf[offset+2] = (sectorCount)>>8;
       if ( atr->WriteSector(dirSector, buf) )
           return 2;
       //atr.WriteVtoc(); // Update VTOC
       // update free sector count
       //if ( atr->ReadSector(360, buf) )
       //    return 4;
       //unsigned short freeSecs;
       //freeSecs = (unsigned short)(buf[3]) | (((unsigned short)buf[4])<<8);
       //freeSecs -= (sectorCount+1);
       //buf[3] = (unsigned char) (freeSecs&255);
       //buf[4] = (unsigned char) (freeSecs>>8);
       //if ( atr->WriteSector(360, buf) )
       //    return 5;

       if ( atr->WriteVtoc() )
            return 99;
       }
   openFlag = 0;
   return 0;
   }

/******************************************************************
 AtariFileSize - Returns size of atari file or -1 on error
******************************************************************/

 long CAtariFile::FileSize( CAtrFile &atr, char *fileName )
   {
   long count = 0;
   long bytes;
   int stat;
   static char buffer[512];
   
   /* open the atari file on the ATR image */
   stat = Open(atr, fileName, ATARI_OPEN_READ);
   if ( stat )
      return -1;  
   /* count how many bytes we can actually read */
   while( (bytes=Read(buffer, sizeof(buffer))) > 0 && count < 65535L*256)
      count+= bytes; 
   Close();
   return(count);
   }

/******************************************************************
 ExtractAtariFile - returns no. files extracted, -no for error.
                    file is stored with same name in dosPath
                    directory.  (don't add the trailing '\').
                    Wildcards are allowed for atari file. Use NULL
                    for dosPath to extract to current directory
******************************************************************/

int CAtariFile::Extract( CAtrFile &atrFile, char *fileName,
                         char *dosPath )
{
   char outName[256];
   int len;
   int count = 0;
   long bytes, bytesOut;
   static char buffer[512];
   FILE *output;
   
   if ( !FindFirst(atrFile, 0, fileName) )
      {
      do {
         if ( dosPath != NULL)
            {
            strcpy(outName, dosPath);
            len =strlen(outName);
            while ( len && outName[len-1] == '\\' )
                outName[ --len] = 0;
            outName[len] = '\\';
            }
         else
            len = -1;
         strcpy(&outName[len+1], fileName);

         output = fopen( outName, "wb");
         if (output == NULL )
            return -count-1;
         if ( Open( atrFile, fileName, ATARI_OPEN_READ) )
             return -count-1;
         while( (bytes=Read(buffer, sizeof(buffer))) > 0 )
            {
            bytesOut = fwrite(buffer, 1, (int)bytes, output);
            if ( bytes != bytesOut )
               {
               fclose( output );
               Close();
               return -count-1;
               }
            }
         fclose( output );
         Close();
         count ++;
         } while ( !FindNext() );
      }
   else
      {
      return 0;
      }
   return(count);
}

CAtariFile::CAtariFile()
{
atrFileName = 0; // not used??
bytesData = 0;
currentOffset = 0;
currentSector = 1;
eofFlag = 0;
memset(fileName,0,sizeof(fileName));
fileSize = 0;
dosType = 0;
openFlag = 0;

}

int CAtrFile::GetDosType(void)
{
return dosType;
}

unsigned short CAtrFile::GetFreeSectors(void)
{
    return freeSectors;
}

int CAtrFile::ReadVtoc(void)
{
int sector;
int i,j=0, stat;
unsigned char secBuf[256];
memset(vtocMap,0,sizeof(vtocMap));
// check for DOS 2.5 Enhanced Density, if so VTOC is located at
// sector 1024 in addition to sector 360.
if ( dosType == DOS_ATARI && sectorCount >= 1010 )
    {
    stat = ReadSector( 1024, secBuf);
    if ( stat )
        return stat;
    for( j=6,i=0; i<122; i++ )
        vtocMap[j++] = secBuf[i];
    stat = ReadSector( 360, secBuf);
    if ( stat )
        return stat;
    for( j=0,i=10; i<100; i++ )
        vtocMap[j++] = secBuf[i];
    }
else if ( dosType == DOS_ATARI || dosType== DOS_MYDOS )
    {
    for(sector=360;sector>360-min(vtocCount,33);sector--)
        {
        stat = ReadSector( sector, secBuf);
        if ( stat )
            return stat;
        if ( sector == 360 )
            {
            freeSectors = secBuf[3] | ((unsigned short)(secBuf[4])<<8);
            }
        for( i=((sector==360)?10:0); i<secSize; i++ )
            vtocMap[j++] = secBuf[i];
        }
    }
else
    {
    freeSectors = 0;
    }
ResetFirstFreeSector();
return 0;
}

// need to add Dos 2.5 enhanced density support!!!!!!!
int CAtrFile::WriteVtoc(void)
{
int sector;
int i,j=0, stat;
unsigned char secBuf[256];

// check for DOS 2.5 Enhanced Density, if so VTOC is located at
// sector 1024 in addition to sector 360.
if ( dosType == DOS_ATARI && sectorCount >= 1010 )
    {
    stat = ReadSector( 1024, secBuf);
    if ( stat )
        return stat;
    for( j=6,i=0; i<122; i++ )
        secBuf[i] = vtocMap[j++] ;
    stat = WriteSector( 1024, secBuf);
    if ( stat )
        return stat;
    stat = ReadSector( 360, secBuf);
    if ( stat )
        return stat;
    for( j=0,i=10; i<100; i++ )
        secBuf[i] = vtocMap[j++];
    stat = WriteSector( 360, secBuf);
    if ( stat )
        return stat;
    }
else if ( dosType == DOS_MYDOS || dosType == DOS_ATARI)
    {
    for(sector=360;sector>360-min(vtocCount,33);sector--)
        {
        stat = ReadSector( sector, secBuf);
        if ( stat )
            return stat;
        for( i=((sector==360)?10:0); i<secSize; i++ )
            secBuf[i] = vtocMap[j++];
        if ( sector == 360 )
            {
            secBuf[3] = (unsigned char)(freeSectors&255);
            secBuf[4] = (unsigned char)(freeSectors>>8);
            }
        stat = WriteSector( sector, secBuf);
        if ( stat )
            return stat;
        }
    }
return 0;

}

int CAtrFile::IsSectorUsed( unsigned short sector)
{
int entry, mask;
unsigned char maskTable[] = {128, 64, 32, 16, 8, 4, 2, 1};
entry = sector / 8;
mask = maskTable[ sector & 7];
return ( !(vtocMap[entry] & mask) );
}

// Returns number of sectors marked as used (unavailable)
unsigned short CAtrFile::GetUsedSectorCount( void )
{
unsigned short i;
unsigned short count = 0;
unsigned short secCount;

if ( dosType == DOS_ATARI && sectorCount == 1040 )
    secCount = 1023;
else
    secCount = sectorCount;

for(i=1;i<=secCount;i++)
    if ( IsSectorUsed(i) )
        count++;
return count;
}

unsigned short CAtrFile::GetVtocSectorCount(void)
{
  return vtocCount;
}

int CAtrFile::MarkSectorUsed( unsigned short sector, int used)
{
int entry, mask;
unsigned char maskTable[] = {128, 64, 32, 16, 8, 4, 2, 1};
entry = sector / 8;
mask = maskTable[ sector & 7];
if ( used != FALSE )
    vtocMap[entry] &= (~mask);
else
    vtocMap[entry] |= mask;

return ( 0 );
}


// returns total number of files deleted

unsigned short CAtariFile::Delete( CAtrFile &atrFile, char *fileName)
{
   int count = 0;
   int count1 = 0, count2 = 0;
   static unsigned char buffer[256];
   unsigned short currentSector;

   count1 = count2 = 0;
   int stat;

   if ( stat = atrFile.GetInfo(&sectorSize, &numberOfSectors,
               &protFlag) )
       {
       return -count-1;
       }

   if (sectorSize == 128)
      sectorLinkOffset = 125;
   else if (sectorSize == 256 )
      sectorLinkOffset = 253;
   else
      return 27;


   if ( !FindFirst(atrFile, 0, fileName) )
      {
      do {
         // read the directory sector, make it as deleted
         atrFile.ReadSector(dirSector, buffer);
         buffer[dirEntry*dirEntrySize] |= DELETED_FLAG;
         atrFile.WriteSector(dirSector, buffer);

         // trace through the file and free up the sectors in the
         // vtoc
         currentSector = startSector;
         while (currentSector)
            {
            atrFile.MarkSectorUsed( currentSector, FALSE );
            if ( currentSector <= 720 )
                count1++;
            else
                count2++;

            atrFile.ReadSector(currentSector, sectorBuffer);
            if (dosType == DOS_MYDOS )
                currentSector =
                    (sectorBuffer[sectorLinkOffset] << 8) |
                    sectorBuffer[sectorLinkOffset+1];
            else /* assume atari dos */
                currentSector =
                    ((sectorBuffer[sectorLinkOffset] & 3) << 8) |
                    (sectorBuffer[sectorLinkOffset+1]);
             }

         count ++;
         } while ( !FindNext() );
      if ( atrFile.WriteVtoc() )
          return 99;
      // now update the free sector count
      if ( dosType == DOS_ATARI && sectorCount == 1040 )
          {
          // we have to update sector 360 and 1024 for Dos 2.5 ED
          int freeSecs;
          // update sector 360
          atrFile.ReadSector(360, buffer);
          freeSecs = buffer[3] | (buffer[4]<<8);
          freeSecs += count1;
          buffer[3] = (unsigned char)(freeSecs&255);
          buffer[4] = (unsigned char)(freeSecs>>8);
          atrFile.WriteSector(360, buffer);
          // update sector 1024
          atrFile.ReadSector(1024, buffer);
          freeSecs = buffer[122] | (buffer[123]<<8);
          freeSecs += count2;
          buffer[122] = (unsigned char)(freeSecs&255);
          buffer[123] = (unsigned char)(freeSecs>>8);
          atrFile.WriteSector(1024, buffer);

          }
      else
          {
          //int freeSecs;
          //atrFile.ReadSector(360, buffer);
          //freeSecs = buffer[3] | (buffer[4]<<8);
          //freeSecs += (count1 + count2);
          //buffer[3] = (unsigned char)(freeSecs&255);
          //buffer[4] = (unsigned char)(freeSecs>>8);
          //atrFile.WriteSector(360, buffer);
          atrFile.AdjustFreeSectors(count1+count2);
          }
      atrFile.WriteVtoc();
      atrFile.ResetFirstFreeSector();
      }
   else
      {
      return 0;
      }
   return(count);

}

unsigned short CAtrFile::GetFreeSector(void)
{
unsigned short i;
for (i=firstFreeSector;i<= sectorCount;i++)
    {
    if (!IsSectorUsed(i) )
        {
        firstFreeSector = i;
        return i;
        }
    }
return  0;
}

void CAtrFile::ResetFirstFreeSector(void)
{
firstFreeSector = 1;
}

long CAtariFile::Write( char *buffer, long bytes )
{
   int lastSector = 0;
   long bytesWrote = 0;
   static unsigned short nextSector;

   if ( !bytes)
      return 0;
   if ( !(openFlag & ATARI_OPEN_WRITE) )
      return 0;

   if ( currentOffset >= sectorLinkOffset )
        {
        // mark the sector as used
        atr->MarkSectorUsed(currentSector, TRUE);
        atr->AdjustFreeSectors(-1);
        // increment counter
        sectorCount++;
        // init for next sector
        currentOffset = 0;
        currentSector = nextSector;
        }
   
   while( bytes )
      {
      while ( currentOffset < sectorLinkOffset && bytes )  
         {
         sectorBuffer[currentOffset++] = *(buffer++);
         bytes--;
         bytesWrote++;
         bytesData++; 
         }

      //if ( currentOffset < sectorLinkOffset && !bytes)
      //    return bytesWrote;

      if ( currentOffset >= sectorLinkOffset )
            {
            // mark the sector as used
            atr->MarkSectorUsed(currentSector, TRUE);
      //      atr->AdjustFreeSectors(-1);
            }

      // read next sector 
      nextSector = atr->GetFreeSector();
      if ( !nextSector )
          {
          openFlag = 0;
          // remove directory entry
          if ( atr->ReadVtoc() )
              return 145;
          if (atr->ReadSector( dirSector, sectorBuffer) )
              return 144;
          int offset = (dirEntry&7)*16;
          sectorBuffer[offset] = DELETED_FLAG;
          if (atr->WriteSector( dirSector, sectorBuffer) )
              return 144;
          return 169;
          }
      if (dosType == DOS_MYDOS )
          {
         sectorBuffer[sectorLinkOffset+1] =
              (unsigned char)(nextSector & 255);
         sectorBuffer[sectorLinkOffset] =
              (unsigned char)(nextSector >> 8);
         sectorBuffer[sectorLinkOffset+2] =
              (unsigned char)(currentOffset);
          }
      else /* assume atari dos */
          {
          sectorBuffer[sectorLinkOffset+1] =
              (unsigned char)(nextSector & 255);
          sectorBuffer[sectorLinkOffset] =
              (unsigned char)(nextSector >> 8);
          sectorBuffer[sectorLinkOffset] |=
              (unsigned char)(dirEntry << 2);
          sectorBuffer[sectorLinkOffset+2] =
              (unsigned char)(currentOffset);
          }

      //write the sector
      //test atr->DecrementFreeSectors(1,1);
      // end test
      if (atr->WriteSector( currentSector, sectorBuffer) )
            return 0;
      if ( currentOffset >= sectorLinkOffset && bytes)
            {
            // mark the sector as used
            atr->MarkSectorUsed(currentSector, TRUE);
            atr->AdjustFreeSectors(-1);
            // increment counter
            sectorCount++;
            // init for next sector
            currentOffset = 0;
            currentSector = nextSector;
            }

       }
   return bytesWrote;
}

int CAtariFile::Insert( CAtrFile &atrFile, char *fileName, char *atariFileName,
                        int mode)
{
   const int UNKNOWN_MODE = 0, CRLF_MODE = 1, CR_MODE = 2, LF_MODE = 3;
   int currentMode = UNKNOWN_MODE;
   int i, j, count = 0;
   long bytes, outBytes, bytesOut;
   static unsigned char buffer[512];
   FILE *input;
   
   input = fopen( fileName, "rb");
   if (input == NULL )
      return 0;

   if ( Open( atrFile, atariFileName, ATARI_OPEN_WRITE) )
       {
       fclose(input);
       return 0;
       }
   
   while ( 1 )
       {
       bytes = fread(buffer, 1, sizeof(buffer), input);
       if ( !bytes )
           break;
       if ( mode == BINARY_MODE )
           outBytes = bytes;
       else
           {
           for(outBytes=j=i=0;i<bytes;i++)
               {
               if ( buffer[i] == 9 )
                   {
                   buffer[j++] = (unsigned char) 127;
                   outBytes++;
                   continue;
                   }
               else if (buffer[i] != (unsigned char) 10 &&
                        buffer[i] != (unsigned char) 13 )
                   {
                   buffer[j++] = buffer[i];
                   outBytes++;
                   continue;
                   }

               switch (currentMode)
                   {
                   case UNKNOWN_MODE:
                       if ( buffer[i] == 10 )
                           {
                           buffer[j++] = (unsigned char)155;
                           currentMode = LF_MODE;
                           outBytes++;
                           }
                       else if ( buffer[i] == 13 )
                           {
                           if ( i < bytes-1 && buffer[i+1] == 10 )
                               {
                               buffer[j++] = (unsigned char)155;
                               currentMode = CRLF_MODE;
                               outBytes++;
                               i++;
                               }
                           else
                               {
                               buffer[j++] = (unsigned char)155;
                               currentMode = CR_MODE;
                               outBytes++;
                               }
                           }
                       else
                           {
                           buffer[j++] = (unsigned char)buffer[i];
                           outBytes++;
                           }
                       break;
                   case CR_MODE:
                       if ( buffer[i] == 13 )
                           {
                           buffer[j++] = (unsigned char)155;
                           outBytes++;
                           }
                       else
                           {
                           buffer[j++] = buffer[i];
                           outBytes++;
                           }
                       break;
                   case LF_MODE:
                       if ( buffer[i] == 10 )
                           {
                           buffer[j++] = (unsigned char)155;
                           outBytes++;
                           }
                       else
                           {
                           buffer[j++] = buffer[i];
                           outBytes++;
                           }
                       break;
                   case CRLF_MODE:
                       if ( buffer[i] == 13 )
                           {
                           buffer[j++] = (unsigned char)155;
                           outBytes++;
                           }
                       else if ( buffer[i] != 10 )
                           {
                           buffer[j++] = buffer[i];
                           outBytes++;
                           }
                       break;
                   } // switch
               } // for
           } // else
       bytesOut=Write((char *)buffer, outBytes) ;
       if ( outBytes != bytesOut )
            {
            fclose(input);
            //Close(); // close the output file
            //Delete(atrFile, fileName); // delete the partial file
            return 0;
            }
       }
   if ( !feof(input) )
       {
       fclose(input);
       //atrFile.Close();
       return 0;
       }
   Close(); // close Atari file
   fclose(input);
//   atrFile.Close();
   return 1;
}

// setDosType should be DOS_ATARI or DOS_MYDOS
// flag should be non zero to zero out the image before formatting
// There should be at least 361 sectors on the image.

int CAtrFile::Format( int setDosType,  int flag)
{
unsigned short availSecs;
int i, sec, stat;
unsigned long vtocBytes;
unsigned char buffer[256] = {0};
dosType = setDosType;
if ( flag )
    for(sec=1;sec<=sectorCount;sec++)
        WriteSector( sec, buffer);

if ( sectorCount < 361 )
    return 123;

vtocBytes = ((sectorCount+1+7)/8)+10 ;
if ( secSize == 128 )
    {
    vtocCount = (unsigned short)((vtocBytes+127)/128);
    // if more than 1 vtoc sector, vtoc count must be even
    if ( (vtocCount > 1) && (vtocCount&1) )
        vtocCount++;
    buffer[0] = (vtocCount+4)/2;
    }
else
    {
    if ( vtocBytes <= 128 )
        vtocCount = 1 ;
    else
        {
        vtocBytes -= 128;
        vtocCount = (unsigned short)((vtocBytes+255)/256+1);
        }
    buffer[0] = vtocCount+2;
    }
if ( dosType == DOS_ATARI )
    {
    if ( sectorCount >= 1010 )
        vtocCount = 2;
    else
        vtocCount = 1;
    buffer[0] = 2; //atari dos flag
    }

availSecs = sectorCount - 3 - vtocCount - 8;

if ( dosType == DOS_ATARI && sectorCount >= 1010 )
    availSecs -= 17;

buffer[1] = buffer[3] = (unsigned char)(availSecs&255);
buffer[2] = buffer[4] = (unsigned char)(availSecs>>8);
if ( dosType )
    {
    stat = WriteSector(360,buffer);
    if ( stat )
        return stat;
    }
stat = ReadVtoc();
if ( stat )
    return stat;

freeSectors = availSecs;

for(sec=4;sec<=359;sec++)
    MarkSectorUsed(sec, FALSE);

if ( dosType == DOS_ATARI && sectorCount >= 1010 )
    for(sec=369;sec<=min(sectorCount,1023);sec++)
        MarkSectorUsed(sec, FALSE);
else
    for(sec=369;sec<=sectorCount;sec++)
        MarkSectorUsed(sec, FALSE);

for(i=0,sec=360;i<vtocCount;i++)
    MarkSectorUsed(sec--,TRUE);

stat = WriteVtoc();
if ( stat )
    return stat;

freeSectors = availSecs;


return 0;
}

CAtariFile::~CAtariFile()
{
   Close();
}



int CAtariFile::Rename( CAtrFile &atrFile, char *oldName, char *newName)
{
   static unsigned char buffer[256];
   int i;

   if ( !FindFirst(atrFile, 0, newName) )
       return 2; // new file already exists
   if ( !FindFirst(atrFile, 0, oldName) )
      {
      // read the directory sector, make it as deleted
      atrFile.ReadSector(dirSector, buffer);
      // change the name
      memset( &buffer[dirEntry*dirEntrySize+5], ' ', 8+3);
      for(i=0;i<8;i++)
          {
          if (*newName == '.' || *newName == ' ' || *newName == 0 )
              break;
          buffer[dirEntry*dirEntrySize+5+i] = *(newName++);
          }
      if (*newName == '.' )
          newName++;
      for(i=0;i<3;i++)
          {
          if ( *newName == ' ' || *newName == 0 )
              break;
          buffer[dirEntry*dirEntrySize+13+i] = *(newName++);
          }
      // update the directory sector
      atrFile.WriteSector(dirSector, buffer);
      }
   else
       {
       return 1;
       }
   return 0;
}
