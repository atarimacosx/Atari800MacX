/* PasteManager.m - 
 PasteManager class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "PasteManager.h"

int PasteManagerStartPaste(void)
{
	return([[PasteManager sharedInstance] startPaste]);
}

int PasteManagerGetChar(unsigned short *character)
{
	return([[PasteManager sharedInstance] getChar:character]);
}

void PasteManagerStartCopy(char *string)
{
	[[PasteManager sharedInstance] startCopy:string];
}

@implementation PasteManager
static PasteManager *sharedInstance = nil;

+ (PasteManager *)sharedInstance {
    return sharedInstance ? sharedInstance : [[self alloc] init];
}

- (id)init {	
    if (sharedInstance) {
		[self dealloc];
    } else {
        [super init];
        sharedInstance = self;
		charCount = 0;
    }
    return sharedInstance;
}

- (int)getChar:(unsigned short *) character
{
	if (charCount) {
		*character = [pasteString characterAtIndex:([pasteString length] - charCount)];
		charCount--;
		if (charCount)
			return(TRUE);
		else {
			[pasteString release];
			return(FALSE);
			}
		}
	else {
		return(FALSE);
		}
}

- (int)startPaste {
	NSPasteboard *pb = [NSPasteboard generalPasteboard];
	NSArray *pasteTypes = [NSArray arrayWithObjects: NSStringPboardType, nil];
	NSString *bestType = [pb availableTypeFromArray:pasteTypes];

	if (bestType != nil) {
		pasteString = [pb stringForType:bestType];
		charCount = [pasteString length];
		[pasteString retain];
		return TRUE;
		}
	else {
		pasteString = nil;
		charCount = 0;
		return FALSE;
		}
	}

- (void)startCopy:(char *)string
{
	NSPasteboard *pb = [NSPasteboard generalPasteboard];
	NSArray *types = [NSArray arrayWithObjects:
					  NSStringPboardType, nil];
	[pb declareTypes:types owner:self];
	[pb setString:[NSString stringWithCString:string encoding:NSASCIIStringEncoding] forType:NSStringPboardType];
}

@end
