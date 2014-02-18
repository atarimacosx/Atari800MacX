/* Atari800FunctionKeysWindow.m - 
 Atari800FunctionKeysWindow class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 */

#import "Atari800FunctionKeysWindow.h"

extern int functionKeysWindowOpen;

@implementation Atari800FunctionKeysWindow
/*------------------------------------------------------------------------------
*  init -
*-----------------------------------------------------------------------------*/
-(id) init
{
	id me;
	
	me = [super init];
	
	return(me);
}

/*------------------------------------------------------------------------------
*  init -
*-----------------------------------------------------------------------------*/
- (BOOL)windowShouldClose:(id)sender
{
	functionKeysWindowOpen = FALSE;
	return YES;
}

@end
