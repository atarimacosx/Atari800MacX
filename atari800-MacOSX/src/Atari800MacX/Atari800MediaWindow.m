/* Atari800MediaWindow.h - Window class external
   to the SDL library to support Drag and Drop
   to the Window. For the Macintosh OS X SDL port 
   of Atari800
   Mark Grebe <atarimac@kc.rr.com>
   
   Based on the QuartzWindow.c implementation of
   libSDL.

*/
#import <Cocoa/Cocoa.h>
#import "Atari800MediaWindow.h"

extern int mediaStatusWindowOpen;

/* Subclass of NSWindow to allow for drag and drop and other specific functions  */
@implementation Atari800MediaWindow

/*------------------------------------------------------------------------------
*  init -
*-----------------------------------------------------------------------------*/
-(id) init
{
	id me;
	
	me = [super init];
	
	return(me);
}

/*------------------------------------------------------------------------------
*  init -
*-----------------------------------------------------------------------------*/
- (BOOL)windowShouldClose:(id)sender
{
	mediaStatusWindowOpen = FALSE;
	return YES;
}

- (BOOL)canBecomeKeyWindow
{
    return NO;
}

@end

