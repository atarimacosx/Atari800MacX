/* DisasmTableView.h - DisasmTableView 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */


#import <Cocoa/Cocoa.h>

@interface DisasmTableView : NSTableView
{
    int menuActionRow;
    int menuActionColumn;
}

/***************/
/*  accessors  */
/***************/

- (int)menuActionRow;
- (int)menuActionColumn;
- (IBAction) deleteBreakpoint:(id)sender;
- (IBAction) editBreakpoint:(id)sender;
- (IBAction) toggleEnableBreakpoint:(id)sender;
- (IBAction) addBreakpoint:(id)sender;

@end
