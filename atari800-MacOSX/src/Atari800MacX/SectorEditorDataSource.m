/* SectorEditorDataSource.m - 
 SectorEditorDataSource class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 */

#import "atari.h"
#import "atrUtil.h"
#import "atrErr.h"
#import "atrMount.h"
#import "SectorEditorWindow.h"
#import "SectorEditorDataSource.h"

@implementation SectorEditorDataSource

-(id) init
{
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
	char cellNum[3];
	NSString *rowStr;

	if ([[aTableColumn identifier] isEqual:@"RowHeader"]) {
		sprintf(cellNum,"%02X",rowIndex*16);
		rowStr = [NSString stringWithCString:cellNum encoding:NSASCIIStringEncoding];
		return(rowStr);
		}
	else if ([[aTableColumn identifier] isEqual:@"ASCII"]) {
		char asciiBuffer[17];
		int i,start;
		
		start = rowIndex*16;
		for (i=0;i<16;i++) {
			if ((sectorBuffer[start+i] >= 0x20) && (sectorBuffer[start+i] <= 0x7E))
				asciiBuffer[i] = sectorBuffer[start+i];
			else
				asciiBuffer[i] = '.';
			} 
		asciiBuffer[16] = 0;
		rowStr = [NSString stringWithCString:asciiBuffer encoding:NSASCIIStringEncoding];
		return(rowStr);
		}
	else {
		int colNum = [[aTableColumn identifier] intValue];

		sprintf(cellNum,"%02X",sectorBuffer[rowIndex*16+colNum]);
		rowStr = [NSString stringWithCString:cellNum encoding:NSASCIIStringEncoding];
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
	int i;

	[cellStr getCString:cellCStr maxLength:16 encoding:NSASCIIStringEncoding];
	for (i=strlen(cellCStr);i<16;i++)
		cellCStr[i] = ' ';
	cellCStr[16] = 0;
	
	if ([[aTableColumn identifier] isEqual:@"ASCII"]) {
		int start = rowIndex*16;
		for (i=0;i<16;i++) {
			if (cellCStr[i] == '.') {
				if ((sectorBuffer[start+i] >= 0x20) && (sectorBuffer[start+i] <= 0x7E)) {
					if (sectorBuffer[start+i] != cellCStr[i])
						[owner setDirty];
					sectorBuffer[start+i] = cellCStr[i];
					}
				}
			else {
				if (sectorBuffer[start+i] != cellCStr[i])
					[owner setDirty];
				sectorBuffer[start+i] = cellCStr[i];
				}
			}
		}
	else {
		int colNum = [[aTableColumn identifier] intValue];
		unsigned char newValue = strtol(cellCStr,NULL,16);
		
		if (sectorBuffer[rowIndex*16+colNum] != newValue)
			[owner setDirty];
		sectorBuffer[rowIndex*16+colNum] = newValue;
		}
}

/*------------------------------------------------------------------------------
*  numberOfRowsInTableView - Return the number of rows in the table.
*-----------------------------------------------------------------------------*/
-(int) numberOfRowsInTableView:(NSTableView *)aTableView
	{
	return(sectorSize/16);
	}
	
/*------------------------------------------------------------------------------
*  readSector - Read a sector from disk into our buffer.
*-----------------------------------------------------------------------------*/
- (void)readSector:(int)sector
{
	AtrReadSector([owner getDiskInfo], sector, sectorBuffer);
	sectorSize = AtrSectorNumberSize([owner getDiskInfo], sector);
	[owner clearDirty];
}

/*------------------------------------------------------------------------------
*  writeSector - Write our buffer out to disk.
*-----------------------------------------------------------------------------*/
- (void)writeSector:(int)sector
{
	AtrWriteSector([owner getDiskInfo], sector, sectorBuffer);
	[owner clearDirty];
}

@end
