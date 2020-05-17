/* SectorEditorDataSource.h - SectorEditorDataSource 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */


#import <Cocoa/Cocoa.h>


@interface SectorEditorDataSource : NSObject {
	id owner;
	int sectorSize;
	unsigned char sectorBuffer[512];
}

- (void) setOwner:(id)theOwner;
- (id) tableView:(NSTableView *) aTableView 
      objectValueForTableColumn:(NSTableColumn *)aTableColumn
	  row:(int) rowIndex;
- (void)tableView:(NSTableView *)aTableView 
	  setObjectValue:(id)anObject 
	  forTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;
- (int) numberOfRowsInTableView:(NSTableView *)aTableView;
- (void)readSector:(int)sector;
- (void)writeSector:(int)sector;

@end
