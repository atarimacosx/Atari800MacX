/* BreakpointEditorDataSource.m - 
 BreakpointEditorDataSource class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "BreakpointEditorDataSource.h"
#import "ControlManager.h"

#define EDITOR_BREAKPOINT_PC          0
#define EDITOR_BREAKPOINT_A           1
#define EDITOR_BREAKPOINT_X           2
#define EDITOR_BREAKPOINT_Y           3
#define EDITOR_BREAKPOINT_S           4
#define EDITOR_BREAKPOINT_MEM         5
#define EDITOR_BREAKPOINT_READ        6
#define EDITOR_BREAKPOINT_WRITE       7
#define EDITOR_BREAKPOINT_ACCESS      8
#define EDITOR_BREAKPOINT_CLRN        9 
#define EDITOR_BREAKPOINT_SETN        10 
#define EDITOR_BREAKPOINT_CLRV        11 
#define EDITOR_BREAKPOINT_SETV        12 
#define EDITOR_BREAKPOINT_CLRB        13 
#define EDITOR_BREAKPOINT_SETB        14 
#define EDITOR_BREAKPOINT_CLRD        15 
#define EDITOR_BREAKPOINT_SETD        16 
#define EDITOR_BREAKPOINT_CLRI        17 
#define EDITOR_BREAKPOINT_SETI        18 
#define EDITOR_BREAKPOINT_CLRZ        19 
#define EDITOR_BREAKPOINT_SETZ        20 
#define EDITOR_BREAKPOINT_CLRC        21 
#define EDITOR_BREAKPOINT_SETC        22 

#define EDITOR_COMPARE_EQUAL		  0
#define EDITOR_COMPARE_NOTEQUAL		  1
#define EDITOR_COMPARE_LESS			  2
#define EDITOR_COMPARE_GREATER		  3
#define EDITOR_COMPARE_LESSEQUAL	  4
#define EDITOR_COMPARE_GREATEREQUAL	  5

@implementation BreakpointEditorDataSource

static MONITOR_breakpoint_cond newCond = 
{
	MONITOR_BREAKPOINT_A | MONITOR_BREAKPOINT_EQUAL,
	TRUE,
	0,
	0
};

-(id) init
{
	myBreakpoint = nil;
	newCondition = [[BreakpointCondition alloc] initWithBreakpointCondition:&newCond];
	[newCondition retain];
	
	return self;
}

/*------------------------------------------------------------------------------
 *  setOwner - Set our owner so we can communicate with it later.
 *-----------------------------------------------------------------------------*/
-(void) setOwner:(id)theOwner
{
	owner = theOwner;
}

/*------------------------------------------------------------------------------
 *  objectValueForTableColumn - Get the value to display in a cell.
 *-----------------------------------------------------------------------------*/
-(id) tableView:(NSTableView *) aTableView 
objectValueForTableColumn:(NSTableColumn *)aTableColumn
			row:(int) rowIndex
{
	MONITOR_breakpoint_cond *cond;
	
	if ([[aTableColumn identifier] isEqual:@"PLUS"]) {
		return nil;
	}
	else if ([[aTableColumn identifier] isEqual:@"SEP"]) {
		return @"";
	}
	else if ([[aTableColumn identifier] isEqual:@"ENA"]) {
		cond = [[myBreakpoint conditionAtIndex:rowIndex] getCondition];
		
		if (cond->enabled)
			return ([NSNumber numberWithInt:1]);
		else
			return ([NSNumber numberWithInt:0]);
	}
	else if ([[aTableColumn identifier] isEqual:@"REG"]) {
		int index = -1;
		
		cond = [[myBreakpoint conditionAtIndex:rowIndex] getCondition];
		
		if (cond->condition <= MONITOR_BREAKPOINT_SETC) {
			switch(cond->condition) {
				case MONITOR_BREAKPOINT_CLRN:
					index = EDITOR_BREAKPOINT_CLRN;
					break;
				case MONITOR_BREAKPOINT_SETN:
					index = EDITOR_BREAKPOINT_SETN;
					break;
				case MONITOR_BREAKPOINT_CLRV:
					index = EDITOR_BREAKPOINT_CLRV;
					break;
				case MONITOR_BREAKPOINT_SETV:
					index = EDITOR_BREAKPOINT_SETV;
					break;
				case MONITOR_BREAKPOINT_CLRB:
					index = EDITOR_BREAKPOINT_CLRB;
					break;
				case MONITOR_BREAKPOINT_SETB:
					index = EDITOR_BREAKPOINT_SETB;
					break;
				case MONITOR_BREAKPOINT_CLRD:
					index = EDITOR_BREAKPOINT_CLRD;
					break;
				case MONITOR_BREAKPOINT_SETD:
					index = EDITOR_BREAKPOINT_SETD;
					break;
				case MONITOR_BREAKPOINT_CLRI:
					index = EDITOR_BREAKPOINT_CLRI;
					break;
				case MONITOR_BREAKPOINT_SETI:
					index = EDITOR_BREAKPOINT_SETI;
					break;
				case MONITOR_BREAKPOINT_CLRZ:
					index = EDITOR_BREAKPOINT_CLRZ;
					break;
				case MONITOR_BREAKPOINT_SETZ:
					index = EDITOR_BREAKPOINT_SETZ;
					break;
				case MONITOR_BREAKPOINT_CLRC:
					index = EDITOR_BREAKPOINT_CLRC;
					break;
				case MONITOR_BREAKPOINT_SETC:
					index = EDITOR_BREAKPOINT_SETC;
					break;
			}
		}
		else {
			switch(cond->condition & 0xF8) {
				case MONITOR_BREAKPOINT_PC:
					index = EDITOR_BREAKPOINT_PC;
					break;
				case MONITOR_BREAKPOINT_A:
					index = EDITOR_BREAKPOINT_A;
					break;
				case MONITOR_BREAKPOINT_X:
					index = EDITOR_BREAKPOINT_X;
					break;
				case MONITOR_BREAKPOINT_Y:
					index = EDITOR_BREAKPOINT_Y;
					break;
				case MONITOR_BREAKPOINT_S:
					index = EDITOR_BREAKPOINT_S;
					break;
				case MONITOR_BREAKPOINT_MEMORY:
					index = EDITOR_BREAKPOINT_MEM;
					break;
				case MONITOR_BREAKPOINT_READ:
					index = EDITOR_BREAKPOINT_READ;
					break;
				case MONITOR_BREAKPOINT_WRITE:
					index = EDITOR_BREAKPOINT_WRITE;
					break;
				case MONITOR_BREAKPOINT_ACCESS:
					index = EDITOR_BREAKPOINT_ACCESS;
					break;
			}
		}
		return [NSNumber numberWithInt:index];
	}
	else if ([[aTableColumn identifier] isEqual:@"VAL1"]) {		
		cond = [[myBreakpoint conditionAtIndex:rowIndex] getCondition];
		
		if ((cond->condition & 0xF8) == MONITOR_BREAKPOINT_MEMORY) {
			NSString *value;
			
			value = [NSString stringWithFormat:@"%04X",cond->m_addr];
			return(value);
		}
		else
			return nil;
	}
	else if ([[aTableColumn identifier] isEqual:@"REL"]) {
		int index = -1;
		
		cond = [[myBreakpoint conditionAtIndex:rowIndex] getCondition];
		
		if (cond->condition <= MONITOR_BREAKPOINT_SETC)
			return(nil);
		else {
			switch (cond->condition & 0x07) {
				case MONITOR_BREAKPOINT_EQUAL:
					index = EDITOR_COMPARE_EQUAL;
					break;
				case MONITOR_BREAKPOINT_LESS | MONITOR_BREAKPOINT_GREATER:
					index = EDITOR_COMPARE_NOTEQUAL;
					break;
				case MONITOR_BREAKPOINT_LESS:
					index = EDITOR_COMPARE_LESS;
					break;
				case MONITOR_BREAKPOINT_LESS | MONITOR_BREAKPOINT_EQUAL:
					index = EDITOR_COMPARE_LESSEQUAL;
					break;
				case MONITOR_BREAKPOINT_GREATER:
					index = EDITOR_COMPARE_GREATER;
					break;
				case MONITOR_BREAKPOINT_GREATER | MONITOR_BREAKPOINT_EQUAL:
					index = EDITOR_COMPARE_GREATEREQUAL;
					break;
			}
			return [NSNumber numberWithInt:index];
		}
	}
	else if ([[aTableColumn identifier] isEqual:@"VAL2"]) {
		cond = [[myBreakpoint conditionAtIndex:rowIndex] getCondition];
		
		if (cond->condition <= MONITOR_BREAKPOINT_SETC)
			return nil;
		else {
			NSString *value;
			
			switch(cond->condition & 0xF8) {
				case MONITOR_BREAKPOINT_PC:
				case MONITOR_BREAKPOINT_READ:
				case MONITOR_BREAKPOINT_WRITE:
				case MONITOR_BREAKPOINT_ACCESS:
					value = [NSString stringWithFormat:@"%04X",cond->value];
					break;
				default:
  				    value = [NSString stringWithFormat:@"%02X",cond->value];
					break;
			}
			return(value);
		}
	}
	else
	{
		if (rowIndex == editableConditionCount - 1)
			return @"";
		else
			return @" AND";
	}
}

/*------------------------------------------------------------------------------
 *  setObjectValue - Change the value in the data source based on what the user
 *      has edited in the table.
 *-----------------------------------------------------------------------------*/
- (void)tableView:(NSTableView *)aTableView 
   setObjectValue:(id)anObject 
   forTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex
{
	MONITOR_breakpoint_cond *cond;

	if ([[aTableColumn identifier] isEqual:@"PLUS"]) {
		[myBreakpoint deleteConditionAtIndex:rowIndex];
		editableConditionCount--;
	}
	else if ([[aTableColumn identifier] isEqual:@"ENA"]) {
		cond = [[myBreakpoint conditionAtIndex:rowIndex] getCondition];
		
		if ([anObject intValue] == 0) 
			cond->enabled = FALSE;
		else
			cond->enabled = TRUE;
				
		if (editableConditionCount != [myBreakpoint conditionCount]) {
			int i;
			BOOL anyOn = FALSE;
			
			for (i=0;i<editableConditionCount;i++) {
				cond = [[myBreakpoint conditionAtIndex:i] getCondition];
				if (cond->enabled)
					anyOn = TRUE;
			}
			
			// If any of the conditions are on, the OR condition should be on
			// If they are all off, it should be off
			cond = [[myBreakpoint conditionAtIndex:editableConditionCount] getCondition];
			cond->enabled = anyOn;
		}
	}
	else if ([[aTableColumn identifier] isEqual:@"REG"]) {
		int oldCondition;

		cond = [[myBreakpoint conditionAtIndex:rowIndex] getCondition];
		oldCondition = cond->condition;
		
		if ([anObject intValue] >= EDITOR_BREAKPOINT_CLRN) {
			int condition = 0;
			
			switch([anObject intValue]) {
				case EDITOR_BREAKPOINT_CLRN:
					condition = MONITOR_BREAKPOINT_CLRN;
					break;
				case EDITOR_BREAKPOINT_SETN:
					condition = MONITOR_BREAKPOINT_SETN;
					break;
				case EDITOR_BREAKPOINT_CLRV:
					condition = MONITOR_BREAKPOINT_CLRV;
					break;
				case EDITOR_BREAKPOINT_SETV:
					condition = MONITOR_BREAKPOINT_SETV;
					break;
				case EDITOR_BREAKPOINT_CLRB:
					condition = MONITOR_BREAKPOINT_CLRB;
					break;
				case EDITOR_BREAKPOINT_SETB:
					condition = MONITOR_BREAKPOINT_SETB;
					break;
				case EDITOR_BREAKPOINT_CLRD:
					condition = MONITOR_BREAKPOINT_CLRD;
					break;
				case EDITOR_BREAKPOINT_SETD:
					condition = MONITOR_BREAKPOINT_SETD;
					break;
				case EDITOR_BREAKPOINT_CLRI:
					condition = MONITOR_BREAKPOINT_CLRI;
					break;
				case EDITOR_BREAKPOINT_SETI:
					condition = MONITOR_BREAKPOINT_SETI;
					break;
				case EDITOR_BREAKPOINT_CLRZ:
					condition = MONITOR_BREAKPOINT_CLRZ;
					break;
				case EDITOR_BREAKPOINT_SETZ:
					condition = MONITOR_BREAKPOINT_SETZ;
					break;
				case EDITOR_BREAKPOINT_CLRC:
					condition = MONITOR_BREAKPOINT_CLRC;
					break;
				case EDITOR_BREAKPOINT_SETC:
					condition = MONITOR_BREAKPOINT_SETC;
					break;
			}
			cond->condition = condition;
		}
		else {
			int partialCondition = 0;
			
			switch([anObject intValue]) {
				case EDITOR_BREAKPOINT_PC:
					partialCondition = MONITOR_BREAKPOINT_PC;
					break;
				case EDITOR_BREAKPOINT_A:
					partialCondition = MONITOR_BREAKPOINT_A;
					break;
				case EDITOR_BREAKPOINT_X:
					partialCondition = MONITOR_BREAKPOINT_X;
					break;
				case EDITOR_BREAKPOINT_Y:
					partialCondition = MONITOR_BREAKPOINT_Y;
					break;
				case EDITOR_BREAKPOINT_S:
					partialCondition = MONITOR_BREAKPOINT_S;
					break;
				case EDITOR_BREAKPOINT_MEM:
					partialCondition = MONITOR_BREAKPOINT_MEMORY;
					break;
				case EDITOR_BREAKPOINT_READ:
					partialCondition = MONITOR_BREAKPOINT_READ;
					break;
				case EDITOR_BREAKPOINT_WRITE:
					partialCondition = MONITOR_BREAKPOINT_WRITE;
					break;
				case EDITOR_BREAKPOINT_ACCESS:
					partialCondition = MONITOR_BREAKPOINT_ACCESS;
					break;
			}
			if ((cond->condition & 0x07) == 0 || (cond->condition <= MONITOR_BREAKPOINT_SETC))
				cond->condition = MONITOR_BREAKPOINT_EQUAL | partialCondition;
			else
				cond->condition = (cond->condition & 0x07) | partialCondition;
		}
		if (cond->condition != oldCondition) {
			cond->value = 0;
			cond->m_addr = 0;
		}
	}
	else if ([[aTableColumn identifier] isEqual:@"VAL1"]) {
		char fieldStr[81];
		unsigned short val;
		
		cond = [[myBreakpoint conditionAtIndex:rowIndex] getCondition];

		// Don't allow changes in this field on anything but memory address condition
		if ((cond->condition & 0xF8) != MONITOR_BREAKPOINT_MEMORY)
			return;
		[anObject getCString:fieldStr maxLength:80 encoding:NSASCIIStringEncoding];
		
		if (get_val_gui(fieldStr, &val) == FALSE)
			return;
		cond->m_addr = val;
	}
	else if ([[aTableColumn identifier] isEqual:@"REL"]) {
		int partialCondition = 0;
		
		cond = [[myBreakpoint conditionAtIndex:rowIndex] getCondition];
		
		// Don't allow changes in this field on a set/clr flag condition
		if (cond->condition <= MONITOR_BREAKPOINT_SETC)
			return;
				
		switch ([anObject intValue]) {
			case EDITOR_COMPARE_EQUAL:
				partialCondition = MONITOR_BREAKPOINT_EQUAL;
				break;
			case EDITOR_COMPARE_NOTEQUAL:
				partialCondition = MONITOR_BREAKPOINT_LESS | MONITOR_BREAKPOINT_GREATER;
				break;
			case EDITOR_COMPARE_LESS:
				partialCondition = MONITOR_BREAKPOINT_LESS;
				break;
			case EDITOR_COMPARE_LESSEQUAL:
				partialCondition = MONITOR_BREAKPOINT_LESS | MONITOR_BREAKPOINT_EQUAL;
				break;
			case EDITOR_COMPARE_GREATER:
				partialCondition = MONITOR_BREAKPOINT_GREATER;
				break;
			case EDITOR_COMPARE_GREATEREQUAL:
				partialCondition = MONITOR_BREAKPOINT_GREATER | MONITOR_BREAKPOINT_EQUAL;
				break;
			}
		cond->condition = (cond->condition & 0xF8) | partialCondition;
	}
	else if ([[aTableColumn identifier] isEqual:@"VAL2"]) {
		char fieldStr[81];
		unsigned short val;

		cond = [[myBreakpoint conditionAtIndex:rowIndex] getCondition];

		// Don't allow changes in this field on a set/clr flag condition
		if (cond->condition <= MONITOR_BREAKPOINT_SETC)
			return;		
		
		[anObject getCString:fieldStr maxLength:80 encoding:NSASCIIStringEncoding];
		if (get_val_gui(fieldStr, &val) == FALSE)
			return;
		switch(cond->condition & 0xF8) {
			case MONITOR_BREAKPOINT_PC:
			case MONITOR_BREAKPOINT_READ:
			case MONITOR_BREAKPOINT_WRITE:
			case MONITOR_BREAKPOINT_ACCESS:
				cond->value = val;
				break;
			default:
				cond->value = val;
				break;
		}
	}
	else
		return;

	[(ControlManager *)owner reloadBreakpointEditor];
}

/*------------------------------------------------------------------------------
 *  numberOfRowsInTableView - Return the number of rows in the table.
 *-----------------------------------------------------------------------------*/
-(int) numberOfRowsInTableView:(NSTableView *)aTableView
{
	if (myBreakpoint == nil)
		return 0;
	else
		return(editableConditionCount);
}

-(void) startWithBreakpoint:(Breakpoint *) theBreakpoint
{
	MONITOR_breakpoint_cond *cond;
	
	if (myBreakpoint != nil)
		[myBreakpoint release];
	myBreakpoint = [theBreakpoint copy];
	[myBreakpoint retain];
	
	// Get the last condition
	cond = [[myBreakpoint conditionAtIndex:([myBreakpoint conditionCount]-1)] getCondition];
	
	if (cond->condition == MONITOR_BREAKPOINT_OR)
		editableConditionCount = [myBreakpoint conditionCount] - 1;
	else
		editableConditionCount = [myBreakpoint conditionCount];
}

-(void) addCondition
{
    BreakpointCondition *addedCondition = [newCondition copy];
    [newCondition retain];
	[myBreakpoint addCondition:addedCondition atIndex:editableConditionCount];
	editableConditionCount++;
	[(ControlManager *)owner reloadBreakpointEditor];	
}

-(Breakpoint *) getBreakpoint
{
	return(myBreakpoint);
}

@end
