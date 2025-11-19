/* maxflash.h -
   Emulation of SIC! and SIC!+ cartridges for the Atari
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   Copyright (C) 2025 Mark Grebe

   Ported and Adapted from Altirra
   Copyright (C) 2008-2011 Avery Lee
*/

#ifndef sic_h
#define sic_h

void  SIC_Init(int type, unsigned char *image, int size);
void  SIC_Shutdown(void);
void  SIC_Cold_Reset(void);
UBYTE SIC_Read_Byte(UWORD address);
void  SIC_Write_Byte(UWORD address, UBYTE value);

#endif /* sic_h */
