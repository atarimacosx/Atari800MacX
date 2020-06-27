/* RegDataSource.m - 
 RegDataSource class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "RegDataSource.h"
#import "ControlManager.h"
#import "memory.h"
#import "monitor.h"

@implementation RegDataSource

-(id) init
{
	int i;
	
	[super init];
	
	// init colors
	black = [NSColor labelColor];
	red = [NSColor redColor];
    white = [NSColor whiteColor];
	blackDict =[NSDictionary dictionaryWithObjectsAndKeys:
				black, NSForegroundColorAttributeName,
				nil];
	[blackDict retain];
	redDict =[NSDictionary dictionaryWithObjectsAndKeys:
			  red, NSForegroundColorAttributeName,
              white, NSBackgroundColorAttributeName,
			  nil];
	[redDict retain];
	
	nameStrings = [NSMutableArray arrayWithCapacity:regCount];
	[nameStrings retain];
	regStrings = [NSMutableArray arrayWithCapacity:regCount];
	[regStrings retain];
	for (i=0;i<regCount;i++) {
		// Note, because this uses regDefs to populate the data, it is
		//  a "virtual" class, and must be subclassed.
		[nameStrings addObject:[NSString stringWithCString:regDefs[i].name encoding:NSASCIIStringEncoding]];
		[regStrings addObject:@""];
	}
	oldRegs = calloc(regCount, sizeof(unsigned char));
	firstTime = YES;
	
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
	if ([[aTableColumn identifier] isEqual:@"N"]) {
		return([nameStrings objectAtIndex:rowIndex]);
    } else  {
		return([regStrings objectAtIndex:rowIndex]);
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
	char buffer[81];
	unsigned short hexval;
	
	if (regDefs[rowIndex].writeAddr == 0) {
		NSBeep();
	}
	
	[(NSString *) anObject getCString:buffer maxLength:80 encoding:NSASCIIStringEncoding]; 
	
	if (get_hex(buffer, &hexval)) {
		MEMORY_PutByte(regDefs[rowIndex].writeAddr, hexval);
		[(ControlManager *)owner updateMonitorGUIHardware];
	}
}	

/*------------------------------------------------------------------------------
 *  numberOfRowsInTableView - Return the number of rows in the table.
 *-----------------------------------------------------------------------------*/
-(int) numberOfRowsInTableView:(NSTableView *)aTableView
{
	return(regCount);
}
		
-(void) loadRegs
{	
	int i;
	NSString *regString;
	NSAttributedString *regAttribString;
	
	for (i=0;i<regCount;i++) {
		regString = [NSString stringWithFormat:@"%02X",*(regDefs[i].addr)];
		if (firstTime || (*(regDefs[i].addr) == oldRegs[i]))
			regAttribString = [[NSAttributedString alloc] initWithString:regString attributes:blackDict];
		else
			regAttribString = [[NSAttributedString alloc] initWithString:regString attributes:redDict];
		[regStrings replaceObjectAtIndex:i withObject:regAttribString];
		oldRegs[i] = *(regDefs[i].addr);
	}
	firstTime = NO;
}

@end
