/* PrinterView.h - PrinterView 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */

#import <AppKit/AppKit.h>
#import "PrintOutputController.h"


@interface PrinterView : NSView {
	PrintOutputController *controller;
	float pageLength;
	float vertPosition;
}
- (id)initWithFrame:(NSRect)frame:(PrintOutputController *)owner:(float)pageLen:(float)vert;
- (void)updateVerticlePosition:(float)vert;

@end
