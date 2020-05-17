/* WatchTableView.h - WatchTableView 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */


#import <Cocoa/Cocoa.h>

@interface WatchTableView : NSTableView
{
    int menuActionRow;
    int menuActionColumn;
}

- (IBAction) deleteWatch:(id)sender;
- (IBAction) addWatch:(id)sender;
- (IBAction) setSize:(id)sender;
- (IBAction) setFormat:(id)sender;
- (IBAction) showInMemoryView:(id)sender;
- (IBAction) addReadBreak:(id)sender;
- (IBAction) addWriteBreak:(id)sender;
- (IBAction) addAccessBreak:(id)sender;
- (IBAction) addValueBreak:(id)sender;
- (IBAction) editBreak:(id)sender;
- (IBAction) delBreak:(id)sender;
- (int) getActionRow;
- (int) getActionColumn;

@end
