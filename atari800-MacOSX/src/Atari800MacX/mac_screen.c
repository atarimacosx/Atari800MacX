/*
 * screen.c - Atari screen handling
 *
 * Copyright (c) 2001 Robert Golias and Piotr Fusik
 * Copyright (C) 2001-2008 Atari800 development team (see DOC/CREDITS)
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
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LIBPNG
#include <png.h>
#endif

#include "antic.h"
#include "atari.h"
#include "colours.h"
#include "log.h"
#include "screen.h"
#include "sio.h"
#include "util.h"

#define ATARI_VISIBLE_WIDTH 336
#define ATARI_LEFT_MARGIN 24

ULONG *Screen_atari = NULL;
#ifdef DIRTYRECT
UBYTE *Screen_dirty = NULL;
#endif
#ifdef BITPL_SCR
ULONG *Screen_atari_b = NULL;
ULONG *Screen_atari1 = NULL;
ULONG *Screen_atari2 = NULL;
#endif

/* The area that can been seen is Screen_visible_x1 <= x < Screen_visible_x2,
   Screen_visible_y1 <= y < Screen_visible_y2.
   Full Atari screen is 336x240. Screen_WIDTH is 384 only because
   the code in antic.c sometimes draws more than 336 bytes in a line.
   Currently Screen_visible variables are used only to place
   disk led and snailmeter in the corners of the screen.
*/
int Screen_visible_x1 = 24;				/* 0 .. Screen_WIDTH */
int Screen_visible_y1 = 0;				/* 0 .. Screen_HEIGHT */
int Screen_visible_x2 = 360;			/* 0 .. Screen_WIDTH */
int Screen_visible_y2 = Screen_HEIGHT;	/* 0 .. Screen_HEIGHT */

int Screen_show_atari_speed = FALSE;
int Screen_show_disk_led = TRUE;
int Screen_show_sector_counter = FALSE;

#ifdef HAVE_LIBPNG
#define DEFAULT_SCREENSHOT_FILENAME_FORMAT "atari%03d.png"
#else
#define DEFAULT_SCREENSHOT_FILENAME_FORMAT "atari%03d.pcx"
#endif

static char screenshot_filename_format[FILENAME_MAX] = DEFAULT_SCREENSHOT_FILENAME_FORMAT;
static int screenshot_no_max = 1000;

/* converts "foo%bar##.pcx" to "foo%%bar%02d.pcx" */
static void Screen_SetScreenshotFilenamePattern(const char *p)
{
	char *f = screenshot_filename_format;
	char no_width = '0';
	screenshot_no_max = 1;
	/* 9 because sprintf'ed "no" can be 9 digits */
	while (f < screenshot_filename_format + FILENAME_MAX - 9) {
		/* replace a sequence of hashes with e.g. "%05d" */
		if (*p == '#') {
			if (no_width > '0') /* already seen a sequence of hashes */
				break;          /* invalid */
			/* count hashes */
			do {
				screenshot_no_max *= 10;
				p++;
				no_width++;
				/* now no_width is the number of hashes seen so far
				   and p points after the counted hashes */
			} while (no_width < '9' && *p == '#'); /* no more than 9 hashes */
			*f++ = '%';
			*f++ = '0';
			*f++ = no_width;
			*f++ = 'd';
			continue;
		}
		if (*p == '%')
			*f++ = '%'; /* double the percents */
		*f++ = *p;
		if (*p == '\0')
			return; /* ok */
		p++;
	}
	Log_print("Invalid filename pattern for screenshots, using default.");
	strcpy(screenshot_filename_format, DEFAULT_SCREENSHOT_FILENAME_FORMAT);
	screenshot_no_max = 1000;
}

void Screen_Initialise(int *argc, char *argv[])
{
	int i;
	int j;
	int help_only = FALSE;

	for (i = j = 1; i < *argc; i++) {
		if (strcmp(argv[i], "-screenshots") == 0) {
			Screen_SetScreenshotFilenamePattern(argv[++i]);
		}
		if (strcmp(argv[i], "-showspeed") == 0) {
			Screen_show_atari_speed = TRUE;
		}
		else {
			if (strcmp(argv[i], "-help") == 0) {
				help_only = TRUE;
				Log_print("\t-screenshots <p> Set filename pattern for screenshots");
				Log_print("\t-showspeed       Show percentage of actual speed");
			}
			argv[j++] = argv[i];
		}
	}
	*argc = j;

	/* don't bother mallocing Screen_atari with just "-help" */
	if (help_only)
		return;

	if (Screen_atari == NULL) { /* platform-specific code can initialize it in theory */
		Screen_atari = (ULONG *) Util_malloc(Screen_HEIGHT * Screen_WIDTH);
		memset(Screen_atari, 0, (Screen_HEIGHT * Screen_WIDTH));
#ifdef DIRTYRECT
		Screen_dirty = (UBYTE *) Util_malloc(Screen_HEIGHT * Screen_WIDTH / 8);
		memset(Screen_dirty, 0, (Screen_HEIGHT * Screen_WIDTH));
		Screen_EntireDirty();
#endif
#ifdef BITPL_SCR
		Screen_atari_b = (ULONG *) Util_malloc(Screen_HEIGHT * Screen_WIDTH);
		memset(Screen_atari_b, 0, (Screen_HEIGHT * Screen_WIDTH));
		Screen_atari1 = Screen_atari;
		Screen_atari2 = Screen_atari_b;
#endif
	}
}

void Screen_DrawAtariSpeed(double cur_time)
{
}

void Screen_DrawDiskLED(void)
{
}

void Screen_FindScreenshotFilename(char *buffer)
{
}

int Screen_SaveScreenshot(const char *filename, int interlaced)
{
	return TRUE;
}

void Screen_SaveNextScreenshot(int interlaced)
{
}

void Sound_Pause(void)
{
}

void Sound_Continue(void)
{
}
