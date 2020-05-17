/* Atari825Simulator.m - 
 Atari825Simulator class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "Atari825Simulator.h"
#import "PrintOutputController.h"

// States of the simulated printer state machine
#define ATARI825_IDLE						0
#define ATARI825_GOT_ESC					1
#define ATARI825_GOT_BS						2

// Print Styles

#define STYLE_EXPANDED_BIT								8
#define STYLE_BASE_MASK									3

#define STYLE_PICA										0
#define STYLE_COMPRESSED								1
#define STYLE_PROPORTIONAL								2
#define STYLE_EXPANDED_PICA								(STYLE_PICA | STYLE_EXPANDED_BIT)
#define STYLE_EXPANDED_COMPRESSED						(STYLE_COMPRESSED | STYLE_EXPANDED_BIT)
#define STYLE_EXPANDED_PROPORTIONAL						(STYLE_PROPORTIONAL | STYLE_EXPANDED_BIT)

#define PITCH_PICA										0
#define PITCH_COMPRESSED								1
#define PITCH_PROPORTIONAL								2

#define MODE_US											0
#define MODE_FRANCE										1
#define MODE_UK											2
#define MODE_GERMANY									3
#define MODE_ITALY										4
#define MODE_SWEDEN										5

static float horizWidths[8] = {7.2,72.0/16.7,7.2,7.2,14.4,144.0/16.7,14.4,14.4};

static int fontGroup[256] = 
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,2,4,2,0,2,2,0,2,0,0,0,0,
 2,2,2,2,2,2,2,2,2,2,0,0,2,2,2,2,
 5,1,1,3,2,2,1,2,2,0,0,1,1,2,2,2,
 2,3,1,4,1,2,1,3,1,1,4,0,0,0,1,2,
 2,2,2,3,2,2,0,2,2,0,1,1,0,3,2,2,
 2,2,0,1,0,2,1,2,1,1,3,0,0,0,2,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,3,3,0,0,5,3,0,0,0,0,0,0,0,
 2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,3,0,3,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,3,0,0,0,0,0,3,0,0,4,
 4,0,0,0,4,0,0,4,4,4,0,0,0,0,0,0,
 0,0,4,0,0,0,4,0,0,4,0,0,4,0,0,0};

static float propWidths[256] = 
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 3.36,3.36,4.8,7.2,5.76,7.68,6.72,3.36,3.36,3.36,5.76,5.76,3.36,5.76,3.36,5.76,
 5.76,5.76,5.76,5.76,5.76,5.76,5.76,5.76,5.76,5.76,3.36,3.36,5.76,5.76,5.76,5.76,
 6.72,7.68,7.2,6.72,7.68,6.72,6.72,7.68,7.68,4.8,6.72,7.68,6.72,8.64,7.68,7.68,
 6.72,6.72,7.2,5.76,6.72,7.68,7.68,8.64,7.68,7.68,4.8,5.76,5.76,5.76,5.76,5.76,
 3.36,5.76,5.76,4.8,5.76,5.76,4.8,5.76,5.76,3.84,2.88,5.76,3.84,7.68,5.76,5.76,
 5.76,5.76,4.8,5.76,4.8,5.76,5.76,7.68,5.76,5.76,4.8,4.8,3.36,4.8,5.76,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,7.2,5.76,0,0,5.76,5.76,0,0,0,0,0,0,0,
 6.72,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,7.68,7.68,0,0,0,6.72,0,0,0,0,0,0,
 0,0,0,0,0,0,7.68,0,0,0,0,0,7.68,0,0,5.76,
 5.76,0,0,0,5.76,0,0,4.8,5.76,5.76,0,0,3.85,0,0,0,
 0,0,5.76,0,0,0,5.76,0,0,5.76,0,0,5.76,0,0,0};

static unsigned short countryMap[6][128] = {
	// US
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 
	 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f},
	// France
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	 0x20, 0x21, 0x22, 0xa3, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	 0xe0, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0xb0, 0xe7, 0xa7, 0x5e, 0x5f, 
	 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0xe9, 0xf9, 0xe8, 0xa8, 0x7f}, 
	// UK
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	 0x20, 0x21, 0x22, 0xa3, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 
	 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f}, 
	// Germany
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	 0xa7, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0xc5, 0xd6, 0xdc, 0x5e, 0x5f, 
	 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0xe4, 0xf6, 0xfc, 0xdf, 0x7f}, 
	// Italy
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	 0x20, 0x21, 0x22, 0xa3, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	 0xa7, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0xb0, 0xe9, 0x7c, 0x5e, 0x5f, 
	 0xf9, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0xe0, 0xf2, 0xe8, 0xec, 0x7f}, 
	// Sweeden/Finland
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	 0x20, 0x21, 0x22, 0x23, 0xa4, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	 0xc9, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0xc4, 0xd6, 0xc5, 0xdc, 0x5f, 
	 0xe8, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0xe4, 0xf6, 0xe0, 0xfc, 0x7f}, 
};

ATARI825_PREF prefs825;

// Other constants
#define ATARI825_LEFT_PRINT_EDGE			10.8

@implementation Atari825Simulator
static Atari825Simulator *sharedInstance = nil;

+ (Atari825Simulator *)sharedInstance {
    return sharedInstance ? sharedInstance : [[self alloc] init];
}

- (id)init {
	double aFontMatrix [6];
	
    if (sharedInstance) {
		[self dealloc];
    } else {
        [super init];
	}
	
    sharedInstance = self;
	
	aFontMatrix [1] = aFontMatrix [2] = aFontMatrix [4] = aFontMatrix [5] = 0.0;  // Always
	aFontMatrix [3] = 12.0;
	
	aFontMatrix [0] = 12.0;
    styles[STYLE_PICA] = [[NSFont fontWithName : @"Courier" size : 12.0] retain];
	
	aFontMatrix [0] = 80.0/11.0;
	styles[STYLE_COMPRESSED] = [[NSFont fontWithName : @"Courier" matrix : aFontMatrix] retain];

	aFontMatrix [0] = 12.0;	
	styles[STYLE_PROPORTIONAL+0] = [[NSFont fontWithName : @"Helvetica" matrix : aFontMatrix] retain];
	
	aFontMatrix [0] = 9.95;	
	styles[STYLE_PROPORTIONAL+1] = [[NSFont fontWithName : @"Helvetica" matrix : aFontMatrix] retain];
	
	aFontMatrix [0] = 9.85;	
	styles[STYLE_PROPORTIONAL+2] = [[NSFont fontWithName : @"Helvetica" matrix : aFontMatrix] retain];
	
	aFontMatrix [0] = 8.63;	
	styles[STYLE_PROPORTIONAL+3] = [[NSFont fontWithName : @"Helvetica" matrix : aFontMatrix] retain];
	
	aFontMatrix [0] = 8.55;	
	styles[STYLE_PROPORTIONAL+4] = [[NSFont fontWithName : @"Helvetica" matrix : aFontMatrix] retain];
	
	aFontMatrix [0] = 6.55;	
	styles[STYLE_PROPORTIONAL+5] = [[NSFont fontWithName : @"Helvetica" matrix : aFontMatrix] retain];
	
	aFontMatrix [0] = 24.0;
	styles[STYLE_EXPANDED_PICA] = [[NSFont fontWithName : @"Courier" matrix : aFontMatrix] retain];
	
	aFontMatrix [0] = 160.0/11.0;
	styles[STYLE_EXPANDED_COMPRESSED] = [[NSFont fontWithName : @"Courier" matrix : aFontMatrix] retain];
	
	aFontMatrix [0] = 24.0;	
	styles[STYLE_EXPANDED_PROPORTIONAL] = [[NSFont fontWithName : @"Helvetica" matrix : aFontMatrix] retain];
			
	aFontMatrix [0] = 2*9.95;	
	styles[STYLE_EXPANDED_PROPORTIONAL+1] = [[NSFont fontWithName : @"Helvetica" matrix : aFontMatrix] retain];
			
	aFontMatrix [0] = 2*9.85;	
	styles[STYLE_EXPANDED_PROPORTIONAL+2] = [[NSFont fontWithName : @"Helvetica" matrix : aFontMatrix] retain];
			
	aFontMatrix [0] = 2*8.63;	
	styles[STYLE_EXPANDED_PROPORTIONAL+3] = [[NSFont fontWithName : @"Helvetica" matrix : aFontMatrix] retain];
			
	aFontMatrix [0] = 2*8.55;	
	styles[STYLE_EXPANDED_PROPORTIONAL+4] = [[NSFont fontWithName : @"Helvetica" matrix : aFontMatrix] retain];
			
	aFontMatrix [0] = 2*6.55;	
	styles[STYLE_EXPANDED_PROPORTIONAL+5] = [[NSFont fontWithName : @"Helvetica" matrix : aFontMatrix] retain];
			
	printBuffer = [[PrintableString alloc] init];
	[printBuffer retain];
	
	[self reset];

    return sharedInstance;
}

- (void)printChar:(char) character
{
	switch (state)
	{
		case ATARI825_IDLE:
			[self idleState:character];
			break;
		case ATARI825_GOT_ESC:
			[self escState:character];
			break;
		case ATARI825_GOT_BS:
			[self bsState:character];
			break;
	}
}

- (void)addChar:(unsigned short)unicharacter
{
	NSAttributedString *newString;
	float currRightMargin;
	int styleNum = style;
	
	if ((style & STYLE_BASE_MASK) == STYLE_PROPORTIONAL)
		styleNum += fontGroup[unicharacter];

	newString = [NSAttributedString alloc];
	if (underlineMode)
		{
		[newString initWithString:[NSString stringWithCharacters:&unicharacter 
									length:1]
					attributes:[NSDictionary dictionaryWithObjectsAndKeys:
							   styles[styleNum], NSFontAttributeName,
							   [NSNumber numberWithInt:1],NSUnderlineStyleAttributeName,
							   nil]];
		}
	else
		{
		[newString initWithString:[NSString stringWithCharacters:&unicharacter 
									length:1]
			   attributes:[NSDictionary dictionaryWithObjectsAndKeys:
						   styles[styleNum], NSFontAttributeName,
						   nil]];
		}

	[printBuffer appendAttributedString:newString];
	//	Increment horizontal position by character width;
	if ((style & STYLE_BASE_MASK) == STYLE_PROPORTIONAL)
		{
		startHorizPosition += (propWidths[unicharacter] - [newString size].width)/2;
		if (style & STYLE_EXPANDED_BIT)
			nextHorizPosition += 2*propWidths[unicharacter];
		else
			nextHorizPosition += propWidths[unicharacter];
		}
	else
		nextHorizPosition += horizWidth;

	[newString release];
	
	if (pitch == PITCH_PICA)  
		currRightMargin = rightMargin;
	else
		currRightMargin = compressedRightMargin;
				
	if (nextHorizPosition >= currRightMargin) 
		{
			[self executeLineFeed:lineSpacing];            // move to next line
			startHorizPosition = leftMargin;		//  move to left margin;
			nextHorizPosition = leftMargin;		//  move to left margin;
		}
			
}

- (void)idleState:(char) character
{
	unsigned short unicharacter = (unsigned short) ((unsigned char) character);

	unicharacter &= 0x7F;
	
	if ((unicharacter >= 0x20) &&
		 (unicharacter <= 0x7E))
		{
		if (countryMode != MODE_US)
			{
			unicharacter = countryMap[countryMode][unicharacter];
			}
	
		[self addChar:unicharacter];
		if ((style & STYLE_BASE_MASK) == STYLE_PROPORTIONAL)
			[self emptyPrintBuffer];
		}
	else 
		{
		switch(unicharacter)
		{
			case 0x08: // BS
				state = ATARI825_GOT_BS;
				break;
			case 0x0A: // LF
			case 0x8A:
				[self executeLineFeed:lineSpacing];            // move to next line
				break;
			case 0x0D: // CR
			case 0x8D:
				[self emptyPrintBuffer];
				if (autoLineFeed == YES)
					{
					[self executeLineFeed:lineSpacing];            // move to next line
					}
				startHorizPosition = leftMargin;		//  move to left margin;
				nextHorizPosition = leftMargin;		//  move to left margin;
				break;
			case 0x0E: // SO
			case 0x8E:
				underlineMode = YES;
				break;
			case 0x0F: // SI
				underlineMode = NO;
				break;
			case 0x1B: // ESC
				state = ATARI825_GOT_ESC;
				break;
			default:
				break;
			}
		}
		
}
- (void)escState:(char) character
{
	float currRightMargin;
	
	switch(character)
		{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			[self emptyPrintBuffer];
			startHorizPosition += character * (72.0/150.0);
			nextHorizPosition = startHorizPosition;
			
			if (pitch == PITCH_PICA)  
				currRightMargin = rightMargin;
			else
				currRightMargin = compressedRightMargin;
				
			if (nextHorizPosition >= currRightMargin) 
				{
				[self executeLineFeed:lineSpacing];            // move to next line
				startHorizPosition = leftMargin;		//  move to left margin;
				nextHorizPosition = leftMargin;		//  move to left margin;
			}
			break;
		case 10:
			[self executeLineFeed:-lineSpacing];            
			break;
		case 14:
			[self emptyPrintBuffer];
			expandedMode = YES;
			[self setStyle];
			break;
		case 15:
			[self emptyPrintBuffer];
			expandedMode = NO;
			[self setStyle];
			break;
		case 17:
			[self emptyPrintBuffer];
			proportionalMode = YES;
			compressedMode = NO;
			[self setStyle];
			break;
		case 19:
			[self emptyPrintBuffer];
			proportionalMode = NO;
			compressedMode = NO;
			[self setStyle];
			break;
		case 20:
			[self emptyPrintBuffer];
			proportionalMode = NO;
			compressedMode = YES;
			[self setStyle];
			break;
		case 28:
			[self executeLineFeed:(lineSpacing/2)];            
			break;
		case 30:
			[self executeLineFeed:-(lineSpacing/2)];            
			break;
		}
	state = ATARI825_IDLE;
}

- (void)bsState:(char) character
{
	[self emptyPrintBuffer];
	startHorizPosition -= character * (72.0/150.0);
	if (startHorizPosition < leftMargin)
		startHorizPosition = leftMargin;	
	nextHorizPosition = startHorizPosition;
	state = ATARI825_IDLE;
}

-(void)reset
{
	state = ATARI825_IDLE;
	style = STYLE_PICA;
	pitch = PITCH_PICA;
	rightMargin = 576.0 + ATARI825_LEFT_PRINT_EDGE;
	compressedRightMargin = 569.28 + ATARI825_LEFT_PRINT_EDGE;
	leftMargin = 0.0 + ATARI825_LEFT_PRINT_EDGE;
	startHorizPosition = leftMargin;
	nextHorizPosition = leftMargin;
	
	if (prefs825.autoLinefeed)
		autoLineFeed = YES;
	else
		autoLineFeed = NO;
	lineSpacing = 12.0;
	formLength = 72.0*prefs825.formLength;
	expandedMode = NO;
	compressedMode = NO;
	underlineMode = NO;
	proportionalMode = NO;
	horizWidth = horizWidths[style];
	countryMode = prefs825.charSet;
	skipOverPerf = 0;
	splitPerfSkip = NO;
	if (splitPerfSkip == NO)
		vertPosition = 0.0;
	else
		vertPosition = (skipOverPerf / 2) * lineSpacing;
		
}

-(void)setStyle
{
	if (proportionalMode == YES)
		{
		style = STYLE_PROPORTIONAL;
		pitch = PITCH_PICA;
		}
	else if (compressedMode == YES)
		{
		style = STYLE_COMPRESSED;
		pitch = PITCH_COMPRESSED;
		}
	else
		{
		style = STYLE_PICA;
		pitch = PITCH_PROPORTIONAL;
		}

	if (expandedMode) 
		style |= STYLE_EXPANDED_BIT;
			
	horizWidth = horizWidths[style];
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

@end

