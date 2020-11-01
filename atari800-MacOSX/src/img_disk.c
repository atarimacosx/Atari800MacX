//
//  img_disk.c
//  Atari800MacX
//
//  Created by markg on 9/23/20.
//

#include <stdlib.h>
#include <string.h>
#include "img_disk.h"
#include "img_raw.h"
#include "img_vhd.h"

int IMG_last_op_read;
int IMG_last_op_time;
int IMG_last_sector;

void *IMG_Image_Open(const char *path, int write, int solidState)
{
    void *img;
    char *ext;
    int is_vhd;
    IMGImage *image = malloc(sizeof(IMGImage));
    
    ext = strrchr(path, '.');
    if (!ext) {
        is_vhd = 0;
    } else {
        is_vhd = (strcmp(ext+1, "vhd") == 0) ||
                 (strcmp(ext+1, "VHD") == 0);
    }
    
    if (is_vhd) {
        img = VHD_Image_Open(path, write, solidState);
        if (img == NULL) {
            free(image);
            return(NULL);
        } else {
            image->imgType = DiskTypeVHD;
            image->image = img;
            return(image);
        }
    } else {
        img = RAW_Image_Open(path, write, solidState);
        if (img == NULL) {
            free(image);
            return(NULL);
        } else {
            image->imgType = DiskTypeRaw;
            image->image = img;
            return(image);
        }
    }
}

int IMG_Is_Read_Only(void *img)
{
    IMGImage *image = (IMGImage *) img;
    
    if (image->imgType == DiskTypeRaw)
        return RAW_Is_Read_Only(image->image);
    else
        return VHD_Is_Read_Only(image->image);
}

void IMG_Flush(void *img)
{
    IMGImage *image = (IMGImage *) img;
    
    if (image->imgType == DiskTypeRaw)
        RAW_Flush(image->image);
    else
        VHD_Flush(image->image);
}

BlockDeviceGeometry *IMG_Get_Geometry(void *img)
{
    IMGImage *image = (IMGImage *) img;
    
    if (image->imgType == DiskTypeRaw)
        return RAW_Get_Geometry(image->image);
    else
        return VHD_Get_Geometry(image->image);
}

uint32_t IMG_Get_Sector_Count(void *img)
{
    IMGImage *image = (IMGImage *) img;
    
    if (image->imgType == DiskTypeRaw)
        return RAW_Get_Sector_Count(image->image);
    else
        return VHD_Get_Sector_Count(image->image);
}

uint32_t IMG_Get_Serial_Number(void *img)
{
    IMGImage *image = (IMGImage *) img;
    
    if (image->imgType == DiskTypeRaw)
        return RAW_Get_Serial_Number(image->image);
    else
        return VHD_Get_Serial_Number(image->image);
}

void IMG_Read_Sectors(void *img, void *data, uint32_t lba, uint32_t n)
{
    IMGImage *image = (IMGImage *) img;
    
    IMG_last_op_read = 1;
    IMG_last_op_time = n;
    IMG_last_sector = lba;

    if (image->imgType == DiskTypeRaw)
        RAW_Read_Sectors(image->image, data, lba, n);
    else
        VHD_Read_Sectors(image->image, data, lba, n);
}

void IMG_Write_Sectors(void *img, const void *data, uint32_t lba, uint32_t n)
{
    IMGImage *image = (IMGImage *) img;
    
    IMG_last_op_read = 0;
    IMG_last_op_time = n;
    IMG_last_sector = lba;

    if (image->imgType == DiskTypeRaw)
        RAW_Write_Sectors(image->image, data, lba, n);
    else
        VHD_Write_Sectors(image->image, data, lba, n);
}

void IMG_Image_Close(void *img)
{
    IMGImage *image = (IMGImage *) img;
    
    if (image->imgType == DiskTypeRaw)
        RAW_Image_Close(image->image);
    else
        VHD_Image_Close(image->image);

}
