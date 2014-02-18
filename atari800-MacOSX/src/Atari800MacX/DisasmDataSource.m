/* DisasmDataSource.m - 
 DisasmDataSource class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 */

#import "atari.h"
#import "memory.h"
#import "monitor.h"
#import "DisasmDataSource.h"
#import "BreakpointsController.h"

#define FLAG_PC       1
#define FLAG_BRKPOINT 2
#define FLAG_BRKENAB  4

extern unsigned short show_instruction_string(char*buff, unsigned short  pc, int wid);

@implementation DisasmDataSource

-(id) init
{
	char filename[FILENAME_MAX];

	address = 0;
	black = [NSColor blackColor];
	red = [NSColor redColor];
	pcColor = [NSColor colorWithDeviceRed:0.694 
					  green:0.761 
					  blue:0.976 
					  alpha:1.0];
	blackDataDict =[NSDictionary dictionaryWithObjectsAndKeys:
					black, NSForegroundColorAttributeName,
					nil];
	[blackDataDict retain];
	redDataDict =[NSDictionary dictionaryWithObjectsAndKeys:
				  red, NSForegroundColorAttributeName,
				  nil];	
	[redDataDict retain];
	pcColorBackgroundDict =[NSDictionary dictionaryWithObjectsAndKeys:
				  pcColor, NSBackgroundColorAttributeName,
				  nil];	
	[pcColorBackgroundDict retain];
	
	breakpointImage = [NSImage alloc];
    strcpy(filename, "Contents/Resources/Breakpoint.png");    
	[breakpointImage initWithContentsOfFile:[NSString stringWithCString:filename encoding:NSASCIIStringEncoding]];
	[breakpointImage retain];
	
	breakpointCurrentLineImage = [NSImage alloc];
    strcpy(filename, "Contents/Resources/BreakpointCurrentLine.png");    
	[breakpointCurrentLineImage initWithContentsOfFile:[NSString stringWithCString:filename encoding:NSASCIIStringEncoding]];
	[breakpointCurrentLineImage retain];
	
	currentLineImage = [NSImage alloc];
    strcpy(filename, "Contents/Resources/CurrentLine.png");    
	[currentLineImage initWithContentsOfFile:[NSString stringWithCString:filename encoding:NSASCIIStringEncoding]];
	[currentLineImage retain];
	
	disabledBreakpointImage = [NSImage alloc];
    strcpy(filename, "Contents/Resources/DisabledBreakpoint.png");    
	[disabledBreakpointImage initWithContentsOfFile:[NSString stringWithCString:filename encoding:NSASCIIStringEncoding]];
	[disabledBreakpointImage retain];
	
	disabledBreakpointCurrentLineImage = [NSImage alloc];
    strcpy(filename, "Contents/Resources/DisabledBreakpointCurrentLine.png");    
	[disabledBreakpointCurrentLineImage initWithContentsOfFile:[NSString stringWithCString:filename encoding:NSASCIIStringEncoding]];
	[disabledBreakpointCurrentLineImage retain];
	
	instructions = nil;
	flags = nil;
	addrs = nil;
	return self;
}

/*------------------------------------------------------------------------------
*  setOwner - Set our owner so we can communicate with it later.
*-----------------------------------------------------------------------------*/
-(void) setOwner:(id)theOwner
{
	owner = theOwner;
}

/*------------------------------------------------------------------------------
*  objectValueForTableColumn - Get the value to display in a cell.
*-----------------------------------------------------------------------------*/
-(id) tableView:(NSTableView *) aTableView 
      objectValueForTableColumn:(NSTableColumn *)aTableColumn
	  row:(int) rowIndex
	{
	if ([[aTableColumn identifier] isEqual:@"CODE"]) {
		return([instructions objectAtIndex:rowIndex]);
	} else {
		switch([[flags objectAtIndex:rowIndex] intValue]) {
			case FLAG_PC:
				return(currentLineImage);
			case (FLAG_PC | FLAG_BRKPOINT):
				return(disabledBreakpointCurrentLineImage);
			case (FLAG_PC | FLAG_BRKPOINT | FLAG_BRKENAB):
				return(breakpointCurrentLineImage);
			case (FLAG_BRKPOINT):
				return(disabledBreakpointImage);
			case (FLAG_BRKPOINT | FLAG_BRKENAB):
				return(breakpointImage);
			default:
				return(nil);
		}
	}
	}

/*------------------------------------------------------------------------------
*  setObjectValue - Change the value in the data source based on what the user
*      has edited in the table.
*-----------------------------------------------------------------------------*/
- (void)tableView:(NSTableView *)aTableView 
	  setObjectValue:(id)anObject 
	  forTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex
{
}

/*------------------------------------------------------------------------------
*  numberOfRowsInTableView - Return the number of rows in the table.
*-----------------------------------------------------------------------------*/
-(int) numberOfRowsInTableView:(NSTableView *)aTableView
	{
	return([instructions count]);
	}
	
/*------------------------------------------------------------------------------
 *  getAddress - Get Address Memory is read from.
 *-----------------------------------------------------------------------------*/
- (unsigned short)getAddress
{
	return address;
}


- (void) disasmMemory:(unsigned short)start:(unsigned short)end:(unsigned short)pc
{
	unsigned short currentAddress = start;
	char buffer[256];
	int row = 0;
	int thisRowPC;
	int breakbits;
	
	pcAddr = pc;
	pcRow = 0;
	if (instructions != nil)
		[instructions release];
	instructions = [NSMutableArray arrayWithCapacity:100];
	[instructions retain];
	if (flags != nil)
		[flags release];
	flags = [NSMutableArray arrayWithCapacity:100];
	[flags retain];
	if (addrs != nil)
		[addrs release];
	addrs = [NSMutableArray arrayWithCapacity:100];
	[addrs retain];
	address = currentAddress;
	while(currentAddress <= end) {
		[addrs addObject:[NSNumber numberWithInt:currentAddress]];
		if (currentAddress == pcAddr) {
			pcRow = row;
			thisRowPC = TRUE;
		}
		else 
			thisRowPC = FALSE;
		breakbits = [self checkBreakpoint:currentAddress];
		currentAddress = show_instruction_string(buffer, currentAddress, 30);
		if (thisRowPC) {
			[instructions addObject:[[NSAttributedString alloc] 
									 initWithString:[NSString stringWithCString:buffer encoding:NSASCIIStringEncoding]
									 attributes:pcColorBackgroundDict]];
			[flags addObject:[NSNumber numberWithInt:(FLAG_PC | breakbits)]];
		}
		else {
			[instructions addObject:[NSString stringWithCString:buffer encoding:NSASCIIStringEncoding]];
			[flags addObject:[NSNumber numberWithInt:breakbits]];
		}
		row++;
	}
}

- (void) updateBreakpoints
{
	int row;
	unsigned short currentAddress;
	int breakbits;
	
	for (row=0;row < [addrs count]; row++) {
		currentAddress = [[addrs objectAtIndex:row] intValue];
		breakbits = [self checkBreakpoint:currentAddress];
		if (row == pcRow) {
			[flags replaceObjectAtIndex:row withObject:[NSNumber numberWithInt:(FLAG_PC | breakbits)]];
		}
		else {
			[flags replaceObjectAtIndex:row withObject:[NSNumber numberWithInt:breakbits]];
		}
	}
}

- (int) checkBreakpoint:(unsigned short) currentAddress
{
	Breakpoint *breakpoint;
	
	breakpoint = [[BreakpointsController sharedInstance] getBreakpointWithPC:currentAddress];
	if (breakpoint == nil)
		return 0;
	else if ([breakpoint isEnabledForPC]) 
		return (FLAG_BRKPOINT | FLAG_BRKENAB);
	else
		return FLAG_BRKPOINT;
}

- (int) getPCRow
{
	return(pcRow);
}

- (unsigned short) getAddress:(int)row
{
	if (addrs != nil)
		return([[addrs objectAtIndex:row] intValue]);
	else
		return(0);
}

- (int) getRow:(unsigned short) addr
{
	int i;
	
	if (addrs == nil)
		return(0);

	for (i=0;i<[addrs count];i++) {
		if (addr == [[addrs objectAtIndex:i] intValue])
			return i;
	}
	
	return([addrs count]/2);
}

@end
