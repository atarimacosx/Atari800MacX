/* BreakpointCondition.h - BreakpointCondition 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */


#import <Cocoa/Cocoa.h>
#import "monitor.h"


@interface BreakpointCondition : NSObject <NSCopying> {
	MONITOR_breakpoint_cond cond;
}

+(BreakpointCondition *) conditionWithIndex:(int) index;
-(BreakpointCondition *) initWithConditionIndex:(int) index;
-(BreakpointCondition *) initWithBreakpointCondition:(MONITOR_breakpoint_cond *) condition;
- (MONITOR_breakpoint_cond *) getCondition;

@end
