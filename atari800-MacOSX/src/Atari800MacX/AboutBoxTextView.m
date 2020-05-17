/* AboutBoxTextView.m - 
   AboutBoxTextView view class for the
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
*/
#import "AboutBoxTextView.h"
#import "AboutBox.h"

@implementation AboutBoxTextView
/*------------------------------------------------------------------------------
*  mouseDown - This method notifies the AboutBox class of a mouse click, then
*    calls the normal text view mouseDown.
*-----------------------------------------------------------------------------*/
- (void)mouseDown:(NSEvent *)theEvent
{
	if ([theEvent clickCount] >= 2)
		[[AboutBox sharedInstance] doubleClicked];
	else
		[[AboutBox sharedInstance] clicked];
	[super mouseDown:theEvent];
}


@end
