
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GUI.h"
#include "GUI_widgets.h"
#include "GUI_output.h"


/* Variables that we'll need to clean up */
GUI *gui;
GUI_TermWin *terminal;
enum image_names {
	IMAGE_HELLO,
	IMAGE_HELLO2,
	IMAGE_SCROLL_UP,
	IMAGE_SCROLL_DN,
	NUM_IMAGES
};
char *image_files[NUM_IMAGES] = {
	"hello.bmp", "hello2.bmp", "scroll_up.bmp", "scroll_dn.bmp"
};
SDL_Surface *images[NUM_IMAGES];


void ShowChar(SDLKey key, Uint16 unicode)
{
	if ( unicode && (unicode <= 255) ) {
		Uint8 ch;

		ch = (Uint8)unicode;
		terminal->AddText((char *)&ch, 1);
	}
}

void cleanup(void)
{
	int i;

	/* Delete the GUI */
	delete gui;

	/* Clean up any images we have */
	for ( i=0; i<NUM_IMAGES; ++i ) {
		if ( images[i] ) {
			SDL_FreeSurface(images[i]);
		}
	}
}

void Output(SDL_Surface *screen, const char *title, const char *text)
{
	GUI_Output *output;

	output = GUI_CreateOutput(screen, 60, 5, NULL);
	if ( output ) {
		unsigned int i, pos;
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
	int x, y, i;
	int error;
	GUI_Widget *widget;
	SDL_Rect null_rect = { 0, 0, 0, 0 };
	GUI_Menu *menu;

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
	gui = new GUI(screen);

	/* Load our images */
	for ( i=0; i<NUM_IMAGES; ++i ) {
		images[i] = NULL;
	}
	error = 0;
	for ( i=0; i<NUM_IMAGES; ++i ) {
		images[i] = SDL_LoadBMP(image_files[i]);
		if ( images[i] == NULL ) {
			fprintf(stderr, "Couldn't load '%s': %s\n",
					image_files[i], SDL_GetError());
			++error;
		}
	}
	if ( error ) {
		cleanup();
		exit(1);
	}

	/* Add our interface widgets:
	   We want a gray background and a centered image button.
	 */
	x = (screen->w-images[IMAGE_HELLO]->w)/2;
	y = (screen->h-images[IMAGE_HELLO]->h)/2;
	widget = new GUI_Button(NULL, x, y, images[IMAGE_HELLO],images[IMAGE_HELLO2]);
	gui->AddWidget(widget);

	/* We also want a small text window with scroll buttons */
	x = images[IMAGE_SCROLL_UP]->w;
	terminal = new GUI_TermWin(x, 18, screen->w-x, 32, NULL, ShowChar, 32);
	terminal->AddText("Keystrokes will go here: ");
	gui->AddWidget(terminal);
	y = images[IMAGE_SCROLL_UP]->h;
	widget = new GUI_ScrollButtons(0,18, images[IMAGE_SCROLL_UP], null_rect,
					0, y+18, images[IMAGE_SCROLL_DN],
					SCROLLBAR_VERTICAL, terminal);
	gui->AddWidget(widget);

	/* Add a small menu which has no sense */
	menu=new GUI_Menu(gui,screen->w,NULL);
	gui->AddWidget(menu);
	menu->AddSubmenu(1,"File");
	menu->AddMenuitem(1,11,"Don't quit...",NULL);

	/* Run the GUI, and then clean up when it's done. */
	gui->Run(NULL);
	Output(screen,"-= Thanks =-","Thanks for trying the C++ GUI interface");
	cleanup();
	exit(0);

	/* To make the compiler happy */
	return(0);
}
