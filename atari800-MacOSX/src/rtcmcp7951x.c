/* rtcmcp7951x.c -
   Emulation of SPI RTC for the Atari
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   Copyright (C) 2021 Mark Grebe

   Ported and Adapted from Altirra
   Copyright (C) 2009-2020 Avery Lee
*/

#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include "atari.h"
#include "rtcmcp7951x.h"

typedef struct mcp7951x {
    UBYTE   State;
    UBYTE   Address;
    UBYTE   Status;
    UBYTE   UnlockState;
    int     WriteEnabled;
    UBYTE RAM[0x60];
    UBYTE EEPROM[0x100];
    UBYTE IDEEPROM[0x10];
} MCP7951X;

static UBYTE ToBCD(UBYTE v) {
		return ((v / 10) << 4) + (v % 10);
}

static void UpdateClock(void *rtc);

void *MCP7951X_Init() {
    MCP7951X *mcpRtc = (MCP7951X *) malloc(sizeof(MCP7951X));
    memset(mcpRtc, 0, sizeof(MCP7951X));

	MCP7951X_ColdReset((void *) mcpRtc);
    return mcpRtc;
}

void MCP7951X_Exit(void *rtc) {
    free(rtc);
}

void MCP7951X_ColdReset(void *rtc) {
}

void MCP7951X_Load(void *rtc, MCP7951X_NVState *state) {
    MCP7951X *mcpRtc = (MCP7951X *)rtc;
    memcpy(mcpRtc->RAM, state->RAM, sizeof( mcpRtc->RAM));
    memcpy(mcpRtc->EEPROM, state->EEPROM, sizeof(mcpRtc->EEPROM));
    memcpy(mcpRtc->IDEEPROM, state->IDEEPROM, sizeof(mcpRtc->IDEEPROM));
}

void MCP7951X_Save(void *rtc, MCP7951X_NVState *state) {
    MCP7951X *mcpRtc = (MCP7951X *)rtc;
    memcpy(state->RAM, mcpRtc->RAM, sizeof(mcpRtc->RAM));
    memcpy(state->EEPROM, mcpRtc->EEPROM, sizeof(mcpRtc->EEPROM));
    memcpy(state->IDEEPROM, mcpRtc->IDEEPROM, sizeof(mcpRtc->IDEEPROM));
}

void MCP7951X_Reselect(void *rtc) {
    MCP7951X *mcpRtc = (MCP7951X *)rtc;
    mcpRtc->State = 0;
}

UBYTE MCP7951X_Transfer(void *rtc, UBYTE dataIn) {
    MCP7951X *mcpRtc = (MCP7951X *)rtc;
    UBYTE readData = 0xFF;

    switch(mcpRtc->State) {
        case 0:     // initial state - process command
            mcpRtc->State = 1;

            // if the command isn't UNLOCK or IDWRITE, reset the unlock state
            if (dataIn != 0x32 && dataIn != 0x14)
                mcpRtc->UnlockState = 0;

            switch(dataIn) {
                case 0x03: mcpRtc->State = 6;  break;  // EEREAD
                case 0x02: mcpRtc->State = 1;  break;  // EEWRITE
                case 0x04: mcpRtc->WriteEnabled = FALSE; break;  // EEWRDI
                case 0x06: mcpRtc->WriteEnabled = TRUE; break;   // EEWREN
                case 0x05: mcpRtc->State = 11; break;  // SRREAD
                case 0x01: mcpRtc->State = 12; break;  // SRWRITE
                case 0x13: mcpRtc->State = 2;  break;  // READ
                case 0x12: mcpRtc->State = 4;  break;  // WRITE
                case 0x14: mcpRtc->State = 17; break;  // UNLOCK
                case 0x32: mcpRtc->State = 15; break;  // IDWRITE
                case 0x33: mcpRtc->State = 13; break;  // IDREAD
                case 0x54: mcpRtc->State = 10; break;  // CLRRAM
            }
            break;

        case 1:     // jam state -- discard and ignore all further data until -CS
            break;

        case 2:     // READ address state
            mcpRtc->Address = dataIn;

            if (mcpRtc->Address < 0x60) {
                if (mcpRtc->Address < 0x20)
                    UpdateClock(rtc);

                mcpRtc->State = 3;
            } else
                mcpRtc->State = 1;
            break;

        case 3:     // READ data state
            readData = mcpRtc->RAM[mcpRtc->Address];

            // wrap within RTCC or SRAM areas
            mcpRtc->Address = mcpRtc->Address + 1;

            if (mcpRtc->Address == 0x20)
                mcpRtc->Address = 0x00;
            else if (mcpRtc->Address == 0x60)
                mcpRtc->Address = 0x20;
            break;

        case 4:     // WRITE address state
            mcpRtc->Address = dataIn;

            if (mcpRtc->Address < 0x60)
                mcpRtc->State = 5;
            else
                mcpRtc->State = 1;
            break;

        case 5:     // WRITE data state
            mcpRtc->RAM[mcpRtc->Address] = dataIn;

            // wrap within RTCC or SRAM areas
            mcpRtc->Address = mcpRtc->Address + 1;

            if (mcpRtc->Address == 0x20)
                mcpRtc->Address = 0x00;
            else if (mcpRtc->Address == 0x60)
                mcpRtc->Address = 0x20;
            break;

        case 6:     // EEREAD address state
            mcpRtc->Address = dataIn;
            mcpRtc->State = 7;
            break;

        case 7:     // EEREAD data state
            readData = mcpRtc->EEPROM[mcpRtc->Address];

            // address autoincrements within whole EEPROM
            mcpRtc->Address = (mcpRtc->Address + 1) & 0xFF;
            break;

        case 8:     // EEWRITE address state
            mcpRtc->Address = dataIn;

            if (mcpRtc->WriteEnabled)
                mcpRtc->State = 9;
            else
                mcpRtc->State = 1;
            break;

        case 9:     // EEWRITE data state
            mcpRtc->EEPROM[mcpRtc->Address] = dataIn;

            // address autoincrements but wraps within a 'page' of 8 bytes
            mcpRtc->Address = (mcpRtc->Address & 0xF8) + ((mcpRtc->Address + 1) & 7);

            // reset WEL on successful EEWRITE
            mcpRtc->WriteEnabled = FALSE;
            break;

        case 10:    // CLRRAM dummy data state
            memset(mcpRtc->RAM + 0x20, 0, 0x40);
            break;

        case 11:    // SRREAD data state
            readData = mcpRtc->Status;

            // merge WEL
            if (mcpRtc->UnlockState)
                readData = readData + 0x02;
            break;

        case 12:    // SRWRITE data state
            // only bits 2-3 can be changed
            mcpRtc->Status = dataIn & 0x0C;

            // reset WEL on successful SRWRITE
            mcpRtc->WriteEnabled = FALSE;
            break;

        case 13:    // IDREAD address state
            mcpRtc->Address = dataIn;
            break;

        case 14:    // IDREAD data state
            readData = mcpRtc->EEPROM[mcpRtc->Address];
            mcpRtc->Address = (mcpRtc->Address + 1) & 0x0F;
            break;

        case 15:    // IDWRITE address state
            mcpRtc->Address = dataIn;

            if (mcpRtc->UnlockState == 2 && mcpRtc->UnlockState)
                mcpRtc->State = 16;
            else
                mcpRtc->State = 1;
            break;

        case 16:    // IDWRITE data state
            mcpRtc->EEPROM[mcpRtc->Address] = dataIn;

            // autoincrement within 8-byte page
            mcpRtc->Address = (mcpRtc->Address & 0x08) + ((mcpRtc->Address + 1) & 0x07);

            // reset WEL on successful IDWRITE
            mcpRtc->WriteEnabled = FALSE;
            break;

        case 17:    // UNLOCK data state
            if (mcpRtc->UnlockState == 0) {
                if (dataIn == 0x55)
                    mcpRtc->UnlockState = 1;
            } else if (mcpRtc->UnlockState == 1) {
                if (dataIn == 0xAA)
                    mcpRtc->UnlockState = 2;
                else
                    mcpRtc->UnlockState = 0;
            } else {
                mcpRtc->UnlockState = 0;
            }
            break;
    }

    return readData;
}

static void UpdateClock(void *rtc) {
    MCP7951X *mcpRtc = (MCP7951X *)rtc;
    // copy clock to RAM
    time_t t;
    struct timeval tv;

    time(&t);
    const struct tm *p = localtime(&t);
    gettimeofday(&tv, NULL);


    // leap years are divisible by 4, but not divisible by 100 unless divisible
    // by 400
    int twoDigitYear = p->tm_year % 100;
    int isLeapYear = (twoDigitYear || (p->tm_year % 400) == 0) && !(twoDigitYear & 3);

    // centiseconds (0-99)
    mcpRtc->RAM[0] = ToBCD(tv.tv_usec / 10000);

    // seconds (0-59), start oscillator bit
    mcpRtc->RAM[1] = ToBCD(p->tm_sec) + 0x80;

    // minutes (0-59)
    mcpRtc->RAM[2] = ToBCD(p->tm_min);

    // hours (0-23)
    mcpRtc->RAM[3] = ToBCD(p->tm_hour);

    // weekday (1-7) + OSCRUN[7] + PWRFAIL[6] + VBATEN[5]
    mcpRtc->RAM[4] = 0x29 + p->tm_wday + 1;

    // day (1-31)
    mcpRtc->RAM[5] = ToBCD(p->tm_mday);

    // month (1-12), leap-year bit
    mcpRtc->RAM[6] = ToBCD(p->tm_mon + 1) + (isLeapYear ? 0x20 : 0);

    // year (0-99)
    mcpRtc->RAM[7] = ToBCD(twoDigitYear);
}
