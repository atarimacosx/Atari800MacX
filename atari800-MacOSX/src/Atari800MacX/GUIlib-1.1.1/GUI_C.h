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

/* This file provides a simple C interface to the C++ GUI classes */

#ifndef _GUI_C_h
#define _GUI_C_h

#include "SDL.h"
#include "GUI_status.h"

#include "begin_code.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Definitions for the C versions of the GUI and widget objects */
struct CGUI;
typedef struct CGUI CGUI;

struct CGUI_Widget;
typedef struct CGUI_Widget CGUI_Widget;

typedef struct widget_info {
	/* A generic pointer to user-specified data for the widget.
	 */
	void *widget_data;

	/* The display surface for the widget */
	SDL_Surface *screen;

	/* The area covered by the widget */
	SDL_Rect area;

} widget_info;

/* Generic widget callback functions (used by C interface) */
typedef void (*GUI_DrawProc)(widget_info *info);
typedef GUI_status (*GUI_EventProc)(widget_info *info, const SDL_Event *event);
typedef void (*GUI_FreeProc)(widget_info *info);


/* Create a GUI */
extern CGUI *GUI_Create(SDL_Surface *screen);

/* Create a generic widget */
extern CGUI_Widget *GUI_Widget_Create(void *data, int x, int y, int w, int h,
	GUI_DrawProc drawproc, GUI_EventProc eventproc, GUI_FreeProc freeproc);

/* Add a widget to a GUI.
   Once the widget has been added, it doesn't need to be freed.
   This function returns 0, or -1 if it ran out of memory.
 */
extern int GUI_AddWidget(CGUI *gui, CGUI_Widget *widget);

/* Move or resize a widget
   If any of the parameters are -1, that parameter is not changed.
 */
extern void GUI_MoveWidget(CGUI_Widget *widget, int x, int y, int w, int h);

/* Run a GUI until the widgets or idleproc request a quit */
extern void GUI_Run(CGUI *gui, GUI_IdleProc idle);

/* Delete a previously created GUI */
extern void GUI_Destroy(CGUI *gui);

#ifdef __cplusplus
};
#endif
#include "close_code.h"

#endif /* _GUI_C_h */
