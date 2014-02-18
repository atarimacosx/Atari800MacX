/* PrinterProtocol.h - PrinterProtocol 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 
 */

#import <Cocoa/Cocoa.h>

@protocol PrinterProtocol
- (void)printChar:(char) character;
-(void)reset;
-(float)getVertPosition;
-(float)getFormLength;
-(NSColor *)getPenColor;
-(void)executeLineFeed;
-(void)executeRevLineFeed;
-(void)executeFormFeed;
-(void)executePenChange;
-(void)topBlankForm;
-(bool)isAutoPageAdjustEnabled;
@end
