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

/* Variables in the C program which are set to request services from Objective-C */
extern int requestRedraw;
extern int requestQuit;
extern int copyStatus;

/* Static window variables.  This class supports only a single window object */
static Atari800Window *our_window = nil;
static Atari800WindowView *our_window_view = nil;
static NSPoint windowOrigin;

/* Functions which provide an interface for C code to call this object's shared Instance functions */
void Atari800WindowCreate(int width, int height) {
    [Atari800Window createApplicationWindow:width:height];
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

void Atari800WindowResize(int width, int height) {
    if (our_window)
        [our_window resizeApplicationWindow:width:height];
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
*  createApplicationWindow - Creates the emulator window of the given size. 
*-----------------------------------------------------------------------------*/
+ (void)createApplicationWindow:(int)width:(int)height
{
     unsigned int style;
     NSRect contentRect;
     char tempStr[40];
    
     /* Release the old window */
     if (our_window) {
        [our_window release];
        our_window = nil;
        our_window_view = nil;
        }

     /* Create the new window */
     contentRect = NSMakeRect (0, 0, width, height);
     style = NSTitledWindowMask;
     style |= (NSMiniaturizableWindowMask | NSClosableWindowMask);
	 if ([[Preferences sharedInstance] getBrushed]) 
		style |= NSTexturedBackgroundWindowMask;
     our_window = [ [ Atari800Window alloc ] 
                     initWithContentRect:contentRect
                     styleMask:style 
                     backing:NSBackingStoreBuffered
                     defer:NO ];

     [ our_window setAcceptsMouseMovedEvents:YES ];
     [ our_window setViewsNeedDisplay:NO ];
     [ our_window setTitle:@"Atari800MacX" ];
     [ our_window setDelegate:
            [ [ [ Atari800WindowDelegate alloc ] init ] autorelease ] ];
     [ our_window registerForDraggedTypes:[NSArray arrayWithObjects:
            NSFilenamesPboardType, nil]]; // Register for Drag and Drop
	
#define USE_QUICK_DRAW	
#ifdef USE_QUICK_DRAW
     /* Create thw window view and display it */
     our_window_view = [ [ Atari800WindowView alloc ] initWithFrame:contentRect ];
     [ our_window_view setAutoresizingMask: NSViewMinYMargin ];
     [ [ our_window contentView ] addSubview:our_window_view ];
     [ our_window_view release ];
     [ our_window makeKeyAndOrderFront:nil ];

     /* Pass the window pointers to libSDL through environment variables */
     sprintf(tempStr,"%d",(int)our_window);
     setenv("SDL_NSWindowPointer",tempStr,1);
	 sprintf(tempStr,"%d",(int)our_window_view);
	 setenv("SDL_NSQuickDrawViewPointer",tempStr,1);     
#else
	our_window_view = [[ [ NSView alloc ] initWithFrame:contentRect ] autorelease];
	[ our_window setContentView: our_window_view ];
	
	/* Pass the window pointers to libSDL through environment variables */
	sprintf(tempStr,"%d",(int)our_window);
	setenv("SDL_NSWindowPointer",tempStr,1);
	sprintf(tempStr,"%d",(int)our_window_view);
	setenv("SDL_NSQuartzViewPointer",tempStr,1);     
#endif	
}

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
   
/*------------------------------------------------------------------------------
*  resizeApplicationWindow - Resizes the emulator window to the given size. 
*-----------------------------------------------------------------------------*/
- (void)resizeApplicationWindow:(int)width:(int)height
{
     NSRect contentRect;
     
     /* Resize the window and the view */
     contentRect = NSMakeRect (0, 0, width, height);
     [ self setContentSize:contentRect.size ];
     [ our_window_view setFrame:contentRect ];
}

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

/*------------------------------------------------------------------------------
*  display - displays the window. Overridden to fix the minimize effect
*-----------------------------------------------------------------------------*/
- (void)display
{
    /* save current visible SDL surface */
    [ self cacheImageInRect:[ our_window_view frame ] ];
    
    /* let the window manager redraw controls, border, etc */
    [ super display ];
    
    /* restore visible SDL surface */
    [ self restoreCachedImage ];
    
}

/*------------------------------------------------------------------------------
*  superDisplay - displays the window. Called after reverting from full screen
*    as window title bar was not being properly redrawn.
*-----------------------------------------------------------------------------*/
- (void)superDisplay
{
    /* let the window manager redraw controls, border, etc */
    [ super display ];    
}
#if 0
- (void)becomeKeyWindow
{
	copyStatus = 1;
	printf("Reseting copy status\n");
	[super becomeKeyWindow];
}
#endif
+ (NSWindow *)ourWindow
{
	return(our_window);
}

@end

/* Delegate for our NSWindow to send SDLQuit() on close */
@implementation Atari800WindowDelegate
- (BOOL)windowShouldClose:(id)sender
{
    requestQuit = 1;
    return NO;
}

@end

@implementation Atari800WindowView
@end
