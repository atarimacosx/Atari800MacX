//
//  thecart.h
//  Atari800MacX
//
//  Created by markg on 11/13/20.
//

#ifndef thecart_h
#define thecart_h

#include <stdio.h>

void  THECART_Init(unsigned char *image, int size);
void  THECART_Cold_Reset(void);
UBYTE THECART_Read_Byte(UWORD address);
void  THECART_Write_Byte(UWORD address, UBYTE value);

#endif /* thecart_h */
