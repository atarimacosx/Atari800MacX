#ifndef IDE_H_
#define IDE_H_

#include <stdint.h>
#include "atari.h"
extern int IDE_enabled;

#define MaxSectorTransferCount 256
#define TransferBufferSize     (512 * MaxSectorTransferCount)

enum {
    IDEStatus_BSY    = 0x80,     // busy
    IDEStatus_DRDY   = 0x40,     // drive ready
    IDEStatus_DWF    = 0x20,     // drive write fault
    IDEStatus_DSC    = 0x10,     // drive seek complete
    IDEStatus_DRQ    = 0x08,     // data request
    IDEStatus_CORR   = 0x04,     // corrected data
    IDEStatus_IDX    = 0x02,     // index
    IDEStatus_ERR    = 0x01      // error
};

enum {
    IDEError_BBK     = 0x80,     // bad block detected
    IDEError_UNC     = 0x40,     // uncorrectable data error
    IDEError_MC      = 0x20,     // media changed
    IDEError_IDNF    = 0x10,     // ID not found
    IDEError_MCR     = 0x08,     // media change request
    IDEError_ABRT    = 0x04,     // aborted command
    IDEError_TK0NF   = 0x02,     // track 0 not found
    IDEError_AMNF    = 0x01      // address mark not found
};

// These control the amount of time that BSY is asserted during a sector
// read or write.
const uint32_t IODelayFast = 100;    //
const uint32_t IODelaySlow = 10000;  // ~5.5ms

typedef struct r_file {
        uint8_t Data;
        uint8_t Errors;
        uint8_t SectorCount;
        uint8_t SectorNumber;
        uint8_t CylinderLow;
        uint8_t CylinderHigh;
        uint8_t Head;
        uint8_t Status;
} REGFile;

typedef struct IDE_Emu {
    union {
        uint8_t   Registers[8];
        REGFile   RFile;
    };

    uint8_t Features;
    uint32_t SectorCount;
    uint32_t SectorsPerTrack;
    uint32_t HeadCount;
    uint32_t CylinderCount;
    uint32_t CurrentSectorsPerTrack;
    uint32_t CurrentHeadCount;
    uint32_t CurrentCylinderCount;
    uint32_t SectorsPerBlock;
    uint32_t IODelaySetting;
    uint32_t TransferIndex;
    uint32_t TransferLength;
    uint32_t TransferSectorCount;
    uint32_t TransferLBA;
    uint32_t ActiveCommandNextTime;
    uint8_t ActiveCommand;
    uint8_t ActiveCommandState;
    int TransferAsWrites;
    int Transfer16Bit;
    int WriteEnabled;
    int WriteInProgress;
    int IsSingle;
    int IsSlave;
    int FastDevice;
    int HardwareReset;
    int SoftwareReset;

    uint8_t TransferBuffer[TransferBufferSize];
    void *Disk;

} IDEEmu;

void IDE_Close_Image(IDEEmu *ide);
void IDE_Open_Image(IDEEmu *ide, char *filename);
void IDE_Set_Reset(IDEEmu *ide, int asserted);
uint32_t IDE_Read_Data_Latch(IDEEmu *ide, int advance);
void IDE_Write_Data_Latch(IDEEmu *ide, uint8_t lo, uint8_t hi);
uint8_t IDE_Debug_Read_Byte(IDEEmu *ide, uint8_t address);
uint8_t IDE_Read_Byte(IDEEmu *ide, uint8_t address);
void IDE_Write_Byte(IDEEmu *ide, uint8_t address, uint8_t value);
uint8_t IDE_Read_Byte_Alt(IDEEmu *ide, uint8_t address);
void IDE_Write_Byte_Alt(IDEEmu *ide, uint8_t address, uint8_t value);
int IDE_Debug_Read_Sector(IDEEmu *ide, uint32_t lba, void *dst);
int IDE_Debug_Write_Sector(IDEEmu *ide, uint32_t lba, const void *dst);

#endif
