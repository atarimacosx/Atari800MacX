/* Breakpoint.h - Breakpoint 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */


#import <Cocoa/Cocoa.h>
#import "BreakpointCondition.h"

@interface Breakpoint : NSObject <NSCopying> {
	int startTableIndex;
	int endTableIndex;
	int pcMin;
	int pcMax;
	int memMin;
	int memMax;
	NSMutableArray *conditions;
}

+(Breakpoint *) breakpointWithStartIndex:(int) start endIndex:(int) end;
-(id) initWithStartIndex:(int) start endIndex:(int) end;
-(void) getStartIndex:(int *) start endIndex:(int *) end;
-(int) conditionCount;
-(BreakpointCondition *) conditionAtIndex:(int) index;
-(void) addCondition:(BreakpointCondition *) condition atIndex:(int) index;
-(void) deleteConditionAtIndex:(int) index;
-(void) setEnabled:(BOOL) flag;
-(int) isEnabled;
-(BOOL) isEnabledForPC;
-(void) getPcMin:(int *) min Max:(int *) max;
-(void) getMemMin:(int *) min Max:(int *) max;

@end
