/* BreakpointTableView.m - 
 BreakpointTableView class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "BreakpointTableView.h"
#import "Breakpoint.h"
#import "BreakpointsController.h"

@implementation BreakpointTableView

- (void)mouseDown: (NSEvent *)theEvent
{
    NSPoint eventPoint;	// the point where mouse was down
    
    eventPoint = [theEvent locationInWindow];	// get the event location from window
    eventPoint = [self convertPoint: eventPoint fromView: nil];
    // convert the location from window to BreakpointTableView
    
    menuActionRow = [super rowAtPoint: eventPoint];	// get the row index where mouse was down
    menuActionColumn = [super columnAtPoint: eventPoint];
    // get the column index where mouse was down
    
	if (menuActionRow == -1)
       menuActionColumn = -1;
        // set both to -1 to indicate there wasn't any menu action

    [super mouseDown: theEvent];	// forward this mouse event to NSTableView
    
}
- (void)rightMouseDown: (NSEvent *)theEvent
{
    NSPoint eventPoint;	// the point where mouse was down
    
    if (![self menu])	// no menu = no need to go any further
    {
        menuActionRow = menuActionColumn = -1;
        // set both to -1 to indicate there wasn't any menu action
        
        return;
    }
    
    eventPoint = [theEvent locationInWindow];	// get the event location from window
    eventPoint = [self convertPoint: eventPoint fromView: nil];
    // convert the location from window to MenuTableView
    
    menuActionRow = [super rowAtPoint: eventPoint];	// get the row index where mouse was down
    menuActionColumn = [super columnAtPoint: eventPoint];
    // get the column index where mouse was down
	
	[[[self menu] itemAtIndex:0] setTarget:self];
	[[[self menu] itemAtIndex:1] setTarget:self];
	[[[self menu] itemAtIndex:2] setTarget:self];
	[[[self menu] itemAtIndex:4] setTarget:self];
	[[[self menu] itemAtIndex:2] setEnabled:YES];
	[[[self menu] itemAtIndex:4] setEnabled:YES];

	if (menuActionRow == -1) {
		menuActionColumn = -1;
		[[[self menu] itemAtIndex:0] setEnabled:NO];
		[[[self menu] itemAtIndex:1] setEnabled:NO];
    }
	else {
		[[[self menu] itemAtIndex:0] setEnabled:YES];
		[[[self menu] itemAtIndex:1] setEnabled:YES];
	}
		
    [NSMenu popUpContextMenu: [self menu] withEvent: theEvent forView: self];
}

- (NSMenu *)menuForEvent: (NSEvent *)theEvent
{
    return nil;
}

- (IBAction) deleteBreakpoint:(id)sender
{
	[[BreakpointsController sharedInstance] deleteBreakpointWithIndex:menuActionRow];
	[[BreakpointsController sharedInstance] loadBreakpoints];
	[self reloadData];
}

- (IBAction) deleteAllBreakpoints:(id)sender
{
	[[BreakpointsController sharedInstance] deleteAllBreakpoints];
	[[BreakpointsController sharedInstance] loadBreakpoints];
	[self reloadData];
}

- (IBAction) editBreakpoint:(id)sender
{
	[[BreakpointsController sharedInstance] editBreakpointWithIndex:menuActionRow];
	[[BreakpointsController sharedInstance] loadBreakpoints];
	[self reloadData];
}

- (IBAction) addBreakpoint:(id)sender
{
	[[BreakpointsController sharedInstance] addBreakpointWithPC:0];
	[[BreakpointsController sharedInstance] loadBreakpoints];
	[self reloadData];
}

- (int) getActionRow
{
    return menuActionRow;
}

- (int) getActionColumn
{
    return menuActionColumn;
}

@end
