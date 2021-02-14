//  Altirra - Atari 800/800XL/5200 emulator
//  Copyright (C) 2008-2011 Avery Lee
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "flash.h"
#include "atari.h"
#include <stdlib.h>
#include <string.h>

#undef FLASH_DEBUG
#ifndef FLASH_DEBUG
#define printf(fmt, ...) (0)
#endif

void SectorErase(FlashEmu *flash, ULONG address);

FlashEmu *Flash_Init(void *mem, int type) {
    FlashEmu *flash = (FlashEmu *) malloc(sizeof(FlashEmu));
    
    flash->Memory = (uint8_t *)mem;
    flash->FlashType = type;

    flash->Dirty = FALSE;
    flash->WriteActivity = FALSE;
    flash->AtmelSDP = (type == Flash_TypeAT29C010A || type == Flash_TypeAT29C040);

    // check if this flash chip checks 11 or 15 address bits during the unlock sequence
    flash->A11Unlock = FALSE;
    flash->A12iUnlock = FALSE;

    switch(flash->FlashType) {
        case Flash_TypeA29040:
        case Flash_TypeAm29F010B:
        case Flash_TypeAm29F040B:
        case Flash_TypeAm29F016D:
        case Flash_TypeAm29F032B:
        case Flash_TypeM29F010B:
        case Flash_TypeHY29F040A:
            flash->A11Unlock = TRUE;
            break;

        case Flash_TypeS29GL01P:
        case Flash_TypeS29GL512P:
        case Flash_TypeS29GL256P:
        case Flash_TypeM29W800DT:
        case Flash_TypeMX29LV640DT:
            flash->A12iUnlock = TRUE;
            break;
    }

    switch(flash->FlashType) {
        case Flash_TypeA29040:
            flash->SectorEraseTimeoutCycles = 1;
            break;

        case Flash_TypeAm29F010:
        case Flash_TypeAm29F010B:
        case Flash_TypeM29F010B:
        case Flash_TypeHY29F040A:
        case Flash_TypeM29W800DT:
        case Flash_TypeMX29LV640DT:
            flash->SectorEraseTimeoutCycles = 1;
            break;

        case Flash_TypeAm29F040:
        case Flash_TypeAm29F040B:
            flash->SectorEraseTimeoutCycles = 1;
            break;

        case Flash_TypeBM29F040:
            flash->SectorEraseTimeoutCycles = 1;
            break;

        default:
            flash->SectorEraseTimeoutCycles = 0;
            break;
    }
    Flash_Cold_Reset(flash);
    
    return(flash);
}

void Flash_Shutdown(FlashEmu *flash) {
    flash->Memory = NULL;
}

void Flash_Cold_Reset(FlashEmu *flash) {

    flash->ReadMode = ReadMode_Normal;
    flash->CommandPhase = 0;

    flash->ToggleBits = 0;
}

int Flash_Is_Control_Read_Enabled(FlashEmu *flash) {
    return flash->ReadMode != ReadMode_Normal;
}

int Flash_Read_Byte(FlashEmu *flash, uint32_t address, uint8_t *data) {
    int result = Flash_Debug_Read_Byte(flash, address, data);

    if (flash->ReadMode == ReadMode_SectorEraseStatus) {
        // Toggle DQ2 and DQ6 after the read.
        flash->ToggleBits ^= 0x44;
    }

    return result;
}

int Flash_Debug_Read_Byte(FlashEmu *flash, uint32_t address, uint8_t *data) {
    *data = 0xFF;

    switch(flash->ReadMode) {
        case ReadMode_Normal:
            *data = flash->Memory[address];
            //printf("Read[$%05X] = $%02X\n", address, *data);
            return TRUE;

        case ReadMode_Autoselect:
            if (flash->FlashType == Flash_TypeS29GL01P ||
                flash->FlashType == Flash_TypeS29GL512P ||
                flash->FlashType == Flash_TypeS29GL256P) {
                // The S29GL01 is a 16-bit device, so this is a bit more annoying:
                static const uint8_t IDData[]={
                    0x01, 0x00,     // Manufacturer ID
                    0x7E, 0x22,     // Device ID
                    0x00, 0x00,     // Protection Verification (stubbed)
                    0x3F, 0xFF,     // Indicator Bits
                    0xFF, 0xFF,     // RFU / Reserved
                    0xFF, 0xFF,     // RFU / Reserved
                    0xFF, 0xFF,     // RFU / Reserved
                    0xFF, 0xFF,     // RFU / Reserved
                    0xFF, 0xFF,     // RFU / Reserved
                    0xFF, 0xFF,     // RFU / Reserved
                    0xFF, 0xFF,     // RFU / Reserved
                    0xFF, 0xFF,     // RFU / Reserved
                    0x04, 0x00,     // Lower Software Bits
                    0xFF, 0xFF,     // Upper Software Bits
                    0x28, 0x22,     // Device ID
                    0x01, 0x22,     // Device ID

                    0x51, 0x00,     // Q
                    0x52, 0x00,     // R
                    0x59, 0x00,     // Y
                    0x02, 0x00,     // Primary OEM Command Set
                    0x00, 0x00,     // (cont.)
                    0x40, 0x00,     // Address for Primary Extended Table
                    0x00, 0x00,     // (cont.)
                    0x00, 0x00,     // Alternate OEM Command Set
                    0x00, 0x00,     // (cont.)
                    0x00, 0x00,     // Address for Alternate OEM Command Set
                    0x00, 0x00,     // (cont.)
                    0x27, 0x00,     // Vcc Min. (erase/program)
                    0x36, 0x00,     // Vcc Max. (erase/program)
                    0x00, 0x00,     // Vpp Min. voltage
                    0x00, 0x00,     // Vpp Max. voltage
                    0x06, 0x00,     // Typical timeout per single word

                    0x06, 0x00,     // Typical timeout for max multi-byte program
                    0x09, 0x00,     // Typical timeout per individual block erase
                    0x13, 0x00,     // Typical timeout for full chip erase
                    0x03, 0x00,     // Max. timeout for single word write
                    0x05, 0x00,     // Max. timeout for buffer write
                    0x03, 0x00,     // Max. timeout per individual block erase
                    0x02, 0x00,     // Max. timeout for full chip erase
                    0x1B, 0x00,     // Device size
                    0x01, 0x00,     // Flash Device Interface Description (x16 only)
                    0x00, 0x00,     // (cont.)
                    0x09, 0x00,     // Max number of byte in multi-byte write
                    0x00, 0x00,     // (cont.)
                    0x01, 0x00,     // Number of Erase Block Regions
                    0xFF, 0x00,     // Erase Block Region 1 Information
                    0x03, 0x00,     // (cont.)
                    0x00, 0x00,     // (cont.)

                    0x02, 0x00,     // (cont.)
                    0x00, 0x00,     // Erase Block Region 2 Information
                    0x00, 0x00,     // (cont.)
                    0x00, 0x00,     // (cont.)
                    0x00, 0x00,     // (cont.)
                    0x00, 0x00,     // Erase Block Region 3 Information
                    0x00, 0x00,     // (cont.)
                    0x00, 0x00,     // (cont.)
                    0x00, 0x00,     // (cont.)
                    0x00, 0x00,     // Erase Block Region 4 Information
                    0x00, 0x00,     // (cont.)
                    0x00, 0x00,     // (cont.)
                    0x00, 0x00,     // (cont.)
                    0xFF, 0xFF,     // Reserved
                    0xFF, 0xFF,     // Reserved
                    0xFF, 0xFF,     // Reserved

                    0x50, 0x00,     // P
                    0x52, 0x00,     // R
                    0x49, 0x00,     // I
                    0x31, 0x00,     // Major version number (1.3)
                    0x33, 0x00,     // Minor version number
                    0x14, 0x00,     // Address Sensitive Unlock, Process Technology (90nm MirrorBit)
                    0x02, 0x00,     // Erase Suspend
                    0x01, 0x00,     // Sector Protect
                    0x00, 0x00,     // Temporary Sector Unprotect
                    0x08, 0x00,     // Sector Protect/Unprotect Scheme
                    0x00, 0x00,     // Simultaneous Operation
                    0x00, 0x00,     // Burst Mode Type
                    0x02, 0x00,     // Page Mode Type
                    0xB5, 0x00,     // ACC Supply Minimum
                    0xC5, 0x00,     // ACC Supply Maximum
                    0x05, 0x00,     // WP# Protection (top)

                    0x01, 0x00,     // Program Suspend
                };

                size_t address8 = address & 0xff;
                if (address8 == 0x0E*2) {       // Device ID
                    switch(flash->FlashType) {
                        case Flash_TypeS29GL256P:
                            *data = 0x22;
                            break;
                        case Flash_TypeS29GL512P:
                            *data = 0x23;
                            break;
                        case Flash_TypeS29GL01P:
                            *data = 0x28;
                            break;
                    }
                } else if (address8 == 0x22*2) {        // Typical Timeout for Full Chip Erase
                    switch(flash->FlashType) {
                        case Flash_TypeS29GL256P:
                            *data = 0x13;
                            break;
                        case Flash_TypeS29GL512P:
                            *data = 0x12;
                            break;
                        case Flash_TypeS29GL01P:
                            *data = 0x11;
                            break;
                    }
                } else if (address8 == 0x27*2) {        // Device Size
                    switch(flash->FlashType) {
                        case Flash_TypeS29GL256P:
                            *data = 0x19;
                            break;
                        case Flash_TypeS29GL512P:
                            *data = 0x1A;
                            break;
                        case Flash_TypeS29GL01P:
                            *data = 0x1B;
                            break;
                    }
                } else if (address8 == 0x2E*2) {        // Erase Block Region 1
                    switch(flash->FlashType) {
                        case Flash_TypeS29GL256P:
                            *data = 0x00;
                            break;
                        case Flash_TypeS29GL512P:
                            *data = 0x01;
                            break;
                        case Flash_TypeS29GL01P:
                            *data = 0x03;
                            break;
                    }
                } else if (address8 < sizeof(IDData))
                    *data = IDData[address];
                else
                    *data = 0xFF;
            } else switch(address & 0xFF) {
                case 0x00:
                    switch(flash->FlashType) {
                        case Flash_TypeAm29F010:
                        case Flash_TypeAm29F010B:
                        case Flash_TypeAm29F040:
                        case Flash_TypeAm29F040B:
                        case Flash_TypeAm29F016D:
                        case Flash_TypeAm29F032B:
                            *data = 0x01;    // XX00 Manufacturer ID: AMD/Spansion
                            break;

                        case Flash_TypeAT29C010A:
                        case Flash_TypeAT29C040:
                            *data = 0x1F;    // XX00 Manufacturer ID: Atmel
                            break;

                        case Flash_TypeSST39SF040:
                            *data = 0xBF;    // XX00 Manufacturer ID: SST
                            break;

                        case Flash_TypeA29040:
                            *data = 0x37;    // XX00 Manufacturer ID: AMIC
                            break;

                        case Flash_TypeBM29F040:
                            *data = 0xAD;    // XX00 Manufacturer ID: Bright Microelectronics Inc. (same as Hyundai)
                            break;

                        case Flash_TypeM29F010B:
                            *data = 0x20;
                            break;

                        case Flash_TypeHY29F040A:
                            *data = 0xAD;    // XX00 Manufacturer ID: Hynix (Hyundai)
                            break;
                        case Flash_TypeM29W800DT:
                            data = 0x20;    // XX00 Manufacturer ID: Numonyx
                            break;

                        case Flash_TypeMX29LV640DT:
                            data = 0xC2;    // XX00 Manufacturer ID: Macronix
                            break;
                    }
                    break;

                case 0x01:
                    switch(flash->FlashType) {
                        case Flash_TypeAm29F010:
                        case Flash_TypeAm29F010B:
                            *data = 0x20;
                            break;
                    
                        case Flash_TypeAm29F040:
                        case Flash_TypeAm29F040B:
                            // Yes, the 29F040 and 29F040B both have the same code even though
                            // the 040 validates A0-A14 and the 040B only does A0-A10.
                            *data = 0xA4;
                            break;

                        case Flash_TypeAm29F016D:
                            *data = 0xAD;
                            break;

                        case Flash_TypeAm29F032B:
                            *data = 0x41;
                            break;

                        case Flash_TypeAT29C010A:
                            *data = 0xD5;
                            break;

                        case Flash_TypeAT29C040:
                            *data = 0x5B;
                            break;

                        case Flash_TypeSST39SF040:
                            *data = 0xB7;
                            break;

                        case Flash_TypeA29040:
                            *data = 0x86;
                            break;

                        case Flash_TypeBM29F040:
                            *data = 0x40;
                            break;

                        case Flash_TypeM29F010B:
                            *data = 0x20;
                            break;

                        case Flash_TypeHY29F040A:
                            *data = 0xA4;
                            break;
                        
                        case Flash_TypeM29W800DT:
                            data = 0xD7;
                            break;

                        case Flash_TypeMX29LV640DT:
                            data = 0xC2;
                            break;
                    }
                    break;

                case 0x02:
                    switch(flash->FlashType) {
                        case Flash_TypeMX29LV640DT:
                            data = 0xC9;
                            break;
                        default:
                            data = 0x00;    // XX02 Sector Protect Verify: 00 not protected
                            break;
                    }
                    break;

                case 0x03:
                    switch(flash->FlashType) {
                        case Flash_TypeMX29LV640DT:
                            data = 0xC9;
                            break;
                        default:
                            data = 0x00;
                            break;
                    }
                    break;

                default:
                    *data = 0x00;    // XX02 Sector Protect Verify: 00 not protected
                    break;
            }
            break;

        case ReadMode_WriteStatusPending:
            *data = ~flash->Memory[address] & 0x80;
            break;

        case ReadMode_SectorEraseStatus:
            // During sector erase timeout:
            //  DQ7 = 0 (complement of erased data)
            //  DQ6 = toggle
            //  DQ5 = 0 (not exceeded timing limits)
            //  DQ3 = 0 (additional commands accepted)
            //  DQ2 = toggle
            *data = flash->ToggleBits;
            break;
    }

    return FALSE;
}

int Flash_Write_Byte(FlashEmu *flash, uint32_t address, uint8_t value) {
    uint32_t address15 = address & 0x7fff;
    uint32_t address11 = address & 0x7ff;

    printf("Write[$%05X] = $%02X\n", address, value);

    switch(flash->CommandPhase) {
        case 0:
            // $F0 written at phase 0 deactivates autoselect mode
            if (value == 0xF0) {
                if (!flash->ReadMode)
                    break;

                printf("Exiting autoselect mode.\n");
                flash->ReadMode = ReadMode_Normal;

                return TRUE;
            }

            switch(flash->FlashType) {
                case Flash_TypeAm29F010:
                case Flash_TypeAm29F040:
                case Flash_TypeSST39SF040:
                case Flash_TypeBM29F040:
                    if (address15 == 0x5555 && value == 0xAA)
                        flash->CommandPhase = 1;
                    break;

                case Flash_TypeAT29C010A:
                case Flash_TypeAT29C040:
                    if (address == 0x5555 && value == 0xAA)
                        flash->CommandPhase = 7;
                    break;

                case Flash_TypeA29040:
                case Flash_TypeAm29F010B:
                case Flash_TypeAm29F040B:
                case Flash_TypeAm29F016D:
                case Flash_TypeAm29F032B:
                case Flash_TypeM29F010B:
                case Flash_TypeHY29F040A:
                    if ((address & 0x7FF) == 0x555 && value == 0xAA)
                        flash->CommandPhase = 1;
                    break;

                case Flash_TypeS29GL01P:
                case Flash_TypeS29GL512P:
                case Flash_TypeM29W800DT:
                case Flash_TypeMX29LV640DT:
                case Flash_TypeS29GL256P:
                    if ((address & 0xFFF) == 0xAAA && value == 0xAA)
                        flash->CommandPhase = 1;
                    break;
            }

            if (flash->CommandPhase)
                printf("Unlock: step 1 OK.\n");
            else
                printf("Unlock: step 1 FAILED [($%05X) = $%02X].\n", address, value);
            break;

        case 1:
            flash->CommandPhase = 0;

            if (value == 0x55) {
                switch(flash->FlashType) {
                    case Flash_TypeS29GL01P:
                    case Flash_TypeS29GL512P:
                    case Flash_TypeS29GL256P:
                    case Flash_TypeM29W800DT:
                    case Flash_TypeMX29LV640DT:
                        if ((address & 0xFFF) == 0x555)
                            flash->CommandPhase = 2;
                        break;

                    default:
                        if (flash->A11Unlock) {
                            if ((address & 0x7FF) == 0x2AA)
                                flash->CommandPhase = 2;
                        } else {
                            if (address15 == 0x2AAA)
                                flash->CommandPhase = 2;
                        }
                        break;
                }
            }

            if (flash->CommandPhase)
                printf("Unlock: step 2 OK.\n");
            else
                printf("Unlock: step 2 FAILED [($%05X) = $%02X].\n", address, value);
            break;

        case 2:
            switch(flash->FlashType) {
                case Flash_TypeS29GL256P:
                case Flash_TypeS29GL512P:
                case Flash_TypeS29GL01P:
                    if (value == 0x25) {
                        printf("Entering write buffer load mode.\n");
                        flash->PendingWriteAddress = address;
                        flash->CommandPhase = 15;
                        return FALSE;
                    }
                    break;
            }

            if (flash->A12iUnlock ? (address & 0xFFF) != 0xAAA :
                flash->A11Unlock ? (address & 0x7FF) != 0x555
                            : address15 != 0x5555) {
                printf("Unlock: step 3 FAILED [($%05X) = $%02X].\n", address, value);
                flash->CommandPhase = 0;
                break;  
            }

            // A non-erase command aborts a multiple sector erase in timeout phase.
            if (value != 0x80 && flash->ReadMode == ReadMode_SectorEraseStatus) {
                flash->ReadMode = ReadMode_Autoselect;
            }

            switch(value) {
                case 0x80:  // $80: sector or chip erase
                    printf("Entering sector erase mode.\n");
                    flash->CommandPhase = 3;
                    break;

                case 0x90:  // $90: autoselect mode
                    printf("Entering autoselect mode.\n");
                    flash->ReadMode = ReadMode_Autoselect;
                    flash->CommandPhase = 0;
                    return TRUE;

                case 0xA0:  // $A0: program mode
                    printf("Entering program mode.\n");
                    flash->CommandPhase = 6;
                    break;

                case 0xF0:  // $F0: reset
                    printf("Exiting autoselect mode.\n");
                    flash->CommandPhase = 0;
                    flash->ReadMode = ReadMode_Normal;
                    return TRUE;

                default:
                    printf("Unknown command $%02X.\n", value);
                    flash->CommandPhase = 0;
                    break;
            }

            break;

        case 3:     // 5555[AA] 2AAA[55] 5555[80]
            flash->CommandPhase = 0;
            if (value == 0xAA) {
                if (flash->A12iUnlock) {
                    if ((address & 0xFFF) == 0xAAA)
                        flash->CommandPhase = 4;
                } else if (flash->A11Unlock) {
                    if (address11 == 0x555)
                        flash->CommandPhase = 4;
                } else {
                    if (address15 == 0x5555)
                        flash->CommandPhase = 4;
                }
            }

            if (flash->CommandPhase)
                printf("Erase: step 4 OK.\n");
            else
                printf("Erase: step 4 FAILED [($%05X) = $%02X].\n", address, value);
            break;

        case 4:     // 5555[AA] 2AAA[55] 5555[80] 5555[AA]
            flash->CommandPhase = 0;
            if (value == 0x55) {
                if (flash->A12iUnlock) {
                    if ((address & 0xFFF) == 0x555)
                        flash->CommandPhase = 5;
                } else if (flash->A11Unlock) {
                    if (address11 == 0x2AA)
                        flash->CommandPhase = 5;
                } else {
                    if (address15 == 0x2AAA)
                        flash->CommandPhase = 5;
                }
            }

            if (flash->CommandPhase)
                printf("Erase: step 5 OK.\n");
            else
                printf("Erase: step 5 FAILED [($%05X) = $%02X].\n", address, value);
            break;

        case 5:     // 5555[AA] 2AAA[55] 5555[80] 5555[AA] 2AAA[55]
            // A non-sector-erase command aborts a multiple sector erase in timeout phase.
            if (value != 0x80 && flash->ReadMode == ReadMode_SectorEraseStatus) {
                flash->ReadMode = ReadMode_Autoselect;
            }

            if (value == 0x10 && (flash->A12iUnlock ? (address & 0xFFF) == 0xAAA : flash->A11Unlock ? address11 == 0x555 : address15 == 0x5555)) {
                // full chip erase
                switch(flash->FlashType) {
                    case Flash_TypeAm29F010:
                    case Flash_TypeAm29F010B:
                    case Flash_TypeM29F010B:
                        memset(flash->Memory, 0xFF, 0x20000);
                        break;

                    case Flash_TypeAm29F040:
                    case Flash_TypeAm29F040B:
                    case Flash_TypeSST39SF040:
                    case Flash_TypeA29040:
                    case Flash_TypeBM29F040:
                    case Flash_TypeHY29F040A:
                        memset(flash->Memory, 0xFF, 0x80000);
                        break;

                    case Flash_TypeM29W800DT:
                    case Flash_TypeMX29LV640DT:
                        memset(flash->Memory, 0xFF, 0x800000);        // 8M (64Mbit)
                        break;

                    case Flash_TypeAm29F016D:
                        memset(flash->Memory, 0xFF, 0x200000);
                        break;

                    case Flash_TypeAm29F032B:
                        memset(flash->Memory, 0xFF, 0x400000);
                        break;

                    case Flash_TypeS29GL01P:
                        memset(flash->Memory, 0xFF, 0x8000000);
                        break;

                    case Flash_TypeS29GL512P:
                        memset(flash->Memory, 0xFF, 0x4000000);
                        break;

                    case Flash_TypeS29GL256P:
                        memset(flash->Memory, 0xFF, 0x2000000);
                        break;
                }

                printf("Erasing entire flash chip.\n");
                flash->WriteActivity = FALSE;
                flash->Dirty = TRUE;

            } else if (value == 0x30) {
                // sector erase
                switch(flash->FlashType) {
                    case Flash_TypeAm29F010:
                    case Flash_TypeAm29F010B:
                    case Flash_TypeM29F010B:
                        address &= 0x1C000;
                        memset(flash->Memory + address, 0xFF, 0x4000);
                        printf("Erasing sector $%05X-%05X\n", address, address + 0x3FFF);
                        break;
                    case Flash_TypeAm29F040:
                    case Flash_TypeAm29F040B:
                    case Flash_TypeA29040:
                    case Flash_TypeBM29F040:
                    case Flash_TypeHY29F040A:
                        address &= 0x70000;
                        memset(flash->Memory + address, 0xFF, 0x10000);
                        printf("Erasing sector $%05X-%05X\n", address, address + 0xFFFF);
                        break;
                    case Flash_TypeSST39SF040:
                        address &= 0x7F000;
                        memset(flash->Memory + address, 0xFF, 0x1000);
                        printf("Erasing sector $%05X-%05X\n", address, address + 0xFFF);
                        break;
                    case Flash_TypeAm29F016D:
                        address &= 0x1F0000;
                        memset(flash->Memory + address, 0xFF, 0x10000);
                        printf("Erasing sector $%06X-%06X\n", address, address + 0xFFFF);
                        break;
                    case Flash_TypeAm29F032B:
                        address &= 0x3F0000;
                        memset(flash->Memory + address, 0xFF, 0x10000);
                        printf("Erasing sector $%06X-%06X\n", address, address + 0xFFFF);
                        break;
                    case Flash_TypeS29GL01P:
                        address &= 0x7FF0000;
                        memset(flash->Memory + address, 0xFF, 0x20000);
                        printf("Erasing sector $%07X-%07X\n", address, address + 0x1FFFF);
                        break;
                    case Flash_TypeS29GL512P:
                        address &= 0x3FF0000;
                        memset(flash->Memory + address, 0xFF, 0x20000);
                        printf("Erasing sector $%07X-%07X\n", address, address + 0x1FFFF);
                        break;
                    case Flash_TypeS29GL256P:
                        address &= 0x1FF0000;
                        memset(flash->Memory + address, 0xFF, 0x20000);
                        printf("Erasing sector $%07X-%07X\n", address, address + 0x1FFFF);
                        break;
                }

                flash->WriteActivity = TRUE;
                flash->Dirty = TRUE;

                if (flash->SectorEraseTimeoutCycles) {
                    // Once a sector erase has happened, other sector erase commands
                    // may be issued... but ONLY sector erase commands. This window is
                    // only guaranteed to last between 50us and 80us.
                    flash->CommandPhase = 14;
                    flash->ReadMode = ReadMode_SectorEraseStatus;
                    return TRUE;
                }
            } else {
                printf("Erase: step 6 FAILED [($%05X) = $%02X].\n", address, value);
            }

            // unknown command
            flash->CommandPhase = 0;
            flash->ReadMode = ReadMode_Normal;
            return TRUE;

        case 6:     // 5555[AA] 2AAA[55] 5555[A0]
            flash->Memory[address] &= value;
            flash->Dirty = TRUE;
            flash->WriteActivity = TRUE;

            flash->CommandPhase = 0;
            flash->ReadMode = ReadMode_Normal;
            return TRUE;

        case 7:     // Atmel 5555[AA]
            if (address == 0x2AAA && value == 0x55)
                flash->CommandPhase = 8;
            else
                flash->CommandPhase = 0;
            break;

        case 8:     // Atmel 5555[AA] 2AAA[55]
            if (address != 0x5555) {
                flash->CommandPhase = 0;
                break;
            }

            switch(value) {
                case 0x80:  // $80: chip erase
                    flash->CommandPhase = 9;
                    break;

                case 0x90:  // $90: autoselect mode
                    flash->ReadMode = ReadMode_Autoselect;
                    flash->CommandPhase = 0;
                    break;

                case 0xA0:  // $A0: program mode
                    flash->CommandPhase = 12;
                    flash->AtmelSDP = TRUE;
                    break;

                case 0xF0:  // $F0: reset
                    flash->CommandPhase = 0;
                    flash->ReadMode = ReadMode_Normal;
                    return TRUE;

                default:
                    flash->CommandPhase = 0;
                    break;
            }
            break;

        case 9:     // Atmel 5555[AA] 2AAA[55] 5555[80]
            if (address == 0x5555 && value == 0xAA)
                flash->CommandPhase = 10;
            else
                flash->CommandPhase = 0;
            break;

        case 10:    // Atmel 5555[AA] 2AAA[55] 5555[80] 2AAA[55]
            if (address == 0x2AAA && value == 0x55)
                flash->CommandPhase = 11;
            else
                flash->CommandPhase = 0;
            break;

        case 11:    // Atmel 5555[AA] 2AAA[55] 5555[80] 2AAA[55]
            if (address == 0x5555) {
                switch(value) {
                    case 0x10:      // chip erase
                        switch(flash->FlashType) {
                            case Flash_TypeAT29C010A:
                                memset(flash->Memory, 0xFF, 0x20000);
                                break;

                            case Flash_TypeAT29C040:
                                memset(flash->Memory, 0xFF, 0x80000);
                                break;
                        }
                        flash->Dirty = TRUE;
                        flash->WriteActivity = TRUE;
                        break;

                    case 0x20:      // software data protection disable
                        flash->AtmelSDP = FALSE;
                        flash->CommandPhase = 12;
                        return FALSE;
                }
            }

            flash->ReadMode = ReadMode_Normal;
            flash->CommandPhase = 0;
            break;

        case 12:    // Atmel SDP program mode - initial sector write
            flash->WriteSector = address & 0xFFF00;
            flash->ReadMode = ReadMode_WriteStatusPending;
            memset(flash->Memory, 0xFF, 256);
            flash->CommandPhase = 13;
            // fall through

        case 13:    // Atmel program mode - sector write
            flash->Memory[address] = value;
            flash->Dirty = TRUE;
            flash->WriteActivity = TRUE;
            return TRUE;

        case 14:    // Multiple sector erase mode (AMD/Amic)
            if (value == 0x30) {
                // sector erase
                switch(flash->FlashType) {
                    case Flash_TypeAm29F010:
                    case Flash_TypeAm29F010B:
                    case Flash_TypeM29F010B:
                        address &= 0x1C000;
                        memset(flash->Memory + address, 0xFF, 0x4000);
                        printf("Erasing sector $%05X-%05X\n", address, address + 0x3FFF);
                        break;
                    case Flash_TypeAm29F040:
                    case Flash_TypeAm29F040B:
                    case Flash_TypeA29040:
                    case Flash_TypeBM29F040:
                    case Flash_TypeHY29F040A:
                        address &= 0x70000;
                        memset(flash->Memory + address, 0xFF, 0x10000);
                        printf("Erasing sector $%05X-%05X\n", address, address + 0xFFFF);
                        break;
                    case Flash_TypeSST39SF040:
                        address &= 0x7F000;
                        memset(flash->Memory + address, 0xFF, 0x1000);
                        printf("Erasing sector $%05X-%05X\n", address, address + 0xFFF);
                        break;
                    case Flash_TypeAm29F016D:
                        address &= 0x1F0000;
                        memset(flash->Memory + address, 0xFF, 0x10000);
                        printf("Erasing sector $%06X-%06X\n", address, address + 0xFFFF);
                        break;
                    case Flash_TypeAm29F032B:
                        address &= 0x3F0000;
                        memset(flash->Memory + address, 0xFF, 0x10000);
                        printf("Erasing sector $%06X-%06X\n", address, address + 0xFFFF);
                        break;
                }

                flash->WriteActivity = TRUE;
                flash->Dirty = TRUE;
            }
            return TRUE;

        case 15:    // Write buffer load - word count byte
            if (value > 31) {
                printf("Aborting write buffer load - invalid word count ($%02X + 1)\n", value);
                flash->CommandPhase = 0;
            } else if ((address ^ flash->PendingWriteAddress) & ~(uint32_t)0x1ffff) {
                printf("Aborting write buffer load - received word count outside of initial sector\n");
            } else {
                memset(flash->WriteBufferData, 0xFF, sizeof(flash->WriteBufferData));
                flash->PendingWriteCount = value + 1;
                flash->CommandPhase = 16;
            }
            break;

        case 16:    // Write buffer load - receiving first byte
            if ((address ^ flash->PendingWriteAddress) & ~(uint32_t)0x1ffff) {
                printf("Aborting write buffer load - received write outside of initial sector\n");
                flash->CommandPhase = 0;
            } else {
                flash->PendingWriteAddress = address;
                flash->WriteBufferData[address & 31] &= value;
                --flash->PendingWriteCount;

                if (flash->PendingWriteCount)
                    flash->CommandPhase = 17;
                else
                    flash->CommandPhase = 18;
            }
            break;

        case 17:    // Write buffer load - receiving subsequent bytes
            if ((address ^ flash->PendingWriteAddress) & ~(uint32_t)31) {
                printf("Aborting write buffer load - received write outside of write page\n");
                flash->CommandPhase = 0;
            } else {
                flash->WriteBufferData[address & 31] &= value;
                --flash->PendingWriteCount;

                if (!flash->PendingWriteCount)
                    flash->CommandPhase = 18;
            }
            break;

        case 18:    // Write buffer load -- waiting for program command
            if (value != 0x29) {
                printf("Aborting buffered write -- received command other than program buffer command ($%02X)\n", value);
            } else {
                switch(flash->FlashType) {
                    case Flash_TypeS29GL256P:
                        flash->PendingWriteAddress &= 0x1FFFFE0;
                        break;

                    case Flash_TypeS29GL512P:
                        flash->PendingWriteAddress &= 0x3FFFFE0;
                        break;

                    case Flash_TypeS29GL01P:
                        flash->PendingWriteAddress &= 0x7FFFFE0;
                        break;
                }

                printf("Programming write page at $%07X\n", flash->PendingWriteAddress);

                for(int i=0; i<32; ++i)
                    flash->Memory[flash->PendingWriteAddress + i] &= flash->WriteBufferData[i];

                flash->WriteActivity = TRUE;
                flash->Dirty = TRUE;
            }

            flash->CommandPhase = 0;
            break;
    }

    return FALSE;
}

void SectorErase(FlashEmu *flash, ULONG address) {
    switch(flash->FlashType) {
        case Flash_TypeAm29F010:
        case Flash_TypeAm29F010B:
        case Flash_TypeM29F010B:
            address &= 0x1C000;
            memset(flash->Memory + address, 0xFF, 0x4000);
            printf("Erasing sector $%05X-%05X\n", address, address + 0x3FFF);
            break;
        case Flash_TypeAm29F040:
        case Flash_TypeAm29F040B:
        case Flash_TypeA29040:
        case Flash_TypeBM29F040:
        case Flash_TypeHY29F040A:
            address &= 0x70000;
            memset(flash->Memory + address, 0xFF, 0x10000);
            printf("Erasing sector $%05X-%05X\n", address, address + 0xFFFF);
            break;
        case Flash_TypeSST39SF040:
            address &= 0x7F000;
            memset(flash->Memory + address, 0xFF, 0x1000);
            printf("Erasing sector $%05X-%05X\n", address, address + 0xFFF);
            break;
        case Flash_TypeAm29F016D:
            address &= 0x1F0000;
            memset(flash->Memory + address, 0xFF, 0x10000);
            printf("Erasing sector $%06X-%06X\n", address, address + 0xFFFF);
            break;
        case Flash_TypeAm29F032B:
            address &= 0x3F0000;
            memset(flash->Memory + address, 0xFF, 0x10000);
            printf("Erasing sector $%06X-%06X\n", address, address + 0xFFFF);
            break;
        case Flash_TypeS29GL01P:
            address &= 0x7FF0000;
            memset(flash->Memory + address, 0xFF, 0x20000);
            printf("Erasing sector $%07X-%07X\n", address, address + 0x1FFFF);
            break;
        case Flash_TypeS29GL512P:
            address &= 0x3FF0000;
            memset(flash->Memory + address, 0xFF, 0x20000);
            printf("Erasing sector $%07X-%07X\n", address, address + 0x1FFFF);
            break;
        case Flash_TypeS29GL256P:
            address &= 0x1FF0000;
            memset(flash->Memory + address, 0xFF, 0x20000);
            printf("Erasing sector $%07X-%07X\n", address, address + 0x1FFFF);
            break;
        case Flash_TypeM29W800DT:
            {
                // All blocks are 64K in the M29W800DT, except for the last 64K which
                // is subdivided into 32K, 16K, 4K, 4K, and 8K, in that order.
                ULONG blockSize = 0x10000;

                address &= 0x7FF000;
                if (address < 0x7F0000) {
                    address &= 0x7F0000;
                } else if (address < 0x7F8000) {
                    address = 0x7F0000;
                    blockSize = 0x8000;
                } else if (address < 0x7FC000) {
                    address = 0x7F8000;
                    blockSize = 0x4000;
                } else if (address < 0x7FD000) {
                    address = 0x7FC000;
                    blockSize = 0x1000;
                } else if (address < 0x7FE000) {
                    address = 0x7FD000;
                    blockSize = 0x1000;
                } else {
                    address = 0x7FE000;
                    blockSize = 0x2000;
                }

                memset(flash->Memory + address, 0xFF, blockSize);
                printf("Erasing block $%07X-%07X\n", address, address + blockSize - 1);
            }
            break;
        case Flash_TypeMX29LV640DT:
            {
                // All blocks are 64K in the MX29LV640D, except for the last 64K which
                // is subdivided into 8K blocks.
                ULONG blockSize = 0x10000;

                address &= 0x7FE000;
                if (address < 0x7F0000)
                    address &= 0x7F0000;
                else
                    blockSize = 0x2000;

                memset(flash->Memory + address, 0xFF, blockSize);
                printf("Erasing block $%07X-%07X\n", address, address + blockSize - 1);
            }
            break;
    }
}
