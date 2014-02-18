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

#include "GUI_image.h"

/* Passed the position and BMP image file */
GUI_Image:: GUI_Image(int x, int y, char *file)
 : GUI_Widget(NULL)
{
	SDL_Surface *picture;

	picture = SDL_LoadBMP(file);
	if ( picture == NULL ) {
		SetError((char *)"Couldn't load %s: %s", file, SDL_GetError());
		return;
	}
//*****	GUI_Image(x, y, picture, 1); ***ERROR*** another constructor****
//*****	instead: set size manually...
	SetRect(x,y,picture->w,picture->h);
	image=picture;
	free_image=1;
}

/* Passed the position and image */
GUI_Image:: GUI_Image(int x, int y, SDL_Surface *pic, int shouldfree)
 : GUI_Widget(NULL, x, y, pic->w, pic->h)
{
	image = pic;
	free_image = shouldfree;
}

GUI_Image::~GUI_Image()
{
	if ( free_image ) {
		SDL_FreeSurface(image);
	}
}

/* Show the widget  */
void
GUI_Image:: Display(void)
{
	SDL_BlitSurface(image, NULL, screen, &area);
}
