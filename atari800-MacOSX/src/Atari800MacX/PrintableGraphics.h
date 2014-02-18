/* PrintableGraphics.h - PrintableGraphics 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 
 */

#import <Cocoa/Cocoa.h>
#import "PrintProtocol.h"


@interface PrintableGraphics : NSData <PrintProtocol> {
	NSPoint printLocation;
	unsigned char *graphBytes;
	unsigned graphLength;
	float pixelWidth;
	float pixelHeight;
	unsigned columnBits;
}
- (id)initWithBytes:(const void *)bytes length:(unsigned)length width:(float)width height:(float) height bits:(unsigned)bits;
@end
