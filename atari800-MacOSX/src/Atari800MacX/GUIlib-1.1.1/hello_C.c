
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GUI_C.h"
#include "GUI_output.h"

/* The button image */
#define IMAGE_FILE	"hello.bmp"

/* Variables that we'll need to clean up */
CGUI *gui;
SDL_Surface *image = NULL;

/* Functions that we'll use to implement our interface.
   Because we're using C, we can't use the predefined widget classes.
*/
void fill_rectangle(widget_info *info)
{
	SDL_Color *rgb;
	Uint32 color;

	rgb = (SDL_Color *)info->widget_data;
	color = SDL_MapRGB(info->screen->format, rgb->r, rgb->g, rgb->b);
	SDL_FillRect(info->screen, &info->area, color);
}
void draw_image(widget_info *info)
{
	SDL_Surface *image;

	image = (SDL_Surface *)info->widget_data;
	SDL_BlitSurface(image, NULL, info->screen, &info->area);
}
GUI_status button_quit(widget_info *info, const SDL_Event *event)
{
	if ( event->type == SDL_MOUSEBUTTONDOWN ) {
		/* We are guaranteed that this event is in our area */
		return(GUI_QUIT);
	}
	return(GUI_PASS);
}

void cleanup(void)
{
	if ( image ) {
		SDL_FreeSurface(image);
	}
	GUI_Destroy(gui);
}

void Output(SDL_Surface *screen, const char *title, const char *text)
{
	GUI_Output *output;

	output = GUI_CreateOutput(screen, 60, 5, NULL);
	if ( output ) {
		int i, pos;
		char formatted_text[1024];
#if 1
		/* Center the text in our window */
		pos = 0;
		formatted_text[pos++] = '\n';
		formatted_text[pos++] = '\n';
		for ( i=0; i<((60-strlen(text))/2); ++i ) {
			formatted_text[pos++] = ' ';
		}
		formatted_text[pos] = '\0';
		strcat(formatted_text, text);
#else
		strcpy(formatted_text, text);
#endif

		/* Run the output window with our text */
		GUI_AddOutput(output, formatted_text);
		GUI_ShowOutput(output, 1);
		GUI_DeleteOutput(output);
	}
}

main(int argc, char *argv[])
{
	SDL_Surface *screen;
	SDL_Color gray;
	int x, y;
	int error;
	CGUI_Widget *widget;

	/* Initialize SDL */
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	/* Get a video mode for display */
	screen = SDL_SetVideoMode(640, 480, 0, SDL_SWSURFACE);
	if ( screen == NULL ) {
		fprintf(stderr, "Couldn't set video mode: %s\n",SDL_GetError());
		exit(1);
	}
	SDL_WM_SetCaption("GUI Hello!", "hello");

	/* Create a GUI container */
	gui = GUI_Create(screen);

	/* Add our interface widgets:
	   We want a gray background and a centered image button.
	 */
	error = 0;
	gray.r = 128;
	gray.g = 128;
	gray.b = 128;
	widget = GUI_Widget_Create(&gray, 0, 0, screen->w, screen->h,
					fill_rectangle, NULL, NULL);
	error += GUI_AddWidget(gui, widget);
	image = SDL_LoadBMP(IMAGE_FILE);
	if ( image == NULL ) {
		cleanup();
		fprintf(stderr, "Couldn't load "IMAGE_FILE": %s\n",
						SDL_GetError());
		exit(1);
	}
	x = (screen->w-image->w)/2;
	y = (screen->h-image->h)/2;
	widget = GUI_Widget_Create(image, x, y, image->w, image->h,
					draw_image, button_quit, NULL);
	error += GUI_AddWidget(gui, widget);

	/* Check to see if there were errors loading the widgets */
	if ( error ) {
		cleanup();
		fprintf(stderr, "Out of memory\n");
	}

	/* Run the GUI, and then clean up when it's done. */
	GUI_Run(gui, NULL);
	cleanup();
	Output(screen, "-= Thanks =-", "Thanks for trying the C GUI interface");
	exit(0);
}
