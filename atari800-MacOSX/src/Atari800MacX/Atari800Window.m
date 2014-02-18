/* Atari800Window.m - Window class external
   to the SDL library to support Drag and Drop
   to the Window. For the Macintosh OS X SDL port 
   of Atari800
   Mark Grebe <atarimac@kc.rr.com>
   
   Based on the QuartzWindow.c implementation of
   libSDL.

*/
#import "Atari800Window.h"
#import "SDLMain.h"
#import "SDL.h"
#import "Preferences.h"
#import "PasteManager.h"
#import <objc/runtime.h>

/* Variables in the C program which are set to request services from Objective-C */
extern int requestRedraw;
extern int requestQuit;
extern int copyStatus;

/* Static window variables.  This class supports only a single window object */
static NSWindow *our_window = nil;
//static Atari800WindowView *our_window_view = nil;
static NSPoint windowOrigin;

/* Functions which provide an interface for C code to call this object's shared Instance functions */
void Atari800WindowCreate(NSWindow *window) {
    our_window = window;
    //[ window registerForDraggedTypes:[NSArray arrayWithObjects:
    //                                  NSFilenamesPboardType, nil]]; // Register for Drag and Drop
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
#if 0
/*------------------------------------------------------------------------------
*  draggingEntered - Checks for a valid drag and drop to this window. 
*-----------------------------------------------------------------------------*/
- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender {
    NSPasteboard *pboard;
    NSDragOperation sourceDragMask;
    int i, filecount;
    NSString *suffix;

    sourceDragMask = [sender draggingSourceOperationMask];
    pboard = [sender draggingPasteboard];
    
    /* Check for filenames type drag */
    if ( [[pboard types] containsObject:NSFilenamesPboardType] ) {
        /* Check for copy being valid */
        if (sourceDragMask & NSDragOperationCopy) {
            /* Check here for valid file types */
            NSArray *files = [pboard propertyListForType:NSFilenamesPboardType];
            
            filecount = [files count];
            for (i=0;i<filecount;i++) {
                suffix = [[files objectAtIndex:0] pathExtension];
            
                if (!([suffix isEqualToString:@"atr"] ||
                      [suffix isEqualToString:@"ATR"] ||
                      [suffix isEqualToString:@"atx"] ||
                      [suffix isEqualToString:@"ATX"] ||
                      [suffix isEqualToString:@"pro"] ||
                      [suffix isEqualToString:@"PRO"] ||
                      [suffix isEqualToString:@"a8s"] ||
                      [suffix isEqualToString:@"a8c"] ||
                      [suffix isEqualToString:@"A8S"] ||
                      [suffix isEqualToString:@"car"] ||
                      [suffix isEqualToString:@"CAR"] ||
                      [suffix isEqualToString:@"rom"] ||
                      [suffix isEqualToString:@"ROM"] ||
                      [suffix isEqualToString:@"bin"] ||
                      [suffix isEqualToString:@"BIN"] ||
                      [suffix isEqualToString:@"xfd"] ||
                      [suffix isEqualToString:@"XFD"] ||
                      [suffix isEqualToString:@"dcm"] ||
                      [suffix isEqualToString:@"DCM"] ||
                      [suffix isEqualToString:@"cas"] ||
                      [suffix isEqualToString:@"CAS"] ||
                      [suffix isEqualToString:@"xex"] ||
                      [suffix isEqualToString:@"XEX"] ||
                      [suffix isEqualToString:@"SET"] ||
                      [suffix isEqualToString:@"set"]))
                    return NSDragOperationNone;
                }
            return NSDragOperationCopy; 
        }
    }
    return NSDragOperationNone;
}

/*------------------------------------------------------------------------------
*  performDragOperation - Executes the actual drap and drop of a filename. 
*-----------------------------------------------------------------------------*/
- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
    NSPasteboard *pboard;
    NSDragOperation sourceDragMask;
    int i, filecount;

    sourceDragMask = [sender draggingSourceOperationMask];
    pboard = [sender draggingPasteboard];

    /* Check for filenames type drag */
    if ( [[pboard types] containsObject:NSFilenamesPboardType] ) {
       NSArray *files = [pboard propertyListForType:NSFilenamesPboardType];
       
       /* For each file in the drag, load it into the emulator */
       filecount = [files count];
       for (i=0;i<filecount;i++)
            [SDLMain loadFile:[files objectAtIndex:i]];  
    }
    return YES;
}
#endif
+ (NSWindow *)ourWindow
{
	return(our_window);
}

@end

/* Delegate for our NSWindow to send SDLQuit() on close */
@implementation Atari800WindowDelegate
- (NSDragOperation)ourDraggingEntered:(id <NSDraggingInfo>)sender {
    return NSDragOperationNone;
}
@end
