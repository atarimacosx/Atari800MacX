/* Breakpoint.m - 
 Breakpoint class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "Breakpoint.h"


@implementation Breakpoint

+(Breakpoint *) breakpointWithStartIndex:(int) start endIndex:(int) end
{
	return([[self alloc] initWithStartIndex:start endIndex:end]);
}

-(id) initWithStartIndex:(int) start endIndex:(int) end
{
	int i;
	BreakpointCondition *newCondition;
	MONITOR_breakpoint_cond *cond;
	
	[super init];
	startTableIndex = start;
	endTableIndex = end;
	pcMin = -1;
	pcMax = -1;
	memMin = -1;
	memMax = -1;
	conditions = [NSMutableArray arrayWithCapacity:10];
	[conditions retain];
	for (i=start;i<=end;i++) {
		newCondition = [BreakpointCondition conditionWithIndex:i];
		[conditions addObject:newCondition];
		cond = [newCondition getCondition];
		switch (cond->condition) {
			case MONITOR_BREAKPOINT_PC | MONITOR_BREAKPOINT_EQUAL:
				pcMin = cond->addr;
				pcMax = cond->addr;
				break;
			case MONITOR_BREAKPOINT_PC | MONITOR_BREAKPOINT_LESS:
				pcMax = cond->addr-1;
				break;
			case MONITOR_BREAKPOINT_PC | MONITOR_BREAKPOINT_LESS | MONITOR_BREAKPOINT_EQUAL:
				pcMax = cond->addr;
				break;
			case MONITOR_BREAKPOINT_PC | MONITOR_BREAKPOINT_GREATER:
				pcMin = cond->addr+1;
				break;
			case MONITOR_BREAKPOINT_PC | MONITOR_BREAKPOINT_GREATER | MONITOR_BREAKPOINT_EQUAL:
				pcMin = cond->addr;
				break;
			case MONITOR_BREAKPOINT_READ | MONITOR_BREAKPOINT_EQUAL:
			case MONITOR_BREAKPOINT_WRITE | MONITOR_BREAKPOINT_EQUAL:
			case MONITOR_BREAKPOINT_ACCESS | MONITOR_BREAKPOINT_EQUAL:
			case MONITOR_BREAKPOINT_MEM | MONITOR_BREAKPOINT_LESS:
			case MONITOR_BREAKPOINT_MEM | MONITOR_BREAKPOINT_GREATER:
			case MONITOR_BREAKPOINT_MEM | MONITOR_BREAKPOINT_EQUAL | MONITOR_BREAKPOINT_LESS:
			case MONITOR_BREAKPOINT_MEM | MONITOR_BREAKPOINT_EQUAL | MONITOR_BREAKPOINT_GREATER:
			case MONITOR_BREAKPOINT_MEM | MONITOR_BREAKPOINT_LESS | MONITOR_BREAKPOINT_GREATER:
				memMin = cond->addr;
				memMax = cond->addr;
				break;
			case MONITOR_BREAKPOINT_MEM | MONITOR_BREAKPOINT_EQUAL:
				if (memMin == -1 && memMax == -1) {
					memMin = cond->addr;
					memMax = cond->addr;
				}
				else {
					memMax = cond->addr;
				}
				break;
			case MONITOR_BREAKPOINT_READ | MONITOR_BREAKPOINT_LESS:
			case MONITOR_BREAKPOINT_WRITE | MONITOR_BREAKPOINT_LESS:
			case MONITOR_BREAKPOINT_ACCESS | MONITOR_BREAKPOINT_LESS:
				memMax = cond->addr-1;
				break;
			case MONITOR_BREAKPOINT_READ | MONITOR_BREAKPOINT_LESS | MONITOR_BREAKPOINT_EQUAL:
			case MONITOR_BREAKPOINT_WRITE | MONITOR_BREAKPOINT_LESS | MONITOR_BREAKPOINT_EQUAL:
			case MONITOR_BREAKPOINT_ACCESS | MONITOR_BREAKPOINT_LESS | MONITOR_BREAKPOINT_EQUAL:
				memMax = cond->addr;
				break;
			case MONITOR_BREAKPOINT_READ | MONITOR_BREAKPOINT_GREATER:
			case MONITOR_BREAKPOINT_WRITE | MONITOR_BREAKPOINT_GREATER:
			case MONITOR_BREAKPOINT_ACCESS | MONITOR_BREAKPOINT_GREATER:
				memMin = cond->addr+1;
				break;
			case MONITOR_BREAKPOINT_READ | MONITOR_BREAKPOINT_GREATER | MONITOR_BREAKPOINT_EQUAL:
			case MONITOR_BREAKPOINT_WRITE | MONITOR_BREAKPOINT_GREATER | MONITOR_BREAKPOINT_EQUAL:
			case MONITOR_BREAKPOINT_ACCESS | MONITOR_BREAKPOINT_GREATER | MONITOR_BREAKPOINT_EQUAL:
				memMin = cond->addr;
				break;
		}
	}
	
	cond = [[conditions objectAtIndex:([conditions count]-1)] getCondition];
	if (cond->condition == MONITOR_BREAKPOINT_OR) {
		BOOL anyOn = FALSE;
		for (i=0;i<[conditions count];i++) {
			cond = [[conditions objectAtIndex:i] getCondition];
			if (cond->on)
				anyOn = TRUE;
		}
		cond = [[conditions objectAtIndex:([conditions count]-1)] getCondition];
		cond->on = anyOn;
		cond = &MONITOR_breakpoint_table[end];
		cond->on = anyOn;
	}
	return(self);
}

-(void) dealloc
{
	[conditions release];
	[super dealloc];
}

-(void) getStartIndex:(int *) start endIndex:(int *) end
{
	*start = startTableIndex;
	*end = endTableIndex;
}

-(void) setStartIndex:(int) start
{
	startTableIndex = start;
}

-(void)  setEndIndex:(int) end
{
	endTableIndex = end;
}

-(void) setConditions:(NSMutableArray *) cond
{
	conditions = cond;
}

-(int) conditionCount
{
	return ([conditions count]);
}

-(BreakpointCondition *) conditionAtIndex:(int) index
{
	return([conditions objectAtIndex:index]);
}

-(void) addCondition:(BreakpointCondition *)condition atIndex:(int) index
{
	[conditions insertObject:condition atIndex:index];
}

-(void) deleteConditionAtIndex:(int) index;
{
	[conditions removeObjectAtIndex:index];
}

-(void) setEnabled:(BOOL) flag
{
	int i;
	int on;
	MONITOR_breakpoint_cond *cond;
	BreakpointCondition *theCondition;
	
	if (flag)
		on = TRUE;
	else
		on = FALSE;
	
	for (i=startTableIndex;i<=endTableIndex;i++) {
		MONITOR_breakpoint_table[i].on = on;
	}
	for (i=0;i<[conditions count];i++) {
		theCondition = [conditions objectAtIndex:i];
		cond = [theCondition getCondition];
		cond->on = on;
	}
}

-(int) isEnabled
{
	int i, enabled;
	BOOL anyOn = FALSE;
	BOOL allOn = TRUE;
	MONITOR_breakpoint_cond *cond;
	
	for (i=0;i<[conditions count];i++) {
		cond = [[conditions objectAtIndex:i] getCondition];
		if (cond->on)
			anyOn = TRUE;
		else
			allOn = FALSE;
	}
	if (allOn)
		enabled = NSOnState;
	else if (anyOn)
		enabled = NSMixedState;
	else
		enabled = NSOffState;

	return(enabled);
}

-(BOOL) isEnabledForPC
{
	MONITOR_breakpoint_cond *cond;
	int i;
	BOOL pc = FALSE;
	
	for (i=0;i<[conditions count];i++) {
		cond = [[conditions objectAtIndex:i] getCondition];
		if ((cond->condition & 0xF8) == MONITOR_BREAKPOINT_PC) {
			switch (cond->condition & 0x07) {
				case MONITOR_BREAKPOINT_EQUAL:
				case MONITOR_BREAKPOINT_LESS:
				case MONITOR_BREAKPOINT_LESS | MONITOR_BREAKPOINT_EQUAL:
				case MONITOR_BREAKPOINT_GREATER:
				case MONITOR_BREAKPOINT_GREATER | MONITOR_BREAKPOINT_EQUAL:
					if (cond->on)
						pc = TRUE;
					break;
				default:
					break;
			}
		}
	}
	
	return(pc);
}

-(void) getPcMin:(int *) min Max:(int *) max
{
	*min = pcMin;
	*max = pcMax;
}

-(void) getMemMin:(int *) min Max:(int *) max
{
	*min = memMin;
	*max = memMax;
}

-(void) setPcMin:(int)min
{
	pcMin = min;
}

-(void) setPcMax:(int)max
{
	pcMin = max;
}

-(void) setMemMin:(int)min
{
	memMin = min;
}

-(void) setMemMax:(int)min
{
	memMax = min;
}

-(id) copyWithZone:(NSZone *)zone 
{
	int start,end,min,max,count;
	NSMutableArray *conds;
	int i;
	
	Breakpoint *copyBreak = [[[self class] allocWithZone:zone] init];
	[self getStartIndex:&start endIndex:&end];
	[copyBreak setStartIndex:start];
	[copyBreak setEndIndex:start];
	conds = [NSMutableArray arrayWithCapacity:10];
	count = [self conditionCount];
	for (i=0;i<count;i++)
		[conds addObject:[[self conditionAtIndex:i] copy]];
	[copyBreak setConditions:conds];
	[self getPcMin:&min Max:&max];
	[copyBreak setPcMin:min];
	[copyBreak setPcMax:max];
	[self getMemMin:&min Max:&max];
	[copyBreak setMemMin:min];
	[copyBreak setMemMax:max];

	return copyBreak;
}

@end
