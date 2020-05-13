/* PrintableString.h - PrintableString 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */

#import <Cocoa/Cocoa.h>
#import "PrintProtocol.h"

@interface PrintableString : NSMutableAttributedString <PrintProtocol> {
	NSMutableAttributedString * _contents;
	NSPoint printLocation;
}
-(id)init;
-(id)initWithAttributedSting:(NSAttributedString *)attributedString;
-(NSString *)string;
-(NSDictionary *)attributesAtIndex:(unsigned)location
				  effectiveRange:(NSRange *)range;
-(void)replaceCharactersInRange:(NSRange)range
				  withString:(NSString *)string;
-(void)setAttributes:(NSDictionary *)attributes
				  range:(NSRange)range;
-(void)dealloc;

@end
