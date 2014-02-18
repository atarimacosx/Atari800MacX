/* MemoryEditorByteFieldCell.h - MemoryEditorByteFieldCell 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 
 */


#import <Cocoa/Cocoa.h>


@interface MemoryEditorByteFieldCell : NSTextFieldCell {
}

- (id)init;
- (int) isValidCharacter:(char) c;
- (void) textDidChange:(NSNotification *) note;

@end
