/* MemoryEditorTableView.m - 
 MemoryEditorTableView class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "MemoryEditorTableView.h"

#define ADD_BYTE_INDEX		0
#define ADD_WORD_INDEX		1

@implementation MemoryEditorTableView

- (id)init
{
	[super init];
	
	[[[self menu] itemAtIndex:ADD_BYTE_INDEX] setTarget:self];
	[[[self menu] itemAtIndex:ADD_WORD_INDEX] setTarget:self];
	[[[self menu] itemAtIndex:ADD_BYTE_INDEX] setEnabled:YES];
	[[[self menu] itemAtIndex:ADD_WORD_INDEX] setEnabled:YES];
	
	return(self);
}

- (void)rightMouseDown: (NSEvent *)theEvent
{
    NSPoint eventPoint;	// the point where mouse was down
    
    if (![self menu])	// no menu = no need to go any further
    {
        menuActionRow = menuActionColumn = -1;
        // set both to -1 to indicate there wasn't any menu action
        
        return;
    }
    
    eventPoint = [theEvent locationInWindow];	// get the event location from window
    eventPoint = [self convertPoint: eventPoint fromView: nil];
    // convert the location from window to MenuTableView
    
    menuActionRow = [super rowAtPoint: eventPoint];	// get the row index where mouse was down
    menuActionColumn = [super columnAtPoint: eventPoint];
    // get the column index where mouse was down
	
	if (menuActionRow >= 0 && menuActionRow <= 7 && 
		menuActionColumn >= 1 && menuActionColumn <= 16) {
		[NSMenu popUpContextMenu: [self menu] withEvent: theEvent forView: self];
	}
}

- (NSMenu *)menuForEvent: (NSEvent *)theEvent
{
    return nil;
}

- (int) menuActionRow
{
    return menuActionRow;
}

- (int) menuActionColumn
{
    return menuActionColumn;
}

@end
