/* PrintProtocol.h - PrintProtocol 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */

@protocol PrintProtocol
-(void)setLocation:(NSPoint)location;
-(float)getYLocation;
-(float)getMinYLocation;
-(void)print:(NSRect)rect:(float)offset;
@end
