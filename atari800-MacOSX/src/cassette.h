#ifndef CASSETTE_H_
#define CASSETTE_H_

#include <stdio.h>		/* for FILE and FILENAME_MAX */

#include "atari.h"		/* for UBYTE */

#define CASSETTE_DESCRIPTION_MAX 256

void CASSETTE_Initialise(int *argc, char *argv[]);

int CASSETTE_CheckFile(const char *filename, FILE **fp, char *description, int *last_block, int *isCAS);
int CASSETTE_CreateFile(const char *filename, FILE **fp, int *isCAS);
int CASSETTE_Insert(const char *filename);
void CASSETTE_Remove(void);

#ifdef MACOSX
extern FILE *cassette_file;  /* MACOSX - Needs this global for GUI */
extern char cassette_filename[FILENAME_MAX];
extern char cassette_description[CASSETTE_DESCRIPTION_MAX];

extern int cassette_current_block;
extern int cassette_max_block;
extern int cassette_savefile;
#endif

extern int CASSETTE_hold_start;
extern int CASSETTE_hold_start_on_reboot; /* preserve hold_start after reboot */
extern int CASSETTE_press_space;

int CASSETTE_AddGap(int gaptime);
void CASSETTE_LeaderLoad(void);
void CASSETTE_LeaderSave(void);
int CASSETTE_Read(void);
int CASSETTE_Write(int length);
int CASSETTE_IOLineStatus(void);
int CASSETTE_GetByte(void);
int CASSETTE_GetInputIRQDelay(void);
int CASSETTE_IsSaveFile(void);
void CASSETTE_PutByte(int byte);
void CASSETTE_TapeMotor(int onoff);
int CASSETTE_AddScanLine(void);
extern UBYTE CASSETTE_buffer[4096];
void CASSETTE_ResetPOKEY(void);

#endif /* CASSETTE_H_ */
