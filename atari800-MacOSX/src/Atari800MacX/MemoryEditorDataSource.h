/* MemoryEditorDataSource.h - MemoryEditorDataSource 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */


#import <Cocoa/Cocoa.h>


@interface MemoryEditorDataSource : NSObject {
	id owner;
	int firstTime;
	unsigned int address;
	NSColor *black;
	NSColor *white;
	NSColor *red;
	NSDictionary *blackDataDict;
	NSDictionary *inverseDataDict;
	NSDictionary *redDataDict;
	unsigned char currMemoryBuffer[128];
	unsigned char backgroudBuffer[128];
	unsigned char oldMemoryBuffer[128];
}

- (void) setOwner:(id)theOwner;
- (id) tableView:(NSTableView *) aTableView 
      objectValueForTableColumn:(NSTableColumn *)aTableColumn
	  row:(int) rowIndex;
- (void)tableView:(NSTableView *)aTableView 
	  setObjectValue:(id)anObject 
	  forTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;
- (int) numberOfRowsInTableView:(NSTableView *)aTableView;
- (void)setAddress:(unsigned int)memAddress;
- (void)setFoundLocation:(unsigned int)addr Length:(int) len;
- (unsigned int)getAddress;
- (void)readMemory;
@end
