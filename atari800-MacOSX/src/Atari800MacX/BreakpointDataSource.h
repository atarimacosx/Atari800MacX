/* BreakpointDataSource.h - BreakpointDataSource header
  For the Macintosh OS X SDL port  of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 
 */

#import <Cocoa/Cocoa.h>


@interface BreakpointDataSource : NSObject {
	id owner;
	int count;
	NSMutableArray *breakpointStrings;
	NSMutableArray *enables;
	char buffer[1024];
	NSColor *gray;
	NSDictionary *grayDict;
}

- (void) setOwner:(id)theOwner;
- (id) tableView:(NSTableView *) aTableView 
objectValueForTableColumn:(NSTableColumn *)aTableColumn
			 row:(int) rowIndex;
- (void)tableView:(NSTableView *)aTableView 
   setObjectValue:(id)anObject 
   forTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;
-(void) loadBreakpoints;
-(void) adjustBreakpointEnables;

@end
