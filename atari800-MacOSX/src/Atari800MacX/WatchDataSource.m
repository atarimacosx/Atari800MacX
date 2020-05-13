/* WatchDataSource.m - 
 WatchDataSource class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "WatchDataSource.h"
#import "BreakpointsController.h"
#import "Breakpoint.h"
#import "ControlManager.h"
#import "memory.h"

#define UNSIGNED_BYTE_FORMAT	((UNSIGNED_FORMAT << 4) | HALFWORD_SIZE)
#define SIGNED_BYTE_FORMAT		((SIGNED_FORMAT << 4) | HALFWORD_SIZE)
#define HEX_BYTE_FORMAT			((HEX_FORMAT << 4) | HALFWORD_SIZE)
#define ASCII_BYTE_FORMAT		((ASCII_FORMAT << 4) | HALFWORD_SIZE)
#define UNSIGNED_WORD_FORMAT	((UNSIGNED_FORMAT << 4) | WORD_SIZE)
#define SIGNED_WORD_FORMAT		((SIGNED_FORMAT << 4) | WORD_SIZE)
#define HEX_WORD_FORMAT			((HEX_FORMAT << 4) | WORD_SIZE)

#define INVALID_EXP_STRING @"----"

@implementation WatchDataSource

-(id) init
{
	char filename[FILENAME_MAX];
	
	[super init];

	expressions = [NSMutableArray arrayWithCapacity:20];
	[expressions retain];
	numbers = [NSMutableArray arrayWithCapacity:20];
	[numbers retain];
	valTypes = [NSMutableArray arrayWithCapacity:20];
	[valTypes retain];
	lastVal = [NSMutableArray arrayWithCapacity:20];
	[lastVal retain];

	breakpointImage = [NSImage alloc];
    strcpy(filename, "Contents/Resources/Breakpoint.png");    
	[breakpointImage initWithContentsOfFile:[NSString stringWithCString:filename encoding:NSASCIIStringEncoding]];
	[breakpointImage retain];
	
	disabledBreakpointImage = [NSImage alloc];
    strcpy(filename, "Contents/Resources/DisabledBreakpoint.png");    
	[disabledBreakpointImage initWithContentsOfFile:[NSString stringWithCString:filename encoding:NSASCIIStringEncoding]];
	[disabledBreakpointImage retain];
	
	// init colors
	black = [NSColor labelColor];
	red = [NSColor redColor];
	blackDict =[NSDictionary dictionaryWithObjectsAndKeys:
				black, NSForegroundColorAttributeName,
				nil];
	[blackDict retain];
	redDict =[NSDictionary dictionaryWithObjectsAndKeys:
			  red, NSForegroundColorAttributeName,
			  nil];	
	[redDict retain];
	
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
	if ([[aTableColumn identifier] isEqual:@"EXP"]) {
		return [expressions objectAtIndex:rowIndex];
	}
    else if ([[aTableColumn identifier] isEqual:@"BRK"]) {
		Breakpoint *theBreak;
		int addr = [[numbers objectAtIndex:rowIndex] intValue];
		
		theBreak = [[BreakpointsController sharedInstance] getBreakpointWithMem:addr];
		if (theBreak == nil)
			return nil;
		
		if ([theBreak isEnabled])
			return(breakpointImage);
		else
			return(disabledBreakpointImage);
	}
	else if ([[aTableColumn identifier] isEqual:@"NUM"]) {
		if ([[numbers objectAtIndex:rowIndex] isEqual:INVALID_EXP_STRING])
			 return(INVALID_EXP_STRING);
		else
			 return ([NSString stringWithFormat:@"%04x", [[numbers objectAtIndex:rowIndex] intValue]]);
	}
	else if ([[aTableColumn identifier] isEqual:@"VAL"]) {
		int type;
		unsigned short val, last;
		int oldVal;
		NSDictionary *theDict;
		NSString *theString;
		NSAttributedString *theAttributedString;
		
		if ([[numbers objectAtIndex:rowIndex] isEqual:INVALID_EXP_STRING])
			return @"";
		
		type = [[valTypes objectAtIndex:rowIndex] intValue];
		if ((type & 0xF) == HALFWORD_SIZE)
			val = MEMORY_GetByte([[numbers objectAtIndex:rowIndex] intValue]);
		else
			val = MEMORY_GetByte([[numbers objectAtIndex:rowIndex] intValue]) +
					MEMORY_GetByte([[numbers objectAtIndex:rowIndex] intValue]+1)*256;
		
		oldVal = [[lastVal objectAtIndex:rowIndex] intValue];
		last = oldVal;
		[lastVal replaceObjectAtIndex:rowIndex withObject:[NSNumber numberWithInt:val]];
		
		if (oldVal == -1) {
			theDict = blackDict;
		}
		else if (val != last) {
			theDict = redDict;
		}
		else {
			theDict = blackDict;
		}
		
		switch(type) {
			case SIGNED_BYTE_FORMAT:
				theString = [NSString stringWithFormat:@"%6d", (char) val];
				break;
			case UNSIGNED_BYTE_FORMAT:
				theString = [NSString stringWithFormat:@"%6u", (unsigned char) val];
				break;
			case HEX_BYTE_FORMAT:
				theString = [NSString stringWithFormat:@"    %02X", (unsigned char) val];
				break;
			case ASCII_BYTE_FORMAT:
				theString = [NSString stringWithFormat:@"   '%c'", (char) val];
				break;
			case SIGNED_WORD_FORMAT:
				theString = [NSString stringWithFormat:@"%6d", (short) val];
				break;
			case UNSIGNED_WORD_FORMAT:
				theString = [NSString stringWithFormat:@"%6u", (unsigned short) val];
				break;
			case HEX_WORD_FORMAT:
				theString = [NSString stringWithFormat:@"  %04X", (unsigned short) val];
				break;
			default:
				return nil;
		}
		theAttributedString = [[NSAttributedString alloc] initWithString:theString attributes:theDict];
		[theAttributedString autorelease];
		return theAttributedString;
	}
	else
	{
		return nil;
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
	char buffer[80];

	if ([[aTableColumn identifier] isEqual:@"EXP"]) {
		unsigned short expVal;
		
		[(NSString *) anObject getCString:buffer maxLength:80 encoding:NSASCIIStringEncoding]; 
		
		if (get_val_gui(buffer, &expVal) == FALSE)
			return;
		[expressions replaceObjectAtIndex:rowIndex withObject:anObject];
		[numbers replaceObjectAtIndex:rowIndex withObject:[NSNumber numberWithInt:expVal]];
		[lastVal replaceObjectAtIndex:rowIndex withObject:[NSNumber numberWithInt:-1]];
		return;
	}
	else if ([[aTableColumn identifier] isEqual:@"VAL"]) {
		int type;
		char byteVal;
		unsigned char ubyteVal;
		short wordVal;
		unsigned short uwordVal;
		unsigned short addr;
		
		if ([[numbers objectAtIndex:rowIndex] isEqual:INVALID_EXP_STRING])
			return;
		
		addr = [[numbers objectAtIndex:rowIndex] intValue];
		type = [[valTypes objectAtIndex:rowIndex] intValue];
		[(NSString *) anObject getCString:buffer maxLength:80 encoding:NSASCIIStringEncoding]; 
		
		switch(type) {
			case SIGNED_BYTE_FORMAT:
				byteVal = (char) strtol(buffer, NULL, 10);
				MEMORY_PutByte(addr, byteVal);
				break;
			case UNSIGNED_BYTE_FORMAT:
				ubyteVal = (unsigned char) strtol(buffer, NULL, 10);
				MEMORY_PutByte(addr, ubyteVal);
				break;
			case HEX_BYTE_FORMAT:
				ubyteVal = (unsigned char) strtol(buffer, NULL, 16);
				MEMORY_PutByte(addr, ubyteVal);
				break;
			case ASCII_BYTE_FORMAT:
				ubyteVal = buffer[0];
				MEMORY_PutByte(addr, ubyteVal);
				break;
			case SIGNED_WORD_FORMAT:
				wordVal = (unsigned short) strtol(buffer, NULL, 10);
				MEMORY_PutByte(addr, wordVal & 0xFF);
				MEMORY_PutByte(addr+1, (wordVal & 0xFF00) >> 8);
				break;
			case UNSIGNED_WORD_FORMAT:
				uwordVal = (unsigned short) strtol(buffer, NULL, 10);
				MEMORY_PutByte(addr, uwordVal & 0xFF);
				MEMORY_PutByte(addr+1, (uwordVal & 0xFF00) >> 8);
				break;
			case HEX_WORD_FORMAT:
				uwordVal = (unsigned short) strtol(buffer, NULL, 16);
				MEMORY_PutByte(addr, uwordVal & 0xFF);
				MEMORY_PutByte(addr+1, (uwordVal & 0xFF00) >> 8);
				break;
		}
		[(ControlManager *)owner updateMonitorGUIWatch];
		[(ControlManager *)owner updateMonitorGUIMemory];
		return;
	}
}

/*------------------------------------------------------------------------------
 *  numberOfRowsInTableView - Return the number of rows in the table.
 *-----------------------------------------------------------------------------*/
-(int) numberOfRowsInTableView:(NSTableView *)aTableView
{
	return([expressions count]);
}

-(void) updateWatches
{
	[(ControlManager *)owner updateMonitorWatchTableData];
}

-(int) getSize:(int) row
{
	int type;
	
	type = [[valTypes objectAtIndex:row] intValue];
	return(type & 0xF);
}

-(int) getFormat:(int) row
{
	int type;
	
	type = [[valTypes objectAtIndex:row] intValue];
	return((type & 0xF0) >> 4);
}

-(int) getAddressWithRow:(int) row
{
	NSObject *theObject = [numbers objectAtIndex:row];
	
	if ([theObject isEqual:INVALID_EXP_STRING]) 
		return(-1);
		
	return([(NSNumber *) theObject intValue]);
}

-(int) getSizeWithRow:(int) row;
{
	NSObject *theObject = [valTypes objectAtIndex:row];
	
	if ([theObject isEqual:INVALID_EXP_STRING]) 
		return(-1);
	
	return([(NSNumber *) theObject intValue] & 0x0F);
}

-(void) addWatch
{
	[expressions addObject:INVALID_EXP_STRING];
	[numbers addObject:INVALID_EXP_STRING];
	[valTypes addObject:[NSNumber numberWithInt:HEX_BYTE_FORMAT]];
	[lastVal addObject:[NSNumber numberWithInt:-1]];
}

-(void) addByteWatch:(unsigned short) addr
{
	[expressions addObject:[NSString stringWithFormat:@"%04x",addr]];
	[numbers addObject:[NSNumber numberWithInt:addr]];
	[valTypes addObject:[NSNumber numberWithInt:HEX_BYTE_FORMAT]];
	[lastVal addObject:[NSNumber numberWithInt:-1]];
	[self updateWatches];
}

-(void) addWordWatch:(unsigned short) addr
{
	[expressions addObject:[NSString stringWithFormat:@"%04x",addr]];
	[numbers addObject:[NSNumber numberWithInt:addr]];
	[valTypes addObject:[NSNumber numberWithInt:HEX_WORD_FORMAT]];
	[lastVal addObject:[NSNumber numberWithInt:-1]];
	[self updateWatches];
	}

-(void) deleteWatchWithIndex:(int) row
{
	[expressions removeObjectAtIndex:row];
	[numbers removeObjectAtIndex:row];
	[valTypes removeObjectAtIndex:row];	
	[lastVal removeObjectAtIndex:row];
}

-(void) setSize:(int) row:(int) size
{
	int type;
	
	type = [[valTypes objectAtIndex:row] intValue];
	type = (type & 0xF0) | size;
	[valTypes replaceObjectAtIndex:row withObject:[NSNumber numberWithInt:type]];
	[lastVal replaceObjectAtIndex:row withObject:[NSNumber numberWithInt:-1]];
}

-(void) setFormat:(int) row:(int) format
{
	int type;
	
	type = [[valTypes objectAtIndex:row] intValue];
	type = (type & 0x0F) | (format << 4);
	[valTypes replaceObjectAtIndex:row withObject:[NSNumber numberWithInt:type]];
}


@end
