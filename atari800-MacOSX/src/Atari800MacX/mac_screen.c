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
#include "cassette.h"
#include "colours.h"
#include "log.h"
#include "pia.h"
#include "screen.h"
#include "sio.h"
#include "util.h"
#include "img_disk.h"

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
int Screen_show_hd_sector_counter = FALSE;
int Screen_show_1200_leds = TRUE;

#ifdef HAVE_LIBPNG
#define DEFAULT_SCREENSHOT_FILENAME_FORMAT "atari%03d.png"
#else
#define DEFAULT_SCREENSHOT_FILENAME_FORMAT "atari%03d.pcx"
#endif

static char screenshot_filename_format[FILENAME_MAX] = DEFAULT_SCREENSHOT_FILENAME_FORMAT;
static int screenshot_no_max = 1000;

#define SMALLFONT_WIDTH    5
#define SMALLFONT_HEIGHT   7
#define SMALLFONT_PERCENT  10
#define SMALLFONT_C        11
#define SMALLFONT_D        12
#define SMALLFONT_L        13
#define SMALLFONT_SLASH    14
#define SMALLFONT_R        15
#define SMALLFONT_W        16
#define SMALLFONT_____ 0x00
#define SMALLFONT___X_ 0x02
#define SMALLFONT__X__ 0x04
#define SMALLFONT__XX_ 0x06
#define SMALLFONT_X___ 0x08
#define SMALLFONT_X_X_ 0x0A
#define SMALLFONT_XX__ 0x0C
#define SMALLFONT_XXX_ 0x0E

static void SmallFont_DrawChar(UBYTE *screen, int ch, UBYTE color1, UBYTE color2)
{
    static const UBYTE font[15][SMALLFONT_HEIGHT] = {
        {
            SMALLFONT_____,
            SMALLFONT__X__,
            SMALLFONT_X_X_,
            SMALLFONT_X_X_,
            SMALLFONT_X_X_,
            SMALLFONT__X__,
            SMALLFONT_____
        },
        {
            SMALLFONT_____,
            SMALLFONT__X__,
            SMALLFONT_XX__,
            SMALLFONT__X__,
            SMALLFONT__X__,
            SMALLFONT__X__,
            SMALLFONT_____
        },
        {
            SMALLFONT_____,
            SMALLFONT_XX__,
            SMALLFONT___X_,
            SMALLFONT__X__,
            SMALLFONT_X___,
            SMALLFONT_XXX_,
            SMALLFONT_____
        },
        {
            SMALLFONT_____,
            SMALLFONT_XX__,
            SMALLFONT___X_,
            SMALLFONT__X__,
            SMALLFONT___X_,
            SMALLFONT_XX__,
            SMALLFONT_____
        },
        {
            SMALLFONT_____,
            SMALLFONT___X_,
            SMALLFONT__XX_,
            SMALLFONT_X_X_,
            SMALLFONT_XXX_,
            SMALLFONT___X_,
            SMALLFONT_____
        },
        {
            SMALLFONT_____,
            SMALLFONT_XXX_,
            SMALLFONT_X___,
            SMALLFONT_XXX_,
            SMALLFONT___X_,
            SMALLFONT_XX__,
            SMALLFONT_____
        },
        {
            SMALLFONT_____,
            SMALLFONT__X__,
            SMALLFONT_X___,
            SMALLFONT_XX__,
            SMALLFONT_X_X_,
            SMALLFONT__X__,
            SMALLFONT_____
        },
        {
            SMALLFONT_____,
            SMALLFONT_XXX_,
            SMALLFONT___X_,
            SMALLFONT__X__,
            SMALLFONT__X__,
            SMALLFONT__X__,
            SMALLFONT_____
        },
        {
            SMALLFONT_____,
            SMALLFONT__X__,
            SMALLFONT_X_X_,
            SMALLFONT__X__,
            SMALLFONT_X_X_,
            SMALLFONT__X__,
            SMALLFONT_____
        },
        {
            SMALLFONT_____,
            SMALLFONT__X__,
            SMALLFONT_X_X_,
            SMALLFONT__XX_,
            SMALLFONT___X_,
            SMALLFONT__X__,
            SMALLFONT_____
        },
        {
            SMALLFONT_____,
            SMALLFONT_X_X_,
            SMALLFONT___X_,
            SMALLFONT__X__,
            SMALLFONT_X___,
            SMALLFONT_X_X_,
            SMALLFONT_____
        },
        {
            SMALLFONT_____,
            SMALLFONT__X__,
            SMALLFONT_X_X_,
            SMALLFONT_X___,
            SMALLFONT_X_X_,
            SMALLFONT__X__,
            SMALLFONT_____
        },
        {
            SMALLFONT_____,
            SMALLFONT_XX__,
            SMALLFONT_X_X_,
            SMALLFONT_X_X_,
            SMALLFONT_X_X_,
            SMALLFONT_XX__,
            SMALLFONT_____
        },
        {
            SMALLFONT_____,
            SMALLFONT_X___,
            SMALLFONT_X___,
            SMALLFONT_X___,
            SMALLFONT_X___,
            SMALLFONT_XXX_,
            SMALLFONT_____
        },
        {
            SMALLFONT_____,
            SMALLFONT___X_,
            SMALLFONT___X_,
            SMALLFONT__X__,
            SMALLFONT__X__,
            SMALLFONT_X___,
            SMALLFONT_____
        }
    };
    int y;
    for (y = 0; y < SMALLFONT_HEIGHT; y++) {
        int src;
        int mask;
        src = font[ch][y];
        for (mask = 1 << (SMALLFONT_WIDTH - 1); mask != 0; mask >>= 1) {
            ANTIC_VideoPutByte(screen, (UBYTE) ((src & mask) != 0 ? color1 : color2));
            screen++;
        }
        screen += Screen_WIDTH - SMALLFONT_WIDTH;
    }
}

/* Returns screen address for placing the next character on the left of the
   drawn number. */
static UBYTE *SmallFont_DrawInt(UBYTE *screen, int n, UBYTE color1, UBYTE color2)
{
    do {
        SmallFont_DrawChar(screen, n % 10, color1, color2);
        screen -= SMALLFONT_WIDTH;
        n /= 10;
    } while (n > 0);
    return screen;
}

static UBYTE *SmallFont_DrawLong(UBYTE *screen, long n, UBYTE color1, UBYTE color2)
{
    do {
        SmallFont_DrawChar(screen, n % 10, color1, color2);
        screen -= SMALLFONT_WIDTH;
        n /= 10;
    } while (n > 0);
    return screen;
}

void Screen_DrawAtariSpeed(int fps)
{
    if (Screen_show_atari_speed) {
            /* space for 5 digits - up to 99999% Atari speed */
        UBYTE *screen = (UBYTE *) Screen_atari + Screen_visible_x1 + 5 * SMALLFONT_WIDTH
                      + (Screen_visible_y2 - SMALLFONT_HEIGHT) * Screen_WIDTH;
        SmallFont_DrawInt(screen - SMALLFONT_WIDTH, fps, 0x0c, 0x00);
    }
}

void Screen_DrawDiskLED(void)
{
    if (Screen_show_disk_led || Screen_show_sector_counter) {
        UBYTE *screen = (UBYTE *) Screen_atari + Screen_visible_x2 - SMALLFONT_WIDTH
                        + (Screen_visible_y2 - SMALLFONT_HEIGHT) * Screen_WIDTH;
        if (SIO_last_op_time > 0) {
            SIO_last_op_time--;
            if (Screen_show_disk_led) {
                SmallFont_DrawChar(screen, SIO_last_drive, 0x00, (UBYTE) (SIO_last_op == SIO_LAST_READ ? 0xac : 0x2b));
                SmallFont_DrawChar(screen -= SMALLFONT_WIDTH, SMALLFONT_D, 0x00, (UBYTE) (SIO_last_op == SIO_LAST_READ ? 0xac : 0x2b));
                screen -= SMALLFONT_WIDTH;
            }

            if (Screen_show_sector_counter)
                screen = SmallFont_DrawInt(screen, SIO_last_sector, 0x00, 0x88);
        }
        if ((CASSETTE_readable && !CASSETTE_record) ||
            (CASSETTE_writable && CASSETTE_record)) {
            if (Screen_show_disk_led)
                SmallFont_DrawChar(screen, SMALLFONT_C, 0x00, (UBYTE) (CASSETTE_record ? 0x2b : 0xac));

            if (Screen_show_sector_counter) {
                /* Displaying tape length during saving is pointless since it would equal the number
                of the currently-written block, which is already displayed. */
                if (!CASSETTE_record) {
                    screen = SmallFont_DrawInt(screen - SMALLFONT_WIDTH, CASSETTE_GetSize(), 0x00, 0x88);
                    SmallFont_DrawChar(screen, SMALLFONT_SLASH, 0x00, 0x88);
                }
                SmallFont_DrawInt(screen - SMALLFONT_WIDTH, CASSETTE_GetPosition(), 0x00, 0x88);
            }
        }
    }
}

void Screen_DrawHDDiskLED(void)
{
    if (Screen_show_hd_sector_counter) {
        UBYTE *screen = (UBYTE *) Screen_atari + Screen_visible_x1 + SMALLFONT_WIDTH * 21
            + (Screen_visible_y2 - SMALLFONT_HEIGHT) * Screen_WIDTH;
        if (IMG_last_op_time > 0) {
            IMG_last_op_time--;
            screen = SmallFont_DrawLong(screen, IMG_last_sector, 0x00, (UBYTE) (IMG_last_op_read ? 0xac : 0x2b));
        }
    }
}

void Screen_Draw1200LED(void)
{
    if (Screen_show_1200_leds && Atari800_keyboard_leds) {
        UBYTE *screen = (UBYTE *) Screen_atari + Screen_visible_x1 + SMALLFONT_WIDTH * 10
            + (Screen_visible_y2 - SMALLFONT_HEIGHT) * Screen_WIDTH;
        UBYTE portb = PIA_PORTB | PIA_PORTB_mask;
        if ((portb & 0x04) == 0) {
            SmallFont_DrawChar(screen, SMALLFONT_L, 0x00, 0x36);
            SmallFont_DrawChar(screen + SMALLFONT_WIDTH, 1, 0x00, 0x36);
        }
        screen += SMALLFONT_WIDTH * 3;
        if ((portb & 0x08) == 0) {
            SmallFont_DrawChar(screen, SMALLFONT_L, 0x00, 0x36);
            SmallFont_DrawChar(screen + SMALLFONT_WIDTH, 2, 0x00, 0x36);
        }
    }
}

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

int Screen_Initialise(int *argc, char *argv[])
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
		return TRUE;

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
    return TRUE;
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
