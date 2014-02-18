/*
    GUILIB:  An example GUI framework library for use with SDL
    Copyright (C) 1997  Sam Lantinga

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    5635-34 Springhouse Dr.
    Pleasanton, CA 94588 (USA)
    slouken@devolution.com
*/

/* This is a set of C functions wrapping C++ classes for popup text output */

#ifndef _GUI_output_h
#define _GUI_output_h

#include "SDL.h"
#include "begin_code.h"
#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
/* C functions for GUI output window support                            */
/*                                                                      */
/************************************************************************/

/* The (really C++) structure holding information for popup text output */
struct GUI_Output;
typedef struct GUI_Output GUI_Output;

/* Function to create a hidden output window 'width' by 'height' characters.
   The 'font' argument should either be a pointer to a 16x16 character font
   image (with an extra pixel row under each row of characters), or NULL to
   use a default internal 8x8-pixel font.
   When shown, the output window will display to the 'screen' surface.
 */
extern GUI_Output *GUI_CreateOutput(SDL_Surface *screen,
				int width, int height, SDL_Surface *font);

/* Add output to an output window.  If the window is visible, the output
   will appear immediately.  Note that the output windows are not overlays,
   and any normal SDL drawing will overwrite the output window display.
   If output causes the window to scroll, previous text will be lost.
*/
extern void GUI_AddOutput(GUI_Output *output, const char *fmt, ...);

/* Clear the contents of an output window */
extern void GUI_ClearOutput(GUI_Output *output);

/* Show an output window, saving the area behind the window, and wait for
   keyboard or mouse press input if 'wait' is non-zero.
 */
extern void GUI_ShowOutput(GUI_Output *output, int wait);

/* Hide an output window, restoring the previous contents of the display */
extern void GUI_HideOutput(GUI_Output *output);

/* Delete an output window */
extern void GUI_DeleteOutput(GUI_Output *output);


/************************************************************************/
/* Simple C message-box functions for quick popup output                */
/*                                                                      */
/************************************************************************/

/* Create a popup message box of the given style, and wait for user input.
   If 'title' is NULL, then no title-text will be used.

   Returns the index of the button pressed, which is style dependent:
*/
#define GUI_MBNONE	0x0000	/* No buttons, any input causes -1 return */
#define GUI_MBOK	0x0001	/* A single "OK" button - causes 1 return */
#define GUI_MBOKCANCEL	0x0002	/* A "Cancel" button on the left - return 0
				   An "OK" button on the right - return 1 */
/* */
extern int GUI_MessageBox(SDL_Surface *screen,
			const char *title, const char *text, Uint32 style);

#ifdef __cplusplus
};
#endif
#include "close_code.h"

#endif /* _GUI_output_h */
