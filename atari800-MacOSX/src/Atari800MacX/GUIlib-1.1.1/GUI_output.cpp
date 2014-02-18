
/* This is a set of C functions wrapping C++ classes for popup text output */

#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "GUI.h"
#include "GUI_widgets.h"
#include "GUI_loadimage.h"
#include "GUI_output.h"

/************************************************************************/
/* C functions for GUI output window support                            */
/*                                                                      */
/************************************************************************/

static GUI_status OK_ButtonProc(void *statusp)
{
	*((int *)statusp) = 1;
	return(GUI_QUIT);
}
static GUI_status Cancel_ButtonProc(void *statusp)
{
	*((int *)statusp) = 0;
	return(GUI_QUIT);
}

extern "C" {

/* The (really C++) structure holding information for popup text output */
struct GUI_Output {
	int visible;
	SDL_Surface *screen;
	GUI_TermWin *window;
	GUI_Area *frame_inner;
	GUI_Area *frame_outer;
	SDL_Surface *behind;
};

/* Function to create a hidden output window 'width' by 'height' characters.
   The 'font' argument should either be a pointer to a 16x16 character font
   image (with an extra pixel row under each row of characters), or NULL to
   use a default internal 8x8-pixel font.
 */
GUI_Output *GUI_CreateOutput(SDL_Surface *screen,
				int width, int height, SDL_Surface *font)
{
	GUI_Output *output;
	int w, h, x, y;

	/* Create a new output window */
	output = new GUI_Output;
	output->visible = 0;
	output->screen = screen;
	if ( font == NULL ) {
		font = GUI_DefaultFont();
	}

	/* Allocate the text window */
	w = width * (font->w/16);
	h = height * ((font->h/16)-1);
	x = (screen->w - w)/2;
	y = (screen->h - h)/2;
	output->window = new GUI_TermWin(x, y, w, h, font);

	/* Allocate the frame areas */
	w += 2; h += 2;
	x -= 1; y -= 1;
	output->frame_inner = new GUI_Area(x, y, w, h, 255, 255, 255);
	w += 2; h += 2;
	x -= 1; y -= 1;
	output->frame_outer = new GUI_Area(x, y, w, h, 0, 0, 0);

	/* Allocate a save buffer for the area behind it */
	output->behind = SDL_AllocSurface(SDL_SWSURFACE, w, h,
				screen->format->BitsPerPixel,
				screen->format->Rmask,
				screen->format->Gmask,
				screen->format->Bmask, 0);

	/* Return the output window */
	return(output);
}

/* Add output to an output window.  If the window is visible, the output
   will appear immediately.  Note that the output windows are not overlays,
   and any normal SDL drawing will overwrite the output window display.
   If output causes the window to scroll, previous text will be lost.
*/
void GUI_AddOutput(GUI_Output *output, const char *fmt, ...)
{
	va_list ap;
	char text[4096];

	/* Get the text string */
	va_start(ap, fmt);
	vsprintf(text, fmt, ap);	/* CAUTION: possible buffer overflow */
	va_end(ap);

	/* Add it to the window */
	output->window->AddText(text, strlen(text));
}

/* Clear the contents of an output window */
void GUI_ClearOutput(GUI_Output *output)
{
	output->window->Clear();
}

/* Show an output window, saving the area behind the window, and wait for
   keyboard or mouse press input if 'wait' is non-zero.
 */
void GUI_ShowOutput(GUI_Output *output, int wait)
{
	/* Display the text output window */
	output->frame_outer->SetDisplay(output->screen);
	output->frame_inner->SetDisplay(output->screen);
	output->window->SetDisplay(output->screen);
	if ( output->behind ) {
		SDL_Rect src;

		src.x = output->frame_outer->X();
		src.y = output->frame_outer->Y();
		src.w = output->frame_outer->W();
		src.h = output->frame_outer->H();
		SDL_BlitSurface(output->screen, &src, output->behind, NULL);
	}
	output->frame_outer->Display();
	output->frame_inner->Display();
	output->window->Display();
	SDL_UpdateRect(output->screen, 0, 0, 0, 0);
	output->visible = 1;

	/* Pump the event queue, waiting for key and mouse press events */
	if ( wait ) {
		SDL_Event event;

		while ( ! SDL_PeepEvents(&event, 1, SDL_GETEVENT,
				(SDL_KEYDOWNMASK|SDL_MOUSEBUTTONDOWNMASK)) ) {
			SDL_Delay(20);
			SDL_PumpEvents();
		}
	}
}

/* Hide an output window, restoring the previous contents of the display */
void GUI_HideOutput(GUI_Output *output)
{
	if ( output->behind ) {
		SDL_Rect dst;

		dst.x = output->frame_outer->X();
		dst.y = output->frame_outer->Y();
		dst.w = output->frame_outer->W();
		dst.h = output->frame_outer->H();
		SDL_BlitSurface(output->behind, NULL, output->screen, &dst);
		SDL_UpdateRects(output->screen, 1, &dst);
	}
	output->visible = 0;
}

/* Delete an output window */
void GUI_DeleteOutput(GUI_Output *output)
{
	if ( output ) {
		if ( output->visible ) {
			GUI_HideOutput(output);
		}
		if ( output->window ) {
			delete output->window;
			output->window = NULL;
		}
		if ( output->behind ) {
			SDL_FreeSurface(output->behind);
			output->behind = NULL;
		}
		delete output;
	}
}


}; /* Extern C */
