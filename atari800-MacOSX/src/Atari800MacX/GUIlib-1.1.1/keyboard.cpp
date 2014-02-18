
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GUI.h"
#include "GUI_termwin.h"


/* Variables that we'll need to clean up */
GUI *gui;

class KeyboardDisplay : public GUI_TermWin {
public:
	KeyboardDisplay(int x, int y, int w, int h, GUI_TermWin *otherwin) :
						GUI_TermWin(x, y, w, h, NULL) {
		textout = otherwin;
	}
	virtual GUI_status KeyDown(SDL_keysym key) {
		GUI_status status;

		KeyInput(key, 1);

		/* Look for Ctrl-Q, and mark us done when we see it */
		status = GUI_YUM;
		if ( (SDL_GetKeyState(NULL)[SDLK_q] == SDL_PRESSED) &&
				(SDL_GetModState() & KMOD_CTRL) ) {
			status = GUI_QUIT;
		}
		return(status);
	}
	virtual GUI_status KeyUp(SDL_keysym key) {
		KeyInput(key, 0);
		return(GUI_YUM);
	}

protected:
	GUI_TermWin *textout;

	void KeyInput(SDL_keysym keysym, int pressed) {
		char textbuf[BUFSIZ];
		unsigned char charbuf[3];
		int pos;

		sprintf(textbuf, "SDL key '%s' (0x%.2x)",
				SDL_GetKeyName(keysym.sym), keysym.scancode);
		if ( pressed ) {
			strcat(textbuf, " pressed!");
		} else {
			strcat(textbuf, " released!");
		}
		Clear();
		AddText(textbuf, strlen(textbuf));

		/* If the key was released, we're done */
		if ( ! pressed ) {
			return;
		}

		/* Print out the character that was pressed */
		pos = 0;
		if ( keysym.unicode && (keysym.unicode <= 255) ) {
			/* <BS> backs over a key */
			if ((keysym.unicode != '\b')&&(keysym.unicode <= 26)) {
				charbuf[pos++] = '^';
				charbuf[pos++] = keysym.unicode-1+'A';
			} else
				charbuf[pos++] = (unsigned char)keysym.unicode;
		}
		charbuf[pos] = 0;
		textout->AddText((char *)charbuf, strlen((char *)charbuf));
	}
};

main(int argc, char *argv[])
{
	SDL_Surface *screen;
	GUI_TermWin *output;
	GUI_TermWin *keywin;

	/* Initialize SDL */
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	/* Get a video mode for display */
	screen = SDL_SetVideoMode(320, 240, 0, SDL_SWSURFACE);
	if ( screen == NULL ) {
		fprintf(stderr, "Couldn't set video mode: %s\n",SDL_GetError());
		exit(1);
	}
	SDL_WM_SetCaption("SDL Keyboard Test", "keyboard");

	/* Create a GUI container */
	gui = new GUI(screen);

	/* Create the output windows */
	output = new GUI_TermWin(0, 1+8+1, screen->w, screen->h-10);
	gui->AddWidget(output);
	keywin = new KeyboardDisplay(0, 1, screen->w, 8, output);
	gui->AddWidget(keywin);

	/* Run the GUI, and then clean up when it's done. */
	gui->Run(NULL);
	delete gui;
	exit(0);

	/* Make the compiler happy */
	return(0);
}
