/* DisasmTableView */

//  Created by Tony S. Wu on Thu Sep 26 2002.
//  No rights reserved.

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