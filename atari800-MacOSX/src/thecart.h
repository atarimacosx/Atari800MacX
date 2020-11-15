/* thecart.h -
   Emulation of The!Cart cartridges for the Atari
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   Copyright (C) 2020 Mark Grebe

   Ported and Adapted from Altirra
   Copyright (C) 2008-2009 Avery Lee
*/

#ifndef thecart_h
#define thecart_h

#include <stdio.h>

void  THECART_Init(unsigned char *image, int size);
void  THECART_Cold_Reset(void);
UBYTE THECART_Read_Byte(UWORD address);
void  THECART_Write_Byte(UWORD address, UBYTE value);

#endif /* thecart_h */
