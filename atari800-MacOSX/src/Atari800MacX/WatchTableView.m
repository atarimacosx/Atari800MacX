/* WatchTableView.m - 
 WatchTableView class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 */

#import "BreakpointsController.h"
#import "ControlManager.h"
#import "WatchTableView.h"
#import "WatchDataSource.h"
#import "monitor.h"

#define ADD_INDEX			0
#define DELETE_INDEX		1
#define BYTE_INDEX			3
#define WORD_INDEX			4
#define UNSIGNED_INDEX		6
#define SIGNED_INDEX		7
#define HEX_INDEX			8
#define ASCII_INDEX			9
#define SHOW_MEMORY_INDEX	11
#define READ_BREAK_INDEX	13
#define WRITE_BREAK_INDEX	14
#define ACCESS_BREAK_INDEX	15
#define VALUE_BREAK_INDEX	16
#define EDIT_BREAK_INDEX	17
#define DEL_BREAK_INDEX	    18

@implementation WatchTableView

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
			unsigned short addr;
			
			addr = [[self dataSource] getAddressWithRow:menuActionRow];
			theBreakpoint = [[BreakpointsController sharedInstance] getBreakpointWithMem:addr];
			if (theBreakpoint == nil) 
				return;
			
			[[BreakpointsController sharedInstance] toggleBreakpointEnable:theBreakpoint];
			[[BreakpointsController sharedInstance] loadBreakpoints];
			[self reloadData];
		}
    }
    else
    {
        menuActionRow = menuActionColumn = -1;
        // set both to -1 to indicate there wasn't any menu action
    }

    [super mouseDown: theEvent];	// forward this mouse event to NSTableView
}

- (void)rightMouseDown: (NSEvent *)theEvent
{
    NSPoint eventPoint;	// the point where mouse was down
	int size, format, memAddr;
    
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
	
	[[[self menu] itemAtIndex:ADD_INDEX] setTarget:self];
	[[[self menu] itemAtIndex:DELETE_INDEX] setTarget:self];
	[[[self menu] itemAtIndex:BYTE_INDEX] setTarget:self];
	[[[self menu] itemAtIndex:WORD_INDEX] setTarget:self];
	[[[self menu] itemAtIndex:UNSIGNED_INDEX] setTarget:self];
	[[[self menu] itemAtIndex:SIGNED_INDEX] setTarget:self];
	[[[self menu] itemAtIndex:HEX_INDEX] setTarget:self];
	[[[self menu] itemAtIndex:ASCII_INDEX] setTarget:self];
	[[[self menu] itemAtIndex:SHOW_MEMORY_INDEX] setTarget:self];
	[[[self menu] itemAtIndex:READ_BREAK_INDEX] setTarget:self];
	[[[self menu] itemAtIndex:WRITE_BREAK_INDEX] setTarget:self];
	[[[self menu] itemAtIndex:ACCESS_BREAK_INDEX] setTarget:self];
	[[[self menu] itemAtIndex:VALUE_BREAK_INDEX] setTarget:self];
	[[[self menu] itemAtIndex:EDIT_BREAK_INDEX] setTarget:self];
	[[[self menu] itemAtIndex:DEL_BREAK_INDEX] setTarget:self];
	[[[self menu] itemAtIndex:ADD_INDEX] setEnabled:YES];
 	
	if (menuActionRow == -1) {
		menuActionColumn = -1;
		[[[self menu] itemAtIndex:DELETE_INDEX] setEnabled:NO];
		[[[self menu] itemAtIndex:BYTE_INDEX] setEnabled:NO];
		[[[self menu] itemAtIndex:WORD_INDEX] setEnabled:NO];
		[[[self menu] itemAtIndex:UNSIGNED_INDEX] setEnabled:NO];
		[[[self menu] itemAtIndex:SIGNED_INDEX] setEnabled:NO];
		[[[self menu] itemAtIndex:HEX_INDEX] setEnabled:NO];
		[[[self menu] itemAtIndex:ASCII_INDEX] setEnabled:NO];
		[[[self menu] itemAtIndex:BYTE_INDEX] setState:NSOffState];
		[[[self menu] itemAtIndex:WORD_INDEX] setState:NSOffState];
		[[[self menu] itemAtIndex:UNSIGNED_INDEX] setState:NSOffState];
		[[[self menu] itemAtIndex:SIGNED_INDEX] setState:NSOffState];
		[[[self menu] itemAtIndex:HEX_INDEX] setState:NSOffState];
		[[[self menu] itemAtIndex:ASCII_INDEX] setState:NSOffState];
		[[[self menu] itemAtIndex:SHOW_MEMORY_INDEX] setEnabled:NO];
		[[[self menu] itemAtIndex:READ_BREAK_INDEX] setEnabled:NO];
		[[[self menu] itemAtIndex:WRITE_BREAK_INDEX] setEnabled:NO];
		[[[self menu] itemAtIndex:ACCESS_BREAK_INDEX] setEnabled:NO];
		[[[self menu] itemAtIndex:VALUE_BREAK_INDEX] setEnabled:NO];
		[[[self menu] itemAtIndex:EDIT_BREAK_INDEX] setEnabled:NO];
		[[[self menu] itemAtIndex:DEL_BREAK_INDEX] setEnabled:NO];
    }
	else {
		[[[self menu] itemAtIndex:DELETE_INDEX] setEnabled:YES];
		[[[self menu] itemAtIndex:BYTE_INDEX] setEnabled:YES];
		[[[self menu] itemAtIndex:WORD_INDEX] setEnabled:YES];
		[[[self menu] itemAtIndex:UNSIGNED_INDEX] setEnabled:YES];
		[[[self menu] itemAtIndex:SIGNED_INDEX] setEnabled:YES];
		[[[self menu] itemAtIndex:HEX_INDEX] setEnabled:YES];
		[[[self menu] itemAtIndex:ASCII_INDEX] setEnabled:YES];
		[[[self menu] itemAtIndex:SHOW_MEMORY_INDEX] setEnabled:YES];
		
		size = [(WatchDataSource *) [super dataSource] getSize:menuActionRow];
		if (size == HALFWORD_SIZE) {
			[[[self menu] itemAtIndex:BYTE_INDEX] setState:NSOnState];
			[[[self menu] itemAtIndex:WORD_INDEX] setState:NSOffState];
		}
		else {
			[[[self menu] itemAtIndex:BYTE_INDEX] setState:NSOffState];
			[[[self menu] itemAtIndex:WORD_INDEX] setState:NSOnState];
		}
		
		format = [(WatchDataSource *) [super dataSource] getFormat:menuActionRow];
		if (format == UNSIGNED_FORMAT) {
			[[[self menu] itemAtIndex:UNSIGNED_INDEX] setState:NSOnState];
			[[[self menu] itemAtIndex:SIGNED_INDEX] setState:NSOffState];
			[[[self menu] itemAtIndex:HEX_INDEX] setState:NSOffState];
			[[[self menu] itemAtIndex:ASCII_INDEX] setState:NSOffState];
		}
		else if (format == SIGNED_FORMAT) {
			[[[self menu] itemAtIndex:UNSIGNED_INDEX] setState:NSOffState];
			[[[self menu] itemAtIndex:SIGNED_INDEX] setState:NSOnState];
			[[[self menu] itemAtIndex:HEX_INDEX] setState:NSOffState];
			[[[self menu] itemAtIndex:ASCII_INDEX] setState:NSOffState];
		}
		else if (format == HEX_FORMAT) {
			[[[self menu] itemAtIndex:UNSIGNED_INDEX] setState:NSOffState];
			[[[self menu] itemAtIndex:SIGNED_INDEX] setState:NSOffState];
			[[[self menu] itemAtIndex:HEX_INDEX] setState:NSOnState];
			[[[self menu] itemAtIndex:ASCII_INDEX] setState:NSOffState];
		}
		else {
			[[[self menu] itemAtIndex:UNSIGNED_INDEX] setState:NSOffState];
			[[[self menu] itemAtIndex:SIGNED_INDEX] setState:NSOffState];
			[[[self menu] itemAtIndex:HEX_INDEX] setState:NSOffState];
			[[[self menu] itemAtIndex:ASCII_INDEX] setState:NSOnState];
		}
		memAddr = [[self dataSource] getAddressWithRow:menuActionRow];
		if (memAddr == -1) {
			[[[self menu] itemAtIndex:READ_BREAK_INDEX] setEnabled:NO];
			[[[self menu] itemAtIndex:WRITE_BREAK_INDEX] setEnabled:NO];
			[[[self menu] itemAtIndex:ACCESS_BREAK_INDEX] setEnabled:NO];
			[[[self menu] itemAtIndex:VALUE_BREAK_INDEX] setEnabled:NO];
			[[[self menu] itemAtIndex:EDIT_BREAK_INDEX] setEnabled:NO];
			[[[self menu] itemAtIndex:DEL_BREAK_INDEX] setEnabled:NO];
		}
		else if ([[BreakpointsController sharedInstance] getBreakpointWithMem:memAddr] != nil) {
			[[[self menu] itemAtIndex:READ_BREAK_INDEX] setEnabled:NO];
			[[[self menu] itemAtIndex:WRITE_BREAK_INDEX] setEnabled:NO];
			[[[self menu] itemAtIndex:ACCESS_BREAK_INDEX] setEnabled:NO];
			[[[self menu] itemAtIndex:VALUE_BREAK_INDEX] setEnabled:NO];
			[[[self menu] itemAtIndex:EDIT_BREAK_INDEX] setEnabled:YES];
			[[[self menu] itemAtIndex:DEL_BREAK_INDEX] setEnabled:YES];
		}
		else {
			[[[self menu] itemAtIndex:READ_BREAK_INDEX] setEnabled:YES];
			[[[self menu] itemAtIndex:WRITE_BREAK_INDEX] setEnabled:YES];
			[[[self menu] itemAtIndex:ACCESS_BREAK_INDEX] setEnabled:YES];
			[[[self menu] itemAtIndex:VALUE_BREAK_INDEX] setEnabled:YES];
			[[[self menu] itemAtIndex:EDIT_BREAK_INDEX] setEnabled:NO];
			[[[self menu] itemAtIndex:DEL_BREAK_INDEX] setEnabled:NO];
		}
		
	}
	
    [NSMenu popUpContextMenu: [self menu] withEvent: theEvent forView: self];
}

- (NSMenu *)menuForEvent: (NSEvent *)theEvent
{
    return nil;
}

- (IBAction) deleteWatch:(id)sender
{
	[(WatchDataSource *)[super dataSource] deleteWatchWithIndex:menuActionRow];
	[self reloadData];
}

- (IBAction) addWatch:(id)sender
{	
	[(WatchDataSource *)[super dataSource] addWatch];
	[self reloadData];
	[self editColumn:0 
				 row:[(WatchDataSource *)[super dataSource] numberOfRowsInTableView:self]-1 
		   withEvent:nil select:YES];
}

- (IBAction) setSize:(id)sender
{
	int size = [sender tag];
	[(WatchDataSource *)[super dataSource] setSize:menuActionRow:size];
	[self reloadData];
}

- (IBAction) setFormat:(id)sender
{
	int format = [sender tag];
	[(WatchDataSource *)[super dataSource] setFormat:menuActionRow:format];
	[self reloadData];
}
	
- (IBAction) showInMemoryView:(id)sender
{
	int addr;
	
	addr = [[self dataSource] getAddressWithRow:menuActionRow];
	[[ControlManager sharedInstance] monitorMemoryAddress:addr];
}

- (IBAction) addReadBreak:(id)sender
{
	int addr, size;
	
	addr = [[self dataSource] getAddressWithRow:menuActionRow];
	if (addr == -1)
		return;
	size = [[self dataSource] getSizeWithRow:menuActionRow];
	
	[[BreakpointsController sharedInstance] addBreakpointWithType:MONITOR_BREAKPOINT_READ Mem:addr Size:size];		
	[[BreakpointsController sharedInstance] loadBreakpoints];
	[self reloadData];
}

- (IBAction) addWriteBreak:(id)sender
{
	int addr, size;
	
	addr = [[self dataSource] getAddressWithRow:menuActionRow];
	if (addr == -1)
		return;
	size = [[self dataSource] getSizeWithRow:menuActionRow];
	
	[[BreakpointsController sharedInstance] addBreakpointWithType:MONITOR_BREAKPOINT_WRITE Mem:addr Size:size];
	[[BreakpointsController sharedInstance] loadBreakpoints];
	[self reloadData];
}
	
- (IBAction) addAccessBreak:(id)sender
{
	int addr, size;
	
	addr = [[self dataSource] getAddressWithRow:menuActionRow];
	if (addr == -1)
		return;
	size = [[self dataSource] getSizeWithRow:menuActionRow];
	
	[[BreakpointsController sharedInstance] addBreakpointWithType:MONITOR_BREAKPOINT_ACCESS Mem:addr Size:size];
	[[BreakpointsController sharedInstance] loadBreakpoints];
	[self reloadData];
}
	
- (IBAction) addValueBreak:(id)sender
{
	int addr,size;
	unsigned short value;
	
	value = [[ControlManager sharedInstance] monitorGetBreakValue];
	
	addr = [[self dataSource] getAddressWithRow:menuActionRow];
	if (addr == -1)
		return;
	size = [[self dataSource] getSizeWithRow:menuActionRow];
	
	[[BreakpointsController sharedInstance] addBreakpointWithMem:addr Size:size Value:value];
	[[BreakpointsController sharedInstance] loadBreakpoints];
	[self reloadData];	
}

- (IBAction) editBreak:(id)sender
{
	int addr;
	
	addr = [[self dataSource] getAddressWithRow:menuActionRow];
	[[BreakpointsController sharedInstance] editBreakpointWithMem:addr];
	[[BreakpointsController sharedInstance] loadBreakpoints];
	[self reloadData];
}

- (IBAction) delBreak:(id)sender
{
	int addr;
	
	addr = [[self dataSource] getAddressWithRow:menuActionRow];
	[[BreakpointsController sharedInstance] deleteBreakpointWithMem:addr];
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
