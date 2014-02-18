/* SectorEditorAsciiFieldCell.h - SectorEditorAsciiFieldCell 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 
 */


#import <Cocoa/Cocoa.h>


@interface SectorEditorAsciiFieldCell : NSTextFieldCell {
}

- (id)init;
- (int) isValidCharacter:(char) c;
- (void) textDidChange:(NSNotification *) note;

@end
