/* StackDataSource.m - 
 StackDataSource class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "StackDataSource.h"
#import "ControlManager.h"
#import "cpu.h"
#import "memory.h"

@implementation StackDataSource

-(id) init
{
	[super init];
	stackStrings = nil;
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
	return([stackStrings objectAtIndex:rowIndex]);
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
	return([stackStrings count]);
}

-(void) loadStack
{
	unsigned short ts,ta;
	NSString *stack;

	if (stackStrings != nil)
		[stackStrings release];
	stackStrings = [NSMutableArray arrayWithCapacity:20];
	[stackStrings retain];
	
	for( ts = 0x101+CPU_regS; ts<0x200; ) {
		if( ts<0x1ff ) {
			ta = MEMORY_dGetByte(ts) | ( MEMORY_dGetByte(ts+1) << 8 );
			if( MEMORY_dGetByte(ta-2)==0x20 ) {
				stack = [NSString stringWithFormat:@"%04X : %02X %02X\t%04X : JSR %04X",
						   ts, MEMORY_dGetByte(ts), MEMORY_dGetByte(ts+1), ta-2,
						   MEMORY_dGetByte(ta-1) | ( MEMORY_dGetByte(ta) << 8 ) ];
				[stackStrings addObject:stack];
				ts+=2;
				continue;
			}
		}
		stack = [NSString stringWithFormat:@"%04X : %02X\n", ts, MEMORY_dGetByte(ts)];
		ts++;
	}
	
	[(ControlManager *)owner updateMonitorStackTableData];
}

@end
