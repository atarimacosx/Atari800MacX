//
//  ultimate1mb.h
//  Atari800MacX
//
//  Created by markg on 8/28/20.
//

#ifndef ultimate1mb_h
#define ultimate1mb_h

extern int ULTIMATE_enabled;
extern char ultimate_rom_filename[FILENAME_MAX];
extern char ultimate_nvram_filename[FILENAME_MAX];

void init_ultimate(void);
int ULTIMATE_Initialise(int *argc, char *argv[]);
void ULTIMATE_Exit(void);
int ULTIMATE_D1GetByte(UWORD addr, int no_side_effects);
void ULTIMATE_D1PutByte(UWORD addr, UBYTE byte);
int ULTIMATE_D1ffPutByte(UBYTE byte);
int ULTIMATE_D3GetByte(UWORD addr, int no_side_effects);
void ULTIMATE_D3PutByte(UWORD addr, UBYTE byte);
int ULTIMATE_D5GetByte(UWORD addr, int no_side_effects);
void ULTIMATE_D5PutByte(UWORD addr, UBYTE byte);
int ULTIMATE_D6D7GetByte(UWORD addr, int no_side_effects);
void ULTIMATE_D6D7PutByte(UWORD addr, UBYTE byte);
void ULTIMATE_ColdStart(void);
void ULTIMATE_WarmStart(void);
void ULTIMATE_LoadRoms(void);

#endif /* ultimate1mb_h */
