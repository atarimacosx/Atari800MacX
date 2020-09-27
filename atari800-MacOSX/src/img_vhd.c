//    Altirra - Atari 800/800XL/5200 emulator
//    Copyright (C) 2009-2012 Avery Lee
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <CoreFoundation/CFUUID.h>
#include "img_vhd.h"

#define bswap_16(value) \
((((value) & 0xff) << 8) | ((value) >> 8))

#define bswap_32(value) \
(((uint32_t)bswap_16((uint16_t)((value) & 0xffff)) << 16) | \
(uint32_t)bswap_16((uint16_t)((value) >> 16)))

#define bswap_64(value) \
(((uint64_t)bswap_32((uint32_t)((value) & 0xffffffff)) \
<< 32) | \
(uint64_t)bswap_32((uint32_t)((value) >> 32)))

#define MAKEFOURCC(byte1, byte2, byte3, byte4) (((uint8_t)byte1) + (((uint8_t)byte2) << 8) + (((uint8_t)byte3) << 16) + (((uint8_t)byte4) << 24))

typedef struct vhdFooter {
    enum {
        DiskTypeFixed = 2,
        DiskTypeDynamic = 3
    };

    uint8_t    Cookie[8];
    uint32_t   Features;
    uint32_t   Version;
    uint64_t   DataOffset;
    uint32_t   Timestamp;
    uint32_t   CreatorApplication;
    uint32_t   CreatorVersion;
    uint32_t   CreatorHostOS;
    uint64_t   OriginalSize;
    uint64_t   CurrentSize;
    uint32_t   DiskGeometry;
    uint32_t   DiskType;
    uint32_t   Checksum;
    uint8_t    UniqueId[16];
    uint8_t    SavedState;
    uint8_t    Reserved[427];
} VHDFooter;

typedef struct vhdDynamicDiskHeader {
    uint8_t    Cookie[8];
    uint64_t   DataOffset;
    uint64_t   TableOffset;
    uint32_t   HeaderVersion;
    uint32_t   MaxTableEntries;
    uint32_t   BlockSize;
    uint32_t   Checksum;
    uint8_t    ParentUniqueId[16];
    uint32_t   ParentTimestamp;
    uint32_t   Reserved;
    uint16_t   ParentUnicodeName[256];
    uint8_t    ParentLocatorEntry[8][24];
    uint8_t    Reserved2[256];
} VHDDynamicDiskHeader;

typedef struct vhdImage {
    FILE      *File;
    char      Path[FILENAME_MAX];
    int       ReadOnly;
    int       SolidState;
    int64_t   FooterLocation;
    uint32_t  SectorCount;
    int       BlockSizeShift;
    uint32_t  BlockLBAMask;
    uint32_t  BlockSize;
    uint32_t  BlockBitmapSize;

    uint32_t  Cylinders;
    uint32_t  Heads;
    uint32_t  SectorsPerTrack;
    
    uint32_t *BlockAllocTable;

    uint32_t  CurrentBlock;
    int64_t   CurrentBlockDataOffset;
    int       CurrentBlockBitmapDirty;
    int       CurrentBlockAllocated;
    uint8_t  *CurrentBlockBitmap;

    VHDFooter Footer;
    VHDDynamicDiskHeader DynamicHeader;
} VHDImage;

static void AllocateBlock(VHDImage *img);
static void CalcCHS(uint32_t totalSectors, VHDImage *img);
static void FlushCurrentBlockBitmap(VHDImage *img);
static void InitCommon(VHDImage *img);
static void ReadDynamicDiskSectors(VHDImage *img, void *data, uint32_t lba, uint32_t n);
static void SetCurrentBlock(VHDImage *img, uint32_t blockIndex);
static void WriteDynamicDiskSectors(VHDImage *img, const void *data, uint32_t lba, uint32_t n);

static uint32_t sumbytes(const uint8_t *src, uint32_t n) {
    uint32_t sum = 0;
    for(uint32_t i=0; i<n; ++i)
        sum += src[i];

    return sum;
}

static uint32_t ComputeFooterChecksum(const VHDFooter *footer) {
    const uint8_t *p = (const uint8_t *)footer;

    // The VHD checksum is a one's complement of the sum of all bytes except
    // for the checksum. Note that this is invariant to endianness.
    return ~(sumbytes(p, 64) + sumbytes(p + 68, 512 - 68));
}

static uint32_t ComputeHeaderChecksum(const VHDDynamicDiskHeader *header) {
    const uint8_t *p = (const uint8_t *)header;

    // The VHD checksum is a one's complement of the sum of all bytes except
    // for the checksum. Note that this is invariant to endianness.
    return ~(sumbytes(p, 36) + sumbytes(p + 40, 1024 - 40));
}

static uint32_t HashString32(const char *s) {
    uint32_t len = (uint32_t)strlen(s);
    uint32_t hash = 2166136261U;

    for(uint32_t i=0; i<len; ++i) {
        hash *= 16777619;
        hash ^= (unsigned char)s[i];
    }

    return hash;
}

static uint32_t ClampToUint32(uint64_t v) {
    return v >= 0x100000000UL ? 0xFFFFFFFFUL : (uint32_t)v;
}

static void WriteUnalignedBEU32(void *p, uint32_t v) { *(uint32_t *)p = bswap_32(v); }

static int FindLowestSetBit(uint32_t v) {
    for(int i=0; i<32; ++i) {
        if (v & 1)
            return i;
        v >>= 1;
    }

    return 32;
}

const uint8_t VHDFooterSignature[8] = { 'c', 'o', 'n', 'e', 'c', 't', 'i', 'x' };
const uint8_t VHDDynamicHeaderSignature[8] = { 'c', 'x', 's', 'p', 'a', 'r', 's', 'e' };

static void SwapEndianFooter(VHDFooter *footer) {
    footer->Features = bswap_32(footer->Features);
    footer->Version = bswap_32(footer->Version);
    footer->DataOffset = bswap_64(footer->DataOffset);
    footer->CreatorApplication = bswap_32(footer->CreatorApplication);
    footer->CreatorHostOS = bswap_32(footer->CreatorHostOS);
    footer->CurrentSize = bswap_64(footer->CurrentSize);
    footer->DiskType = bswap_32(footer->DiskType);
    footer->Checksum = bswap_32(footer->Checksum);
}

static void SwapEndianHeader(VHDDynamicDiskHeader *header) {
    header->DataOffset = bswap_64(header->DataOffset);
    header->TableOffset = bswap_64(header->TableOffset);
    header->HeaderVersion = bswap_32(header->HeaderVersion);
    header->MaxTableEntries = bswap_32(header->MaxTableEntries);
    header->BlockSize = bswap_32(header->BlockSize);
    header->Checksum = bswap_32(header->Checksum);
}

static void SwapEndianUint32Array(uint32_t *p, uint32_t n) {
    while(n--) {
        bswap_32(*p);
        p++;
    }
}

uint32_t VHD_Get_Sector_Count(void *image)  {
    VHDImage *img = (VHDImage *) image;
    return img->SectorCount;
}

int VHD_Is_Read_Only(void *image) {
    VHDImage *img = (VHDImage *) image;
    return img->ReadOnly;
}

BlockDeviceGeometry *VHD_Get_Geometry(void *image) {
    VHDImage *img = (VHDImage *) image;
    BlockDeviceGeometry *geo = malloc(sizeof(BlockDeviceGeometry));

    geo->Cylinders = img->Cylinders;
    geo->Heads = img->Heads;
    geo->SectorsPerTrack = img->SectorsPerTrack;
    geo->SolidState = img->SolidState;

    return geo;
}

uint32_t VHD_Get_Serial_Number(void *image) {
    VHDImage *img = (VHDImage *) image;
    return HashString32(img->Path);
}

static VHDImage *VHDImageAlloc() {
    VHDImage *img = malloc(sizeof(VHDImage));
    memset(img, 0, sizeof(VHDImage));
    
    img->ReadOnly = FALSE;
    img->FooterLocation = 0;
    img->SectorCount = 0;
    img->BlockSizeShift = 0;
    img->BlockLBAMask = 0;
    img->BlockSize = 0;
    img->BlockBitmapSize = 0;
    img->CurrentBlock = 0;
    img->CurrentBlockDataOffset = 0;
    img->CurrentBlockBitmapDirty = FALSE;
    img->CurrentBlockAllocated = FALSE;
    return img;
}

void *VHD_Image_Open(const char *path, int write, int solidState) {
    VHDImage *img = VHDImageAlloc();
    
    strcpy(img->Path, path);
    img->File = fopen(img->Path, write ? "rb+" : "rb");
    if (img->File == NULL)
        return NULL; // TBD
    img->ReadOnly = !write;
    img->SolidState = solidState;

    struct stat info;
    fstat(fileno(img->File), &info);

    uint64_t size = info.st_size;

    if (size < 512)
        return NULL; // TBD Return invalid image error

    img->FooterLocation = size - 512;
    fseek(img->File, img->FooterLocation, SEEK_SET);

    uint8_t footerbuf[512];
    fread(footerbuf, 512, 1, img->File);

    // We need to handle either 511 or 512 byte headers here, per the spec.
    if (!memcmp(footerbuf + 1, VHDFooterSignature, 8)) {
        // 511 byte header -- make it a 512 byte header.
        memmove(footerbuf, footerbuf + 1, 511);
        footerbuf[511] = 0;
        img->FooterLocation++;
    } else if (memcmp(footerbuf, VHDFooterSignature, 8))
        return NULL; // TBD

    memcpy(&img->Footer, footerbuf, 512);

    // swizzle to little endian
    SwapEndianFooter(&img->Footer);

    // validate the footer
    if ((img->Footer.Version & 0xffff0000) != 0x00010000)
        return NULL; // TBD

    if (!(img->Footer.Features & 0x2)) {
        // reserved bit in features mask was not set
        return NULL; // TBD
    }

    // checksum the footer (~ of sum of all bytes except checksum field at 64-67); note
    // that we do this on the *unswizzled* buffer
    if (img->Footer.Checksum != ComputeFooterChecksum(&img->Footer))
        return NULL; // TBD

    // check for a supported disk type
    if (img->Footer.DiskType != DiskTypeFixed && img->Footer.DiskType != DiskTypeDynamic)
        return NULL; // TBD

    // read off drive size
    img->SectorCount = ClampToUint32(img->Footer.CurrentSize >> 9);
    CalcCHS(img->SectorCount, img);

    // if we've got a dynamic disk, read the dyndisk header
    if (img->Footer.DiskType == DiskTypeDynamic) {
        if (img->Footer.DataOffset >= size || size - img->Footer.DataOffset < sizeof(VHDDynamicDiskHeader))
            return NULL; //TBD

        uint8_t rawdynheader[1024];

        fseek(img->File, img->Footer.DataOffset, SEEK_SET);
        fread(rawdynheader, sizeof rawdynheader, 1, img->File);

        // validate signature
        if (memcmp(rawdynheader, VHDDynamicHeaderSignature, 8))
            return NULL; // TBD

        // swizzle to local endian
        memcpy(&img->DynamicHeader, rawdynheader, sizeof(img->DynamicHeader));
        SwapEndianHeader(&img->DynamicHeader);

        // validate the version
        if ((img->DynamicHeader.HeaderVersion & 0xffff0000) != 0x00010000)
            return NULL; // TBD

        // checksum the header
        if (img->DynamicHeader.Checksum != ComputeHeaderChecksum(&img->DynamicHeader))
            return NULL; // TBD

        // block size must always be a power of two
        if (img->DynamicHeader.BlockSize < 512 || (img->DynamicHeader.BlockSize & (img->DynamicHeader.BlockSize - 1)))
            return NULL; // TBD

        img->BlockSize = img->DynamicHeader.BlockSize;
        img->BlockSizeShift = FindLowestSetBit(img->BlockSize);
        img->BlockLBAMask = img->BlockSizeShift < 41 ? (1 << (img->BlockSizeShift - 9)) - 1 : 0xFFFFFFFFU;

        // compute additional size with bitmap -- the bitmap is always padded
        // to a 512 byte sector (4096 bits, or equivalent to 2MB)
        img->BlockBitmapSize = (((img->BlockSize - 1) >> 21) + 1) * 512;
        img->CurrentBlockBitmap = malloc(img->BlockBitmapSize);

        // validate the size of the block allocation table
        uint64_t blockCount64 = ((img->Footer.OriginalSize - 1) >> img->BlockSizeShift) + 1;

        if (blockCount64 != img->DynamicHeader.MaxTableEntries)
            return NULL; // TBD

        uint32_t blockCount = (uint32_t)blockCount64;

        // validate the location of the BAT
        uint32_t batSize = sizeof(uint64_t) * blockCount;
        if (img->DynamicHeader.TableOffset >= size &&
            size - img->DynamicHeader.TableOffset < batSize)
            return NULL; // TBD

        // read in the BAT
        img->BlockAllocTable = malloc(blockCount * sizeof(uint32_t));

        fseek(img->File, img->DynamicHeader.TableOffset, SEEK_SET);
        fread(img->BlockAllocTable, blockCount, sizeof(uint32_t), img->File);

        // swizzle the BAT
        SwapEndianUint32Array(img->BlockAllocTable, blockCount);

        // validate the BAT
        for(uint32_t i=0; i<blockCount; ++i) {
            uint32_t sectorOffset = img->BlockAllocTable[i];

            // check if the block is allocated
            if (sectorOffset != 0xFFFFFFFF) {
                uint64_t byteOffset = (uint64_t)sectorOffset << 9;

                // the block cannot be beyond the end of the file or overlap the footer copy
                // at the beginning
                if (byteOffset >= size || size - byteOffset < img->BlockSize + img->BlockBitmapSize ||
                    byteOffset < sizeof(VHDFooter))
                {
                    return NULL; // TBD
                }
            }
        }
    }

    InitCommon(img);
    return(img);
}

void *VHD_Init_New(const char *path, uint8_t heads, uint8_t spt, uint32_t totalSectorCount, int dynamic) {
    VHDImage *img = VHDImageAlloc();

    img->SectorCount = totalSectorCount;
    img->File = fopen(path, "wb");
    if (img->File == NULL)
        return NULL; // TBD    img->ReadOnly = FALSE;
    img->SolidState = FALSE;

    // set up footer
    const uint32_t vhdEpoch = 946684800;    // January 1, 2000 midnight UTC in seconds

    memset(&img->Footer, 0, sizeof (img->Footer));

    uint32_t cylinders = totalSectorCount / ((uint32_t)heads * (uint32_t)spt);

    if (cylinders > 0xffff)
        cylinders = 0xffff;
    
    img->Cylinders = cylinders;
    img->Heads = heads;
    img->SectorsPerTrack = spt;
    
    memcpy(img->Footer.Cookie, VHDFooterSignature, sizeof (img->Footer.Cookie));
    img->Footer.Features = 0x2;
    img->Footer.Version = 0x00010000;
    img->Footer.DataOffset = dynamic ? sizeof(img->Footer) : 0xFFFFFFFFFFFFFFFFull;
    time_t currTime;
    time(&currTime);
    img->Footer.Timestamp = (uint32_t)(currTime - vhdEpoch);
    img->Footer.CreatorApplication = MAKEFOURCC('A', 'B', 'M', 'X');
    img->Footer.CreatorVersion = 0x00020000;
    img->Footer.CreatorHostOS = 0x4D616320 ;    // Mac
    img->Footer.OriginalSize = (uint64_t)totalSectorCount << 9;
    img->Footer.CurrentSize = img->Footer.OriginalSize;
    img->Footer.DiskGeometry = (cylinders << 16) + ((uint32_t)heads << 8) + spt;
    img->Footer.DiskType = dynamic ? DiskTypeDynamic : DiskTypeFixed;
    CFUUIDRef guid = CFUUIDCreate(NULL);
    CFUUIDBytes bytes = CFUUIDGetUUIDBytes(guid);
    CFRelease(guid);
    memcpy(img->Footer.UniqueId, &bytes, 16);
    img->Footer.SavedState = 0;

    // compute checksum -- fortunately this is endian agnostic
    img->Footer.Checksum = ComputeFooterChecksum(&img->Footer);

    // copy to raw buffer and swizzle
    VHDFooter rawFooter = img->Footer;
    SwapEndianFooter(&rawFooter);

    // check if we are creating a dynamic disk
    if (dynamic) {
        // write out the footer copy
        fwrite(&rawFooter, 1, sizeof(rawFooter), img->File);

        // initialize dynamic parameters and the BAT
        img->BlockSize = 0x200000;        // 2MB
        img->BlockSizeShift = 21;
        img->BlockLBAMask = 0xFFF;
        img->BlockBitmapSize = 0x200;

        const uint32_t blockCount = (totalSectorCount + 4095) >> 12;
        const uint32_t headerSize = sizeof(img->Footer) + sizeof(img->DynamicHeader);

        // set up the dynamic header
        memset(&img->DynamicHeader, 0, sizeof(img->DynamicHeader));
        memcpy(img->DynamicHeader.Cookie, VHDDynamicHeaderSignature, sizeof( img->DynamicHeader.Cookie));
        img->DynamicHeader.DataOffset = 0xFFFFFFFFFFFFFFFFull;
        img->DynamicHeader.TableOffset = headerSize;
        img->DynamicHeader.HeaderVersion = 0x00010000;
        img->DynamicHeader.MaxTableEntries = blockCount;
        img->DynamicHeader.BlockSize = img->BlockSize;

        // compute checksum
        img->DynamicHeader.Checksum = ComputeHeaderChecksum(&img->DynamicHeader);

        // write out the dynamic header
        VHDDynamicDiskHeader rawDynamicHeader;
        memcpy(&rawDynamicHeader, &img->DynamicHeader, sizeof(rawDynamicHeader));
        SwapEndianHeader(&rawDynamicHeader);
        fwrite(&rawDynamicHeader, sizeof(rawDynamicHeader), 1, img->File);

        // write out the BAT
        uint32_t *batBuf= malloc(16384*sizeof(uint32_t));
        memset(batBuf, 0xFF, 16384*sizeof(uint32_t));
        fwrite(batBuf, sizeof(uint32_t), 16384, img->File);
        free(batBuf);

        // init runtime buffers
        
        img->BlockAllocTable = malloc(blockCount * sizeof(uint32_t));
        memset(img->BlockAllocTable, 0xFF, blockCount*sizeof(uint32_t));
        img->CurrentBlockBitmap = malloc(img->BlockBitmapSize);
        memset(img->CurrentBlockBitmap, 0, img->BlockBitmapSize);
    } else {
        // write blank data
        uint8_t *clearData = malloc(262144);

        memset(clearData, 0, 262144);

        uint32_t sectorsToClear = img->SectorCount;
        uint32_t sectorsCleared = 0;

        while(sectorsToClear) {
            uint32_t tc = sectorsToClear < 512 ? sectorsToClear : 512;

            fwrite(clearData, tc, 512, img->File);

            sectorsCleared += tc;
            sectorsToClear -= tc;
        }
    }

    // write out the footer
    fwrite(&rawFooter, sizeof(rawFooter), 1, img->File);
    img->FooterLocation = ftell(img->File);

    InitCommon(img);
    return(img);
}

static void InitCommon(VHDImage *img) {
    img->CurrentBlock = 0xFFFFFFFFU;
    img->CurrentBlockDataOffset = 0;
    img->CurrentBlockBitmapDirty = false;
    img->CurrentBlockAllocated = false;
}

void VHD_Flush(void *image) {
    VHDImage *img = (VHDImage *) image;
    FlushCurrentBlockBitmap(img);
}

void VHD_Read_Sectors(void *image, void *data, uint32_t lba, uint32_t n) {
    VHDImage *img = (VHDImage *) image;
    if (img->Footer.DiskType == DiskTypeDynamic) {
        ReadDynamicDiskSectors(img, data, lba, n);
    } else {
        fseek(img->File, (int64_t)lba << 9, SEEK_SET);

        uint32_t requested = n << 9;
        uint32_t actual = fread(data, 1, requested, img->File);

        if (requested < actual)
            memset((char *)data + actual, 0, requested - actual);
    }
}

void VHD_Write_Sectors(void *image, const void *data, uint32_t lba, uint32_t n) {
    VHDImage *img = (VHDImage *) image;
    if (img->Footer.DiskType == DiskTypeDynamic) {
        WriteDynamicDiskSectors(img, data, lba, n);
    } else {
        fseek(img->File, (int64_t)lba << 9, SEEK_SET);
        fwrite(data, 1, 512 * n, img->File);
    }
}

static void ReadDynamicDiskSectors(VHDImage *img, void *data, uint32_t lba, uint32_t n) {
    // preclear memory
    memset(data, 0, 512*n);

    // compute starting block
    uint32_t blockIndex = lba >> (img->BlockSizeShift - 9);

    while(n) {
        // compute count we can handle in this block
        uint32_t count = (~lba & img->BlockLBAMask) + 1;
        uint32_t blockCount = n < count ? n : count;

        // read in block bitmap if necessary
        SetCurrentBlock(img, blockIndex);

        // read in valid sectors
        uint32_t blockSectorOffset = lba & img->BlockLBAMask;

        for(uint32_t i=0; i<blockCount; ++i) {
            if (img->CurrentBlockBitmap[blockSectorOffset >> 3] & (0x80 >> (blockSectorOffset & 7))) {
                fseek(img->File, img->CurrentBlockDataOffset + ((int64_t)(blockSectorOffset + i) << 9), SEEK_SET);
                fread((char *)data + i*512, 1, 512, img->File);
            }
        }

        // next block
        lba += blockCount;
        data = (char *)data + blockCount * 512;
        n -= blockCount;
        ++blockIndex;
    }
}

static void WriteDynamicDiskSectors(VHDImage *img, const void *data, uint32_t lba, uint32_t n) {
    // compute starting block
    uint32_t blockIndex = lba >> (img->BlockSizeShift - 9);

    while(n) {
        // compute count we can handle in this block
        uint32_t count = (~lba & img->BlockLBAMask) + 1;
        uint32_t blockCount = n < count ? n : count;
        
        // read in block bitmap if necessary
        SetCurrentBlock(img, blockIndex);

        // write sectors
        uint32_t blockSectorOffset = lba & img->BlockLBAMask;

        for(uint32_t i=0; i<blockCount; ++i) {
            uint8_t sectorMaskByte = img->CurrentBlockBitmap[blockSectorOffset >> 3];
            const uint8_t sectorBit = (0x80 >> (blockSectorOffset & 7));

            // check if we're writing zeroes to this sector
            const uint8_t *secsrc = (const uint8_t *)data + i*512;
            int writingZero = true;

            for(uint32_t j=0; j<512; ++j) {
                if (secsrc[j]) {
                    writingZero = false;
                    break;
                }
            }

            // Check if this sector is currently allocated and allocate
            // or deallocate it as necessary.
            //
            // allocated   writing-0    action
            // -----------------------------------------------------
            //    no            no            allocate and write sector
            //    no            yes            do nothing
            //    yes            no            write sector
            //    yes            yes            deallocate and write sector

            int wasZero = !(sectorMaskByte & sectorBit);

            if (wasZero != writingZero) {
                // if this block is not allocated, we must be trying to allocate a sector in it,
                // and must extend the file to allocate the block now
                if (!writingZero && img->CurrentBlockAllocated)
                    AllocateBlock(img);

                sectorMaskByte ^= sectorBit;
                img->CurrentBlockBitmapDirty = TRUE;
            }

            // write out new data if the sector is or was allocated (deallocated
            // sectors must still be zero).
            if (!writingZero || !wasZero) {
                fseek(img->File, img->CurrentBlockDataOffset + ((int64_t)(blockSectorOffset + i) << 9), SEEK_SET);
                fwrite(secsrc, 1, 512, img->File);
            }
        }

        // next block
        lba += blockCount;
        data = (char *)data + blockCount * 512;
        n -= blockCount;
        ++blockIndex;
    }
}

static void SetCurrentBlock(VHDImage *img, uint32_t blockIndex) {
    if (img->CurrentBlock == blockIndex)
        return;

    // if we have dirtied the current block bitmap, write it out
    if (img->CurrentBlockBitmapDirty)
        FlushCurrentBlockBitmap(img);

    // stomp the current block index in case we get an I/O error
    img->CurrentBlock = 0xFFFFFFFFU;
    img->CurrentBlockDataOffset = 0;

    // check if the sector is allocated
    uint32_t sectorOffset = img->BlockAllocTable[blockIndex];
    if (sectorOffset == 0xFFFFFFFFU) {
        // no -- set the block bitmap to all unallocated (0)
        memset(img->CurrentBlockBitmap, 0, img->BlockBitmapSize);
        img->CurrentBlockAllocated = false;
    } else {
        // yes it is -- read in the bitmap from the new block
        fseek(img->File, (int64_t)sectorOffset << 9, SEEK_SET);
        fread(img->CurrentBlockBitmap, 1, img->BlockBitmapSize, img->File);
        img->CurrentBlockAllocated = true;
        img->CurrentBlockDataOffset = ((int64_t)sectorOffset << 9) + img->BlockBitmapSize;
    }

    // all done
    img->CurrentBlock = blockIndex;
}

static void FlushCurrentBlockBitmap(VHDImage *img) {
    if (img->CurrentBlockBitmapDirty) {
        uint32_t sectorOffset = img->BlockAllocTable[img->CurrentBlock];
        fseek(img->File, (uint64_t)sectorOffset << 9, SEEK_SET);
        fwrite(img->CurrentBlockBitmap, 1, img->BlockBitmapSize, img->File);
       img->CurrentBlockBitmapDirty = false;
    }
}

static void AllocateBlock(VHDImage *img) {
    // fast out if the block is already allocated
    if (img->CurrentBlockAllocated)
        return;

    // Compute where we're going to place the new block, based on the footer
    // that we're going to overwrite. Note that we strategically place the block
    // so that the block data is on a 4K boundary.
    const int64_t newBlockDataLoc = (img->FooterLocation + img->BlockBitmapSize + 4095) & ~(int64_t)4095;
    const int64_t newBlockBitmapLoc = newBlockDataLoc - img->BlockBitmapSize;

    // Compute the new footer location.
    int64_t newFooterLocation = newBlockDataLoc + img->BlockSize;

    // Extend the file and rewrite the footer. There is nothing we need to change
    // here so this is pretty easy. We do, however, need to swizzle it back. The
    // checksum should still be valid.
    VHDFooter *rawFooter = malloc(sizeof(img->Footer));
    memcpy(rawFooter, &img->Footer, sizeof(img->Footer));
    SwapEndianFooter(rawFooter);

    fseek(img->File, newFooterLocation, SEEK_SET);
    fwrite(&rawFooter, sizeof(rawFooter), 1, img->File);

    // Flush to disk so we know the VHD is valid again.
    fflush(img->File);
    img->FooterLocation = newFooterLocation;

    // Write the new block bitmap.
    fseek(img->File, newBlockBitmapLoc, SEEK_SET);
    fwrite(img->CurrentBlockBitmap, 1, img->BlockBitmapSize, img->File);

    // Zero the data; technically not needed with NTFS since the bitmap is always at least
    // as big as the footer and NTFS zeroes new space, but we might be running on FAT32
    uint8_t *zerobuf = malloc(65536);
    memset(zerobuf, 0, 65536);
    fwrite(zerobuf, 1, 65536, img->File);

    // flush again, so the block is OK on disk
    fflush(img->File);

    // now update the BAT; we don't need to flush this as if it doesn't go through
    // the file is still valid
    fseek(img->File, img->DynamicHeader.TableOffset + 4*img->CurrentBlock, SEEK_SET);

    uint32_t sectorOffset = (uint32_t)(newBlockBitmapLoc >> 9);
    img->BlockAllocTable[img->CurrentBlock] = sectorOffset;

    uint8_t rawSectorOffset[4];
    WriteUnalignedBEU32(&rawSectorOffset, sectorOffset);

    fwrite(rawSectorOffset, 1, 4, img->File);

    // all done!
    img->CurrentBlockDataOffset = newBlockDataLoc;
    img->CurrentBlockAllocated = TRUE;
}

static void CalcCHS(uint32_t totalSectors, VHDImage *img) {
    uint64_t cylinderTimesHeads;
    
    if (totalSectors > 65535 * 16 * 255) {
       totalSectors = 65535 * 16 * 255;
    }
    
    if (totalSectors >= 65535 * 16 * 63) {
       img->SectorsPerTrack = 255;
       img->Heads = 16;
       cylinderTimesHeads = totalSectors / img->SectorsPerTrack;
    } else {
       img->SectorsPerTrack = 17;
       cylinderTimesHeads = totalSectors / img->SectorsPerTrack;
    
       img->Heads = (cylinderTimesHeads + 1023) / 1024;
          
       if (img->Heads < 4) {
          img->Heads = 4;
       }
       if (cylinderTimesHeads >= (img->Heads * 1024) || img->Heads > 16) {
          img->SectorsPerTrack = 31;
          img->Heads = 16;
          cylinderTimesHeads = totalSectors / img->SectorsPerTrack;
       }
       if (cylinderTimesHeads >= (img->Heads * 1024)) {
          img->SectorsPerTrack = 63;
          img->Heads = 16;
          cylinderTimesHeads = totalSectors / img->SectorsPerTrack;
       }
    }
    img->Cylinders = cylinderTimesHeads / img->Heads;
}

void VHD_Image_Close(void *image)
{
    VHDImage *img = (VHDImage *) image;

    fclose(img->File);
    if (img->BlockAllocTable)
        free(img->BlockAllocTable);
    if (img->CurrentBlockBitmap)
        free(img->CurrentBlockBitmap);
    free(img);
}
