#ifndef AF80_H_
#define AF80_H_

#include "atari.h"
#include <stdio.h>

#ifdef ATARI800MACX
#define AF80_SCRN_WIDTH    (640)
#define AF80_SCRN_HEIGHT   (250)
#define AF80_CHAR_HEIGHT    (10)
#define AF80_CHAR_WIDTH     (8)
#endif

extern int AF80_palette[16];
int AF80_Initialise(int *argc, char *argv[]);
void AF80_Exit(void);
void AF80_InsertRightCartridge(void);
void AF80_RemoveRightCartridge(void);
int AF80_ReadConfig(char *string, char *ptr);
void AF80_WriteConfig(FILE *fp);
int AF80_D5GetByte(UWORD addr, int no_side_effects);
void AF80_D5PutByte(UWORD addr, UBYTE byte);
int AF80_D6GetByte(UWORD addr, int no_side_effects);
void AF80_D6PutByte(UWORD addr, UBYTE byte);
UBYTE AF80_GetPixels(int scanline, int column, int *colour, int blink);
extern int AF80_enabled;
void AF80_Reset(void);
#ifdef ATARI800MACX
int AF80GetCopyData(int startx, int endx, int starty, int endy, unsigned char *data);
#endif

#endif /* AF80_H_ */
