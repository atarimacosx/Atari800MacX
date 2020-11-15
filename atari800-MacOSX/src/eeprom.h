/* eeprom.h -
   Emulation of SPI EEPROM for the Atari
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   Copyright (C) 2020 Mark Grebe

   Ported and Adapted from Altirra
   Copyright (C) 2009-2014 Avery Lee
*/

#ifndef eeprom_h
#define eeprom_h

void EEPROM_Init(void);
void EEPROM_Cold_Reset(void);
void EEPROM_Load(const UBYTE *data);
void EEPROM_Save(UBYTE *data);
int  EEPROM_State(void);
int  EEPROM_Read_State(void);
void EEPROM_Write_State(int chipEnable, int clock, int data);

#endif /* eeprom_h */
