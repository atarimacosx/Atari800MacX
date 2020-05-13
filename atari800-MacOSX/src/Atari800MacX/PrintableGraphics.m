/* PrintableGraphics.m - 
 PrintableGraphics class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "PrintableGraphics.h"

@implementation PrintableGraphics

- (id)initWithBytes:(const void *)bytes length:(unsigned)length width:(float)width height:(float) height bits:(unsigned)bits
{
	graphLength = length;
	graphBytes = (unsigned char *) NSZoneMalloc(NSDefaultMallocZone(), length);
	bcopy(bytes, graphBytes, length);
	pixelWidth = width;
	pixelHeight = height;
	columnBits = bits;
	return(self);
}

- (void)dealloc
{
	NSZoneFree(NSDefaultMallocZone(), graphBytes);
	[super dealloc];
}

-(void) setLocation:(NSPoint)location
{
   printLocation = location;
}

-(void)print:(NSRect)rect:(float)offset
{
	NSRect r;
	unsigned length = graphLength;
	unsigned i,j;
	unsigned char *bytes = (unsigned char *) graphBytes;
	static unsigned char mask[8] = {128,64,32,16,8,4,2,1};
	
	if ((printLocation.y < rect.origin.y-12.0) ||
		(printLocation.y > (rect.origin.y + rect.size.height +12.0)))
		return;
	
	r.origin.x = printLocation.x;
	r.size.width = pixelWidth;
	r.size.height = pixelHeight;
    NSColor *color = [NSColor blackColor];
	[color set];
	
	if (columnBits>8)
		length /= 2;

	for (i=0;i<length;i++)
		{
		r.origin.y = printLocation.y;
		for (j=0;j<8;j++)
			{
			if (*bytes & mask[j])
				NSRectFill(r);
			r.origin.y += pixelHeight;
			}
		if (columnBits > 8)
			{
			bytes++;
			if (*bytes & 128)
				NSRectFill(r);
			}
		r.origin.x += pixelWidth;
		bytes++;
		}
}

-(float)getYLocation
{
	return printLocation.y;
}

-(float)getMinYLocation
{
	return printLocation.y;
}

@end
