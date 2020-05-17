/* BreakpointEditorDataSource.h - BreakpointEditorDataSource 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */

#import <Cocoa/Cocoa.h>
#import "Breakpoint.h"


@interface BreakpointEditorDataSource : NSObject {
	id owner;
	Breakpoint *myBreakpoint;
	BreakpointCondition *newCondition;
	int editableConditionCount;
}

- (void) setOwner:(id)theOwner;
- (id) tableView:(NSTableView *) aTableView 
objectValueForTableColumn:(NSTableColumn *)aTableColumn
			 row:(int) rowIndex;
- (void)tableView:(NSTableView *)aTableView 
   setObjectValue:(id)anObject 
   forTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;
-(void) startWithBreakpoint:(Breakpoint *) theBreakpoint;
-(void) addCondition;
-(Breakpoint *) getBreakpoint;

@end
