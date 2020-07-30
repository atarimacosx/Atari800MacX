/*
 * ui_basic.c - Atari look&feel user interface driver
 *
 * Copyright (C) 1995-1998 David Firth
 * Copyright (C) 1998-2008 Atari800 development team (see DOC/CREDITS)
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

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* free() */
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* getcwd() */
#endif
#ifdef HAVE_DIRECT_H
#include <direct.h> /* getcwd on MSVC*/
#endif
/* XXX: <sys/dir.h>, <ndir.h>, <sys/ndir.h> */
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif

#include "antic.h"
#include "memory.h"
#include "platform.h"
#include "screen.h" /* Screen_atari */
#include "ui.h"

static int initialised = FALSE;
static UBYTE charset[1024];

void BasicUIInit(void)
{
    if (!initialised) {
        MEMORY_GetCharset(charset);
        initialised = TRUE;
    }
}

static void Plot(int fg, int bg, int ch, int x, int y)
{
	const UBYTE *font_ptr = charset + (ch & 0x7f) * 8;
	UBYTE *ptr = (UBYTE *) Screen_atari + 24 * Screen_WIDTH + 32 + y * (8 * Screen_WIDTH) + x * 8;
	int i;
	int j;

	for (i = 0; i < 8; i++) {
		UBYTE data = *font_ptr++;
		for (j = 0; j < 8; j++) {
#ifdef USE_COLOUR_TRANSLATION_TABLE
			ANTIC_VideoPutByte(ptr++, (UBYTE) colour_translation_table[data & 0x80 ? fg : bg]);
#else
			ANTIC_VideoPutByte(ptr++, (UBYTE) (data & 0x80 ? fg : bg));
#endif
			data <<= 1;
		}
		ptr += Screen_WIDTH - 8;
	}
}

static void Print(int fg, int bg, const char *string, int x, int y, int maxwidth)
{
	char tmpbuf[40];
	if ((int) strlen(string) > maxwidth) {
		int firstlen = (maxwidth - 3) >> 1;
		int laststart = strlen(string) - (maxwidth - 3 - firstlen);
		sprintf(tmpbuf, "%.*s...%s", firstlen, string, string + laststart);
		string = tmpbuf;
	}
	while (*string != '\0')
		Plot(fg, bg, *string++, x++, y);
}

void CenterPrint(int fg, int bg, const char *string, int y)
{
	int length = strlen(string);
	Print(fg, bg, string, (length < 38) ? (40 - length) >> 1 : 1, y, 38);
}


static void ClearRectangle(int bg, int x1, int y1, int x2, int y2)
{
#ifdef USE_CURSES
	curses_clear_rectangle(x1, y1, x2, y2);
#else
	UBYTE *ptr = (UBYTE *) Screen_atari + Screen_WIDTH * 24 + 32 + x1 * 8 + y1 * (Screen_WIDTH * 8);
	int bytesperline = (x2 - x1 + 1) << 3;
	UBYTE *end_ptr = (UBYTE *) Screen_atari + Screen_WIDTH * 32 + 32 + y2 * (Screen_WIDTH * 8);
	while (ptr < end_ptr) {
#ifdef USE_COLOUR_TRANSLATION_TABLE
		ANTIC_VideoMemset(ptr, (UBYTE) colour_translation_table[bg], bytesperline);
#else
		ANTIC_VideoMemset(ptr, (UBYTE) bg, bytesperline);
#endif
		ptr += Screen_WIDTH;
	}
#endif /* USE_CURSES */
}

void ClearScreen(void)
{
#ifdef USE_COLOUR_TRANSLATION_TABLE
	ANTIC_VideoMemset((UBYTE *) Screen_atari, colour_translation_table[0x00], Screen_HEIGHT * Screen_WIDTH);
#else
	ANTIC_VideoMemset((UBYTE *) Screen_atari, 0x00, Screen_HEIGHT * Screen_WIDTH);
#endif
	ClearRectangle(0x94, 0, 0, 39, 23);
}
