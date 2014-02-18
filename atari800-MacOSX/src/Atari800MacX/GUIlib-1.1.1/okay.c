
/* Program to pop up a quick "OK" message box  */

#include "SDL.h"
#include "GUI_output.h"


main(int argc, char *argv[])
{
	SDL_Surface *screen;

	if ( argv[1] == NULL ) {
		fprintf(stderr, "Usage: %s <message>\n", argv[0]);
		exit(1);
	}
		
	/* Initialize SDL */
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	/* Get a video mode for display */
	screen = SDL_SetVideoMode(320, 200, 0, SDL_SWSURFACE);
	if ( screen == NULL ) {
		fprintf(stderr, "Couldn't set video mode: %s\n",SDL_GetError());
		exit(1);
	}
	SDL_WM_SetCaption("Hi there!", "hiya");

	/* Pop up the message and quit */
	GUI_MessageBox(screen, "Message", argv[1], GUI_MBOK);
	exit(0);
}
