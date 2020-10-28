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
#include <stdlib.h>
#include "atari.h"
#include "rtcds1305.h"

typedef struct rtc {
    UBYTE phase;
    UBYTE address;
    UBYTE value;
    int   state;
    int   chipEnable;
    int   SPIInitialClock;
    int   SPIClock;
    UBYTE clockRAM[0x12];
    UBYTE userRAM[0x60];
} RTC;

static void ReadRegister(RTC *rtc);
static void WriteRegister(RTC *rtc);
static void IncrementAddressRegister(RTC *rtc);
static void UpdateClock(RTC *rtc);

static UBYTE ToBCD(UBYTE v) {
    return ((v / 10) << 4) + (v % 10);
}

void *CDS1305_Init() {
    RTC *rtc = malloc(sizeof(RTC));
    memset(rtc->clockRAM, 0, sizeof (rtc->clockRAM));
    memset(rtc->userRAM, 0, sizeof (rtc->userRAM));

    CDS1305_ColdReset(rtc);
    return ( (void *) rtc);
}

void CDS1305_Exit(void *rtc) {
    if (rtc)
        free(rtc);
}

void CDS1305_ColdReset(void *rtc) {
    RTC *thisRTC = (RTC *) rtc;
    thisRTC->phase = 0;
    thisRTC->address = 0;
    thisRTC->state = TRUE;
    thisRTC->chipEnable = FALSE;
    thisRTC->SPIClock = FALSE;
}

void CDS1305_Load(void *rtc, const UBYTE *data) {
    RTC *thisRTC = (RTC *) rtc;
    memcpy(thisRTC->clockRAM, data, 0x12);
    memcpy(thisRTC->userRAM, data + 0x12, 0x60);
}

void CDS1305_Save(void *rtc, UBYTE *data)  {
    RTC *thisRTC = (RTC *) rtc;
    memcpy(data, thisRTC->clockRAM, 0x12);
    memcpy(data + 0x12, thisRTC->userRAM, 0x60);
}

int CDS1305_ReadState(void *rtc) {
    RTC *thisRTC = (RTC *) rtc;
    return thisRTC->state;
}

void CDS1305_WriteState(void *rtc, int ce, int clock, int data) {
    RTC *thisRTC = (RTC *) rtc;
    if (ce != thisRTC->chipEnable) {
        thisRTC->chipEnable = ce;

        if (!thisRTC->chipEnable) {
            thisRTC->phase = 0;
            thisRTC->state = TRUE;
        } else {
            thisRTC->SPIClock = clock;
            thisRTC->SPIInitialClock = clock;
        }
    }

    if (thisRTC->chipEnable && thisRTC->SPIClock != clock) {
        thisRTC->SPIClock = clock;

        if (clock != thisRTC->SPIInitialClock) {   // read (0 -> 1 clock transition)
            if (thisRTC->phase >= 8 && thisRTC->address < 0x80) {
                thisRTC->state = (thisRTC->value >= 0x80);
                thisRTC->value += thisRTC->value;

                if (++thisRTC->phase == 16) {
                    thisRTC->phase = 8;

                    ReadRegister(thisRTC);
                    IncrementAddressRegister(thisRTC);
                }
            }
        } else {        // write (1 -> 0 clock transition)
            if (thisRTC->phase < 8) {
                // shifting in address, MSB first
                thisRTC->address += thisRTC->address;
                if (data)
                    ++thisRTC->address;

                if (++thisRTC->phase == 8) {
                    if (thisRTC->address < 0x80) {
                        if (thisRTC->address < 0x20)
                            UpdateClock(thisRTC);

                        ReadRegister(thisRTC);
                        IncrementAddressRegister(thisRTC);
                    }
                }
            } else if (thisRTC->address >= 0x80) {
                // shifting in data, MSB first
                thisRTC->value += thisRTC->value;
                if (data)
                    ++thisRTC->value;

                // check if we're done with a byte
                if (++thisRTC->phase == 16) {
                    thisRTC->phase = 8;

                    WriteRegister(thisRTC);
                    IncrementAddressRegister(thisRTC);
                }
            }
        }
    }
}

static void ReadRegister(RTC *thisRTC) {
    if (thisRTC->address < 0x12)
        thisRTC->value = thisRTC->clockRAM[thisRTC->address];
    else if (thisRTC->address < 0x20)
        thisRTC->value = 0;
    else if (thisRTC->address < 0x80)
        thisRTC->value = thisRTC->userRAM[thisRTC->address - 0x20];
    else
        thisRTC->value = 0xFF;
}

static void WriteRegister(RTC *thisRTC) {

    if (thisRTC->address < 0x92) {
        static const UBYTE registerWriteMasks[0x12]={
            0x7F, 0x7F, 0x7F, 0x0F, 0x3F, 0x3F, 0xFF, 0xFF,
            0xFF, 0xFF, 0x8F, 0xFF, 0xFF, 0xFF, 0x8F, 0x87,
            0x00, 0xFF
        };

        thisRTC->clockRAM[thisRTC->address - 0x80] = thisRTC->value & registerWriteMasks[thisRTC->address - 0x80];
    } else if (thisRTC->address >= 0xA0)
        thisRTC->userRAM[thisRTC->address - 0xA0] = thisRTC->value;
}

static void IncrementAddressRegister(RTC *thisRTC) {
    ++thisRTC->address;

    UBYTE addr7 = thisRTC->address & 0x7F;
    if (addr7 == 0x20)
        thisRTC->address -= 0x20;
    else if (addr7 == 0)
        thisRTC->address -= 0x60;
}

static void UpdateClock(RTC *thisRTC) {
    // copy clock to RAM
    time_t t;

    time(&t);
    const struct tm *p = localtime(&t);

    thisRTC->clockRAM[0] = ToBCD(p->tm_sec);
    thisRTC->clockRAM[1] = ToBCD(p->tm_min);

    if (thisRTC->clockRAM[2] & 0x40) {
        // 12 hour mode
        UBYTE hr = p->tm_hour;

        if (hr >= 12) {
            // PM
            thisRTC->clockRAM[2] = ToBCD(hr - 11) + 0x20;
        } else {
            // AM
            thisRTC->clockRAM[2] = ToBCD(hr + 1);
        }
    } else {
        // 24 hour mode
        thisRTC->clockRAM[2] = ToBCD(p->tm_hour);
    }

    thisRTC->clockRAM[3] = p->tm_wday + 1;
    thisRTC->clockRAM[4] = ToBCD(p->tm_mday);
    thisRTC->clockRAM[5] = ToBCD(p->tm_mon + 1);
    thisRTC->clockRAM[6] = ToBCD(p->tm_year % 100);
    thisRTC->clockRAM[7] = ToBCD(p->tm_mday);
}
