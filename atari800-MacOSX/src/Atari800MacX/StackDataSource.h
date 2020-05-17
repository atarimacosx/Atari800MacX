/* StackDataSource.h - StackDataSource 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */


#import <Cocoa/Cocoa.h>


@interface StackDataSource : NSObject {
	id owner;
	NSMutableArray *stackStrings;
}

- (void) setOwner:(id)theOwner;
- (id) tableView:(NSTableView *) aTableView 
objectValueForTableColumn:(NSTableColumn *)aTableColumn
			 row:(int) rowIndex;
- (void)tableView:(NSTableView *)aTableView 
   setObjectValue:(id)anObject 
   forTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;
-(void) loadStack;

@end
