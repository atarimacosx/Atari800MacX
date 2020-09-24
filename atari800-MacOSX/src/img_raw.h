//
//  img_raw.h
//  Atari800MacX
//
//  Created by markg on 9/23/20.
//

#ifndef img_raw_h
#define img_raw_h

#include <stdint.h>
#include <stdio.h>
#include "img_disk.h"

void *RAW_Image_Open(const char *path, int write, int solidState);
void *RAW_Init_New(const char *path, uint8_t heads, uint8_t spt, uint32_t totalSectorCount);
int RAW_Is_Read_Only(void *img);
void RAW_Flush(void *img);
BlockDeviceGeometry *RAW_Get_Geometry(void *img);
uint32_t RAW_Get_Sector_Count(void *img);
uint32_t RAW_Get_Serial_Number(void *img);
void RAW_Read_Sectors(void *img, void *data, uint32_t lba, uint32_t n);
void RAW_Write_Sectors(void *img, const void *data, uint32_t lba, uint32_t n);

#endif /* img_raw_h */
