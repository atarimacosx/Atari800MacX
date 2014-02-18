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

/* This is the most generic widget possible, -- it's exported for C functions
 */

#ifndef _GUI_generic_h
#define _GUI_generic_h

#include "GUI_C.h"
#include "GUI_widget.h"


class GUI_GenericWidget : public GUI_Widget {

public:
	GUI_GenericWidget(void *data, GUI_DrawProc drawproc,
			GUI_EventProc eventproc, GUI_FreeProc freeproc);
	~GUI_GenericWidget();

	/* Show the widget.
	   If the surface needs to be locked, it will be locked
	   before this call, and unlocked after it returns.
	 */
	virtual void Display(void);

	/* Main event handler function.
	   This function gets raw SDL events from the GUI.
	 */
	virtual GUI_status HandleEvent(const SDL_Event *event);

protected:
	GUI_DrawProc DrawProc;
	GUI_EventProc EventProc;
	GUI_FreeProc FreeProc;

	/* Fill a widget_info structure with our info */
	virtual void FillInfo(widget_info *info);
};

#endif /* _GUI_generic_h */
