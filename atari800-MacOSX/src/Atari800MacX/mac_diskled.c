/*
 * diskled.c - disk drive LED emulation
 *
 * Copyright (C) 1995-1998 David Firth
 * Copyright (C) 1998-2003 Atari800 development team (see DOC/CREDITS)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <string.h>
#include "atari.h"
#include "screen.h"
#include "mac_diskled.h"

#define DISKLED_FONT_WIDTH		5
#define DISKLED_FONT_HEIGHT		7

#define DISKLED_COLOR_READ		0xAC
#define DISKLED_COLOR_WRITE		0x35
#define DISKLED_COLOR_COUNTER	0x88

int led_status = 0;
int led_off_delay = -1;
int led_sector = 0;

static UBYTE DiskLED[] = {
	0x1f, 0x1b, 0x15, 0x15, 0x15, 0x1b, 0x1f, /* 0 */
	0x1f, 0x1b, 0x13, 0x1b, 0x1b, 0x1b, 0x1f, /* 1 */
	0x1f, 0x13, 0x1d, 0x1b, 0x17, 0x11, 0x1f, /* 2 */
	0x1f, 0x13, 0x1d, 0x1b, 0x1d, 0x13, 0x1f, /* 3 */
	0x1f, 0x1d, 0x19, 0x15, 0x11, 0x1d, 0x1f, /* 4 */
	0x1f, 0x11, 0x17, 0x11, 0x1d, 0x13, 0x1f, /* 5 */
	0x1f, 0x1b, 0x17, 0x13, 0x15, 0x1b, 0x1f, /* 6 */
	0x1f, 0x11, 0x1d, 0x1b, 0x1b, 0x1b, 0x1f, /* 7 */
	0x1f, 0x1b, 0x15, 0x1b, 0x15, 0x1b, 0x1f, /* 8 */
	0x1f, 0x1b, 0x15, 0x19, 0x1d, 0x1b, 0x1f  /* 9 */
};

void LED_Frame(void)
{	
    static int firstDisabled = 0;
	
	if (led_off_delay >= 0)
		if (--led_off_delay < 0) { 
			led_status = 0;
			}

    /* Need to make sure that led info is cleared when in wide screen mode,
	   as Atari doesn't normally update that part of screen ram */
    if (!Screen_show_disk_led || !led_status) {
		if (firstDisabled) {
			UBYTE *screen = (UBYTE *) Screen_atari
						+ (Screen_visible_y2 - DISKLED_FONT_HEIGHT) * Screen_WIDTH
						+ Screen_visible_x2;
		    UBYTE *target = screen - 4*DISKLED_FONT_WIDTH;
			int x, y;
		    firstDisabled--;
			for (y = 0; y < DISKLED_FONT_HEIGHT; y++) {
				for (x = 0; x < 4*DISKLED_FONT_WIDTH; x++)
					*target++ = 0;
				target += Screen_WIDTH - 4*DISKLED_FONT_WIDTH;
				}
		    }
	    }

	if (led_status & Screen_show_disk_led) {
		UBYTE *screen = (UBYTE *) Screen_atari
						+ (Screen_visible_y2 - DISKLED_FONT_HEIGHT) * Screen_WIDTH
						+ Screen_visible_x2;
		UBYTE *source = DiskLED + (led_status % 9) * DISKLED_FONT_HEIGHT;
		UBYTE *target = screen - DISKLED_FONT_WIDTH;

		UBYTE color = led_status < 10 ? DISKLED_COLOR_READ : DISKLED_COLOR_WRITE;
		UBYTE mask  = 1 << (DISKLED_FONT_WIDTH - 1);
		int x, y;
		firstDisabled = 2;

		for (y = 0; y < DISKLED_FONT_HEIGHT; y++) {
			for (x = 0; x < DISKLED_FONT_WIDTH; x++)
				*target++ = (UBYTE)(*source & mask >> x ? color : 0);
			target += Screen_WIDTH - DISKLED_FONT_WIDTH;
			++source;
		}

		if (Screen_show_sector_counter) {
			char sector[6];
			int len, i;

			sprintf(sector, "%d", led_sector);
			len = strlen(sector);

			for (i = 0; i < len; i++)
			{
				source = DiskLED + (sector[i] - '0') * DISKLED_FONT_HEIGHT;
				target = screen - DISKLED_FONT_WIDTH * (len + 1 - i);

				for (y = 0; y < DISKLED_FONT_HEIGHT; y++) {
					for (x = 0; x < DISKLED_FONT_WIDTH; x++)
						*target++ = (UBYTE)(*source & mask >> x ? DISKLED_COLOR_COUNTER : 0);
					target += Screen_WIDTH - DISKLED_FONT_WIDTH;
					++source;
				}
			}
		}
	}
}
