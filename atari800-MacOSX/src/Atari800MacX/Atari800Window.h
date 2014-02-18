/* Atari800Window.h - Window class external
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
@interface Atari800Window : PasteWindow
+ (void)createApplicationWindow:(int)width:(int)height;
+ (NSPoint)applicationWindowOriginSave;
+ (void)applicationWindowOriginSetPrefs;
+ (void)applicationWindowOriginSet:(NSPoint)origin;
- (void)resizeApplicationWindow:(int)width:(int)height;
- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender;
- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender;
- (void)display;
- (void)superDisplay;
+ (NSWindow *)ourWindow;
@end

/* Delegate for our NSWindow to send SDLQuit() on close */
@interface Atari800WindowDelegate : NSObject
{}
- (BOOL)windowShouldClose:(id)sender;
@end

/* Subclass of NSQuickDrawView for the window's subview */
@interface Atari800WindowView : NSQuickDrawView
{}
@end

