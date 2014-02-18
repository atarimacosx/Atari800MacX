/* Atari800MediaWindow.h - Window class external
   to the SDL library to support Drag and Drop
   to the Window. For the Macintosh OS X SDL port 
   of Atari800
   Mark Grebe <atarimac@cox.net>
   
   Based on the QuartzWindow.c implementation of
   libSDL.

*/
#import <Cocoa/Cocoa.h>
#import "PasteWindow.h"

/* Subclass of NSWindow to allow for drag and drop and other specific functions  */
@interface Atari800MediaWindow : PasteWindow
- (BOOL)windowShouldClose:(id)sender;
@end

