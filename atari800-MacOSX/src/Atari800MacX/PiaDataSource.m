/* PiaDataSource.m - 
 PiaDataSource class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "PiaDataSource.h"
#import "ControlManager.h"
#import "pia.h"

@implementation PiaDataSource

static REG_S regs[] = 
{
	{"PACTL",&PIA_PACTL,(0xd300 + PIA_OFFSET_PACTL)},
	{"PBCTL",&PIA_PBCTL,(0xd300 + PIA_OFFSET_PBCTL)},
	{"PORTA",&PIA_PORTA,(0xd300 + PIA_OFFSET_PORTA)},
	{"PORTB",&PIA_PORTB,(0xd300 + PIA_OFFSET_PORTB)},
};

static int numRegs = sizeof(regs)/sizeof(REG_S);

-(id) init
{
	regDefs = regs;
	regCount = numRegs;
	
	return [super init];
}

@end
