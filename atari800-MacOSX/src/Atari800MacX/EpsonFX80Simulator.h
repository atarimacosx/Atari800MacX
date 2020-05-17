/* EpsonFX80Simulator.h - EpsonFX80Simulator 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */

#import <Cocoa/Cocoa.h>
#import "PrintableString.h"
#import "PrintableGraphics.h"
#import "PrinterProtocol.h"

typedef struct EPSON_PREF {
	int charSet;
    int formLength;
	int printPitch;
	int printWeight;
    int autoLinefeed;
	int printSlashedZeros;
	int autoSkip;
	int splitSkip;
} EPSON_PREF;

@interface EpsonFX80Simulator : NSObject <PrinterProtocol> {
	int state;
	int style;
	int pitch;
	NSFont *styles[64];
	float leftMargin;
	float rightMargin;
	float compressedRightMargin;
	float startHorizPosition;
	float nextHorizPosition;
	float vertPosition;
	bool autoLineFeed;
	bool slashedZeroMode;
	float lineSpacing;
	float formLength;
	bool italicMode;
	bool italicInterMode;
	bool printCtrlCodesMode;
	bool eighthBitZero;
	bool eighthBitOne;
	bool emphasizedMode;
	bool doubleStrikeMode;
	bool doubleStrikeWantedMode;
	bool eliteMode;
    bool superscriptMode;
	bool subscriptMode;
	bool expandedMode;
	bool compressedMode;
	bool expandedTempMode;
	bool underlineMode;
	bool proportionalMode;
	int  graphMode;
	int  graphCount;
	int  graphTotal;
	unsigned char graphBuffer[4096];
	int  kGraphMode;
	int  lGraphMode;
	int  yGraphMode;
	int  zGraphMode;
	float horizWidth;
	float propCharSetback;
	int  horizTabCount;
	float horizTabs[32];
	int  vertTabChan;
	int  vertTabCount[8];
	float vertTabs[8][16];
	int graphRedefCode;
	int vertTabChanSetNum;
	int countryMode;
	bool immedMode;
	int skipOverPerf;
	bool splitPerfSkip;

	PrintableString *printBuffer;
}

+ (EpsonFX80Simulator *)sharedInstance;
- (void)addChar:(unsigned short)unicharacter:(bool)italic;
- (void)idleState:(char) character;
- (void)escState:(char) character;
- (void)masterModeState:(char) character;
- (void)graphModeState:(char) character;
- (void)graphModeTypeState:(char) character;
- (void)ninePinGraphState:(char) character;
- (void)ninePinGraphTypeState:(char) character;
- (void)graphCnt1State:(char) character;
- (void)inGraphModeState:(char) character;
- (void)underModeState:(char) character;
- (void)vertTabChanState:(char) character;
- (void)superLineSpaceState:(char) character;
- (void)graphRedefState:(char) character;
- (void)grapRedefCodeState:(char) character;
- (void)lineSpaceState:(char) character;
- (void)vertTabSetState:(char) character;
- (void)formLengthLineState:(char) character;
- (void)formLengthInchState:(char) character;
- (void)horizTabSetState:(char) character;
- (void)printCtrlCodesState:(char) character;
- (void)immedLfState:(char) character;
- (void)skipOverPerfState:(char) character;
- (void)rightMarginState:(char) character;
- (void)interCharSetState:(char) character;
- (void)scriptMode:(char) character;
- (void)expandMode:(char) character;
- (void)vertTabChanSetState:(char) character;
- (void)vertTabChanNumState:(char) character;
- (void)immedModeState:(char) character;
- (void)immedRevLfState:(char) character;
- (void)leftMarginState:(char) character;
- (void)proportionalModeState:(char) character;
- (void)unimpEsc1State:(char) character;

-(void)resetHorizTabs;
-(void)setStyle;
-(void)emptyPrintBuffer:(bool)doubleStrike;
-(void)clearPrintBuffer;
-(void)executeLineFeed:(float)amount;


@end
