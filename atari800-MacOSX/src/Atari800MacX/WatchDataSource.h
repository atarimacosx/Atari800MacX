/* WatchDataSource.h - WatchDataSource 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */


#import <Cocoa/Cocoa.h>

#define HALFWORD_SIZE	0
#define WORD_SIZE		1

#define UNSIGNED_FORMAT	0
#define SIGNED_FORMAT	1
#define HEX_FORMAT		2
#define ASCII_FORMAT    3

@interface WatchDataSource : NSObject {
	id owner;
	NSMutableArray *expressions;
	NSMutableArray *numbers;
	NSMutableArray *valTypes;
	NSMutableArray *lastVal;	
	NSImage *breakpointImage;
	NSImage *disabledBreakpointImage;
	NSColor *black;
	NSColor *red;
	NSDictionary *blackDict;
	NSDictionary *redDict;	
}

- (void) setOwner:(id)theOwner;
- (id) tableView:(NSTableView *) aTableView 
objectValueForTableColumn:(NSTableColumn *)aTableColumn
			 row:(int) rowIndex;
- (void)tableView:(NSTableView *)aTableView 
   setObjectValue:(id)anObject 
   forTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;
-(void) updateWatches;
-(void) addWatch;
-(int) getSize:(int) row;
-(int) getFormat:(int) row;
-(int) getAddressWithRow:(int) row;
-(int) getSizeWithRow:(int) row;
-(void) addWatch;
-(void) addByteWatch:(unsigned short) addr;
-(void) addWordWatch:(unsigned short) addr;
-(void) deleteWatchWithIndex:(int) row;
-(void) setSize:(int) row:(int) size;
-(void) setFormat:(int) row:(int) format;

@end
