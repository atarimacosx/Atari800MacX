//	Altirra - Atari 800/800XL/5200 emulator
//	Copyright (C) 2008-2011 Avery Lee
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <stdint.h>

enum FlashType {
	Flash_TypeAm29F010,	// AMD 128K x 8-bit
	Flash_TypeAm29F010B,	// AMD 128K x 8-bit
	Flash_TypeAm29F040,	// AMD 512K x 8-bit
	Flash_TypeAm29F040B,	// AMD 512K x 8-bit
	Flash_TypeAm29F016D,	// AMD 2M x 8-bit
	Flash_TypeAm29F032B,	// AMD 4M x 8-bit
	Flash_TypeAT29C010A,	// Atmel 128K x 8-bit
	Flash_TypeAT29C040,	// Atmel 512K x 8-bit
	Flash_TypeSST39SF040,// SST 512K x 8-bit
	Flash_TypeA29040,	// Amic 512K x 8-bit
	Flash_TypeS29GL01P,	// Spansion 128M x 8-bit, 90nm (byte mode)
	Flash_TypeS29GL512P,	// Spansion 64M x 8-bit, 90nm (byte mode)
	Flash_TypeS29GL256P,	// Spansion 32M x 8-bit, 90nm (byte mode)
	Flash_TypeBM29F040,	// BRIGHT 512K x 8-bit
	Flash_TypeM29F010B, 	// STMicroelectronics 128K x 8-bit (then Numonyx, now Micron)
	Flash_TypeHY29F040A,	// Hynix HY29F040A 512K x 8-bit
    Flash_TypeM29W800DT,    // Numonyx 1M x 8-bit, top device
    Flash_TypeMX29LV640DT,// Macronyx 1M x 8-bit, top device
};

enum ReadMode {
    ReadMode_Normal,
    ReadMode_Autoselect,
    ReadMode_WriteStatusPending,
    ReadMode_SectorEraseStatus
};

typedef struct flashEmu {
    uint8_t *Memory;
    int      FlashType;
    int      ReadMode;
    int      CommandPhase;
    int      Dirty;
    int      WriteActivity;
    int      AtmelSDP;
    int      A11Unlock;
    int      A12iUnlock;
    int      SectorEraseTimeoutCycles;
    uint8_t  ToggleBits;
    uint32_t WriteSector;
    uint8_t  WriteBufferData[32];
    uint8_t  PendingWriteCount;
    uint32_t PendingWriteAddress;
} FlashEmu;

FlashEmu *Flash_Init(void *mem, int type);
void Flash_Shutdown(FlashEmu *flash);
void Flash_Cold_Reset(FlashEmu *Flash);
int Flash_Read_Byte(FlashEmu *flash, uint32_t address, uint8_t *data);
int Flash_Debug_Read_Byte(FlashEmu *flash, uint32_t address, uint8_t *data);
int Flash_Write_Byte(FlashEmu *flash, uint32_t address, uint8_t value);
int Flash_Is_Control_Read_Enabled(FlashEmu *flash);




