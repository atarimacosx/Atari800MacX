/* RegDataSource.h - RegDataSource 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */


#import <Cocoa/Cocoa.h>

typedef struct reg_s {
	char *name;
	unsigned char *addr;
	unsigned short writeAddr;
} REG_S;

@interface RegDataSource : NSObject {
	id owner;
	NSColor *black;
    NSColor *red;
    NSColor *white;
	NSDictionary *blackDict;
	NSDictionary *redDict;
	NSMutableArray *nameStrings;
	NSMutableArray *regStrings;
	REG_S *regDefs;
	int regCount;
	BOOL firstTime;
	unsigned char *oldRegs;
}

- (void) setOwner:(id)theOwner;
- (id) tableView:(NSTableView *) aTableView 
objectValueForTableColumn:(NSTableColumn *)aTableColumn
			 row:(int) rowIndex;
-(void) loadRegs;

@end
