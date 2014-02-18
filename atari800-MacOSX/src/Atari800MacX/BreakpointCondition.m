/* BreakpointCondition.m - 
 BreakpointCondition class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 */

#import "BreakpointCondition.h"


@implementation BreakpointCondition

+(BreakpointCondition *) conditionWithIndex:(int) index
{
	BreakpointCondition *theBreakpoint;
	
	theBreakpoint = [[self alloc] initWithConditionIndex:index];
	[theBreakpoint autorelease];
	return(theBreakpoint);
}

-(BreakpointCondition *) initWithConditionIndex:(int) index
{
	[super init];
	memcpy(&cond, &MONITOR_breakpoint_table[index], sizeof(MONITOR_breakpoint_cond));
	return(self);
}

-(BreakpointCondition *) initWithBreakpointCondition:(MONITOR_breakpoint_cond *) condition
{
	[super init];
	memcpy(&cond, condition, sizeof(MONITOR_breakpoint_cond));
	return(self);
}

- (MONITOR_breakpoint_cond *) getCondition
{
	return(&cond);
}

-(id) copyWithZone:(NSZone *)zone 
{
	BreakpointCondition *copyCond = [[[self class] allocWithZone:zone] initWithBreakpointCondition:[self getCondition]];
	return copyCond;
}

@end
