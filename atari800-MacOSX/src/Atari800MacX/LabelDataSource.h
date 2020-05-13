/* LabelDataSource.h - LabelDataSource 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */


#import <Cocoa/Cocoa.h>


@interface LabelDataSource : NSObject {
	id owner;
	int count;
	BOOL dirty;
	NSArray *sortedLabels;
	NSMutableArray *labels;
	NSString *currentColumnId;
	BOOL currentOrderAscending;
}

- (void) setOwner:(id)theOwner;
- (void) setDirty;
- (id) tableView:(NSTableView *) aTableView 
objectValueForTableColumn:(NSTableColumn *)aTableColumn
			 row:(int) rowIndex;
- (void)tableView:(NSTableView *)aTableView 
   setObjectValue:(id)anObject 
   forTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;
-(void) loadLabels;
-(void) sortByCurrentColumn;

@end
