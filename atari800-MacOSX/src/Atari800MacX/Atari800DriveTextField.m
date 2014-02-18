/* Atari800DriveTextField.m - Atari800DriveTextField 
   field class for the
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimac@kc.rr.com>
*/
#import "Atari800DriveTextField.h"
#import "Atari800ImageView.h"
#import "MediaManager.h"


@implementation Atari800DriveTextField

- (void)mouseDown:(NSEvent *)theEvent
{
	[[[MediaManager sharedInstance] getDiskImageView:[self tag]] mouseDown:theEvent];
}

@end
