/* SectorEditorAsciiFieldCell.m - 
 SectorEditorAsciiFieldCell class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "SectorEditorAsciiFieldCell.h"
#define maxChar 16

@implementation SectorEditorAsciiFieldCell
- (id)init
{
	[super init];
	[self setStringValue:@""];
	[self setEditable:YES];
	[self setSelectable:YES];
	[self setScrollable:YES];
	return self;
}

-(int) isValidCharacter:(char) c
{
	if(c>=0x20 && c<=0x7E)
		return 1;
	return 0;
}

- (void) textDidChange:(NSNotification *) note
{
	NSText *text=(NSText *) [note object];
	NSString *ascii= [text string];
	NSRange r;
	char blankString[17];
	int length;
	NSRange curRange = [text selectedRange];
	int curPos= [text selectedRange].location; // insert point position
		
	/* discard any character that it's not valid */
	while( curPos-- > 0 )		
		{
		char c= [ascii characterAtIndex:curPos];
		if( [self isValidCharacter:c] == 0 )
			{
			r=NSMakeRange(curPos,1);
			[text replaceCharactersInRange:r withString:@""];
			}
		}

	length = [[text string] length];
	/* Add spaces until we are equal to maximum length */
	if (length < maxChar) {
		r=NSMakeRange(length,0);
		int i;
		
		for (i=0;i < maxChar-length;i++)
			blankString[i] = ' ';
		blankString[i] = 0;
		[text replaceCharactersInRange:r withString:[NSString stringWithCString:blankString encoding:NSASCIIStringEncoding]];
		[text setSelectedRange:curRange];
		}
	/* discard any character exceeding the maximum length	*/
	else if (length > maxChar) {
		r=NSMakeRange(maxChar,length-maxChar);
		[text replaceCharactersInRange:r withString:@""];
		}
}

@end
