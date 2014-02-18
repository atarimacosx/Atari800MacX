/* MemoryEditorTableView.h - MemoryEditorTableView 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 
 */


#import <Cocoa/Cocoa.h>
#import "WatchDataSource.h"

@interface MemoryEditorTableView : NSTableView
{
    int menuActionRow;
    int menuActionColumn;
	WatchDataSource *watchDataSource;
}

- (int) menuActionRow;
- (int) menuActionColumn;

@end