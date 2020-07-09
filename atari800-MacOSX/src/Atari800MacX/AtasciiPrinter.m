/* Atari825Simulator.m - 
 Atari825Simulator class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "AtasciiPrinter.h"
#import "PrintOutputController.h"

// Other constants
#define ATASCII_MIN_CHAR_SIZE               7
#define ATARI825_LEFT_PRINT_EDGE            10.8

ATASCII_PREF prefsAtascii;

@implementation AtasciiPrinter
static AtasciiPrinter *sharedInstance = nil;

+ (AtasciiPrinter *)sharedInstance {
    return sharedInstance ? sharedInstance : [[self alloc] init];
}

- (id)init {
    int size;
    
    if (sharedInstance) {
		[self dealloc];
    } else {
        [super init];
	}
	
    sharedInstance = self;
	
    for (size = ATASCII_MIN_CHAR_SIZE; size <= 24; size++)
        styles[size - ATASCII_MIN_CHAR_SIZE] = [[NSFont fontWithName : @"AtariClassic-Regular" size : prefsAtascii.charSize] retain];

	printBuffer = [[PrintableString alloc] init];
	[printBuffer retain];
	
	[self reset];

    return sharedInstance;
}

- (void)printChar:(char) character
{
    unsigned short unicharacter = (unsigned short) ((unsigned char) character);

    unicharacter = unicharacter + 0xE000;

    if ((unsigned char) character == 0x9b)
        [self executeLineFeed];
    else
        [self addChar:unicharacter];
}

- (void)addChar:(unsigned short)unicharacter
{
	NSAttributedString *newString;
	float currRightMargin;
	int styleNum = style;
	
	newString = [NSAttributedString alloc];
    [newString initWithString:[NSString stringWithCharacters:&unicharacter
                                length:1]
           attributes:[NSDictionary dictionaryWithObjectsAndKeys:
                       styles[styleNum], NSFontAttributeName,
                       nil]];
        
    [printBuffer appendAttributedString:newString];
	
    //	Increment horizontal position by character width;
    nextHorizPosition += horizWidth;

	[newString release];
	
    currRightMargin = rightMargin;
				
	if (nextHorizPosition + horizWidth >= currRightMargin)
		{
			[self executeLineFeed:lineSpacing];            // move to next line
		}
			
}

-(void)setStyle
{
    style = prefsAtascii.charSize - ATASCII_MIN_CHAR_SIZE;
}

-(void)emptyPrintBuffer
{
	NSPoint location = NSMakePoint(startHorizPosition, vertPosition);
	if ([[printBuffer string] length] == 0)
		return;

	location = NSMakePoint(startHorizPosition, vertPosition);
		
	[printBuffer setLocation:location];
	[[PrintOutputController sharedInstance] addToPrintArray:printBuffer];
	[printBuffer release];
	printBuffer = [[PrintableString alloc] init];
	
	startHorizPosition = nextHorizPosition;
}

-(float)getVertPosition
{
	return(vertPosition);
}

-(float)getFormLength
{
	return(formLength);
}

-(void)executeLineFeed
{
	[self executeLineFeed:lineSpacing];
}

-(void)executeRevLineFeed
{
	[self executeLineFeed:-lineSpacing];
}

-(void)executePenChange
{
}

-(NSColor *)getPenColor
{
	return(nil);
}

-(void)executeLineFeed:(float) amount
{
	float posOnPage;
	int linesToSkip;
	
	[self emptyPrintBuffer];
	vertPosition += amount;
	if (vertPosition < 0.0)
		vertPosition = 0.0;
	[self setStyle];
	// Skip over the paper perferation.
	if (skipOverPerf)
		{
		posOnPage = vertPosition -  (((int) (vertPosition/formLength)) * formLength) + lineSpacing;
		if (splitPerfSkip == NO)
			linesToSkip = skipOverPerf;
		else
			linesToSkip = skipOverPerf / 2;
			
		if (posOnPage >= (formLength - (linesToSkip * lineSpacing)))
			[self executeFormFeed];
		}
    startHorizPosition = leftMargin;        //  move to left margin;
    nextHorizPosition = leftMargin;        //  move to left margin;
}

-(void)executeFormFeed
{
	vertPosition = (((int) (vertPosition/formLength)) + 1) * formLength;
	if (splitPerfSkip)
		vertPosition += (skipOverPerf / 2) * lineSpacing;
}

-(void)topBlankForm
{
	if (splitPerfSkip == NO)
		vertPosition = 0.0;
	else
		vertPosition = (skipOverPerf / 2) * lineSpacing;
	startHorizPosition = leftMargin;
	nextHorizPosition = leftMargin;
}

-(bool)isAutoPageAdjustEnabled
{
	return(NO);
}

- (void)reset {
    [self setStyle];
    rightMargin = 576.0 + ATARI825_LEFT_PRINT_EDGE;
    leftMargin = 0.0 + ATARI825_LEFT_PRINT_EDGE;
    startHorizPosition = leftMargin;
    nextHorizPosition = leftMargin;
    
    lineSpacing = prefsAtascii.charSize + prefsAtascii.lineGap;
    formLength = 72.0*prefsAtascii.formLength;
    horizWidth = prefsAtascii.charSize;
    skipOverPerf = 1;
    splitPerfSkip = YES;
    if (splitPerfSkip == NO)
        vertPosition = 0.0;
    else
        vertPosition = (skipOverPerf / 2) * lineSpacing;
}


@end

