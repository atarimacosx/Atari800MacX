/* ByteTextField.m - 
 ByteTextField class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "ByteTextField.h"
#define maxChar 2

@implementation ByteTextField
- (id)init
{
	[super init];
	[self setStringValue:@""];
	[self setEditable:YES];
	[self setSelectable:YES];
	return self;
}

-(int) isValidCharacter:(char) c
{
	c = toupper(c);
		
	if( (c>='0' && c<='9') ||
		(c>='A' && c<='F') ||
		(c==' ') )
		return 1;
	return 0;
}

- (void) textDidChange:(NSNotification *) note
{
	NSText *text=(NSText *) [note object];
	NSString *hex= [text string];
	int curPos= [text selectedRange].location; // insert point position
		
	/* discard any character that it's not valid or exceeding the maximum length,
	   we need to test to the beginning of the string, because there's no easy way to
	   know how many characters has been inserted
	*/
	while( curPos-- > 0 )		
		{
		char c= [hex characterAtIndex:curPos];
		if( ([[text string] length] > maxChar) ||
			( [self isValidCharacter:c] == 0) )
			{
			NSRange r=NSMakeRange(curPos,1);
			[text replaceCharactersInRange:r withString:@""];
			}
		}
}

@end
