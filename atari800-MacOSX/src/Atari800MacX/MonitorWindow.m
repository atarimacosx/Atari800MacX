/* MonitorWindow.m - 
 MonitorWindow class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 */

#import "ControlManager.h"
#import "MonitorWindow.h"


@implementation MonitorWindow
/*------------------------------------------------------------------------------
*  init -
*-----------------------------------------------------------------------------*/
-(id) init
{
	id me;
	
	me = [super init];
	
	return(me);
}

- (void)sendEvent:(NSEvent *)anEvent
{
	unichar firstChar;
	
    if ([anEvent type] == NSKeyDown) {
		firstChar = [[anEvent characters] characterAtIndex:0];
		if (firstChar == NSUpArrowFunctionKey) {
			[[ControlManager sharedInstance] monitorUpArrow];
			return;
			}
		else if (firstChar == NSDownArrowFunctionKey) {
			[[ControlManager sharedInstance] monitorDownArrow];
			return;
			}
    }
    [super sendEvent:anEvent];
}

/*------------------------------------------------------------------------------
*  selectAll - All window classes need this in this program, even if they have
*   nothing to select, since this method is send to the key window whenever
*   the Select All menu item is chosen.
*-----------------------------------------------------------------------------*/
- (void)selectAll:(id)sender
{
}

@end
