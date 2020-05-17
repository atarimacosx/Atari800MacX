/* PrintableString.m - 
 PrintableString class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "PrintableString.h"

@implementation PrintableString
-(id)init
{
	return [self initWithAttributedSting:nil];
}

-(id)initWithAttributedSting:(NSAttributedString *)attributedString
{
	if (self = [super init]) {
		_contents = attributedString ? [attributedString mutableCopy] :
					[[NSMutableAttributedString alloc] init];
		}
	return self;
}

-(NSString *)string
{
	return [_contents string];
}

-(NSDictionary *)attributesAtIndex:(unsigned)location
				  effectiveRange:(NSRange *)range
{
	return [_contents attributesAtIndex:location effectiveRange:range];
}
				  
-(void)replaceCharactersInRange:(NSRange)range
				  withString:(NSString *)string
{
	[_contents replaceCharactersInRange:range withString:string];
}
				  
-(void)setAttributes:(NSDictionary *)attributes
				  range:(NSRange)range
{
	[_contents setAttributes:attributes range:range];
}
				  
-(void)dealloc
{
	[_contents release];
	[super dealloc];
}

-(void) setLocation:(NSPoint)location
{
   printLocation = location;
}

-(void)print:(NSRect)rect:(float)offset
{
	if ((printLocation.y >= rect.origin.y-12.0) &&
		(printLocation.y <= (rect.origin.y + rect.size.height + 12.0)))
			[self drawAtPoint:printLocation];
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
