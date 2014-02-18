/* PrintablePath.m - 
 PrintablePath class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 */

#import "PrintablePath.h"

@implementation PrintablePath

-(void) setLocation:(NSPoint)location
{
}

-(void) setColor:(NSColor *)pathColor
{
	color = pathColor;
}

-(void)print:(NSRect)rect:(float)offset
{
	float origY, origSizeY;
	NSBezierPath *outputPath;
	static float lastOffset = 0.0;
	static NSAffineTransform *transform;

	if (offset == 0.0)
		outputPath = self;
	else
		{
		if (offset != lastOffset)
			{
			[transform release];
			transform = [NSAffineTransform transform];
			[transform retain];
			// Translate the glyph the apporpriate amount
			[transform translateXBy:0.0 yBy:offset];
			}
		outputPath = [transform transformBezierPath:self];
		}
		
	lastOffset = offset; 
		
	origY = [outputPath bounds].origin.y;
	origSizeY = origY + [outputPath bounds].size.height;
	
	if (!(origY > rect.origin.y + rect.size.height ||
	      origSizeY < rect.origin.y))
		  {
		  [color set];
		  [outputPath stroke];
		  }
}

-(float)getYLocation
{
	return ([self bounds].origin.y + [self bounds].size.height);
}

-(float)getMinYLocation
{
	return ([self bounds].origin.y);
}

@end