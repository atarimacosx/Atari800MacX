/* rtcmcp7951x.h -
   Emulation of SPI RTC for the Atari
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   Copyright (C) 2021 Mark Grebe

   Ported and Adapted from Altirra
   Copyright (C) 2009-2020 Avery Lee
*/

typedef struct mcp7951X_nvstate {
    UBYTE RAM[0x60];
    UBYTE EEPROM[0x100];
    UBYTE IDEEPROM[0x10];
} MCP7951X_NVState;

void *MCP7951X_Init(void);
void MCP7951X_Exit(void *rtc);
void MCP7951X_ColdReset(void *rtc);
void MCP7951X_Load(void *rtc, MCP7951X_NVState *state);
void MCP7951X_Save(void *rtc, MCP7951X_NVState *state);
void MCP7951X_Reselect(void *rtc);
UBYTE MCP7951X_Transfer(void *rtc, UBYTE dataIn);
