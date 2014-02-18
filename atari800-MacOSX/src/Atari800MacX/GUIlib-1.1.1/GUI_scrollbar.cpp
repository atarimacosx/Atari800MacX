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

#include "GUI_scrollbar.h"
#include "math.h"


#define SCROLL_INTERVAL	100	/* Scroll every 100 ms */
#define SCROLL_WAIT	2	/* Intervals to wait before scrolling */

/* Passed the sensitive areas, orientation, and initial widget */
GUI_ScrollBar:: GUI_ScrollBar(SDL_Rect &first, SDL_Rect &middle, SDL_Rect &last,
			orientation direction,  float percent_full, GUI_Scrollable *widget)
 : GUI_Widget(NULL)
{
	/* Set the sensitive areas */
	sensitive_neg = first;
	sensitive_mid = middle;
	sensitive_pos = last;

	/* Set the orientation and next scrolling time */
	whichxy = direction;
	next_scroll = 0;
	
	/* Set the percentage full for the scroll bar */
	full = percent_full;

	/* Set the default scrollable widget */
	target = widget;
}

GUI_ScrollBar:: GUI_ScrollBar(orientation direction, float percent_full,
                              GUI_Scrollable *widget)
 : GUI_Widget(NULL)
{
	/* Set the orientation */
	whichxy = direction;

	/* Set the default scrollable widget */
	target = widget;

	/* Set the percentage full for the scroll bar */
	full = percent_full;
}

void
GUI_ScrollBar:: FindBounds(void)
{
	SDL_Rect *rects[4];

	/* Find the bounds of this widget */
	rects[0] = &sensitive_neg;
	rects[1] = &sensitive_mid;
	rects[2] = &sensitive_pos;
	rects[3] = NULL;
	SetRect(rects);
}

/* Link a scrollable widget to this scrollbar */
int
GUI_ScrollBar:: AddScrollable(GUI_Scrollable *widget)
{
	target = widget;
	return(0);
}

/* Continue scrolling while mouse is held down */
GUI_status
GUI_ScrollBar:: Idle(void)
{
	GUI_status status;

	status = GUI_PASS;
	if ( next_scroll && (next_scroll <= SDL_GetTicks()) ) {
		int x, y;
		Uint8 state;

		state = SDL_GetMouseState(&x, &y);
		if ( (state & SDL_BUTTON(1)) == SDL_BUTTON(1) ) {
			status = MouseDown(x, y, 1);
			next_scroll /= SCROLL_WAIT;
		} else {
			next_scroll = 0;
		}
	}
	return(status);
}

/* Mouse hits activate us */
GUI_status 
GUI_ScrollBar:: MouseDown(int x, int y, int button)
{
	GUI_status status;
    
	/* Don't do anything if we're not linked to a widget */
	if ( target == NULL ) {
		return(GUI_PASS);
	}

	/* Scroll depending on where we were hit */
	status = GUI_REDRAW;
	if ( HitRect(x, y, sensitive_neg) ) {
		Scroll(-1);
	} else
	if ( HitRect(x, y, sensitive_pos) ) {
		Scroll(1);
	} else
	if ( HitRect(x, y, sensitive_mid) ) {
		int first, last;
		float percent;
		int bar_start, bar_end;
		int position;
		float percentage;
		
		percent = Percentage();
		
		if ( whichxy == SCROLLBAR_HORIZONTAL ) {
			bar_start = (int) (sensitive_mid.y + sensitive_mid.h*(1-full) - 
							percent*sensitive_mid.h*(1-full));
			bar_end = (int) (bar_start + sensitive_mid.h*full);
			target->Range(first, last);
			if (x<bar_start) {
				Scroll((int) ((last-first)*-full));
				}
			else if (x>=bar_end) {
				Scroll((int) ((last-first)*full));
				}
			else {
				/* Scale the position by the hit coordinate */
				percentage = (float)(x-sensitive_mid.x)/sensitive_mid.w;
				position = (int)(first+percentage*(last-first)+0.5);
				ScrollTo(position);
				}
			}
		else {
			bar_start = (int) (sensitive_mid.y + sensitive_mid.h*(1-full) - 
							percent*sensitive_mid.h*(1-full));
			bar_end = (int) (bar_start + sensitive_mid.h*full);

			target->Range(first, last);
			if (y<bar_start) {
				Scroll((int) ((last-first)*-full));
				}
			else if (y>=bar_end) {
				Scroll((int) ((last-first)*full));
				}
			else {
				/* Scale the position by the hit coordinate */
				percentage = (float)(y-sensitive_mid.y)/sensitive_mid.h;
				position = (int)(first+percentage*(last-first)+0.5);
				ScrollTo(position);
				}
			}
		
	} else {
		status = GUI_PASS;
	}
	if ( status == GUI_REDRAW ) {
		next_scroll = SDL_GetTicks()+SCROLL_WAIT*SCROLL_INTERVAL;
	} else {
		next_scroll = 0;
	}
	return(status);
}

float
GUI_ScrollBar:: Percentage(void)
{
	int first, last, current;
	float percent;
	
	current = target->Scroll(0);
	target->Range(first,last);
	percent = current;
	percent /= last-first;
	percent = fabs(percent);
	return(percent);
}

/* The functions to activate the scrolling */
void
GUI_ScrollBar:: Scroll(int amount)
{
	target->Scroll(amount);
}
void
GUI_ScrollBar:: ScrollTo(int position)
{
	target->Scroll(position-target->Scroll(0));
}


/* Passed the sensitive images, orientation, and initial widget */
GUI_ScrollButtons:: GUI_ScrollButtons(int x1, int y1, SDL_Surface *image1,
			SDL_Rect &middle, int x2, int y2, SDL_Surface *image2,
                        	orientation direction, float percent_full,
							GUI_Scrollable *widget)
 : GUI_ScrollBar(direction, percent_full, widget)
{
	/* Set the image and rectangle of the negative button */
	image_neg = image1;
	sensitive_neg.x = x1;
	sensitive_neg.y = y1;
	sensitive_neg.w = image1->w;
	sensitive_neg.h = image1->h;

	/* Set the image and rectangle of the positive button */
	image_pos = image2;
	sensitive_pos.x = x2;
	sensitive_pos.y = y2;
	sensitive_pos.w = image2->w;
	sensitive_pos.h = image2->h;

	/* Set the middle sensitive rectangle */
	sensitive_mid = middle;
	bar_area.x = sensitive_mid.x+1;
	bar_area.y = sensitive_mid.y+1;
	bar_area.w = sensitive_mid.w-2;
	bar_area.h = sensitive_mid.h-2;

	/* Find the bounds of this widget */
	FindBounds();
}

/* Show the widget  */
void
GUI_ScrollButtons:: Display(void)
{
	float percent;
	SDL_Rect control;
	
	SDL_BlitSurface(image_neg, NULL, screen, &sensitive_neg);
	SDL_BlitSurface(image_pos, NULL, screen, &sensitive_pos);
	SDL_FillRect(screen,&bar_area,back_color);
	percent = Percentage();
	control.x = bar_area.x;
	control.w = bar_area.w;
	control.h = (Uint16) (bar_area.h * full + 0.5);
	control.y = (Sint16) (bar_area.y + bar_area.h*(1-full) - percent*bar_area.h*(1-full) + 0.5);
	SDL_FillRect(screen,&control,fore_color);	
}

/* change FG/BG colors and transparency */
void GUI_ScrollButtons::SetColoring(Uint8 fr,Uint8 fg,Uint8 fb, int bg_opaque,
				Uint8 br, Uint8 bg, Uint8 bb)
{
	fore_color = SDL_MapRGBA(screen->format,fr,fg,fb,0);
	back_color = SDL_MapRGBA(screen->format,br,bg,bb,0);
}

