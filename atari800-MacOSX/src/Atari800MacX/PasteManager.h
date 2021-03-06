/* PasteManager.h - PasteManager 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */


#import <Cocoa/Cocoa.h>

#define COPY_BUFFER_SIZE (64*1024)
#define PASTE_BUFFER_SIZE (2048)

@interface PasteManager : NSObject {
    NSDictionary *parseDict;
	NSString *pasteString;
    int charCount;
    char copyBuffer[COPY_BUFFER_SIZE];
    int pasteIndex;
    NSMutableArray *pasteBuffer;
    BOOL useEscapeCopy;
    NSString *startupPasteString;
    BOOL startupPasteEnabled;
    IBOutlet id escapeCopyMenuItem;
}

+ (PasteManager *)sharedInstance;
- (int)getScancode:(unsigned char *) code;
- (void)pasteStringToKeys;
- (BOOL)scancodeForCharacter:(char) c:(UInt8 *) ch;
- (void)startCopy:(char *)string;
- (int)startPaste;
- (void)startPasteWithString;
- (void)nonEscapeCopy:(char *) string;
- (void)escapeCopy:(char *) string;
- (void)setEscapeCopy:(BOOL) escape;
- (NSNumber *)getEscapeCopy;
- (void)setStartupPasteString:(NSString *)string;
- (void)setStartupPasteEnabled:(BOOL)enabled;
- (BOOL)getStartupPasteEnabled;
- (IBAction)handleEscapeCopyMenu:(id)sender;
- (void)updateEscapeCopyMenu;

@end
