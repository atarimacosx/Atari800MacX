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

/* This is a very simple image widget */

#ifndef _GUI_image_h
#define _GUI_image_h

#include "GUI_widget.h"

class GUI_Image : public GUI_Widget {

public:
	/* Passed the position and BMP image file */
	GUI_Image(int x, int y, char *file);

	/* Passed the position and image surface */
	GUI_Image(int x, int y, SDL_Surface *pic, int shouldfree = 0);

	virtual ~GUI_Image();

	/* Show the widget  */
	virtual void Display(void);

protected:
	/* The display image */
	SDL_Surface *image;

	/* Whether or not we should free the image when we're done */
	int free_image;
};

#endif /* _GUI_image_h */
