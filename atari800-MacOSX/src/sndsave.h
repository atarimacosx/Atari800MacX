#ifndef SNDSAVE_H_
#define SNDSAVE_H_

#include "atari.h"

#ifdef MACOSX
char *SndSave_Find_AIFF_name( void );
#endif
int SndSave_IsSoundFileOpen(void);
int SndSave_CloseSoundFile(void);
int SndSave_OpenSoundFile(const char *szFileName);
int SndSave_WriteToSoundFile(const UBYTE *ucBuffer, unsigned int uiSize);

#endif /* SNDSAVE_H_ */

