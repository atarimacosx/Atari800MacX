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

/* This is a C++ class for handling a GUI, and associated widgets */

#include <stdlib.h>

#include "GUI.h"

/* Number of widget elements to allocate at once */
#define WIDGET_ARRAYCHUNK	32


GUI:: GUI(SDL_Surface *display, int w, int h, int openGLSurface, GLuint textureID, SDL_Window *guiWindow)
{
	screen = display;
	openGL = openGLSurface;
	GLTextureID = textureID;
    window = guiWindow;
	numwidgets = 0;
	maxwidgets = 0;
	widgets = NULL;
	width = w;
	height = h;
	guiTexCoord[0] = 0.0f;
	guiTexCoord[1] = 0.0f;
	guiTexCoord[2] = (GLfloat) width / display->w;
	guiTexCoord[3] = (GLfloat) height / display->h;
}

GUI:: ~GUI()
{
	if ( widgets != NULL ) {
		for ( int i=0; i<numwidgets; ++i ) {
			delete widgets[i];
		}
		free(widgets);
	}
}

/* Add a widget to the GUI.
   The widget will be automatically deleted when the GUI is deleted.
 */
int
GUI:: AddWidget(GUI_Widget *widget)
{
	int i;

	/* Look for deleted widgets */
	for ( i=0; i<numwidgets; ++i ) {
		if ( widgets[i]->Status() == WIDGET_DELETED ) {
			delete widgets[i];
			break;
		}
	}
	if ( i == numwidgets ) {
		/* Expand the widgets array if necessary */
		if ( numwidgets == maxwidgets ) {
			GUI_Widget **newarray;
			int maxarray;

			maxarray = maxwidgets + WIDGET_ARRAYCHUNK;
			if ( (newarray=(GUI_Widget **)realloc(widgets,
					maxarray*sizeof(*newarray))) == NULL ) {
				return(-1);
			}
			widgets = newarray;
			maxwidgets = maxarray;
		}
		++numwidgets;
	}
	widgets[i] = widget;
	widget->SetDisplay(screen);
	return(0);
}

void
GUI:: Display(void)
{
	int i;

	for ( i=0; i<numwidgets; ++i ) {
		if ( widgets[i]->Status() == WIDGET_VISIBLE ) {
			widgets[i]->Display();
		}
	}
	if (openGL) {
		glBindTexture(GL_TEXTURE_2D, GLTextureID);
		glEnable(GL_TEXTURE_2D);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screen->w, screen->h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
						screen->pixels);
		glBegin(GL_QUADS);
		glTexCoord2f(guiTexCoord[0], guiTexCoord[1]); glVertex2i(0, 0);
		glTexCoord2f(guiTexCoord[2], guiTexCoord[1]); glVertex2i(width, 0);
		glTexCoord2f(guiTexCoord[2], guiTexCoord[3]); glVertex2i(width,height);
		glTexCoord2f(guiTexCoord[0], guiTexCoord[3]); glVertex2i(0, height);
		glEnd();

		// Now show all changes made to the textures
        SDL_GL_SwapWindow(window);

		// Added this to work around problem in 10.4 where if OpenGL window was occluded, the frame rate
		//   would drop to half normal (30fps). 
		glFlush();
		}
#if 0 // TBD
	else
		SDL_UpdateRect(screen, 0, 0, 0, 0);
#endif
}

/* Function to handle a GUI status */
void
GUI:: HandleStatus(GUI_status status)
{
	switch (status) {
		case GUI_QUIT:
			running = 0;
			break;
		case GUI_REDRAW:
			display = 1;
			break;
		default:
			break;
	}
}

/* Handle an event, passing it to widgets until they return a status */
void 
GUI:: HandleEvent(const SDL_Event *event)
{
	int i;
	GUI_status status;

	switch (event->type) {
		/* SDL_QUIT events quit the GUI */
		case SDL_QUIT:
			status = GUI_QUIT;
			break;

		/* Keyboard and mouse events go to widgets */
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
        case SDL_TEXTINPUT:
			/* Go through widgets, topmost first */
			status = GUI_PASS;
			for (i=numwidgets-1; (i>=0)&&(status==GUI_PASS); --i) {
				if ( widgets[i]->Status() == WIDGET_VISIBLE ) {
					status = widgets[i]->HandleEvent(event);
				}
			}
			break;

		/* Ignore unhandled events */
		default:
			status = GUI_PASS;
			break;
	}
	HandleStatus(status);
}

/* Run the GUI.
   This returns when either a widget requests a quit, the idle
   function requests a quit, or the SDL window has been closed.
 */
void
GUI:: Run(GUI_IdleProc idle, int once, int multitaskfriendly)
{
	int i;
	SDL_Event event;

	/* If there's nothing to do, return immediately */
	if ( (numwidgets == 0) && (idle == NULL) ) {
		return;
	}

	running = 1;
	if ( ! once ) {
		display = 1;
	}
	do {
		/* Garbage collection */
		for ( i=0; i<numwidgets; ++i ) {
			if ( widgets[i]->Status() == WIDGET_DELETED ) {
				delete widgets[i];
				for ( int j=i+1; j<numwidgets; ++j ) {
					widgets[j-1] = widgets[j];
				}
				--numwidgets;
			}
		}

		/* Display widgets if necessary */
		if ( display ) {
			Display();
			display = 0;
		}

///////////////////////////////////////////////////////////////// Polling is time consuming - instead:
		if (multitaskfriendly && (idle==NULL))
		{
		  SDL_WaitEvent(&event);
		  HandleEvent(&event);
		}
		else
/////////////////////////////////////////////////////////////////
		/* Handle events, or run idle functions */
		if ( SDL_PollEvent(&event) )
		{
			/* Handle all pending events */
			do {
				HandleEvent(&event);
			} while ( SDL_PollEvent(&event) );
		}
		else
		{
			if ( idle != NULL )
			{
				HandleStatus(idle());
			}
			for ( i=numwidgets-1; i>=0; --i ) {
				HandleStatus(widgets[i]->Idle());
			}
		}
		SDL_Delay(10);
	} while ( running && ! once );
}
