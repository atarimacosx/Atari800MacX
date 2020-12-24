/* PasteManager.h - PasteManager 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */


#import <Cocoa/Cocoa.h>


@interface PasteManager : NSObject {
    NSDictionary *parseDict;
	NSString *pasteString;
    int charCount;
    UInt8 pasteBuffer[2048];
    int pasteIndex;
}

+ (PasteManager *)sharedInstance;
- (int)getScancode:(unsigned short *) code;
- (void)pasteStringToKeys;
- (BOOL)scancodeForCharacter:(char) c:(UInt8 *) ch;
- (void)startCopy:(char *)string;
- (int)startPaste;

@end
