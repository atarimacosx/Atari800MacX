/* Atari800Window.m - Window class external
   to the SDL library to support Drag and Drop
   to the Window. For the Macintosh OS X SDL port 
   of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   
   Based on the QuartzWindow.c implementation of
   libSDL.

*/
#import "Atari800Window.h"
#import "Preferences.h"

/* Static window variables.  This class supports only a single window object */
static NSWindow *our_window = nil;
//static Atari800WindowView *our_window_view = nil;
static NSPoint windowOrigin;

/* Functions which provide an interface for C code to call this object's shared Instance functions */
void Atari800WindowCreate(NSWindow *window) {
    our_window = window;
}

void Atari800WindowAspectSet(int w, int h) {
    our_window.aspectRatio = NSMakeSize(w,h);
}

void Atari800WindowFullscreen() {
    [our_window toggleFullScreen:nil];
}

void Atari800OriginSet() {
    [Atari800Window applicationWindowOriginSetPrefs];
}

void Atari800OriginSave() {
    windowOrigin = [Atari800Window applicationWindowOriginSave];
}

void Atari800OriginRestore() {
    [Atari800Window applicationWindowOriginSet:windowOrigin];
}

void Atari800WindowCenter(void) {
    if (our_window)
        [our_window center];
}

void Atari800WindowDisplay(void) {
    if (our_window) {
        [our_window display];
		[[our_window standardWindowButton:NSWindowMiniaturizeButton] setEnabled:YES];
	}
}

int Atari800IsKeyWindow(void) {
    if (!our_window)
		return FALSE;
	else if ([our_window isKeyWindow] == YES)
        return TRUE;
	else
		return FALSE;
}

void Atari800MakeKeyWindow(void) {
	[our_window makeKeyWindow]; 
}

/* Subclass of NSWindow to allow for drag and drop and other specific functions  */

@implementation Atari800Window
/*------------------------------------------------------------------------------
*  applicationWindowOriginSave - This method saves the position of the media status
*    window
*-----------------------------------------------------------------------------*/
+ (NSPoint)applicationWindowOriginSave
{
	NSRect frame = NSMakeRect(0,0,0,0);
	
	if (our_window)
		frame = [our_window frame];
	return(frame.origin);
}
 
/*------------------------------------------------------------------------------
*  applicationWindowOriginSetPrefs - This method sets the position of the application
*    window from the values stored in the preferences object.
*-----------------------------------------------------------------------------*/
+ (void)applicationWindowOriginSetPrefs
{
	windowOrigin = [[Preferences sharedInstance] applicationWindowOrigin];
	
	if (our_window) {
		if (windowOrigin.x != 59999.0)
			[our_window setFrameOrigin:windowOrigin];
		else
			[our_window center];
		}
}
   
/*------------------------------------------------------------------------------
*  applicationWindowOriginSet - This method sets the position of the application
*    window to the value specified.
*-----------------------------------------------------------------------------*/
+ (void)applicationWindowOriginSet:(NSPoint)origin
{
	if (our_window) {
		[our_window setFrameOrigin:origin];
		}
}
+ (NSWindow *)ourWindow
{
	return(our_window);
}

@end
