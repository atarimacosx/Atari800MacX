/* Atari825Simulator.m - 
 Atari825Simulator class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "AtasciiPrinter.h"
#import "PrintOutputController.h"

static float horizWidths[8] = {6,16};

// Other constants
#define ATARI825_LEFT_PRINT_EDGE            10.8

@implementation AtasciiPrinter
static AtasciiPrinter *sharedInstance = nil;

+ (AtasciiPrinter *)sharedInstance {
    return sharedInstance ? sharedInstance : [[self alloc] init];
}

- (id)init {
    if (sharedInstance) {
		[self dealloc];
    } else {
        [super init];
	}
	
    sharedInstance = self;
	
    styles[0] = [[NSFont fontWithName : @"AtariClassic-Regular" size : 7] retain];

    styles[1] = [[NSFont fontWithName : @"AtariClassic-Regular" size : 16] retain];
						
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
				
	if (nextHorizPosition >= currRightMargin) 
		{
			[self executeLineFeed:lineSpacing];            // move to next line
		}
			
}

-(void)setStyle
{
    style = 0;
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
	expandedMode = NO;
	[self setStyle];
	// Skip over the paper perferation.
	if (skipOverPerf)
		{
		posOnPage = vertPosition -  (((int) (vertPosition/formLength)) * formLength);
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
    style = 0;
    rightMargin = 576.0 + ATARI825_LEFT_PRINT_EDGE;
    leftMargin = 0.0 + ATARI825_LEFT_PRINT_EDGE;
    startHorizPosition = leftMargin;
    nextHorizPosition = leftMargin;
    
    lineSpacing = 12.0;
    formLength = 72.0*12.0; //prefs825.formLength;
    expandedMode = NO;
    compressedMode = NO;
    underlineMode = NO;
    proportionalMode = NO;
    horizWidth = horizWidths[style];
    skipOverPerf = 0;
    splitPerfSkip = NO;
    if (splitPerfSkip == NO)
        vertPosition = 0.0;
    else
        vertPosition = (skipOverPerf / 2) * lineSpacing;
}


@end

