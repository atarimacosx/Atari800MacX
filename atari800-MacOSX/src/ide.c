//  Altirra - Atari 800/800XL/5200 emulator
//  Copyright (C) 2009-2011 Avery Lee
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

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "img_disk.h"
#include "ide.h"

#undef IDE_DEBUG
#ifndef IDE_DEBUG
#define printf(fmt, ...) (0)
#endif

void AbortCommand(IDEEmu *ide, uint8_t error);
void AdjustCHSTranslation(IDEEmu *ide);
void BeginReadTransfer(IDEEmu *ide, uint32_t bytes);
void BeginWriteTransfer(IDEEmu *ide, uint32_t bytes);
void ColdReset(IDEEmu *ide);
void CompleteCommand(IDEEmu *ide);
int ReadLBA(IDEEmu *ide, uint32_t *lba);
void ResetCHSTranslation(IDEEmu *ide);
void Shutdown(IDEEmu *ide);
void StartCommand(IDEEmu *ide, uint8_t cmd);
void UpdateStatus(IDEEmu *ide);
void WriteLBA(IDEEmu *ide, uint32_t lba);

// read or write.
const uint32_t IODelayFast = 100;    //
const uint32_t IODelaySlow = 10000;  // ~5.5ms

void VDWriteUnalignedLEU32(void *p, uint32_t v) { *(uint32_t *)p = v; }
void VDWriteUnalignedLEU64(void *p, uint64_t v) { *(uint64_t *)p = v; }

uint32_t GetTimeTicks() {
    struct timespec time;
    uint32_t usecs;
    double altirraTicks;
    
    clock_gettime(CLOCK_REALTIME, &time);

    usecs = time.tv_nsec / 1000;
    
    altirraTicks = (double) usecs * 1.81818181;
    
    return ((uint32_t) altirraTicks);
}

IDEEmu *IDE_Init()
{
    IDEEmu *ide = (IDEEmu *) malloc(sizeof(IDEEmu));
    memset(ide, 0, sizeof(IDEEmu));
    
    ide->Disk = NULL;
    ide->HardwareReset = FALSE;
    ide->SoftwareReset = FALSE;
    ide->TransferLength = 0;
    ide->TransferIndex = 0;
    ide->SectorCount = 0;
    ide->SectorsPerTrack = 0;
    ide->HeadCount = 0;
    ide->CylinderCount = 0;
    ide->IODelaySetting = 0;
    ide->IsSingle = TRUE;
    ide->IsSlave = FALSE;

    ColdReset(ide);
    
    return(ide);
}

void IDE_Exit(IDEEmu *ide) {
    Shutdown(ide);
}

void IDE_Set_Is_Single(IDEEmu *ide, int single) {
    ide->IsSingle = single;
}

void Shutdown(IDEEmu *ide) {
    IDE_Close_Image(ide);
}

void IDE_Open_Image(IDEEmu *ide, char *filename) {
    IDE_Close_Image(ide);

    ide->Disk = IMG_Image_Open(filename, TRUE, TRUE);
    if (!ide->Disk)
        return;

    ide->WriteEnabled =  !IMG_Is_Read_Only(ide->Disk);
    ide->SectorCount = IMG_Get_Sector_Count(ide->Disk);

    BlockDeviceGeometry *geo = IMG_Get_Geometry(ide->Disk);

    // Check if the geometry is IDE and BIOS compatible.
    //
    // IDE compatibility requires:
    //  cylinders <= 16777216
    //  heads <= 16
    //  sectors <= 255
    //
    // BIOS compatibility requires:
    //  cylinders <= 65535
    //  heads <= 16
    //  sectors <= 63
    //
    if (ide->SectorCount > 16514064) {          // >16*63*16383 - advertise max to BIOS
        ide->SectorsPerTrack = 63;
        ide->HeadCount = 15;                    // ATA-4 B.2.2 -- yes, we switch to 15 for BIGGER drives.
        ide->CylinderCount = 16383;
    } else if (geo->Cylinders && geo->Heads && geo->SectorsPerTrack && geo->Cylinders <= 1024 && geo->Heads <= 16 && geo->SectorsPerTrack <= 63) {
        // BIOS compatible - use geometry directly
        ide->CylinderCount = geo->Cylinders;
        ide->HeadCount = geo->Heads;
        ide->SectorsPerTrack = geo->SectorsPerTrack;
    } else {
        // create alternate geometry (see ATA-4 Annex B)
        ide->SectorsPerTrack = 63;

        if (ide->SectorCount < 8257536 || !geo->Heads || geo->Heads > 16)
            ide->HeadCount = 16;
        else
            ide->HeadCount = geo->Heads;

        ide->CylinderCount = ide->SectorCount / (ide->SectorsPerTrack * ide->HeadCount);
        if (ide->CylinderCount < 1)
            ide->CylinderCount = 1;

        if (ide->CylinderCount > 16383)
            ide->CylinderCount = 16383;
    }

    ResetCHSTranslation(ide);

    ide->IODelaySetting = geo->SolidState ? IODelayFast : IODelaySlow;
    ide->FastDevice = geo->SolidState;

    ColdReset(ide);
}

void IDE_Close_Image(IDEEmu *ide) {
    ide->Disk = NULL;

    ide->SectorCount = 0;
    ide->CylinderCount = 0;
    ide->HeadCount = 0;
    ide->SectorsPerTrack = 0;

    ResetCHSTranslation(ide);

    ColdReset(ide);
}

void ColdReset(IDEEmu *ide) {
    ide->HardwareReset = FALSE;
    ide->SoftwareReset = FALSE;
    IDE_Reset_Device(ide);
}

void IDE_Set_Reset(IDEEmu *ide, int asserted) {
    if (ide->HardwareReset == asserted)
        return;

    ide->HardwareReset = asserted;

    if (asserted && ide->SoftwareReset)
        IDE_Reset_Device(ide);
}

uint32_t IDE_Read_Data_Latch(IDEEmu *ide, int advance) {
    if (ide->HardwareReset || ide->SoftwareReset || !ide->Disk)
        return 0xFFFF;

    uint32_t v = 0xFF;
    
    if (ide->TransferIndex < TransferBufferSize) {
        v = ide->TransferBuffer[ide->TransferIndex];

        if (ide->Transfer16Bit) {
            if (ide->TransferIndex + 1 < TransferBufferSize)
                v += (uint32_t)ide->TransferBuffer[ide->TransferIndex + 1] << 8;
            else
                v += 0xFF00;
        } else
            v += 0xFF00;
    }

    if (advance && !ide->TransferAsWrites && ide->TransferIndex < ide->TransferLength) {
        ++(ide->TransferIndex);

        if (ide->Transfer16Bit)
            ++(ide->TransferIndex);

        if (ide->TransferIndex >= ide->TransferLength)
            CompleteCommand(ide);
    }

    return v;
}

void IDE_Write_Data_Latch(IDEEmu *ide, uint8_t lo, uint8_t hi) {
    if (ide->HardwareReset || ide->SoftwareReset || !ide->Disk)
        return;

    if (ide->TransferAsWrites && ide->TransferIndex < ide->TransferLength) {
        ide->TransferBuffer[ide->TransferIndex] = lo;

        ++(ide->TransferIndex);

        if (ide->Transfer16Bit) {
            if (ide->TransferIndex < ide->TransferLength) {
                ide->TransferBuffer[ide->TransferIndex] = hi;

                ++ide->TransferIndex;
            }
        }

        if (ide->TransferIndex >= ide->TransferLength) {
            ide->RFile.Status &= ~IDEStatus_DRQ;
            ide->RFile.Status |= IDEStatus_BSY;

            UpdateStatus(ide);
        }
    }
}

uint8_t IDE_Debug_Read_Byte(IDEEmu *ide, uint8_t address) {
    if (ide->HardwareReset || ide->SoftwareReset || !ide->Disk)
        return 0xD0;

    if (address >= 8)
        return 0xFF;

    uint32_t idx = address & 7;

    UpdateStatus(ide);

    // ATA/ATAPI-4 9.16.0 -- when device 1 is selected with only device 0 present,
    // status and alternate status return 00h, while all other reads are the same as
    // device 0.
    if (ide->IsSingle) {
        if (ide->RFile.Head & 0x10) {
            if (idx == 7)
                return 0;
        }
    }

    // ATA-1 7.2.13 - if BSY=1, all reads of the command block return the status register
    if (ide->RFile.Status & IDEStatus_BSY)
        return ide->RFile.Status;

    if (idx == 0)
        return (uint8_t)IDE_Read_Data_Latch(ide, FALSE);

    return ide->Registers[idx];
}

uint8_t IDE_Read_Byte(IDEEmu *ide, uint8_t address) {
    if (ide->HardwareReset || ide->SoftwareReset || !ide->Disk)
        return 0xD0;

    if (address >= 8)
        return 0xFF;

    uint32_t idx = address & 7;

    UpdateStatus(ide);

    // ATA/ATAPI-4 9.16.0 -- when device 1 is selected with only device 0 present,
    // status and alternate status return 00h, while all other reads are the same as
    // device 0.
    if (ide->IsSingle) {
        if (ide->RFile.Head & 0x10) {
            if (idx == 7)
                return 0;
        }
    }

    // ATA-1 7.2.13 - if BSY=1, all reads of the command block return the status register
    if (ide->RFile.Status & IDEStatus_BSY)
        return ide->RFile.Status;

    if (idx == 0) {
        uint8_t v = (uint8_t)IDE_Read_Data_Latch(ide, TRUE);
        return v;
    }

    return ide->Registers[idx];
}

void IDE_Write_Byte(IDEEmu *ide, uint8_t address, uint8_t value) {
    if (address >= 8 || ide->HardwareReset || ide->SoftwareReset || !ide->Disk)
        return;

    uint32_t idx = address & 7;

    switch(idx) {
        case 0:     // data
            IDE_Write_Data_Latch(ide, value, 0xFF);
            break;

        case 1:     // features
            ide->Features = value;
            break;

        case 2:     // sector count
        case 3:     // sector number / LBA 0-7
        case 4:     // cylinder low / LBA 8-15
        case 5:     // cylinder high / LBA 16-23
        case 6:     // drive/head / LBA 24-27
            UpdateStatus(ide);

            if (ide->RFile.Status & IDEStatus_BSY) {
                printf("IDE: Attempted write of $%02x to register file index $%02x while drive is busy.\n", value, idx);
//              g_sim.PostInterruptingEvent(kATSimEvent_VerifierFailure);
            } else {
                // bits 7 and 5 in the drive/head register are always 1
                if (idx == 6)
                    value |= 0xa0;

                ide->Registers[idx] = value;
            }
            break;

        case 7:     // command
            // ignore other drive commands except for EXECUTE DEVICE DIAGNOSTIC
            if (((ide->RFile.Head & 0x10) != (ide->IsSlave ? 0x10 : 0x00)) && value != 0x90)
                return;

            // check if drive is busy
            UpdateStatus(ide);

            if (ide->RFile.Status & IDEStatus_BSY) {
                printf("IDE: Attempt to start command $%02x while drive is busy.\n", value);
            } else {
                StartCommand(ide, value);
            }
            break;
    }
}

uint8_t IDE_Read_Byte_Alt(IDEEmu *ide, uint8_t address) {
    switch(address & 7) {
        default:
            return 0xFF;

        case 0x06:  // alternate status
            if (ide->IsSingle && (ide->RFile.Head & 0x10))
                return 0;

            if (ide->HardwareReset || ide->SoftwareReset)
                return 0xD0;

            UpdateStatus(ide);
            return ide->RFile.Status;

        case 0x07:  // device address (drive address)
            UpdateStatus(ide);

            return (ide->IsSlave ? 0x01 : 0x02) + ((~ide->RFile.Head & 0x0f) << 2) + (ide->WriteInProgress ? 0x00 : 0x40);
    }
}

void IDE_Write_Byte_Alt(IDEEmu *ide, uint8_t address, uint8_t value) {
    if ((address & 7) == 6) {   // device control
        // bit 2 = SRST (software reset)
        // bit 1 = nIEN (inverted interrupt enable)

        int srst = (value & 0x02) != 0;

        if (ide->SoftwareReset != srst) {
            ide->SoftwareReset = srst;

            if (srst && !ide->HardwareReset)
                IDE_Reset_Device(ide);
        }
    }
}

int IDE_Debug_Read_Sector(IDEEmu *ide, uint32_t lba, void *dst) {
    if (ide->Disk)
        return(-1);
    
    if (lba >= ide->SectorCount)
        return(-1);

    IMG_Read_Sectors(ide->Disk, dst, lba, 1);
    return(0);
}

int IDE_Debug_Write_Sector(IDEEmu *ide, uint32_t lba, const void *dst) {
    if (ide->Disk)
        return(-1);

    if (lba >= ide->SectorCount)
        return(-1);

    if (!ide->WriteEnabled)
        return(-1);

    IMG_Write_Sectors(ide->Disk, dst, lba, 1);
    return(0);
}

void IDE_Reset_Device(IDEEmu *ide) {
    ide->ActiveCommand = 0;
    ide->ActiveCommandNextTime = 0;
    ide->ActiveCommandState = 0;

    // ATA-1 8.1 Reset Response / ATA-4 9.1 Signature and persistence
    ide->RFile.Data            = 0x00;
    ide->RFile.Errors          = 0x01;
    ide->RFile.SectorCount     = 0x01;
    ide->RFile.SectorNumber    = 0x01;
    ide->RFile.CylinderLow     = 0x00;
    ide->RFile.CylinderHigh    = 0x00;
    ide->RFile.Head            = 0x00;
    ide->RFile.Status          = IDEStatus_DRDY | IDEStatus_DSC;

    ide->Features = 0;
    ide->Transfer16Bit = TRUE;

    ide->WriteInProgress = FALSE;

    memset(ide->TransferBuffer, 0, TransferBufferSize);

    // Default to READ/WRITE MULTIPLE commands being enabled and specify a preferred
    // default of 32 sectors.
    ide->SectorsPerBlock = 32;
}

void UpdateStatus(IDEEmu *ide) {
    if (!ide->ActiveCommandState || !ide->Disk)
        return;

    uint32_t t = GetTimeTicks();

    if (0) //(int32_t)(t - ide->ActiveCommandNextTime) < 0)
        return;

    switch(ide->ActiveCommand) {
        case 0x10:      case 0x11:      case 0x12:      case 0x13:
        case 0x14:      case 0x15:      case 0x16:      case 0x17:
        case 0x18:      case 0x19:      case 0x1A:      case 0x1B:
        case 0x1C:      case 0x1D:      case 0x1E:      case 0x1F:
            // recalibrate (ATA-1 mandatory)
            switch(ide->ActiveCommandState) {
                case 1:
                    ide->RFile.Status |= IDEStatus_BSY;
                    ++(ide->ActiveCommandState);
                    ide->ActiveCommandNextTime = t + 100000;
                    break;

                case 2:
                    CompleteCommand(ide);
                    break;
            }
            break;

        case 0x20:  // read sector(s) w/retry
        case 0x21:  // read sector(s) w/o retry
        case 0xC4:  // read multiple
            switch(ide->ActiveCommandState) {
                case 1:
                    ide->RFile.Status |= IDEStatus_BSY;
                    ++(ide->ActiveCommandState);

                    // BOGUS: FDISK.BAS requires a delay before BSY drops since it needs to see
                    // BSY=1. ATA-4 7.15.6.1 BSY (Busy) states that this is not safe as the drive
                    // may operate too quickly to spot this.
                    ide->ActiveCommandNextTime = t + ide->IODelaySetting;
                    break;

                case 2:
                    // If the command is READ MULTIPLE and multiple commands are disabled, fail the command.
                    if (ide->ActiveCommand == 0xC4 && ide->SectorsPerBlock == 0) {
                        printf("Failing READ MULTIPLE command as multiple commands are disabled.");
                        AbortCommand(ide, 0);
                        return;
                    }

                    {
                        uint32_t lba;
                        uint32_t nsecs = ide->RFile.SectorCount;

                        if (!nsecs)
                            nsecs = 256;

                        if (!ReadLBA(ide, &lba)) {
                            AbortCommand(ide, 0);
                            return;
                        }

                        printf("IDE: Reading %u sectors starting at LBA %u.\n", nsecs, lba);

                        if (lba >= ide->SectorCount || ide->SectorCount - lba < nsecs || nsecs > MaxSectorTransferCount) {
                            ide->RFile.Status |= IDEStatus_ERR;
                            CompleteCommand(ide);
                        } else {
                            IMG_Read_Sectors(ide->Disk, ide->TransferBuffer, lba, nsecs);

                            WriteLBA(ide, lba + nsecs - 1);

                            BeginReadTransfer(ide, nsecs << 9);

                            // ATA-1 7.2.11 specifies that at command completion the Sector Count register
                            // should contain the number of unsuccessful sectors or zero for completion.
                            // Modern CF devices still do this.
                            ide->RFile.SectorCount = 0;
                        }

                        ide->ActiveCommandState = 0;
                    }
                    break;

            }
            break;

        case 0x30:  // write sector(s) w/retry
        case 0x31:  // write sector(s) w/o retry
        case 0xC5:  // write multiple
            switch(ide->ActiveCommandState) {
                case 1:
                    ide->RFile.Status |= IDEStatus_BSY;
                    ++(ide->ActiveCommandState);
                    ide->ActiveCommandNextTime = t + 250;
                    break;

                case 2:
                    // If the command is WRITE MULTIPLE and multiple commands are disabled, fail the command.
                    if (ide->ActiveCommand == 0xC5 && ide->SectorsPerBlock == 0) {
                        printf("Failing WRITE MULTIPLE command as multiple commands are disabled.");
                        AbortCommand(ide, 0);
                        return;
                    }

                    {
                        uint32_t lba;
                        if (!ReadLBA(ide, &lba)) {
                            AbortCommand(ide, 0);
                            return;
                        }

                        uint32_t nsecs = ide->RFile.SectorCount;

                        if (!nsecs)
                            nsecs = 256;

                        printf("IDE: Writing %u sectors starting at LBA %u .\n", nsecs, lba);

                        if (!ide->WriteEnabled) {
                            printf("IDE: Write blocked due to read-only status.\n");
                            AbortCommand(ide, 0);
                        }

                        if (lba >= ide->SectorCount || ide->SectorCount - lba < nsecs || nsecs >= MaxSectorTransferCount) {
                            printf("IDE: Returning error due to invalid command parameters.\n");
                            ide->RFile.Status |= IDEStatus_ERR;
                            CompleteCommand(ide);
                        } else {
                            // Note that we are actually transferring 256 words, but the Atari only reads
                            // the low bytes.
                            ide->TransferLBA = lba;
                            ide->TransferSectorCount = nsecs;
                            BeginWriteTransfer(ide, nsecs << 9);
                            ++(ide->ActiveCommandState);
                        }
                    }
                    break;

                case 3:
                    if (ide->TransferIndex < ide->TransferLength)
                        break;

                    IMG_Write_Sectors(ide->Disk, ide->TransferBuffer, ide->TransferLBA, ide->TransferSectorCount);

                    IMG_Flush(ide->Disk);

                    WriteLBA(ide, ide->TransferLBA + ide->TransferSectorCount - 1);

                    // ATA-1 7.2.11 specifies that at command completion the Sector Count register
                    // should contain the number of unsuccessful sectors or zero for completion.
                    // Modern CF devices still do this.
                    ide->RFile.SectorCount = 0;

                    ide->RFile.Status |= IDEStatus_BSY;
                    ++(ide->ActiveCommandState);
                    ide->ActiveCommandNextTime = t + ide->IODelaySetting;
                    ide->WriteInProgress = TRUE;
                    break;

                case 4:
                    CompleteCommand(ide);
                    break;
            }
            break;

        case 0x40:  // read verify sectors w/retry (ATA-1 mandatory)
        case 0x41:  // read verify sectors w/o retry (ATA-1 mandatory)
            switch(ide->ActiveCommandState) {
                case 1:
                    ide->RFile.Status |= IDEStatus_BSY;
                    ++(ide->ActiveCommandState);

                    // BOGUS: FDISK.BAS requires a delay before BSY drops since it needs to see
                    // BSY=1. ATA-4 7.15.6.1 BSY (Busy) states that this is not safe as the drive
                    // may operate too quickly to spot this.
                    ide->ActiveCommandNextTime = t + ide->IODelaySetting;
                    break;

                case 2:
                    {
                        uint32_t lba;
                        uint32_t nsecs = ide->RFile.SectorCount;

                        if (!nsecs)
                            nsecs = 256;

                        if (!ReadLBA(ide, &lba)) {
                            AbortCommand(ide, 0);
                            return;
                        }

                        printf("IDE: Verifying %u sectors starting at LBA %u .\n", nsecs, lba);

                        if (lba >= ide->SectorCount || ide->SectorCount - lba < nsecs || nsecs >= MaxSectorTransferCount) {
                            ide->RFile.Status |= IDEStatus_ERR;
                            CompleteCommand(ide);
                        } else {
                            WriteLBA(ide, lba + nsecs - 1);
                        }

                        ide->ActiveCommandState = 0;
                    }
                    break;

            }
            break;

        case 0x70:      case 0x71:      case 0x72:      case 0x73:
        case 0x74:      case 0x75:      case 0x76:      case 0x77:
        case 0x78:      case 0x79:      case 0x7A:      case 0x7B:
        case 0x7C:      case 0x7D:      case 0x7E:      case 0x7F:
            // seek (ATA-1 mandatory)
            switch(ide->ActiveCommandState) {
                case 1:
                    ide->RFile.Status |= IDEStatus_BSY;
                    ide->RFile.Status &= ~IDEStatus_DSC;
                    ++(ide->ActiveCommandState);
                    ide->ActiveCommandNextTime = t + 5000;
                    break;

                case 2:
                    ide->RFile.Status |= IDEStatus_DSC;
                    CompleteCommand(ide);
                    break;
            }
            break;

        case 0x90:  // execute device diagnostic
            switch(ide->ActiveCommandState) {
                case 1:
                    ide->RFile.Status |= IDEStatus_BSY;
                    ++(ide->ActiveCommandState);
                    ide->ActiveCommandNextTime = t + 500;
                    break;

                case 2:
                    // ATA/ATAPI-4 Table 10 (p.72)
                    ide->RFile.Errors = 0x01;
                    ide->RFile.SectorCount = 0x01;
                    ide->RFile.SectorNumber = 0x01;
                    ide->RFile.CylinderLow = 0;
                    ide->RFile.CylinderHigh = 0;
                    CompleteCommand(ide);
                    break;
            }
            break;

        case 0x91:  // initialize device parameters
            switch(ide->ActiveCommandState) {
                case 1:
                    ide->RFile.Status |= IDEStatus_BSY;
                    ++(ide->ActiveCommandState);
                    ide->ActiveCommandNextTime = t + 500;
                    break;

                case 2:
                    if (!ide->RFile.SectorCount) {
                        AbortCommand(ide, 0);
                        break;
                    }

                    ide->CurrentSectorsPerTrack = ide->RFile.SectorCount;
                    ide->CurrentHeadCount = (ide->RFile.Head & 15) + 1;
                    ide->CurrentCylinderCount = ide->SectorCount / (ide->CurrentHeadCount * ide->CurrentSectorsPerTrack);
                    AdjustCHSTranslation(ide);
                    CompleteCommand(ide);
                    break;
            }
            break;

        case 0xc6:  // set multiple mode
            switch(ide->ActiveCommandState) {
                case 1:
                    ide->RFile.Status |= IDEStatus_BSY;
                    ++(ide->ActiveCommandState);
                    ide->ActiveCommandNextTime = t + 500;
                    break;

                case 2:
                    // sector count must be a power of two and cannot be 0
                    if (ide->RFile.SectorCount >= 2 && !(ide->RFile.SectorCount & (ide->RFile.SectorCount - 1))) {
                        ide->SectorsPerBlock = ide->RFile.SectorCount;
                        CompleteCommand(ide);
                    } else
                        AbortCommand(ide, 0);
                    break;
            }
            break;

        case 0xec:  // identify drive
            switch(ide->ActiveCommandState) {
                case 1:
                    ide->RFile.Status |= IDEStatus_BSY;
                    ++(ide->ActiveCommandState);
                    ide->ActiveCommandNextTime = t + 10000;
                    break;

                case 2:
                    {
                        uint8_t *dst = ide->TransferBuffer;

                        // See ATA-1 Table 11 for format.
                        memset(dst, 0, 512);

                        // word 0: capabilities
                        dst[ 0*2+0] = 0x4c;     // soft sectored, not MFM encoded, fixed drive
                        dst[ 0*2+1] = 0x04;     // xfer >10Mbps

                        // word 1: cylinder count
                        dst[ 1*2+1] = (uint8_t)(ide->CylinderCount >> 8);
                        dst[ 1*2+0] = (uint8_t)ide->CylinderCount;    // cylinder count

                        // word 2: reserved
                        dst[ 2*2+0] = 0;            // reserved
                        dst[ 2*2+1] = 0;

                        // word 3: number of logical heads
                        // ATA-4 B.2.2 specifies the value of this parameter.
                        dst[ 3*2+0] = (uint8_t)ide->HeadCount;            // number of heads
                        dst[ 3*2+1] = 0;

                        // word 4: number of unformatted bytes per track
                        dst[ 4*2+0] = 0;
                        dst[ 4*2+1] = (uint8_t)(2 * ide->SectorsPerTrack);

                        // word 5: number of unformatted bytes per sector (ATA-1), retired (ATA-4)
                        dst[ 5*2+0] = 0;
                        dst[ 5*2+1] = 2;

                        // word 6: number of sectors per track (ATA-1), retired (ATA-4)
                        // ATA-4 B.2.3 specifies the value of this parameter.
                        dst[ 6*2+0] = (uint8_t)ide->SectorsPerTrack;      // sectors per track
                        dst[ 6*2+1] = 0;

                        // words 7-9: vendor unique (ATA-1), retired (ATA-4)

                        // words 10-19: serial number
                        char buf[40+1] = {};
                        sprintf(buf, "%010u", ide->Disk ? IMG_Get_Serial_Number(ide->Disk) : 0);

                        for(int i=0; i<20; ++i)
                            dst[10*2 + (i ^ 1)] = buf[i] ? (uint8_t)buf[i] : 0x20;

                        // word 20: buffer type (ATA-1), retired (ATA-4)
                        dst[20*2+1] = 0x00;
                        dst[20*2+0] = 0x03;     // dual ported w/ read caching

                        // word 21: buffer size (ATA-1), retired (ATA-4)
                        dst[21*2+1] = 0x00;
                        dst[21*2+0] = 0x00;     // not specified

                        // word 22: ECC bytes for read/write long commands (ATA-1), obsolete (ATA-4)
                        dst[22*2+1] = 0x00;
                        dst[22*2+0] = 0x04;

                        // words 23-26: firmware revision
                        dst[23*2+1] = '1';
                        dst[23*2+0] = '.';
                        dst[24*2+1] = '0';
                        dst[24*2+0] = ' ';
                        dst[25*2+1] = ' ';
                        dst[25*2+0] = ' ';
                        dst[26*2+1] = ' ';
                        dst[26*2+0] = ' ';

                        // words 27-46: model number (note that we need to byte swap)
                        sprintf(buf, "GENERIC %uM %s", (unsigned)ide->SectorCount >> 11, ide->FastDevice ? "SSD" : "HDD");
                        memset(&dst[27*2], ' ', 40);
                        memcpy(&dst[27*2], buf, strlen(buf));

                        for(int i=0; i<40; i+=2) {
                            uint8_t tmp;
                            tmp = dst[27*2+i];
                            dst[27*2+i] = dst[27*2+i+1];
                            dst[27*2+i+1] = tmp;
                        }
                        
                        // word 47
                        dst[47*2+0] = 0xFF;     // max sectors/interrupt
                        dst[47*2+1] = 0x80;

                        // word 48: reserved
                        // word 49: capabilities
                        dst[49*2+1] = 0x0F;
                        dst[49*2+0] = 0;        // capabilities (LBA supported, DMA supported)

                        // word 50: reserved (ATA-1), capabilities (ATA-4)
                        dst[50*2+1] = 0x40;
                        dst[50*2+0] = 0x00;

                        // word 51: PIO data transfer timing mode (ATA-1)
                        dst[51*2+1] = 2;
                        dst[51*2+0] = 0;        // PIO data transfer timing mode (PIO 2)

                        // word 52: DMA data transfer timing mode (ATA-1)
                        dst[52*2+1] = 0;
                        dst[52*2+0] = 0;        // DMA data transfer timing mode (DMA 0)

                        // word 53: misc
                        dst[53*2+1] = 0x00;
                        dst[53*2+0] = 0x03;     // words 54-58 are valid

                        // word 54: number of current logical cylinders
                        dst[54*2+1] = (uint8_t)(ide->CurrentCylinderCount >> 8);
                        dst[54*2+0] = (uint8_t)(ide->CurrentCylinderCount     );

                        // word 55: number of current logical heads
                        dst[55*2+1] = (uint8_t)(ide->CurrentHeadCount >> 8);
                        dst[55*2+0] = (uint8_t)(ide->CurrentHeadCount     );

                        // word 56: number of current logical sectors per track
                        dst[55*2+1] = (uint8_t)(ide->CurrentSectorsPerTrack >> 8);
                        dst[55*2+0] = (uint8_t)(ide->CurrentSectorsPerTrack     );

                        // words 57-58: current capacity in sectors
                        VDWriteUnalignedLEU32(&dst[57*2], ide->CurrentCylinderCount * ide->CurrentHeadCount * ide->CurrentSectorsPerTrack);

                        // word 59: multiple sector setting
                        dst[59*2+1] = ide->SectorsPerBlock ? 1 : 0;
                        dst[59*2+0] = ide->SectorsPerBlock;

                        // words 60-61: total number of user addressable LBA sectors (28-bit)
                        VDWriteUnalignedLEU32(&dst[60*2], ide->SectorCount < 0x0FFFFFFF ? ide->SectorCount : 0x0FFFFFFF);

                        // words 62-63: single/multiword DMA status
                        dst[62*2+1] = 0x01;     // DMA 0 active
                        dst[62*2+0] = 0x07;     // DMA 0-2 supported
                        dst[63*2+1] = 0x01;     // DMA 0 active
                        dst[63*2+0] = 0x07;     // DMA 0-2 supported

                        if (ide->FastDevice) {
                            // word 64: PIO transfer modes supported
                            dst[64*2+1] = 0x00;
                            dst[64*2+0] = 0x0F;     // PIO modes 3-6 supported

                            // word 65: minimum multiword DMA transfer cycle time per word
                            dst[65*2+1] = 0x00;
                            dst[65*2+0] = 0x50;     // 80ns rate (25MB/sec)

                            // word 66: recommended multiword DMA transfer cycle time
                            dst[66*2+1] = 0x00;
                            dst[66*2+0] = 0x50;     // 80ns rate (25MB/sec)

                            // word 67: minimum PIO transfer without flow control cycle time
                            dst[67*2+1] = 0x00;
                            dst[67*2+0] = 0x50;     // 80ns rate (25MB/sec)

                            // word 68: minimum PIO transfer with IORDY
                            dst[68*2+1] = 0x00;
                            dst[68*2+0] = 0x50;     // 80ns rate (25MB/sec)
                        } else {
                            // word 64: PIO transfer modes supported
                            dst[64*2+1] = 0x00;
                            dst[64*2+0] = 0x03;     // PIO modes 3-4 supported

                            // word 65: minimum multiword DMA transfer cycle time per word
                            dst[65*2+1] = 0x00;
                            dst[65*2+0] = 0x78;     // 120ns rate (16.7MB/sec)

                            // word 66: recommended multiword DMA transfer cycle time
                            dst[66*2+1] = 0x00;
                            dst[66*2+0] = 0x78;     // 120ns rate (16.7MB/sec)

                            // word 67: minimum PIO transfer without flow control cycle time
                            dst[67*2+1] = 0x00;
                            dst[67*2+0] = 0x78;     // 120ns rate (16.7MB/sec)

                            // word 68: minimum PIO transfer with IORDY
                            dst[68*2+1] = 0x00;
                            dst[68*2+0] = 0x78;     // 120ns rate (16.7MB/sec)
                        }

                        // words 100-103: total user addressable sectors in 48-bit LBA mode
                        VDWriteUnalignedLEU64(&dst[100*2], ide->SectorCount);

                        BeginReadTransfer(ide, 512);
                        ide->ActiveCommandState = 0;
                    }
                    break;

            }
            break;

        case 0xef:  // set features
            switch(ide->ActiveCommandState) {
                case 1:
                    ide->RFile.Status |= IDEStatus_BSY;
                    ++(ide->ActiveCommandState);
                    ide->ActiveCommandNextTime = t + 250;
                    break;

                case 2:
                    switch(ide->Features) {
                        case 0x01:      // enable 8-bit data transfers
                            ide->Transfer16Bit = FALSE;
                            CompleteCommand(ide);
                            break;

                        case 0x03:      // set transfer mode (based on sector count register)
                            switch(ide->RFile.SectorCount) {
                                case 0x00:  // PIO default mode
                                case 0x01:  // PIO default mode, disable IORDY
                                case 0x08:  // PIO mode 0
                                case 0x09:  // PIO mode 1
                                case 0x0A:  // PIO mode 2
                                case 0x0B:  // PIO mode 3
                                case 0x0C:  // PIO mode 4
                                case 0x20:  // DMA mode 0
                                case 0x21:  // DMA mode 1
                                case 0x22:  // DMA mode 2
                                    CompleteCommand(ide);
                                    break;

                                case 0x0D:  // PIO mode 5 (CF)
                                case 0x0E:  // PIO mode 6 (CF)
                                    if (ide->FastDevice) {
                                        CompleteCommand(ide);
                                        break;
                                    }

                                    // fall through

                                default:
                                    printf("Unsupported transfer mode: %02x\n", ide->RFile.SectorCount);
                                    AbortCommand(ide, 0);
                                    break;
                            }
                            break;

                        case 0x05:  // Enable advanced power management
                        case 0x0A:  // Enable CFA power mode 1
                            CompleteCommand(ide);
                            break;

                        case 0x81:      // disable 8-bit data transfers
                            ide->Transfer16Bit = TRUE;
                            CompleteCommand(ide);
                            break;

                        default:
                            printf("Unsupported set feature parameter: %02x\n", ide->Features);
                            AbortCommand(ide, 0);
                            break;
                    }
                    break;
            }
            break;

        default:
            printf("IDE: Unrecognized command $%02x.\n", ide->ActiveCommand);
            AbortCommand(ide, 0);
            break;
    }
}

void StartCommand(IDEEmu *ide, uint8_t cmd) {
    ide->RFile.Status &= ~IDEStatus_ERR;

    // BOGUS: This is unfortunately necessary to get FDISK.BAS to work, but it shouldn't
    // be necessary: ATA-4 7.15.6.6 ERR (Error) states that the ERR register shall
    // be ignored by the host when the ERR bit is 0.
    ide->RFile.Errors = 0;

    ide->ActiveCommand = cmd;
    ide->ActiveCommandState = 1;
    ide->ActiveCommandNextTime = GetTimeTicks();

    printf("Executing command: %02X %02X %02X %02X %02X %02X %02X %02X\n"
        , ide->Registers[0]
        , ide->Registers[1]
        , ide->Registers[2]
        , ide->Registers[3]
        , ide->Registers[4]
        , ide->Registers[5]
        , ide->Registers[6]
        , cmd
    );

    UpdateStatus(ide);
}

void BeginReadTransfer(IDEEmu *ide, uint32_t bytes) {
    ide->RFile.Status |= IDEStatus_DRQ;
    ide->RFile.Status &= ~IDEStatus_BSY;
    ide->TransferIndex = 0;
    ide->TransferLength = bytes;
    ide->TransferAsWrites = FALSE;
}

void BeginWriteTransfer(IDEEmu *ide, uint32_t bytes) {
    ide->RFile.Status |= IDEStatus_DRQ;
    ide->RFile.Status &= ~IDEStatus_BSY;
    ide->TransferIndex = 0;
    ide->TransferLength = bytes;
    ide->TransferAsWrites = TRUE;
}

void CompleteCommand(IDEEmu *ide) {
    ide->RFile.Status &= ~IDEStatus_BSY;
    ide->RFile.Status &= ~IDEStatus_DRQ;
    ide->ActiveCommand = 0;
    ide->ActiveCommandState = 0;
    ide->WriteInProgress = FALSE;
}

void AbortCommand(IDEEmu *ide, uint8_t error) {
    ide->RFile.Status &= ~IDEStatus_BSY;
    ide->RFile.Status &= ~IDEStatus_DRQ;
    ide->RFile.Status |= IDEStatus_ERR;
    ide->RFile.Errors = error | IDEError_ABRT;
    ide->ActiveCommand = 0;
    ide->ActiveCommandState = 0;
    ide->WriteInProgress = FALSE;
}

int ReadLBA(IDEEmu *ide, uint32_t *lba) {
    if (ide->RFile.Head & 0x40) {
        // LBA mode
        *lba = ((uint32_t)(ide->RFile.Head & 0x0f) << 24)
             + ((uint32_t)ide->RFile.CylinderHigh << 16)
             + ((uint32_t)ide->RFile.CylinderLow << 8)
             + (uint32_t)ide->RFile.SectorNumber;

        if (*lba >= ide->SectorCount) {
            printf("IDE: Invalid LBA %u >= %u\n", *lba, ide->SectorCount);
            return FALSE;
        }

        return TRUE;
    } else {
        // CHS mode
        uint32_t head = ide->RFile.Head & 15;
        uint32_t sector = ide->RFile.SectorNumber;
        uint32_t cylinder = ((uint32_t)ide->RFile.CylinderHigh << 8) + ide->RFile.CylinderLow;

        if (!sector || sector > ide->CurrentSectorsPerTrack) {
            printf("IDE: Invalid CHS %u/%u/%u (bad sector number)\n", cylinder, head, sector);
            return FALSE;
        }

        lba = (sector - 1) + (cylinder*ide->CurrentHeadCount + head)*ide->CurrentSectorsPerTrack;
        if (*lba >= ide->SectorCount) {
            printf("IDE: Invalid CHS %u/%u/%u (beyond total sector count of %u)\n", cylinder, head, sector, ide->SectorCount);
            return FALSE;
        }

        return TRUE;
    }
}

void WriteLBA(IDEEmu *ide, uint32_t lba) {
    if (ide->RFile.Head & 0x40) {
        // LBA mode
        ide->RFile.Head = (ide->RFile.Head & 0xf0) + ((lba >> 24) & 0x0f);
        ide->RFile.CylinderHigh = (uint8_t)(lba >> 16);
        ide->RFile.CylinderLow = (uint8_t)(lba >> 8);
        ide->RFile.SectorNumber = (uint8_t)lba;
    } else if (ide->CurrentSectorsPerTrack && ide->CurrentHeadCount) {
        // CHS mode
        uint32_t track = lba / ide->CurrentSectorsPerTrack;
        uint32_t sector = lba % ide->CurrentSectorsPerTrack;
        uint32_t cylinder = track / ide->CurrentHeadCount;
        uint32_t head = track % ide->CurrentHeadCount;

        ide->RFile.Head = (ide->RFile.Head & 0xf0) + head;
        ide->RFile.CylinderHigh = (uint8_t)(cylinder >> 8);
        ide->RFile.CylinderLow = (uint8_t)cylinder;
        ide->RFile.SectorNumber = sector + 1;
    } else {
        // uh...

        ide->RFile.Head = (ide->RFile.Head & 0xf0);
        ide->RFile.CylinderHigh = 0;
        ide->RFile.CylinderLow = 0;
        ide->RFile.SectorNumber = 1;
    }
}

void ResetCHSTranslation(IDEEmu *ide) {
    ide->CurrentCylinderCount = ide->CylinderCount;
    ide->CurrentSectorsPerTrack = ide->SectorsPerTrack;
    ide->CurrentHeadCount = ide->HeadCount;

    if (ide->CurrentSectorsPerTrack > 63) {
        ide->CurrentSectorsPerTrack = 63;
        ide->CurrentCylinderCount = ide->SectorCount / (ide->CurrentHeadCount * 63);
    }

    AdjustCHSTranslation(ide);
}

void AdjustCHSTranslation(IDEEmu *ide) {
    if (ide->CurrentCylinderCount > 65535)
        ide->CurrentCylinderCount = 65535;

    if (ide->SectorCount >= 16514064) {
        uint32_t limitCyl = 16514064 / (ide->CurrentHeadCount * ide->CurrentSectorsPerTrack);

        if (ide->CurrentCylinderCount > limitCyl)
            ide->CurrentCylinderCount = limitCyl;
    }
}
