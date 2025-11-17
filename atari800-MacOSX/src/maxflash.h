/* maxflash.h -
   Emulation of MaxFlash cartridges for the Atari
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   Copyright (C) 2025 Mark Grebe

   Ported and Adapted from Altirra
   Copyright (C) 2008-2011 Avery Lee
*/

#ifndef maxflash_h
#define maxflash_h

void  MAXFLASH_Init(int type, unsigned char *image, int size);
void  MAXFLASH_Shutdown(void);
void  MAXFLASH_Cold_Reset(void);
UBYTE MAXFLASH_Read_Byte(UWORD address);
void  MAXFLASH_Write_Byte(UWORD address, UBYTE value);

#endif /* maxflash_h */
