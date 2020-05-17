/* PrintablePath.h - PasteWindow 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */

#import <Cocoa/Cocoa.h>
#import "PrintProtocol.h"

@interface PrintablePath : NSBezierPath <PrintProtocol> {
   NSColor *color;
}

-(void) setColor:(NSColor *)pathColor;

@end
