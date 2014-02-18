/* PasteWindow.h - PasteWindow 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 
 */


#import <Cocoa/Cocoa.h>


@interface PasteWindow : NSWindow {

}

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)anItem;
- (void) paste:(id) sender;

@end
