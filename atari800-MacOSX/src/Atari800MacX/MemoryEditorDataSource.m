/* MemoryEditorDataSource.m - 
 MemoryEditorDataSource class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 */

#import "atari.h"
#import "memory.h"
#import "MemoryEditorDataSource.h"

@implementation MemoryEditorDataSource

-(id) init
{
	address = 0;
	black = [NSColor blackColor];
	white = [NSColor whiteColor];
	red = [NSColor redColor];
	blackDataDict =[NSDictionary dictionaryWithObjectsAndKeys:
					black, NSForegroundColorAttributeName,
					nil];
	[blackDataDict retain];
	inverseDataDict =[NSDictionary dictionaryWithObjectsAndKeys:
					  black, NSBackgroundColorAttributeName,
					  white, NSForegroundColorAttributeName,
					nil];
	[inverseDataDict retain];
	redDataDict =[NSDictionary dictionaryWithObjectsAndKeys:
				  red, NSForegroundColorAttributeName,
				  nil];	
	[redDataDict retain];
	memset(backgroudBuffer,0,128);
	return self;
}

/*------------------------------------------------------------------------------
*  setOwner - Set our owner so we can communicate with it later.
*-----------------------------------------------------------------------------*/
-(void) setOwner:(id)theOwner
{
	owner = theOwner;
	firstTime = TRUE;
}

/*------------------------------------------------------------------------------
*  objectValueForTableColumn - Get the value to display in a cell.
*-----------------------------------------------------------------------------*/
-(id) tableView:(NSTableView *) aTableView 
      objectValueForTableColumn:(NSTableColumn *)aTableColumn
	  row:(int) rowIndex
	{
	char cellNum[10];
	NSMutableAttributedString *rowStr;

	rowStr = [NSMutableAttributedString alloc];

	if ([[aTableColumn identifier] isEqual:@"RowHeader"]) {
		sprintf(cellNum,"%04X",rowIndex*16+address);
		rowStr = [[NSMutableAttributedString alloc] initWithString:[NSString stringWithCString:cellNum encoding:NSASCIIStringEncoding] attributes:blackDataDict];
		return(rowStr);
		}
	else if ([[aTableColumn identifier] isEqual:@"ASCII"]) {
		char asciiBuffer[17];
		NSRange theRange;
		int i,start;
		
		start = rowIndex*16;
		for (i=0;i<16;i++) {
			UBYTE data = currMemoryBuffer[start+ i];
			if ((data >= 0x20) && (data <= 0x7E))
				asciiBuffer[i] = data;
			else
				asciiBuffer[i] = '.';
			} 
		asciiBuffer[16] = 0;
		rowStr = [[NSMutableAttributedString alloc] initWithString:[NSString stringWithCString:asciiBuffer encoding:NSASCIIStringEncoding] attributes:blackDataDict];
		theRange.length = 1;
		for (i=0;i<16;i++) {
			if (oldMemoryBuffer[start+i] != currMemoryBuffer[start+i]) {
				theRange.location = i;
				[rowStr setAttributes:redDataDict range:theRange];
				}
			}
		return(rowStr);
		}
	else {
		NSDictionary *colorDict;
		int colNum = [[aTableColumn identifier] intValue];
		UBYTE data = currMemoryBuffer[rowIndex*16+colNum];

		sprintf(cellNum,"%02X",data);
		if (backgroudBuffer[rowIndex*16+colNum] == 0xFF)
			colorDict = inverseDataDict;
		else if (oldMemoryBuffer[rowIndex*16+colNum] == data)
			colorDict = blackDataDict;
		else
			colorDict = redDataDict;
		rowStr = [[NSMutableAttributedString alloc] initWithString:[NSString stringWithCString:cellNum encoding:NSASCIIStringEncoding] attributes:colorDict];
		return(rowStr);
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
	char cellCStr[17];
	NSString *cellStr = (NSString *) anObject;

	if ([[aTableColumn identifier] isEqual:@"ASCII"]) {
#if 0
		int start = rowIndex*16 + address;

		[cellStr getCString:cellCStr];
		for (i=strlen(cellCStr);i<16;i++)
			cellCStr[i] = '.';
		cellCStr[17] = 0;
		
		for (i=0;i<16;i++) {
			if (cellCStr[i] != '.') {
				MEMORY_PutByte(start+i,cellCStr[i]);
				}
			}
#endif
		}
	else {
		unsigned char newValue;
		int colNum = [[aTableColumn identifier] intValue];
		
		[cellStr getCString:cellCStr maxLength:16 encoding:NSASCIIStringEncoding];
		newValue = strtol(cellCStr,NULL,16);
		MEMORY_PutByte(rowIndex*16+colNum+address, (UBYTE) newValue);
		newValue = MEMORY_GetByte(rowIndex*16+colNum+address);
		currMemoryBuffer[rowIndex*16+colNum] = newValue;
		//oldMemoryBuffer[rowIndex*16+colNum] = newValue;
		}
}

/*------------------------------------------------------------------------------
*  numberOfRowsInTableView - Return the number of rows in the table.
*-----------------------------------------------------------------------------*/
-(int) numberOfRowsInTableView:(NSTableView *)aTableView
	{
	return(8);
	}
	
/*------------------------------------------------------------------------------
*  setAddress - Set Address in Memory to read from.
*-----------------------------------------------------------------------------*/
- (void)setAddress:(unsigned int)memAddress
{
	address = memAddress & 0xFFF0;
	firstTime = TRUE;
}

- (void)setFoundLocation:(unsigned int)addr Length:(int) len
{
	int i;
	
	memset(backgroudBuffer,0,128);
	for (i=0;i<len;i++)
		backgroudBuffer[addr - address + i] = 0xFF;
}

/*------------------------------------------------------------------------------
 *  getAddress - Get Address Memory is read from.
 *-----------------------------------------------------------------------------*/
- (unsigned int)getAddress
{
	return address;
}


- (void)readMemory
{
	int i;
	
	if (!firstTime)
		memcpy(oldMemoryBuffer,currMemoryBuffer,128);
	for (i=0;i<128;i++)
		currMemoryBuffer[i] = MEMORY_GetByte(i+address);
	if (firstTime)
		memcpy(oldMemoryBuffer,currMemoryBuffer,128);
	firstTime = FALSE;
	memset(backgroudBuffer,0,128);
}
@end
