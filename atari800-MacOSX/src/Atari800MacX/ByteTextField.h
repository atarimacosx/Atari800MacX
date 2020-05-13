/* ByteTextField.h - ByteTextField class to
 support Drag and Drop to the disk image editor.
 For the Macintosh OS X SDL port 
 of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */

#import <Cocoa/Cocoa.h>


@interface ByteTextField : NSTextField {
}

- (id)init;
- (int) isValidCharacter:(char) c;
- (void) textDidChange:(NSNotification *) note;

@end
