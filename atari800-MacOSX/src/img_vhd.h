//
//  img_vhd.h
//  Atari800MacX
//
//  Created by markg on 9/21/20.
//

#ifndef img_vhd_h
#define img_vhd_h

#include <stdio.h>

typedef struct blockDeviceGeometry {
    uint32_t SectorsPerTrack;
    uint32_t Heads;
    uint32_t Cylinders;
    int SolidState;
} BlockDeviceGeometry;


#endif /* img_vhd_h */
