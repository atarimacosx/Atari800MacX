/* PokeyDataSource.m - 
 PokeyDataSource class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 */

#import "PokeyDataSource.h"
#import "ControlManager.h"
#import "pokey.h"
#import "pokeysnd.h"

@implementation PokeyDataSource
	
static REG_S regs[] = 
{
	{"AUDF1",&POKEY_AUDF[POKEY_CHAN1],(0xd200 + POKEY_OFFSET_AUDF1)},
	{"AUDF2",&POKEY_AUDF[POKEY_CHAN2],(0xd200 + POKEY_OFFSET_AUDF2)},
	{"AUDF3",&POKEY_AUDF[POKEY_CHAN3],(0xd200 + POKEY_OFFSET_AUDF3)},
	{"AUDF4",&POKEY_AUDF[POKEY_CHAN4],(0xd200 + POKEY_OFFSET_AUDF4)},
	{"AUDCTL",&POKEY_AUDCTL[0],(0xd200 + POKEY_OFFSET_AUDCTL)},
	{"KBCODE",&POKEY_KBCODE,(0xd200 + POKEY_OFFSET_KBCODE)},
	{"AUDC1",&POKEY_AUDC[POKEY_CHAN1],(0xd200 + POKEY_OFFSET_AUDC1)},
	{"AUDC2",&POKEY_AUDC[POKEY_CHAN2],(0xd200 + POKEY_OFFSET_AUDC2)},
	{"AUDC3",&POKEY_AUDC[POKEY_CHAN3],(0xd200 + POKEY_OFFSET_AUDC3)},
	{"AUDC4",&POKEY_AUDC[POKEY_CHAN4],(0xd200 + POKEY_OFFSET_AUDC4)},
	{"IRQEN",&POKEY_IRQEN,(0xd200 + POKEY_OFFSET_IRQEN)},
	{"IRQST",&POKEY_IRQST,(0xd200 + POKEY_OFFSET_IRQST)},
	{"SKSTAT",&POKEY_SKSTAT,(0xd200 + POKEY_OFFSET_SKSTAT)},
	{"SKCTL",&POKEY_SKCTL,(0xd200 + POKEY_OFFSET_SKCTL)},
	{"2nd AUDF1",&POKEY_AUDF[POKEY_CHAN1 + POKEY_CHIP2],(0xd200 + POKEY_OFFSET_AUDF1 + POKEY_OFFSET_POKEY2)},
	{"2nd AUDF2",&POKEY_AUDF[POKEY_CHAN2 + POKEY_CHIP2],(0xd200 + POKEY_OFFSET_AUDF2 + POKEY_OFFSET_POKEY2)},
	{"2nd AUDF3",&POKEY_AUDF[POKEY_CHAN3 + POKEY_CHIP2],(0xd200 + POKEY_OFFSET_AUDF3 + POKEY_OFFSET_POKEY2)},
	{"2nd AUDF4",&POKEY_AUDF[POKEY_CHAN4 + POKEY_CHIP2],(0xd200 + POKEY_OFFSET_AUDF4 + POKEY_OFFSET_POKEY2)},
	{"2nd AUDCTL",&POKEY_AUDCTL[1],(0xd200 + POKEY_OFFSET_AUDCTL + POKEY_OFFSET_POKEY2)},
	{"2nd AUDC1",&POKEY_AUDC[POKEY_CHAN1 + POKEY_CHIP2],(0xd200 + POKEY_OFFSET_AUDC1 + POKEY_OFFSET_POKEY2)},
	{"2nd AUDC2",&POKEY_AUDC[POKEY_CHAN2 + POKEY_CHIP2],(0xd200 + POKEY_OFFSET_AUDC2 + POKEY_OFFSET_POKEY2)},
	{"2nd AUDC3",&POKEY_AUDC[POKEY_CHAN3 + POKEY_CHIP2],(0xd200 + POKEY_OFFSET_AUDC3 + POKEY_OFFSET_POKEY2)},
	{"2nd AUDC4",&POKEY_AUDC[POKEY_CHAN4 + POKEY_CHIP2],(0xd200 + POKEY_OFFSET_AUDC4 + POKEY_OFFSET_POKEY2)},
};

#define NUM_2ND_REGS 9

static int numRegs = sizeof(regs)/sizeof(REG_S);

-(id) init
{
	regDefs = regs;
	regCount = numRegs;
	
	return [super init];
}

/*------------------------------------------------------------------------------
 *  numberOfRowsInTableView - Return the number of rows in the table.
 *-----------------------------------------------------------------------------*/
-(int) numberOfRowsInTableView:(NSTableView *)aTableView
{
	if (POKEYSND_stereo_enabled)
		return(regCount);
	else
		return(regCount - NUM_2ND_REGS);
}

@end
