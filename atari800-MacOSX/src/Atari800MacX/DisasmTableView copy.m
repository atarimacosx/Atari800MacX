//  Created by Tony S. Wu on Thu Sep 26 2002.
//  No rights reserved.

#import "DisasmTableView.h"
#import "BreakpointsController.h"
#import "ControlManager.h"

@implementation DisasmTableView

/**********************/
/*  override methods  */
/**********************/

- (void)mouseDown: (NSEvent *)theEvent
{
    NSPoint eventPoint;	// the point where mouse was down
    
    //[super mouseDown: theEvent];	// forward this mouse event to NSTableView
    
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
    
	if (menuActionRow != -1)
    {
		if (menuActionColumn == 0) {
			Breakpoint *theBreakpoint;
			
			theBreakpoint = [[BreakpointsController sharedInstance] getBreakpointWithRow:menuActionRow];
			if (theBreakpoint == nil) 
				[[BreakpointsController sharedInstance] addBreakpointWithRow:menuActionRow];
			else
				[[BreakpointsController sharedInstance] toggleBreakpointEnableWithRow:menuActionRow];
			[[BreakpointsController sharedInstance] loadBreakpoints];
			[self reloadData];
		}
		//else
        //   [NSMenu popUpContextMenu: [self menu] withEvent: theEvent forView: self];
        // show the menu
    }
    else
    {
        menuActionRow = menuActionColumn = -1;
        // set both to -1 to indicate there wasn't any menu action
    }
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
    
    if (menuActionRow != -1)
    {
		Breakpoint *theBreakpoint;
		
		theBreakpoint = [[BreakpointsController sharedInstance] getBreakpointWithRow:menuActionRow];
		[[[self menu] itemAtIndex:1] setTarget:self];
		[[[self menu] itemAtIndex:2] setTarget:self];
		[[[self menu] itemAtIndex:3] setTarget:self];
		[[[self menu] itemAtIndex:4] setTarget:self];
		if (theBreakpoint == nil) {
			[[[self menu] itemAtIndex:0] setEnabled:YES];
			[[[self menu] itemAtIndex:1] setEnabled:NO];
			[[[self menu] itemAtIndex:2] setEnabled:NO];
			[[[self menu] itemAtIndex:3] setEnabled:NO];
			[[[self menu] itemAtIndex:4] setEnabled:YES];
		}
		else {
			[[[self menu] itemAtIndex:0] setEnabled:YES];
			[[[self menu] itemAtIndex:1] setEnabled:YES];
			[[[self menu] itemAtIndex:2] setEnabled:YES];
			[[[self menu] itemAtIndex:3] setEnabled:YES];
			[[[self menu] itemAtIndex:4] setEnabled:NO];
			if ([theBreakpoint isEnabledForPC]) {  
				[[[self menu] itemAtIndex:2] setTitle:@"Disable Breakpoint"];
			}
			else {
				[[[self menu] itemAtIndex:2] setTitle:@"Enable Breakpoint"];
			}
		}
			
        [NSMenu popUpContextMenu: [self menu] withEvent: theEvent forView: self];
        // show the menu
    }
    else
    {
        menuActionRow = menuActionColumn = -1;
        // set both to -1 to indicate there wasn't any menu action
    }
}

- (NSMenu *)menuForEvent: (NSEvent *)theEvent
{
    return nil;
}

- (IBAction) deleteBreakpoint:(id)sender
{
	[[BreakpointsController sharedInstance] deleteBreakpointWithRow:menuActionRow];
	[[BreakpointsController sharedInstance] loadBreakpoints];
	[self reloadData];
}

- (IBAction) editBreakpoint:(id)sender
{
	[[BreakpointsController sharedInstance] editBreakpointWithRow:menuActionRow];
	[[BreakpointsController sharedInstance] loadBreakpoints];
	[self reloadData];
}

- (IBAction) toggleEnableBreakpoint:(id)sender
{
	[[BreakpointsController sharedInstance] toggleBreakpointEnableWithRow:menuActionRow];
	[[BreakpointsController sharedInstance] loadBreakpoints];
	[self reloadData];
}

- (IBAction) addBreakpoint:(id)sender
{
	[[BreakpointsController sharedInstance] addBreakpointWithRow:menuActionRow];
	[[BreakpointsController sharedInstance] loadBreakpoints];
	[self reloadData];
}

/***************/
/*  accessors  */
/***************/

- (int)menuActionRow
{
    return menuActionRow;
}

- (int)menuActionColumn
{
    return menuActionColumn;
}

@end