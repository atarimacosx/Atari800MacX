/* PrinterSimulator.m - 
 PrinterSimulator class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */
#import "PrinterSimulator.h"

@implementation PrinterSimulator
- (void)printChar:(char) character
{
}

-(void)reset
{
}

-(float)getVertPosition
{
	return(0.0);
}

-(float)getFormLength
{
	return(0.0);
}

-(NSColor *)getPenColor
{
	return(nil);
}

-(bool)isAutoPageAdjustEnabled
{
	return(NO);
}

-(void)executeLineFeed
{
}

-(void)executeRevLineFeed
{
}

-(void)executeFormFeed
{
}

-(void)executePenChange
{
}

-(void)topBlankForm;
{
}


@end
