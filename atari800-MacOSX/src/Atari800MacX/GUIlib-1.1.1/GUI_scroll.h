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

/* This is a virtual base class for a scrollable widget */

#ifndef _GUI_scroll_h
#define _GUI_scroll_h

#include "GUI_widget.h"


class GUI_Scrollable : public GUI_Widget {

public:
	/* Passed the generic widget parameters */
	GUI_Scrollable(void *data)	: GUI_Widget(data) {
	}
	GUI_Scrollable(void *data, int x, int y, int w, int h)
					: GUI_Widget(data, x, y, w, h) {
	}

	/* Virtual functions to scroll forward and backward
	   This function returns the final position of the cursor.
	 */
	virtual int Scroll(int amount) = 0;

	/* Virtual function to get the scrollable range */
	virtual void Range(int &first, int &last) = 0;
};

#endif /* _GUI_scroll_h */
