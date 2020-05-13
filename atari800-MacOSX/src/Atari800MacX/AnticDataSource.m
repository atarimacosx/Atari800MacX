/* AnticDataSource.m - 
 AnticDataSource class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "AnticDataSource.h"
#import "ControlManager.h"
#import "antic.h"

@implementation AnticDataSource

static REG_S regs[] = 
{
	{"DMACTL",&ANTIC_DMACTL,(0xd400 + ANTIC_OFFSET_DMACTL)},
	{"CHACTL",&ANTIC_CHACTL,(0xd400 + ANTIC_OFFSET_CHACTL)},
#ifdef WORDS_BIGENDIAN
	{"DLISTL",((unsigned char *) &ANTIC_dlist) + 1,(0xd400 + ANTIC_OFFSET_DLISTL)},
	{"DLISTH",(unsigned char *) &ANTIC_dlist,(0xd400 + ANTIC_OFFSET_DLISTH)},
#else
	{"DLISTL",(unsigned char *) &ANTIC_dlist,(0xd400 + ANTIC_OFFSET_DLISTL)},
	{"DLISTH",((unsigned char *) &ANTIC_dlist) + 1,(0xd400 + ANTIC_OFFSET_DLISTH)},
#endif
	{"HSCROL",&ANTIC_HSCROL,(0xd400 + ANTIC_OFFSET_HSCROL)},
	{"VSCROL",&ANTIC_VSCROL,(0xd400 + ANTIC_OFFSET_VSCROL)},
	{"PMBASE",&ANTIC_PMBASE,(0xd400 + ANTIC_OFFSET_PMBASE)},
	{"CHBASE",&ANTIC_CHBASE,(0xd400 + ANTIC_OFFSET_CHBASE)},
	{"VCOUNT",(unsigned char *)&ANTIC_ypos,0},
	{"NMIEN",&ANTIC_NMIEN,(0xd400 + ANTIC_OFFSET_NMIEN)},
	{"ypos",(unsigned char *)&ANTIC_ypos,0},
};

static int numRegs = sizeof(regs)/sizeof(REG_S);

-(id) init
{
	regDefs = regs;
	regCount = numRegs;
	
	return [super init];
}

/*------------------------------------------------------------------------------
 *  objectValueForTableColumn - Get the value to display in a cell.
 *-----------------------------------------------------------------------------*/
-(id) tableView:(NSTableView *) aTableView 
objectValueForTableColumn:(NSTableColumn *)aTableColumn
			row:(int) rowIndex
{
	static int oldYpos;
	static int callCount = 0;
	NSString *theString;
	NSAttributedString *theAttributeString;
	NSDictionary *theDict;
	
	if ([[aTableColumn identifier] isEqual:@"N"]) {
		return([super tableView:aTableView objectValueForTableColumn:aTableColumn row:rowIndex]);
    } else if (strcmp(regDefs[rowIndex].name,"VCOUNT") == 0) {
		theString = [NSString stringWithFormat:@"%02X",ANTIC_ypos >> 1];
		
		if (oldYpos == ANTIC_ypos) {
			theDict = blackDict;
		} else {
			theDict = redDict;
		}
		
		theAttributeString = [[NSAttributedString alloc] initWithString:theString attributes:theDict];
		[theAttributeString autorelease];
		
		if (callCount % 2 == 1) {
			oldYpos = ANTIC_ypos;
		}
		callCount++;
		return(theAttributeString);
    } else if (strcmp(regDefs[rowIndex].name,"ypos") == 0) {
		theString = [NSString stringWithFormat:@"%03X",ANTIC_ypos];

		if (oldYpos == ANTIC_ypos) {
			theDict = blackDict;
		} else {
			theDict = redDict;
		}
		
		theAttributeString = [[NSAttributedString alloc] initWithString:theString attributes:theDict];
		[theAttributeString autorelease];
		
		if (callCount % 2 == 1) {
			oldYpos = ANTIC_ypos;
		}
		callCount++;
		return(theAttributeString);
    } else  {
		return([super tableView:aTableView objectValueForTableColumn:aTableColumn row:rowIndex]);
	} 
}

@end
