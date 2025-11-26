/* maxflash.h -
   Emulation of MultiCart cartridges for the Atari
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   Copyright (C) 2025 Mark Grebe

   Ported and Adapted from Altirra
   Copyright (C) 2008-2011 Avery Lee
*/

#ifndef megacart_h
#define megacart_h

void  MEGACART_Init(int type, unsigned char *image, int size);
void  MEGACART_Shutdown(void);
int MEGACART_Is_Dirty(void);
void  MEGACART_Cold_Reset(void);
UBYTE MEGACART_Read_Byte(UWORD address);
void  MEGACART_Write_Byte(UWORD address, UBYTE value);
void MEGACART_Update_Cart_Banks(void);

#endif // megacart_h
