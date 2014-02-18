/*
    GUILIB:  An example GUI framework library for use with SDL
    Copyright (C) 1997  Sam Lantinga

    This program is free sof
	tware; you can redistribute it and/or modify
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

#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "GUI_termwin.h"
#include "GUI_loadimage.h"

#define KEYREPEAT_TIME	100		/* Repeat every 100 ms */

GUI_TermWin:: GUI_TermWin(int x, int y, int w, int h, SDL_Surface *Font,
				int (*KeyProc)(SDL_Keycode key, char *text), int scrollback)
 : GUI_Scrollable(NULL, x, y, w, h)
{
	/* The font surface should be a 16x16 character pixmap */
	if ( Font == NULL ) {
		font = GUI_DefaultFont();
	} else {
		font = Font;
	}
	charh = font->h/16;
	charw = font->w/16;
	rows = h/(charh-1);   /* Bottom row in font is not displayed */
	cols = w/charw;
	if ( scrollback == 0 ) {
		scrollback = rows;
	}
	total_rows = scrollback;

	/* Allocate and clear the character buffer */
	vscreen = new Uint8[total_rows*cols];
	Clear();

	/* Set the user-defined keyboard handler */
	keyproc = KeyProc;
	repeat_key = SDL_SCANCODE_UNKNOWN;
	repeat_unicode = 0;

	/* Set key event translation on */
	// TBD translated = SDL_EnableUNICODE(1);
}

GUI_TermWin:: ~GUI_TermWin()
{
	delete[] vscreen;

	/* Reset key event translation */
	// TBD SDL_EnableUNICODE(translated);
}

void
GUI_TermWin:: Display(void)
{
	int row, i, j;
	Uint8 ch;
	SDL_Rect src;
	SDL_Rect dst;
	SDL_Rect cursor;

	row = first_row+scroll_row;
	if ( row < 0 ) {
		row = total_rows+row;
	}
	src.w = charw;
	src.h = charh-1;
	dst.w = charw;
	dst.h = charh-1;
	for ( i=0; i<rows; ++i ) {
		for ( j=0; j<cols; ++j ) {
			ch = vscreen[row*cols+j];
			src.x = (ch%16)*charw;
			src.y = (ch/16)*charh;
			dst.x = area.x+j*charw;
			dst.y = area.y+i*(charh-1);
			SDL_BlitSurface(font, &src, screen, &dst);
		}
		row = (row+1)%total_rows;
	}
	cursor.x = area.x+cur_col*charw;
	cursor.w = charw;
	cursor.y = area.y+(cur_row-scroll_row)*(charh-1)+(charh-2);
	cursor.h = 1;
	SDL_FillRect(screen,&cursor,cursor_color);
	changed = 0;
}

int
GUI_TermWin:: Scroll(int amount)
{
	if ( amount ) {
		scroll_row += amount;
		if ( scroll_row < scroll_min ) {
			scroll_row = scroll_min;
		} else
		if ( scroll_row > scroll_max ) {
			scroll_row = scroll_max;
		}
		changed = 1;
	}
	return(scroll_row);
}

void
GUI_TermWin:: Range(int &first, int &last)
{
	first = scroll_min; last = scroll_max;
}

GUI_status
GUI_TermWin:: KeyDown(SDL_Keysym key)
{
    GUI_status status;
    int keystat;

    status = GUI_PASS;
    if (key.sym == SDLK_BACKSPACE ||
        key.sym == SDLK_DELETE    ||
        key.sym == SDLK_LEFT      ||
        key.sym == SDLK_RIGHT     ||
        key.sym == SDLK_DOWN      ||
        key.sym == SDLK_UP        ||
        key.sym == SDLK_RETURN) {
            keystat = keyproc(key.sym, NULL);
            if (keystat)
                status = GUI_QUIT;
            else {
                status = GUI_YUM;
            }
    }
    return(status);
}

GUI_status
GUI_TermWin:: TextInput(char *text)
{
    GUI_status status;
    int keystat;
    
    status = GUI_PASS;
    if (strlen(text) != 1)
        return(status);
    if ( keyproc ) {
        keystat = keyproc(SDLK_UNKNOWN, text);
        if (keystat)
            status = GUI_QUIT;
        else {
            status = GUI_YUM;
        }
    }
    return(status);
}

void
GUI_TermWin:: NewLine(void)
{
	if ( cur_row == (rows-1) ) {
		int newrow;

		first_row = (first_row+1)%total_rows;
		newrow = (first_row+rows-1)%total_rows;
		cur_prompt_row--;
		memset(&vscreen[newrow*cols], ' ', cols);
	} else {
		cur_row += 1;
	}
	cur_col = 0;
}

void
GUI_TermWin:: AddText(const char *text, int len)
{
	int row;

	while ( len-- ) {
		switch (*text) {
			case '\b': {
				/* Backspace */
				if ( cur_col > 0 ) {
					--cur_col;
					row = (first_row+cur_row)%total_rows;
					vscreen[row*cols+cur_col] = ' ';
				}
			}
			break;

			case '\r':
				/* Skip '\n' in "\r\n" sequence */
				if ( (len > 0) && (*(text+1) == '\n') ) {
					--len; ++text;
				}
				/* Fall through to newline */
			case '\n': {
				NewLine();
			}
			break;

			default: {  /* Put characters on screen */
				if ( cur_col == cols ) {
					NewLine();
				}
				row = (first_row+cur_row)%total_rows;
				vscreen[row*cols+cur_col] = *text;
				cur_col += 1;
			}
			break;
		}
		++text;
	}
	/* Reset any scrolling, and mark us for update on idle */
	scroll_row = 0;
	changed = 1;
}

void
GUI_TermWin:: AddPromptText(const char *text, int len, int edit_pos)
{
	int i;
	
	cur_col = cur_prompt_col;
	cur_row = cur_prompt_row;
	AddText(text,len);
	for (i=len;i<last_prompt_len;i++)
		AddText(" ",1);
	cur_row = cur_prompt_row;
	cur_col = cur_prompt_col + edit_pos;
	while (cur_col > cols-1) {
		cur_col -= cols;
		cur_row++;
		}
	last_prompt_len = len;
}

void
GUI_TermWin:: StartPrompt(void)
{
	cur_prompt_col = cur_col;
	cur_prompt_row = cur_row;
	last_prompt_len = 0;
}

void
GUI_TermWin:: AddText(const char *fmt, ...)
{
	char text[1024];		/* Caution!  Buffer overflow!! */
	va_list ap;

	va_start(ap, fmt);
	vsprintf(text, fmt, ap);
	AddText(text, strlen(text));
	va_end(ap);
}

void
GUI_TermWin:: Clear(void)
{
	/* Clear the virtual screen */
	memset(vscreen, ' ', total_rows*cols);

	/* Set the first row in buffer, display row in buffer, and
	   current cursor offset.
	 */
	first_row = total_rows-rows;		/* Actual address */
	scroll_min = -(total_rows-rows);	/* Virtual address */
	scroll_row = 0;				/* Virtual address */
	scroll_max = 0;				/* Virtual address */
	cur_row = 0;				/* Virtual address */
	cur_col = 0;				/* Virtual address */

	/* Mark the display for update */
	changed = 1;
}

int
GUI_TermWin:: Changed(void)
{
	return(changed);
}

GUI_status
GUI_TermWin:: Idle(void)
{
	GUI_status status;

	status = GUI_PASS;

	/* Check to see if display contents have changed */
	if ( changed ) {
		status = GUI_REDRAW;
		changed = 0;
	}
	return(status);
}

/* change FG/BG colors and transparency */
void GUI_TermWin::SetColoring(Uint8 fr,Uint8 fg,Uint8 fb, int bg_opaque,
				Uint8 br, Uint8 bg, Uint8 bb)
{
	SDL_Color colors[3]={{br,bg,bb,0},{fr,fg,fb,0}};
	if (bg_opaque)
	{
      SDL_SetPaletteColors(font->format->palette, colors, 0, 2);
	  SDL_SetColorKey(font,0,0);
	}
	else
	{
      SDL_SetPaletteColors(font->format->palette, &colors[1],1,1);
	  SDL_SetColorKey(font,SDL_TRUE,0);
	}
	cursor_color = SDL_MapRGBA(screen->format,fr,fg,fb,0);
}
