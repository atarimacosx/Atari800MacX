/*
 * sndsave.c - reading and writing sound to files
 *
 * Copyright (C) 1995-1998 David Firth
  * Copyright (C) 1998-2005 Atari800 development team (see DOC/CREDITS)
 * Copyright (C) 2004 Mark Grebe
 *
 * This file is part of the Atari800 emulator project which emulates
 * the Atari 400, 800, 800XL, 130XE, and 5200 8-bit computers.
 *
 * Atari800 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atari800 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari800; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include "pokeysnd.h"
#include "sndsave.h"

#ifdef ATARI800MACX
#define AIFF_CHUNK_HEADER   "FORM\000\000\000\000AIFFCOMM\000\000\000\022\000\002\000\000\000\000\000\010\100\016\254\104\000\000\000\000\000\000"
#define AIFF_CHUNK_HEADER_LENGTH    38
#define AIFF_DATA_CHUNK_HEADER	"SSND\000\000\000\000\000\000\000\000\000\000\000\000"
#define AIFF_DATA_CHUNK_HEADER_LENGTH	16
#define FRAGSIZE        9      // 1<<FRAGSIZE is size of sound buffer
#endif

/* sndoutput is just the file pointer for the current sound file */
static FILE *sndoutput = NULL;

#ifdef ATARI800MACX
char *SndSave_Find_AIFF_name(void)
{
	int aiff_no = -1;
	static char filename[20];
	FILE *fp;

	while (++aiff_no < 1000) {
		sprintf(filename, "atari%03i.aiff", aiff_no);
		if ((fp = fopen(filename, "r")) == NULL)
			return filename; /*file does not exist - we can create it */
		fclose(fp);
	}
	return NULL;
}
#else
static ULONG byteswritten;

/* write 32-bit word as little endian */
static void write32(long x)
{
	fputc(x & 0xff, sndoutput);
	fputc((x >> 8) & 0xff, sndoutput);
	fputc((x >> 16) & 0xff, sndoutput);
	fputc((x >> 24) & 0xff, sndoutput);
}
#endif

/* SndSave_IsSoundFileOpen simply returns true if the sound file is currently open and able to receive writes
   RETURNS: TRUE is file is open, FALSE if it is not */
int SndSave_IsSoundFileOpen(void)
{
	return sndoutput != NULL;
}


/* SndSave_CloseSoundFile should be called when the program is exiting, or when all data required has been
   written to the file. SndSave_CloseSoundFile will also be called automatically when a call is made to
   SndSave_OpenSoundFile, or an error is made in SndSave_WriteToSoundFile. Note that CloseSoundFile has to back track
   to the header written out in SndSave_OpenSoundFile and update it with the length of samples written

   RETURNS: TRUE if file closed with no problems, FALSE if failure during close */

#ifdef ATARI800MACX
int SndSave_CloseSoundFile(void)
{
	int	bSuccess = TRUE;

	if( sndoutput )
	{
		int	iPos = 0;
		int frameCount = 0;

		/* Sound file is finished, so modify header and close it. */
		if( fseek( sndoutput, 0, SEEK_END ) != 0 )
			bSuccess = FALSE;

		iPos = ftell( sndoutput );

		/* iPos must be greater than the data headers plus one data element for this file to make sense */
		if( iPos > ( AIFF_CHUNK_HEADER_LENGTH + AIFF_DATA_CHUNK_HEADER_LENGTH + 1 ) )
		{

			if( fseek( sndoutput, 4, SEEK_SET )!= 0 )	/* Seek past FORM */
				bSuccess = FALSE;

			iPos -= 8;
			if( fwrite( &iPos, 1, 4, sndoutput ) != 4 ) /* Write out file size - 8 */
				bSuccess = FALSE;

			if( fseek( sndoutput, 22, SEEK_SET ) != 0) /* Seek into Common header */
				bSuccess = FALSE;

			iPos -= 46;
			frameCount = iPos / 2;						
			if( fwrite( &frameCount, 1, 4, sndoutput ) != 4)	/* Write out size of just sample frames */
				bSuccess = FALSE;

			if( fseek( sndoutput, 42, SEEK_SET ) != 0) /* Seek into sound header */
				bSuccess = FALSE;

			iPos -= 8;
			if( fwrite( &iPos, 1, 4, sndoutput ) != 4)	/* Write out size of just sample data */
				bSuccess = FALSE;
		}
		else
		{
			bSuccess = FALSE;
		}
		
		fclose( sndoutput );
	}
	sndoutput = NULL;

	return bSuccess;
}
#else
int SndSave_CloseSoundFile(void)
{
	int bSuccess = TRUE;
	char aligned = 0;

	if (sndoutput != NULL) {
		/* A RIFF file's chunks must be word-aligned. So let's align. */
		if (byteswritten & 1) {
			if (putc(0, sndoutput) == EOF)
				bSuccess = FALSE;
			else
				aligned = 1;
		}

		if (bSuccess) {
			/* Sound file is finished, so modify header and close it. */
			if (fseek(sndoutput, 4, SEEK_SET) != 0)	/* Seek past RIFF */
				bSuccess = FALSE;
			else {
				/* RIFF header's size field must equal the size of all chunks
				 * with alignment, so the alignment byte is added.
				 */
				write32(byteswritten + 36 + aligned);
				if (fseek(sndoutput, 40, SEEK_SET) != 0)
					bSuccess = FALSE;
				else {
					/* But in the "data" chunk size field, the alignment byte
					 * should be ignored. */
					write32(byteswritten);
				}
			}
		}
		fclose(sndoutput);
		sndoutput = NULL;
	}

	return bSuccess;
}
#endif

/* SndSave_OpenSoundFile will start a new sound file and write out the header. If an existing sound file is
   already open it will be closed first, and the new file opened in it's place

   RETURNS: TRUE if file opened with no problems, FALSE if failure during open */

#ifdef ATARI800MACX
int SndSave_OpenSoundFile(const char *szFileName)
{
	SndSave_CloseSoundFile();

	sndoutput = fopen(szFileName, "wb");

	if (sndoutput == NULL)
		return FALSE;
	/*
	The AIFF header: 

	  Offset  Length   Contents
	  0       4 bytes  'FORM'
	  4       4 bytes  <file length - 8>
	  8       4 bytes  'AIFF'
	
	The common chunk: 

	  12      4 bytes  'COMM'
	  16      4 bytes  0x00000012       // Length of the fmt data (18 bytes)
	  20      2 bytes  <channels>       // Channels: 1 = mono, 2 = stereo
	  22      4 bytes  <number frames>  // Number of sample frames
	  26      2 bytes  <sample size >   // sample size in bits
	  28	 10 bytes  <sample rate >   // extended float (80bits) rate

	The data chunk: 

	  38      4 bytes  'SSND'
	  42      4 bytes  <length of the data block>
	  46        bytes  <sample data>
	*/

	if( fwrite( AIFF_CHUNK_HEADER, 1, AIFF_CHUNK_HEADER_LENGTH, sndoutput ) != AIFF_CHUNK_HEADER_LENGTH )
	{
		fclose( sndoutput );
		sndoutput = NULL;
		return FALSE;
	}

	if( fwrite( AIFF_DATA_CHUNK_HEADER, 1, AIFF_DATA_CHUNK_HEADER_LENGTH, sndoutput ) != AIFF_DATA_CHUNK_HEADER_LENGTH )
	{
		fclose( sndoutput );
		return FALSE;
	}

	return TRUE;
}
#else
int SndSave_OpenSoundFile(const char *szFileName)
{
	SndSave_CloseSoundFile();

	sndoutput = fopen(szFileName, "wb");

	if (sndoutput == NULL)
		return FALSE;
	/*
	The RIFF header:

	  Offset  Length   Contents
	  0       4 bytes  'RIFF'
	  4       4 bytes  <file length - 8>
	  8       4 bytes  'WAVE'

	The fmt chunk:

	  12      4 bytes  'fmt '
	  16      4 bytes  0x00000010     // Length of the fmt data (16 bytes)
	  20      2 bytes  0x0001         // Format tag: 1 = PCM
	  22      2 bytes  <channels>     // Channels: 1 = mono, 2 = stereo
	  24      4 bytes  <sample rate>  // Samples per second: e.g., 44100
	  28      4 bytes  <bytes/second> // sample rate * block align
	  32      2 bytes  <block align>  // channels * bits/sample / 8
	  34      2 bytes  <bits/sample>  // 8 or 16

	The data chunk:

	  36      4 bytes  'data'
	  40      4 bytes  <length of the data block>
	  44        bytes  <sample data>

	All chunks must be word-aligned.

	Good description of WAVE format: http://www.sonicspot.com/guide/wavefiles.html
	*/

	if (fwrite("RIFF\0\0\0\0WAVEfmt \x10\0\0\0\1\0", 1, 22, sndoutput) != 22) {
		fclose(sndoutput);
		sndoutput = NULL;
		return FALSE;
	}

	fputc(POKEYSND_num_pokeys, sndoutput);
	fputc(0, sndoutput);
	write32(POKEYSND_playback_freq);


	write32(POKEYSND_playback_freq * (POKEYSND_snd_flags & POKEYSND_BIT16 ? POKEYSND_num_pokeys << 1 : POKEYSND_num_pokeys));

	fputc(POKEYSND_snd_flags & POKEYSND_BIT16 ? POKEYSND_num_pokeys << 1 : POKEYSND_num_pokeys, sndoutput);
	fputc(0, sndoutput);

	fputc(POKEYSND_snd_flags & POKEYSND_BIT16? 16: 8, sndoutput);

	if (fwrite("\0data\0\0\0\0", 1, 9, sndoutput) != 9) {
		fclose(sndoutput);
		sndoutput = NULL;
		return FALSE;
	}

	byteswritten = 0;
	return TRUE;
}
#endif

/* SndSave_WriteToSoundFile will dump PCM data to the WAV file. The best way to do this for Atari800 is
   probably to call it directly after POKEYSND_Process(buffer, size) with the same values (buffer, size)

   RETURNS: the number of bytes written to the file (should be equivalent to the input uiSize parm) */

#ifdef ATARI800MACX
int SndSave_WriteToSoundFile(const unsigned char *ucBuffer, unsigned int uiSize)
{
	static unsigned char buffer[1 << FRAGSIZE];
	short signedVal;
	const unsigned char *in;
	unsigned char *out;
	int i, numWritten = 0;

	if( sndoutput && ucBuffer && uiSize )
	{
		in = ucBuffer;
		out = buffer;
		for (i=0;i<uiSize;i++,in++,out++)
		{
			signedVal = ((short) *in) - 128;
			*out = signedVal;
		}
		numWritten = fwrite(buffer,uiSize,1,sndoutput);
	}

	return( numWritten );
}
#else
int SndSave_WriteToSoundFile(const unsigned char *ucBuffer, unsigned int uiSize)
{
	/* XXX FIXME: doesn't work with big-endian architectures */
	if (sndoutput && ucBuffer && uiSize) {
		int result;
		if (POKEYSND_snd_flags & POKEYSND_BIT16)
			uiSize <<= 1;
		result = fwrite(ucBuffer, 1, uiSize, sndoutput);
		byteswritten += result;
		if (result != uiSize) {
			SndSave_CloseSoundFile();
		}

		return result;
	}

	return 0;
}
#endif
