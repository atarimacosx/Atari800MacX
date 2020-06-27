/* LabelDataSource.m - 
 LabelDataSource class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "LabelDataSource.h"
#import "Label.h"
#import "ControlManager.h"

@implementation LabelDataSource

-(id) init
{
	labels = nil;
	sortedLabels = nil;
	currentColumnId = [@"L" retain];
	currentOrderAscending = YES;
	dirty = YES;

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
 *  setDirty - Set dirty flag so labels will get reloaded
 *-----------------------------------------------------------------------------*/
-(void) setDirty
{
	dirty = YES;
}

/*------------------------------------------------------------------------------
 *  objectValueForTableColumn - Get the value to display in a cell.
 *-----------------------------------------------------------------------------*/
-(id) tableView:(NSTableView *) aTableView 
objectValueForTableColumn:(NSTableColumn *)aTableColumn
			row:(int) rowIndex
{
	Label *l;

	if (currentOrderAscending)
		l = [sortedLabels objectAtIndex:rowIndex];
	else
		l = [sortedLabels objectAtIndex:([sortedLabels count] - 1 - rowIndex)];
	
	if ([[aTableColumn identifier] isEqual:@"L"]) {
		return([NSString stringWithCString:[l labelName] encoding:NSASCIIStringEncoding]);
    } else if ([[aTableColumn identifier] isEqual:@"V"]) {
		return([NSString stringWithFormat:@"%04X",[l addr]]);
	} else if ([[aTableColumn identifier] isEqual:@"B"]) {
		if ([l builtin]) 
			return(@"B");
		else 
			return(@"U");
	} else if ([[aTableColumn identifier] isEqual:@"R"]) {
			if ([l read])
				return(@"R");
			else if ([l write])
				return(@"W");
			else
				return nil;
	}
	return nil;
}

/*------------------------------------------------------------------------------
 *  setObjectValue - Change the value in the data source based on what the user
 *      has edited in the table.
 *-----------------------------------------------------------------------------*/
- (void)tableView:(NSTableView *)aTableView 
   setObjectValue:(id)anObject 
   forTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex
{
}

/*------------------------------------------------------------------------------
 *  numberOfRowsInTableView - Return the number of rows in the table.
 *-----------------------------------------------------------------------------*/
-(int) numberOfRowsInTableView:(NSTableView *)aTableView
{
	static int firstTime = TRUE;
	
	if (firstTime) {
		[aTableView setHighlightedTableColumn:[aTableView tableColumnWithIdentifier:@"L"]];
		[aTableView setIndicatorImage: (currentOrderAscending ? 
									   [NSImage imageNamed:@"NSAscendingSortIndicator"] :
									   [NSImage imageNamed:@"NSDescendingSortIndicator"])
						inTableColumn:[aTableView tableColumnWithIdentifier:@"L"]];			
		
		firstTime = FALSE;
	}
	return([sortedLabels count]);
}

- (void)tableView:(NSTableView *)tableView didClickTableColumn:(NSTableColumn *)tableColumn
{
	if ([[tableColumn identifier] isEqual:currentColumnId]) {
		currentOrderAscending = !currentOrderAscending;
	}
	else {
		[tableView setIndicatorImage:nil inTableColumn:[tableView tableColumnWithIdentifier:currentColumnId]];
		[tableView setHighlightedTableColumn:tableColumn];
		currentOrderAscending = YES;
	}
	currentColumnId = [tableColumn identifier];
	
	[tableView setIndicatorImage: (currentOrderAscending ? 
								   [NSImage imageNamed:@"NSAscendingSortIndicator"] :
								   [NSImage imageNamed:@"NSDescendingSortIndicator"])
								    inTableColumn:tableColumn];			
	[self sortByCurrentColumn];
	[(ControlManager *)owner updateMonitorGUILabels];
}
		
-(void) loadLabels
{	
	int i;
	const symtable_rec *p;
	Label *l;
	
	if (!dirty)
		return;
	
	if (labels != nil)
		[labels release];
	labels = [NSMutableArray arrayWithCapacity:20];
	[labels retain];
	
	for (i = 0; i < symtable_user_size; i++) {
		p = &symtable_user[i];
		l = [Label labelWithName:p->name Addr:p->addr Builtin:NO Read:NO Write:NO];
		[labels addObject:l];
	}
	if (symtable_builtin_enable) {
		for (p = (Atari800_machine_type == Atari800_MACHINE_5200 ? symtable_builtin_5200 : symtable_builtin); p->name != NULL; p++) {
			if (p[1].addr == p[0].addr) {
				l = [Label labelWithName:p->name Addr:p->addr Builtin:YES Read:YES Write:NO];
				[labels addObject:l];
				p++;
				l = [Label labelWithName:p->name Addr:p->addr Builtin:YES Read:NO Write:YES];
				[labels addObject:l];
			}
			else {
				l = [Label labelWithName:p->name Addr:p->addr Builtin:YES Read:NO Write:NO];
				[labels addObject:l];
			}
		}
	}
	
	[self sortByCurrentColumn];
	dirty = NO;
}

-(void) sortByCurrentColumn
{
	if (sortedLabels != nil)
		[sortedLabels release];
	if ([currentColumnId isEqual:@"L"]) {
		sortedLabels = [labels sortedArrayUsingSelector:@selector(compareLabels:)];
	} else if ([currentColumnId isEqual:@"V"]) {
		sortedLabels = [labels sortedArrayUsingSelector:@selector(compareValues:)];
	} else if ([currentColumnId isEqual:@"B"]) {
		sortedLabels = [labels sortedArrayUsingSelector:@selector(compareBuiltins:)];
	} else if ([currentColumnId isEqual:@"R"]) {
		sortedLabels = [labels sortedArrayUsingSelector:@selector(compareReadWrites:)];
	}
	[sortedLabels retain];
}

@end
