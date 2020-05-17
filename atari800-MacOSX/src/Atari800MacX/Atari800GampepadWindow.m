/* Atari800GamepadWindow.m - 
 Atari800GamepadWindow class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import <Cocoa/Cocoa.h>


@interface Atari800GamepadWindow : NSWindow {
}

@end

@implementation Atari800GamepadWindow
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

- (BOOL)canBecomeKeyWindow
{
    return NO;
}

@end
