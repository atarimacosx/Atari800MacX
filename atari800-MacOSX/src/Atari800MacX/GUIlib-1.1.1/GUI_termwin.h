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

/* A simple dumb terminal scrollable widget */

#ifndef _GUI_termwin_h
#define _GUI_termwin_h

#include "GUI_widget.h"
#include "GUI_scroll.h"


class GUI_TermWin : public GUI_Scrollable {

public:
	/* The font surface should be a 16x16 character pixmap */
	GUI_TermWin(int x, int y, int w, int h, SDL_Surface *font = NULL,
			int (*KeyProc)(SDLKey key, Uint16 unicode) = NULL, int scrollback = 0);
	~GUI_TermWin();

	/* Show the text window */
	virtual void Display(void);

	/* Function to scroll forward and backward */
	virtual int Scroll(int amount);

	/* Function to get the scrollable range */
	virtual void Range(int &first, int &last);

	/* Handle keyboard input */
	virtual GUI_status KeyDown(SDL_keysym key);
	virtual GUI_status KeyUp(SDL_keysym key);

	/* Function to add text to the visible buffer */
	virtual void AddText(const char *text, int len);
	virtual void AddText(const char *fmt, ...);
	
	/* Function to handle the user input */
	virtual void StartPrompt(void);
	virtual void AddPromptText(const char *text, int len, int edit_pos);

	/* Function to clear the visible buffer */
	virtual void Clear(void);

	/* Function returns true if the window has changed */
	virtual int Changed(void);

	/* GUI idle function -- run when no events pending */
	virtual GUI_status Idle(void);

	/* change FG/BG colors and transparency */
	virtual void SetColoring(Uint8 fr, Uint8 fg, Uint8 fb, int bg_opaque = 0,
					 Uint8 br = 0, Uint8 bg = 0, Uint8 bb = 0);

protected:
	/* The actual characters representing the screen */
	Uint8 *vscreen;

	/* The character forground color for use as a cursor */
	Uint32 cursor_color;

	/* Information about the size and position of the buffer */
	int total_rows;
	int rows, cols;
	int first_row;
	int cur_row;
	int cur_col;
	int cur_prompt_col;
	int cur_prompt_row;
	int last_prompt_len;
	int scroll_min, scroll_row, scroll_max;

	/* Font information used to display characters */
	SDL_Surface *font;
	int charw, charh;
	int translated;	/* Whether or not UNICODE translation was enabled */

	/* The keyboard handling function */
	int (*keyproc)(SDLKey key, Uint16 unicode);

	/* The last key that was pressed, along with key repeat time */
	SDLKey repeat_key;
	Uint16 repeat_unicode;
	Uint32 repeat_next;

	/* Flag -- whether or not we need to redisplay ourselves */
	int changed;

	/* Terminal manipulation routines */
	void NewLine(void);			/* <CR>+<LF> */
};

#endif /* _GUI_termwin_h */
