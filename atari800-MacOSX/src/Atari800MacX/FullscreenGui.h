/* FullscreenGUI.h - FullscreenGUI 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */

#include "GUI.h"
#include "GUI_termwin.h"
#include "GUI_loadimage.h"
#include "GUI_scrollbar.h"
#include "GUI_area.h"
#include "GUI_button.h"
#include "GUI_status.h"

extern "C" {
	int FullscreenGUIRun(SDL_Renderer *renderer, SDL_Window *window);
	int FullscreenCrashGUIRun(SDL_Renderer *renderer);
	void FullscreenGUIPrintf(const char *format,...);
	void FullscreenGUIPuts(const char *string);
}
