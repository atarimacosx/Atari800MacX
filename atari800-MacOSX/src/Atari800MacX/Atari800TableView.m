/* Atari800TableView.m - TableView class to
   support Drag and Drop to the disk image editor.
   For the Macintosh OS X SDL port 
   of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   
*/
#import "Atari800TableView.h"
#import "Preferences.h"
#import "atari.h"
#import "atrUtil.h"
#import "atrMount.h"
#import "MediaManager.h"
#import "DiskEditorDataSource.h"

/* Subclass of NSTableView to allow for drag and drop and other specific functions  */

@implementation Atari800TableView

/*------------------------------------------------------------------------------
*  mouseDown - This method makes sure row is selected if is not a shift or 
*    command select extension.  This is needed for the hack that is used
*    to allow a drag of file promises, since NSTableView does not directly
*    support this.
*-----------------------------------------------------------------------------*/
- (void)mouseDown:(NSEvent *)theEvent
{
	int row;
	int flags;
	
	[[self dataSource] saveEvent:theEvent];
	
	row = [self rowAtPoint:[self convertPoint:[theEvent locationInWindow]
                        fromView:nil]];
	flags = [theEvent modifierFlags];
	if (([self isRowSelected:row] == NO) && 
        !(flags & NSEventModifierFlagShift) &&
        !(flags & NSEventModifierFlagCommand))
        [self selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
	/* Call the normal mouseDown processing */	
	[super mouseDown:theEvent];
}

/*------------------------------------------------------------------------------
*  selectAll - Provide a method to connect Select All menu item to.
*-----------------------------------------------------------------------------*/
- (void)selectAll:(id)sender
{
	[[self dataSource] allSelected];
	[super selectAll:sender];
}

@end
