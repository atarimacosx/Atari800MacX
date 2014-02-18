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

#include "GUI.h"
#include "GUI_generic.h"

extern "C" {

/* Create a GUI */
CGUI *GUI_Create(SDL_Surface *screen)
{
	return((CGUI *)new GUI(screen));
}

/* Create a generic widget */
CGUI_Widget *GUI_Widget_Create(void *data, int x, int y, int w, int h,
	GUI_DrawProc drawproc, GUI_EventProc eventproc, GUI_FreeProc freeproc)
{
	GUI_Widget *widget;

	widget = new GUI_GenericWidget(data, drawproc, eventproc, freeproc);
	widget->SetRect(x, y, w, h);
	return((CGUI_Widget *)widget);
}

/* Add a widget to a GUI */
int GUI_AddWidget(CGUI *gui, CGUI_Widget *widget)
{
	return(((GUI *)gui)->AddWidget((GUI_Widget *)widget));
}

/* Move or resize a widget 
   If any of the parameters are -1, that parameter is not changed. 
 */
void GUI_MoveWidget(CGUI_Widget *widget, int x, int y, int w, int h)
{
	((GUI_Widget *)widget)->SetRect(x, y, w, h);
	((GUI_Widget *)widget)->Display();
}

/* Run a GUI until the widgets request a quit */
void GUI_Run(CGUI *gui, GUI_IdleProc idle)
{
	((GUI *)gui)->Run(idle);
}

/* Delete a previously created GUI */
void GUI_Destroy(CGUI *gui)
{
	delete (GUI *)gui;
}

/* End of extern C declaration */
};
