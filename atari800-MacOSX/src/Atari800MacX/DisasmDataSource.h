/* DisasmDataSource.h - DisasmDataSource 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 
 */


#import <Cocoa/Cocoa.h>


@interface DisasmDataSource : NSObject {
	id owner;
	int firstTime;
	unsigned int address;
	unsigned short pcAddr;
	unsigned short pcRow;
	NSColor *black;
	NSColor *red;
	NSColor *pcColor;
	NSDictionary *blackDataDict;
	NSDictionary *redDataDict;
	NSDictionary *pcColorBackgroundDict;
	NSMutableArray *instructions;
	NSMutableArray *flags;
	NSMutableArray *addrs;
	NSImage *breakpointImage;
	NSImage *breakpointCurrentLineImage;
	NSImage *disabledBreakpointImage;
	NSImage *disabledBreakpointCurrentLineImage;
	NSImage *currentLineImage;
}

- (void) setOwner:(id)theOwner;
- (id) tableView:(NSTableView *) aTableView 
      objectValueForTableColumn:(NSTableColumn *)aTableColumn
	  row:(int) rowIndex;
- (void)tableView:(NSTableView *)aTableView 
	  setObjectValue:(id)anObject 
	  forTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;
- (int) numberOfRowsInTableView:(NSTableView *)aTableView;
- (void) disasmMemory:(unsigned short)start:(unsigned short)end:(unsigned short)pc;
- (void) updateBreakpoints;
- (int) getPCRow;
- (unsigned short) getAddress:(int)row;
- (unsigned short) getAddress;
- (int) getRow:(unsigned short) addr;
- (int) checkBreakpoint:(unsigned short) currentAddress;
@end
