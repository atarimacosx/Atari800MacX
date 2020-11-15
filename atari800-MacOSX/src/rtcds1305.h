/* rtcds1305.h -
   Emulation of SPI RTC for the Atari
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   Copyright (C) 2020 Mark Grebe

   Ported and Adapted from Altirra
   Copyright (C) 2008-2011 Avery Lee
*/

void *CDS1305_Init(void);
void CDS1305_Exit(void *rtc);
void CDS1305_ColdReset(void *rtc);
void CDS1305_Load(void *rtc, const UBYTE *data);
void CDS1305_Save(void *rtc, UBYTE *data);
int CDS1305_ReadState(void *rtc);
void CDS1305_WriteState(void *rtc, int ce, int clock, int data);
