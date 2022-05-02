/*
 * ultimate1mb.c - Emulation of the Ultimate 1MB Expansion
 *
 * Adapted from Altirra
 * Copyright (C) 2008-2012 Avery Lee
 * Copyright (C) 2020 Mark Grebe
 *
*/

#ifndef ultimate1mb_h
#define ultimate1mb_h

extern int ULTIMATE_enabled;
extern int ULTIMATE_have_rom;
extern int SIDE2_enabled;
extern int ULTIMATE_Flash_Type;

#ifdef ATARI800MACX
extern char ultimate_rom_filename[FILENAME_MAX];
extern char ultimate_nvram_filename[FILENAME_MAX];
#else
extern tatic char ultimate_rom_filename[FILENAME_MAX];
#endif

void init_ultimate(void);
int ULTIMATE_Initialise(int *argc, char *argv[]);
void ULTIMATE_Exit(void);
UBYTE ULTIMATE_D1GetByte(UWORD addr, int no_side_effects);
void ULTIMATE_D1PutByte(UWORD addr, UBYTE byte);
int ULTIMATE_D1ffPutByte(UBYTE byte);
UBYTE ULTIMATE_D3GetByte(UWORD addr, int no_side_effects);
void ULTIMATE_D3PutByte(UWORD addr, UBYTE byte);
int ULTIMATE_D5GetByte(UWORD addr, int no_side_effects);
void ULTIMATE_D5PutByte(UWORD addr, UBYTE byte);
UBYTE ULTIMATE_D6D7GetByte(UWORD addr, int no_side_effects);
void ULTIMATE_D6D7PutByte(UWORD addr, UBYTE byte);
void ULTIMATE_ColdStart(void);
void ULTIMATE_WarmStart(void);
void ULTIMATE_LoadRoms(void);
int ULTIMATE_Save_Rom(char *filename);
int ULTIMATE_Change_Rom(char *filename, int new);
UBYTE ULTIMATE_Flash_Read(UWORD addr);
void ULTIMATE_Flash_Write(UWORD addr, UBYTE value);
void ULTIMATE_Subcart_Left_Active(int active);
#endif /* ultimate1mb_h */
