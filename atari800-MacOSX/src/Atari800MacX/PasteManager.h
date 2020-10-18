/* PasteManager.h - PasteManager 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */


#import <Cocoa/Cocoa.h>


@interface PasteManager : NSObject {
	NSString *pasteString;
	int charCount;
    int ctrlMode;
}

+ (PasteManager *)sharedInstance;
- (int)startPaste;
- (int)getChar:(unsigned short *) character;
- (void)startCopy:(char *)string;

@end
