//
//  img_raw.h
//  Atari800MacX
//
//  Created by markg on 9/23/20.
//

#ifndef img_disk_h
#define img_disk_h

#include <stdint.h>
#include <stdio.h>

enum {
    DiskTypeRaw = 1,
    DiskTypeVHD = 2
};

typedef struct blockDeviceGeometry {
    uint32_t SectorsPerTrack;
    uint32_t Heads;
    uint32_t Cylinders;
    int SolidState;
} BlockDeviceGeometry;

typedef struct imgImage {
    int  imgType;
    void *image;
} IMGImage;

void *IMG_Image_Open(const char *path, int write, int solidState);
int IMG_Is_Read_Only(void *img);
void IMG_Flush(void *img);
BlockDeviceGeometry *IMG_Get_Geometry(void *img);
uint32_t IMG_Get_Sector_Count(void *img);
uint32_t IMG_Get_Serial_Number(void *img);
void IMG_Read_Sectors(void *img, void *data, uint32_t lba, uint32_t n);
void IMG_Write_Sectors(void *img, const void *data, uint32_t lba, uint32_t n);
void IMG_Image_Close(void *img);

#endif /* img_disk_h */
