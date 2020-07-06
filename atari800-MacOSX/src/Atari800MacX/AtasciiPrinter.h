/* Atari825Simulator.h - Atari825Simulator 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */

#import <Cocoa/Cocoa.h>
#import "PrintableString.h"
#import "PrintableGraphics.h"
#import "PrinterProtocol.h"

#if 0
typedef struct ATARI825_PREF {
	int charSet; 
    int formLength;
    int autoLinefeed; 
} ATARI825_PREF;
#endif

@interface AtasciiPrinter : NSObject <PrinterProtocol> {
	int state;
	int style;
	int pitch;
	NSFont *styles[16];
	float leftMargin;
	float rightMargin;
	float compressedRightMargin;
	float startHorizPosition;
	float nextHorizPosition;
	float vertPosition;
	bool autoLineFeed;
	float lineSpacing;
	float formLength;
	bool expandedMode;
	bool compressedMode;
	bool underlineMode;
	bool proportionalMode;
	float horizWidth;
	float propCharSetback;
	int countryMode;
	int skipOverPerf;
	bool splitPerfSkip;

	PrintableString *printBuffer;
}

+ (AtasciiPrinter *)sharedInstance;
- (void)addChar:(unsigned short)unicharacter;

-(void)setStyle;
-(void)emptyPrintBuffer;
-(void)executeLineFeed:(float)amount;

@end
