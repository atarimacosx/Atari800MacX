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

/* This is a very generic scrollbar widget */

#ifndef _GUI_scrollbar_h
#define _GUI_scrollbar_h

#include "GUI_widget.h"
#include "GUI_scroll.h"

/* The possible orientation of the scrollbar */
typedef enum { SCROLLBAR_HORIZONTAL, SCROLLBAR_VERTICAL } orientation;

class GUI_ScrollBar : public GUI_Widget {

public:
	/* Passed the sensitive areas, orientation, and initial widget */
	GUI_ScrollBar(SDL_Rect &first, SDL_Rect &middle, SDL_Rect &last,
			orientation direction, float percent_full,
			GUI_Scrollable *widget = NULL);
	GUI_ScrollBar(orientation direction, float percent_full, 
			GUI_Scrollable *widget = NULL);

	/* Link a scrollable widget to this scrollbar */
	virtual int AddScrollable(GUI_Scrollable *widget);

	/* Show the widget  */
	virtual void Display(void) = 0;

	/* Continue scrolling while mouse is held down */
	virtual GUI_status Idle(void);

	/* Mouse hits activate us */
	virtual GUI_status MouseDown(int x, int y, int button);

protected:
	/* The scrollable widget */
	GUI_Scrollable *target;

	/* The areas:  Scroll up 1,  Position at, Scroll down 1 */
	SDL_Rect sensitive_neg;
	SDL_Rect sensitive_mid;
	SDL_Rect sensitive_pos;
	orientation whichxy;
	float full;

	/* Time at which the display will be scrolled while the mouse button
	   is being held down.
	 */
	Uint32 next_scroll;

	/* Function to find the bounds of the widget */
	virtual void FindBounds(void);

	/* Function to find the current scroll percentage of the widget */
	virtual float Percentage(void);

	/* The functions to activate the scrolling */
	virtual void Scroll(int amount);
	virtual void ScrollTo(int position);
};


class GUI_ScrollButtons : public GUI_ScrollBar {

public:
	/* The possible orientation of the scrollbar:
		GUI_ScrollBar::HORIZONTAL, GUI_ScrollBar::VERTICAL
	 */

	/* Passed the sensitive images, orientation, and initial widget */
	GUI_ScrollButtons(int x1, int y1, SDL_Surface *image1, SDL_Rect &middle,
	                  int x2, int y2, SDL_Surface *image2,
			orientation direction, float percent_full,
			GUI_Scrollable *widget = NULL);

	/* Show the widget  */
	virtual void Display(void);
	
	/* change FG/BG colors and transparency */
	virtual void SetColoring(Uint8 fr, Uint8 fg, Uint8 fb, int bg_opaque = 0,
					 Uint8 br = 0, Uint8 bg = 0, Uint8 bb = 0);

protected:
	/* scroll button images */
	SDL_Surface *image_neg;
	SDL_Surface *image_pos;
	SDL_Rect bar_area;
	Uint32 fore_color, back_color;
};

#endif /* _GUI_scrollbar_h */
