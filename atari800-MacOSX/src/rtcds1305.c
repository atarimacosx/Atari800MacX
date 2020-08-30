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

#include <string.h>
#include <time.h>
#include "atari.h"
#include "rtcds1305.h"

static UBYTE phase;
static UBYTE address;
static UBYTE value;
static int   state;
static int   chipEnable;
static int   SPIInitialClock;
static int   SPIClock;
static UBYTE clockRAM[0x12];
static UBYTE userRAM[0x60];

static void ReadRegister();
static void WriteRegister();
static void IncrementAddressRegister();
static void UpdateClock();

static UBYTE ToBCD(UBYTE v) {
    return ((v / 10) << 4) + (v % 10);
}

void CDS1305_Init() {
    memset(clockRAM, 0, sizeof clockRAM);
    memset(userRAM, 0, sizeof userRAM);

    CDS1305_ColdReset();
}

void CDS1305_ColdReset() {
    phase = 0;
    address = 0;
    state = TRUE;
    chipEnable = FALSE;
    SPIClock = FALSE;
}

void CDS1305_Load(const UBYTE *data) {
    memcpy(clockRAM, data, 0x12);
    memcpy(userRAM, data + 0x12, 0x60);
}

void CDS1305_Save(UBYTE *data)  {
    memcpy(data, clockRAM, 0x12);
    memcpy(data + 0x12, userRAM, 0x60);
}

int CDS1305_ReadState() {
    return state;
}

void CDS1305_WriteState(int ce, int clock, int data) {
    if (ce != chipEnable) {
        chipEnable = ce;

        if (!chipEnable) {
            phase = 0;
            state = TRUE;
        } else {
            SPIClock = clock;
            SPIInitialClock = clock;
        }
    }

    if (chipEnable && SPIClock != clock) {
        SPIClock = clock;

        if (clock != SPIInitialClock) {   // read (0 -> 1 clock transition)
            if (phase >= 8 && address < 0x80) {
                state = (value >= 0x80);
                value += value;

                if (++phase == 16) {
                    phase = 8;

                    ReadRegister();
                    IncrementAddressRegister();
                }
            }
        } else {        // write (1 -> 0 clock transition)
            if (phase < 8) {
                // shifting in address, MSB first
                address += address;
                if (data)
                    ++address;

                if (++phase == 8) {
                    if (address < 0x80) {
                        if (address < 0x20)
                            UpdateClock();

                        ReadRegister();
                        IncrementAddressRegister();
                    }
                }
            } else if (address >= 0x80) {
                // shifting in data, MSB first
                value += value;
                if (data)
                    ++value;

                // check if we're done with a byte
                if (++phase == 16) {
                    phase = 8;

                    WriteRegister();
                    IncrementAddressRegister();
                }
            }
        }
    }
}

static void ReadRegister() {
    if (address < 0x12)
        value = clockRAM[address];
    else if (address < 0x20)
        value = 0;
    else if (address < 0x80)
        value = userRAM[address - 0x20];
    else
        value = 0xFF;
}

static void WriteRegister() {

    if (address < 0x92) {
        static const UBYTE registerWriteMasks[0x12]={
            0x7F, 0x7F, 0x7F, 0x0F, 0x3F, 0x3F, 0xFF, 0xFF,
            0xFF, 0xFF, 0x8F, 0xFF, 0xFF, 0xFF, 0x8F, 0x87,
            0x00, 0xFF
        };

        clockRAM[address - 0x80] = value & registerWriteMasks[address - 0x80];
    } else if (address >= 0xA0)
        userRAM[address - 0xA0] = value;
}

static void IncrementAddressRegister() {
    ++address;

    UBYTE addr7 = address & 0x7F;
    if (addr7 == 0x20)
        address -= 0x20;
    else if (addr7 == 0)
        address -= 0x60;
}

static void UpdateClock() {
    // copy clock to RAM
    time_t t;

    time(&t);
    const struct tm *p = localtime(&t);

    clockRAM[0] = ToBCD(p->tm_sec);
    clockRAM[1] = ToBCD(p->tm_min);

    if (clockRAM[2] & 0x40) {
        // 12 hour mode
        UBYTE hr = p->tm_hour;

        if (hr >= 12) {
            // PM
            clockRAM[2] = ToBCD(hr - 11) + 0x20;
        } else {
            // AM
            clockRAM[2] = ToBCD(hr + 1);
        }
    } else {
        // 24 hour mode
        clockRAM[2] = ToBCD(p->tm_hour);
    }

    clockRAM[3] = p->tm_wday + 1;
    clockRAM[4] = ToBCD(p->tm_mday);
    clockRAM[5] = ToBCD(p->tm_mon + 1);
    clockRAM[6] = ToBCD(p->tm_year % 100);
    clockRAM[7] = ToBCD(p->tm_mday);
}
