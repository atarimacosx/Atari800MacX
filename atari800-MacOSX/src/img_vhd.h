//
//  img_vhd.h
//  Atari800MacX
//
//  Created by markg on 9/21/20.
//

#ifndef img_vhd_h
#define img_vhd_h

#include <stdint.h>
#include <stdio.h>
#include "img_disk.h"

void *VHD_Image_Open(const char *path, int write, int solidState);
void *VHD_Init_New(const char *path, uint8_t heads, uint8_t spt, uint32_t totalSectorCount, int dynamic);
int VHD_Is_Read_Only(void *img);
void VHD_Flush(void *img);
BlockDeviceGeometry *VHD_Get_Geometry(void *img);
uint32_t VHD_Get_Sector_Count(void *img);
uint32_t VHD_Get_Serial_Number(void *img);
void VHD_Read_Sectors(void *img, void *data, uint32_t lba, uint32_t n);
void VHD_Write_Sectors(void *img, const void *data, uint32_t lba, uint32_t n);
void VHD_Image_Close(void *img);

#endif /* img_vhd_h */
