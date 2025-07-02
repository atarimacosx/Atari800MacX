/* BreakpointDataSource.m - 
 BreakpointDataSource class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "BreakpointDataSource.h"
#import "BreakpointsController.h"
#import "BreakpointCondition.h"
#import "Breakpoint.h"
#import "ControlManager.h"

@implementation BreakpointDataSource

-(id) init
{
	enables = nil;
	breakpointStrings = nil;
	gray = [NSColor grayColor];
	grayDict =[NSDictionary dictionaryWithObjectsAndKeys:
				gray, NSForegroundColorAttributeName,
				nil];
	[gray retain];
	[grayDict retain];
	
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
	if ([[aTableColumn identifier] isEqual:@"NUM"]) {
		return([NSNumber numberWithInt:rowIndex]);
	} else 	if ([[aTableColumn identifier] isEqual:@"ENA"]) {
		return([enables objectAtIndex:rowIndex]);
    } else {
		return([breakpointStrings objectAtIndex:rowIndex]);
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
	return(count);
}

-(void) loadBreakpoints
{
	NSMutableArray *breakpoints;
	int i,j;
	
	breakpoints = [[BreakpointsController sharedInstance] getBreakpoints];
	if (breakpoints == nil)
		count = 0;
	else
		count = [breakpoints count];
	if (enables != nil)
		[enables release];
	enables = [NSMutableArray arrayWithCapacity:20];
	[enables retain];
	if (breakpointStrings != nil)
		[breakpointStrings release];
	breakpointStrings = [NSMutableArray arrayWithCapacity:20];
	[breakpointStrings retain];
	
	for (i=0;i<count;i++) {
		int condCount;
		Breakpoint *theBreakpoint;
		NSMutableAttributedString *breakString;
		MONITOR_breakpoint_cond *lastCond;
		BreakpointCondition *lastCondition;
		
		theBreakpoint = [breakpoints objectAtIndex:i];
		breakString = [[NSMutableAttributedString alloc] initWithString:@""];
		
		[enables addObject:[NSNumber numberWithInt:[theBreakpoint isEnabled]]]; 

		condCount = [theBreakpoint conditionCount];
		lastCondition = [theBreakpoint conditionAtIndex:condCount-1];
		lastCond = [lastCondition getCondition];
		
		for (j=0;j<condCount;j++) {
			MONITOR_breakpoint_cond *cond;
			BreakpointCondition *condition;
			NSMutableString *condString;
			NSAttributedString *condAttribString;
			int bytes;
			
			condition = [theBreakpoint conditionAtIndex:j];
			cond = [condition getCondition];
			
			condString = [[NSMutableString alloc] initWithString:@""];
			if (j != 0 && !(j == condCount - 1 && lastCond->condition == MONITOR_BREAKPOINT_OR))
				[condString appendString:@" AND "];
			
			switch (cond->condition) {
				case MONITOR_BREAKPOINT_NONE:
				case MONITOR_BREAKPOINT_OR:
					[condString appendString:@""];
					break;
				case MONITOR_BREAKPOINT_CLRN:
					[condString appendString:@"Flag N Clear"];
					break;
				case MONITOR_BREAKPOINT_SETN:
					[condString appendString:@"Flag N Set"];
					break;
				case MONITOR_BREAKPOINT_CLRV:
					[condString appendString:@"Flag V Clear"];
					break;
				case MONITOR_BREAKPOINT_SETV:
					[condString appendString:@"Flag V Set"];
					break;
				case MONITOR_BREAKPOINT_CLRB:
					[condString appendString:@"Flag B Clear"];
					break;
				case MONITOR_BREAKPOINT_SETB:
					[condString appendString:@"Flag B Set"];
					break;
				case MONITOR_BREAKPOINT_CLRD:
					[condString appendString:@"Flag D Clear"];
					break;
				case MONITOR_BREAKPOINT_SETD:
					[condString appendString:@"Flag D Set"];
					break;
				case MONITOR_BREAKPOINT_CLRI:
					[condString appendString:@"Flag I Clear"];
					break;
				case MONITOR_BREAKPOINT_SETI:
					[condString appendString:@"Flag I Set"];
					break;
				case MONITOR_BREAKPOINT_CLRZ:
					[condString appendString:@"Flag Z Clear"];
					break;
				case MONITOR_BREAKPOINT_SETZ:
					[condString appendString:@"Flag Z Set"];
					break;
				case MONITOR_BREAKPOINT_CLRC:
					[condString appendString:@"Flag C Clear"];
					break;
				case MONITOR_BREAKPOINT_SETC:
					[condString appendString:@"Flag C Set"];
					break;
					
				default:
					switch (cond->condition>>3) {
						case MONITOR_BREAKPOINT_PC>>3:
							[condString appendString:@"PC Reg"];
							bytes=2;
							break;
						case MONITOR_BREAKPOINT_A>>3:
							[condString appendString:@"A Reg"];
							bytes=1;
							break;
						case MONITOR_BREAKPOINT_X>>3:
							[condString appendString:@"X Reg"];
							bytes=1;
							break;
						case MONITOR_BREAKPOINT_Y>>3:
							[condString appendString:@"Y Reg"];
							bytes=1;
							break;
						case MONITOR_BREAKPOINT_S>>3:
							[condString appendString:@"S Reg"];
							bytes=1;
							break;
						case MONITOR_BREAKPOINT_MEMORY>>3:
							[condString appendString:[NSMutableString stringWithFormat:@"Memory Location %04X",cond->m_addr]];
							bytes=1;
							break;
						case MONITOR_BREAKPOINT_READ>>3:
							[condString appendString:@"Read Address"];
							bytes=2;
							break;
						case MONITOR_BREAKPOINT_WRITE>>3:
							[condString appendString:@"Write Address"];
							bytes=2;
							break;
						case (MONITOR_BREAKPOINT_READ|MONITOR_BREAKPOINT_WRITE)>>3:
							[condString appendString:@"Access Address"];
							bytes=2;
							break;
						default:
							bytes=0;
					}
					if (bytes) {
						switch (cond->condition & 7) {
							case MONITOR_BREAKPOINT_LESS:
								[condString appendString:@"<"];
								break;
							case MONITOR_BREAKPOINT_EQUAL:
								[condString appendString:@"="];
								break;
							case MONITOR_BREAKPOINT_LESS|MONITOR_BREAKPOINT_EQUAL:
								[condString appendString:@"<="];
								break;
							case MONITOR_BREAKPOINT_GREATER:
								[condString appendString:@">"];
								break;
							case MONITOR_BREAKPOINT_GREATER|MONITOR_BREAKPOINT_LESS:
								[condString appendString:@"<>"];
								break;
							case MONITOR_BREAKPOINT_GREATER|MONITOR_BREAKPOINT_EQUAL:
								[condString appendString:@">="];
								break;
							default:
								break;
						}
						if (bytes==1) [condString appendString:[NSString stringWithFormat:@"%02X", cond->value]];
						else[condString appendString:[NSString stringWithFormat:@"%04X", cond->value]];
					}
			}
			if (!cond->enabled) {
				condAttribString = [[NSAttributedString alloc] initWithString:condString attributes:grayDict];
			}
			else {
				condAttribString = [[NSAttributedString alloc] initWithString:condString];
			}
			[breakString appendAttributedString:condAttribString];
		}
		[breakpointStrings addObject:breakString]; 
	}
	[(ControlManager *)owner updateMonitorBreakpointTableData];
}

-(void) adjustBreakpointEnables
{
	NSMutableArray *breakpoints;
	int i;
	
	breakpoints = [[BreakpointsController sharedInstance] getBreakpoints];
	if (breakpoints == nil)
		count = 0;
	else
		count = [breakpoints count];
	if (enables != nil)
		[enables release];
	enables = [NSMutableArray arrayWithCapacity:20];
	[enables retain];
	
	for (i=0;i<count;i++) {
		Breakpoint *theBreakpoint;
		
		theBreakpoint = [breakpoints objectAtIndex:i];
		
		[enables addObject:[NSNumber numberWithInt:[theBreakpoint isEnabled]]]; 
	}	
	[(ControlManager *)owner updateMonitorBreakpointTableData];
}

@end
