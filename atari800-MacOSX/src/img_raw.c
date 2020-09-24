//
//  img_raw.c
//  Atari800MacX
//
//  Created by markg on 9/23/20.
//

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "img_raw.h"

#define STD_HEADS   16
#define STD_SECTORS 63

typedef struct rawImage {
    FILE * File;
    char Path[FILENAME_MAX];
    uint32_t SectorCount;
    uint32_t SectorCountLimit;
    int ReadOnly;

    BlockDeviceGeometry Geometry;
} RAWImage;

static uint32_t HashString32(const char *s) {
    uint32_t len = (uint32_t)strlen(s);
    uint32_t hash = 2166136261U;

    for(uint32_t i=0; i<len; ++i) {
        hash *= 16777619;
        hash ^= (unsigned char)s[i];
    }

    return hash;
}

void *RAW_Image_Open(const char *path, int write, int solidState)
{
    RAWImage *img = (RAWImage *) malloc(sizeof(RAWImage));

    strcpy(img->Path, path);
    img->File = fopen(img->Path, write ? "rb+" : "rb");
    if (img->File == NULL)
        return -10; // TBD
    img->ReadOnly = !write;

    struct stat info;
    fstat(fileno(img->File), &info);

    uint64_t size = info.st_size;

    uint64_t sectors = size >> 9;
    img->SectorCount = sectors > 0xFFFFFFFFU ? 0xFFFFFFFFU : (uint32_t)sectors;

    img->Geometry.SolidState = solidState;

    /* use standard physical disk geometry */
    img->Geometry.Cylinders = img->SectorCount / (STD_HEADS * STD_SECTORS);

    if (img->Geometry.Cylinders > 16383)
        img->Geometry.Cylinders = 16383;
    else if (img->Geometry.Cylinders < 2) {
        fclose(img->File);
        return NULL;
    }

    img->Geometry.Heads = STD_HEADS;
    img->Geometry.SectorsPerTrack = STD_SECTORS;

    return img;
}

void *RAW_Init_New(const char *path, uint8_t heads, uint8_t spt, uint32_t totalSectorCount)
{
    RAWImage *img = (RAWImage *) malloc(sizeof(RAWImage));

    return img;
}

int RAW_Is_Read_Only(void *image)
{
    RAWImage *img = (RAWImage *)image;
    return img->ReadOnly;
}

void RAW_Flush(void *image)
{
}

BlockDeviceGeometry *RAW_Get_Geometry(void *image)
{
    RAWImage *img = (RAWImage *)image;
    return &img->Geometry;
}

uint32_t RAW_Get_Sector_Count(void *image)
{
    RAWImage *img = (RAWImage *)image;
    return &img->SectorCount;
}

uint32_t RAW_Get_Serial_Number(void *image)
{
    RAWImage *img = (RAWImage *) image;
    return HashString32(img->Path);
}

void RAW_Read_Sectors(void *image, void *data, uint32_t lba, uint32_t n)
{
    RAWImage *img = (RAWImage *) image;
    
    fseek(img->File, (int64_t)lba << 9, SEEK_SET);

    uint32_t requested = n << 9;
    uint32_t actual = fread(img->File, data, 1, requested);

    if (requested < actual)
        memset((char *)data + actual, 0, requested - actual);

}

void RAW_Write_Sectors(void *image, const void *data, uint32_t lba, uint32_t n)
{
    RAWImage *img = (RAWImage *) image;

    fseek(img->File, (int64_t)lba << 9, SEEK_SET);
    fwrite(img->File, data, 512, n);

    if (lba + n > img->SectorCount)
        img->SectorCount = lba + n;
}
