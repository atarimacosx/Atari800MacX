/* BreakpointsController.m - 
 BreakpointsController class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "BreakpointsController.h"
#import "BreakpointEditorDataSource.h"
#import "Breakpoint.h"
#import "ControlManager.h"
#import "cpu.h"

#define BREAK_TABLE_FULL @"Sorry, Breakpoint Table is Full!"

void BreakpointsControllerSetDirty(void)
{
	[[BreakpointsController sharedInstance] setDirty];
}

int BreakpointsControllerGetBreakpointNumForConditionNum(int num) 
{
	return([[BreakpointsController sharedInstance] getBreakpointNumForConditionNum:num]);
}

@implementation BreakpointsController
static BreakpointsController *sharedInstance = nil;

+ (BreakpointsController *)sharedInstance {
	return sharedInstance ? sharedInstance : [[self alloc] init];
}

- (id)init {
    if (sharedInstance) {
		[self dealloc];
    } else {
        [super init];
        sharedInstance = self;
	}
	dirty = YES;
	breakpoints = nil;
	
    return sharedInstance;
}

-(void) setDisasmDataSource:(DisasmDataSource *)dataSource
{
	disDataSource = dataSource;
}

-(void) setBreakpointDataSource:(BreakpointDataSource *)dataSource
{
	breakpointDataSource = dataSource;
}

-(void) setBreakpointEditorDataSource:(BreakpointEditorDataSource *)dataSource;
{
	breakpointEditorDataSource = dataSource;
}

-(NSMutableArray *) getBreakpoints
{
	return breakpoints;
}

-(void) checkBreakpointsEnabled
{
	int one_break_entry_on = FALSE;
	int i;
	
	for (i=0;i<MONITOR_breakpoint_table_size;i++)
		if (MONITOR_breakpoint_table[i].on)
			one_break_entry_on = TRUE;
	if (one_break_entry_on && break_table_on)
		MONITOR_breakpoints_enabled = TRUE;
	else
		MONITOR_breakpoints_enabled = FALSE;
}

-(void) loadBreakpoints
{
	int i,start,end;
	
	if (!dirty) {
		[disDataSource updateBreakpoints];
		return;
	}
	
	if (breakpoints != nil)
		[breakpoints release];
	breakpoints = [NSMutableArray arrayWithCapacity:40];
	[breakpoints retain];
	if (MONITOR_breakpoint_table_size == 0) {
		dirty = NO;
		[disDataSource updateBreakpoints];
		[breakpointDataSource loadBreakpoints];
		return;
	}
	start = 0;
	for (i=0;i<MONITOR_breakpoint_table_size;i++) {
		if (MONITOR_breakpoint_table[i].condition == MONITOR_BREAKPOINT_OR) {
			end = i;
			[breakpoints addObject:[Breakpoint breakpointWithStartIndex:start endIndex:end]];
			if (i != MONITOR_breakpoint_table_size-1)
				start = i+1;
		}
	}
	end = i-1;
	[breakpoints addObject:[Breakpoint breakpointWithStartIndex:start endIndex:end]];
	[disDataSource updateBreakpoints];
	[breakpointDataSource loadBreakpoints];
	[self checkBreakpointsEnabled];
	dirty = NO;
}

-(void) adjustBreakpointEnables
{
	int i;
	int lastIndexOn = -1;
	
	for (i=MONITOR_breakpoint_table_size-1;i>=0;i--) {
		if (MONITOR_breakpoint_table[i].on) {
			lastIndexOn = i;
			break;
		}
	}
	
	if (lastIndexOn != -1) {
		if (MONITOR_breakpoint_table[lastIndexOn].condition == MONITOR_BREAKPOINT_OR)
			MONITOR_breakpoint_table[lastIndexOn].on = FALSE;
	}
	[disDataSource updateBreakpoints];
	[breakpointDataSource adjustBreakpointEnables];
	[self checkBreakpointsEnabled];
}

- (void) setDirty
{
	dirty = YES;
}

-(Breakpoint *) getBreakpointWithPC:(unsigned short) pc;
{
	int i, min, max;
	Breakpoint * breakpoint;
	
	for (i=0;i<[breakpoints count];i++) {
		breakpoint = [breakpoints objectAtIndex:i];
		[breakpoint getPcMin:&min Max:&max];
		if (min == -1 && max == -1)
			continue;
		else if (min == max && pc == min) 
			return(breakpoint);
		else if (min==-1 && pc <= max) 
			return(breakpoint);
		else if (max==-1 && pc >= min)
			return(breakpoint);
		else if (pc >= min && pc <= max)
			return(breakpoint);
	}
	return(nil);
}

-(Breakpoint *) getBreakpointWithMem:(unsigned short) addr
{
	int i, min, max;
	Breakpoint * breakpoint;
	
	for (i=0;i<[breakpoints count];i++) {
		breakpoint = [breakpoints objectAtIndex:i];
		[breakpoint getMemMin:&min Max:&max];
		if (min == -1 && max == -1)
			continue;
		else if (min == max && addr == min) 
			return(breakpoint);
		else if (min==-1 && addr <= max) 
			return(breakpoint);
		else if (max==-1 && addr >= min)
			return(breakpoint);
		else if (addr >= min && addr <= max)
			return(breakpoint);
	}
	return(nil);
}

-(Breakpoint *) getBreakpointWithPCRow:(unsigned short) row
{
	unsigned short pc;
	
	pc = [disDataSource getAddress:row];
	return([self getBreakpointWithPC:pc]);
}

-(void) addBreakpointWithPC:(unsigned short) pc
{
	MONITOR_breakpoint_cond *cond;
	Breakpoint *theBreakpoint;
	
	if (MONITOR_breakpoint_table_size == MONITOR_BREAKPOINT_TABLE_MAX) {
		[[ControlManager sharedInstance] error:BREAK_TABLE_FULL];
		return;
	}
	
	if (MONITOR_breakpoint_table_size == 0)
		cond = MONITOR_breakpoint_table;
	else if (MONITOR_breakpoint_table[MONITOR_breakpoint_table_size-1].condition == MONITOR_BREAKPOINT_OR)
		cond = &MONITOR_breakpoint_table[MONITOR_breakpoint_table_size];
	else {
		cond = &MONITOR_breakpoint_table[MONITOR_breakpoint_table_size];
		cond->condition = MONITOR_BREAKPOINT_OR;
		theBreakpoint = [breakpoints objectAtIndex:([breakpoints count]-1)];
		if ([theBreakpoint isEnabled] != 0)
			cond->on = TRUE;
		else
			cond->on = FALSE;
		MONITOR_breakpoint_table_size++;
		cond++;
	}
	cond->condition = MONITOR_BREAKPOINT_PC | MONITOR_BREAKPOINT_EQUAL;
	cond->addr = pc;
	cond->on = TRUE;
	MONITOR_breakpoint_table_size++;
	
	// Check if we are setting Break at current PC, and if so do something to
	//             stop it from hitting the first time.
	if (pc == CPU_regPC) {
		CPU_hit_breakpoint = TRUE;
	}
	dirty = YES;
}

-(void) addBreakpointWithType:(int) type Mem:(unsigned short) memAddr Size:(int) size
{
	MONITOR_breakpoint_cond *cond;
	Breakpoint *theBreakpoint;
	
	if (MONITOR_breakpoint_table_size >= MONITOR_BREAKPOINT_TABLE_MAX-size) {
		[[ControlManager sharedInstance] error:BREAK_TABLE_FULL];
		return;
	}
	
	if (MONITOR_breakpoint_table_size == 0)
		cond = MONITOR_breakpoint_table;
	else if (MONITOR_breakpoint_table[MONITOR_breakpoint_table_size-1].condition == MONITOR_BREAKPOINT_OR)
		cond = &MONITOR_breakpoint_table[MONITOR_breakpoint_table_size];
	else {
		cond = &MONITOR_breakpoint_table[MONITOR_breakpoint_table_size];
		cond->condition = MONITOR_BREAKPOINT_OR;
		theBreakpoint = [breakpoints objectAtIndex:([breakpoints count]-1)];
		if ([theBreakpoint isEnabled] != 0)
			cond->on = TRUE;
		else
			cond->on = FALSE;
		MONITOR_breakpoint_table_size++;
		cond++;
	}
	
	if (size == HALFWORD_SIZE) {
		cond->condition = type | MONITOR_BREAKPOINT_EQUAL;
		cond->addr = memAddr;
		cond->on = TRUE;
		MONITOR_breakpoint_table_size++;
	}
	else {
		cond->condition = type | MONITOR_BREAKPOINT_EQUAL | MONITOR_BREAKPOINT_GREATER;
		cond->addr = memAddr;
		cond->on = TRUE;
		MONITOR_breakpoint_table_size++;
		cond++;
		cond->condition = type | MONITOR_BREAKPOINT_EQUAL | MONITOR_BREAKPOINT_LESS;
		cond->addr = memAddr+1;
		cond->on = TRUE;
		MONITOR_breakpoint_table_size++;
	}
	dirty = YES;
}

-(void) addBreakpointWithMem:(unsigned short) memAddr Size:(int) size Value: (unsigned short) memValue
{
	MONITOR_breakpoint_cond *cond;
	Breakpoint *theBreakpoint;
	
	if (MONITOR_breakpoint_table_size == MONITOR_BREAKPOINT_TABLE_MAX) {
		[[ControlManager sharedInstance] error:BREAK_TABLE_FULL];
		return;
	}
	
	if (MONITOR_breakpoint_table_size == 0)
		cond = MONITOR_breakpoint_table;
	else if (MONITOR_breakpoint_table[MONITOR_breakpoint_table_size-1].condition == MONITOR_BREAKPOINT_OR)
		cond = &MONITOR_breakpoint_table[MONITOR_breakpoint_table_size];
	else {
		cond = &MONITOR_breakpoint_table[MONITOR_breakpoint_table_size];
		cond->condition = MONITOR_BREAKPOINT_OR;
		theBreakpoint = [breakpoints objectAtIndex:([breakpoints count]-1)];
		if ([theBreakpoint isEnabled] != 0)
			cond->on = TRUE;
		else
			cond->on = FALSE;
		MONITOR_breakpoint_table_size++;
		cond++;
	}
	
	if (size == HALFWORD_SIZE) {
		cond->condition = MONITOR_BREAKPOINT_MEM | MONITOR_BREAKPOINT_EQUAL;
		cond->addr = memAddr;
		cond->val = memValue & 0xFF;
		cond->on = TRUE;
		MONITOR_breakpoint_table_size++;
	}
	else {
		cond->condition = MONITOR_BREAKPOINT_MEM | MONITOR_BREAKPOINT_EQUAL;
		cond->addr = memAddr;
		cond->val = memValue & 0xFF;
		cond->on = TRUE;
		MONITOR_breakpoint_table_size++;
		cond++;
		cond->condition = MONITOR_BREAKPOINT_MEM | MONITOR_BREAKPOINT_EQUAL;
		cond->addr = memAddr+1;
		cond->val = (memValue & 0xFF00) >> 8;
		cond->on = TRUE;
		MONITOR_breakpoint_table_size++;
	}
	dirty = YES;
}

-(void) addBreakpointWithRow:(int) row
{
	unsigned short pc;
	
	pc = [disDataSource getAddress:row];
	[self addBreakpointWithPC:pc];
}

-(void) replaceBreakpoint:(Breakpoint *) old with:(Breakpoint *) new
{
	int start, end;
	int newCount, oldCount;
	int numToMove, distToMove, i;
	
	[old getStartIndex:&start endIndex:&end];
	oldCount = end-start+1;
	newCount = [new conditionCount];
	
	if ((MONITOR_breakpoint_table_size + abs(newCount - oldCount)) >= MONITOR_BREAKPOINT_TABLE_MAX) {
		[[ControlManager sharedInstance] error:BREAK_TABLE_FULL];
		return;
	}
	
	if (newCount > oldCount) {
		int from;
		
		// Grow the table
		distToMove = newCount - oldCount;
		numToMove = MONITOR_breakpoint_table_size-1-end;
		for (i=0;i<numToMove;i++) {
			from = MONITOR_breakpoint_table_size-1-i;
			memcpy(&MONITOR_breakpoint_table[from+distToMove], 
				   &MONITOR_breakpoint_table[from], 
				   sizeof(MONITOR_breakpoint_cond));
		}
		MONITOR_breakpoint_table_size += distToMove;
	}
	else if (newCount < oldCount) {
		int from;
		
		// shrink the table
		distToMove = oldCount - newCount;
		numToMove = MONITOR_breakpoint_table_size-1-end;

		for (i=0;i<distToMove;i++) {
			from = end+1+i;
			memcpy(&MONITOR_breakpoint_table[from-numToMove], 
				   &MONITOR_breakpoint_table[from], 
				   sizeof(MONITOR_breakpoint_cond));
		}
		MONITOR_breakpoint_table_size -= distToMove;
	}
	
	// Move the new breakpoint into the table in place of the old one.
    for (i=0;i<newCount;i++) {
        MONITOR_breakpoint_cond *cond = [[new conditionAtIndex:i] getCondition];
        memcpy(&MONITOR_breakpoint_table[start+i],
               cond,
               sizeof(MONITOR_breakpoint_cond));
    }
	dirty = YES;
}

-(void) toggleBreakpointEnable:(Breakpoint *) theBreakpoint
{
	BOOL enable;
	
	if ([theBreakpoint isEnabled] == 0)
		enable = YES;
	else
		enable = NO;
	[theBreakpoint setEnabled:enable];
	[self adjustBreakpointEnables];
}

-(void) toggleBreakpointEnableWithPC:(unsigned short) pc
{
	Breakpoint *theBreakpoint;
	
	theBreakpoint = [self getBreakpointWithPC:pc];
	if (theBreakpoint == nil)
		return;
	[self toggleBreakpointEnable:theBreakpoint];
}

-(void) toggleBreakpointEnableWithRow:(int) row
{
	unsigned short pc;
	
	pc = [disDataSource getAddress:row];
	[self toggleBreakpointEnableWithPC:pc];
}

-(void) toggleBreakpointEnableWithIndex:(int) index;
{
	Breakpoint *theBreakpoint;
	
	theBreakpoint = [breakpoints objectAtIndex:index];
	if (theBreakpoint == nil)
		return;
	[self toggleBreakpointEnable:theBreakpoint];
}

-(void) editBreakpoint:(Breakpoint *) theBreakpoint
{
	int status;
	
	[breakpointEditorDataSource startWithBreakpoint:theBreakpoint];
	status = [[ControlManager sharedInstance] runBreakpointEditor:self];
	if (status == 1) {
		[self replaceBreakpoint:theBreakpoint with:[breakpointEditorDataSource getBreakpoint]];
	}
}

-(void) editBreakpointWithPC:(unsigned short) pc
{
	Breakpoint *theBreakpoint;
	
	theBreakpoint = [self getBreakpointWithPC:pc];
	if (theBreakpoint == nil)
		return;
	[self editBreakpoint:theBreakpoint];
}

-(void) editBreakpointWithMem:(unsigned short) memAddr
{
	Breakpoint *theBreakpoint;
	
	theBreakpoint = [self getBreakpointWithMem:memAddr];
	if (theBreakpoint == nil)
		return;
	[self editBreakpoint:theBreakpoint];
}


-(void) editBreakpointWithRow:(int) row
{
	unsigned short pc;
	
	pc = [disDataSource getAddress:row];
	[self editBreakpointWithPC:pc];
}

-(void) editBreakpointWithIndex:(int) index
{
	Breakpoint *theBreakpoint;
	
	theBreakpoint = [breakpoints objectAtIndex:index];
	if (theBreakpoint == nil)
		return;
	[self editBreakpoint:theBreakpoint];
}

-(void) deleteBreakpoint:(Breakpoint *) theBreakpoint
{
	int from,start,end,numToMove,distToMove;
	int i;

	[theBreakpoint getStartIndex:&start endIndex:&end];
	// If it is the last breakpoint
	if (end == MONITOR_breakpoint_table_size-1) {
		// If this is not the only breakpoint
		if ([breakpoints count] > 1) {
			// remove the OR from the previous breakpoint
			start--;
		}
	}
	distToMove = end-start+1;
	numToMove = MONITOR_breakpoint_table_size-1-end;
	for (i=0;i<numToMove;i++) {
		from = end+1+i;
		memcpy(&MONITOR_breakpoint_table[from-distToMove], 
			   &MONITOR_breakpoint_table[from], 
			   sizeof(MONITOR_breakpoint_cond));
	}
	
	for (i=0;i<numToMove;i++)
		memcpy(&MONITOR_breakpoint_table[start+i], &MONITOR_breakpoint_table[end+1+i], sizeof(MONITOR_breakpoint_cond));
	
	MONITOR_breakpoint_table_size -= end-start+1;
	dirty = YES;
}

-(void) deleteBreakpointWithMem:(unsigned short) memAddr
{
	Breakpoint *theBreakpoint;
	
	theBreakpoint = [self getBreakpointWithMem:memAddr];
	if (theBreakpoint == nil)
		return;
	[self deleteBreakpoint:theBreakpoint];
}

-(void) deleteBreakpointWithPC:(unsigned short) pc
{
	Breakpoint *theBreakpoint;
	
	theBreakpoint = [self getBreakpointWithPC:pc];
	if (theBreakpoint == nil)
		return;
	[self deleteBreakpoint:theBreakpoint];
}

-(void) deleteBreakpointWithRow:(int) row
{
	unsigned short pc;
	
	pc = [disDataSource getAddress:row];
	[self deleteBreakpointWithPC:pc];
}

-(void) deleteBreakpointWithIndex:(int) index
{
	Breakpoint *theBreakpoint;
	
	theBreakpoint = [breakpoints objectAtIndex:index];
	if (theBreakpoint == nil)
		return;
	[self deleteBreakpoint:theBreakpoint];
}

-(void) deleteAllBreakpoints
{
	MONITOR_breakpoint_table_size = 0;
	dirty = YES;
}

-(int) getBreakpointCount
{
	return [breakpoints count];
}

-(int) getBreakpointNumForConditionNum:(int) num
{
	int count, i, start, end;
	Breakpoint *theBreakpoint;
	
	count = [breakpoints count];
	for (i=0;i<count;i++) {
		theBreakpoint = [breakpoints objectAtIndex:i];
		[theBreakpoint getStartIndex:&start endIndex:&end];
		if (num>=start && num<=end)
			return(i);
	}
	return(i);
}

@end
