/* BreakpointTableView.h - BreakpointTableView 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 
 */

#import <Cocoa/Cocoa.h>

@interface BreakpointTableView : NSTableView
{
    int menuActionRow;
    int menuActionColumn;
}

- (IBAction) deleteBreakpoint:(id)sender;
- (IBAction) deleteAllBreakpoints:(id)sender;
- (IBAction) editBreakpoint:(id)sender;
- (IBAction) addBreakpoint:(id)sender;
- (int) getActionRow;
- (int) getActionColumn;

@end