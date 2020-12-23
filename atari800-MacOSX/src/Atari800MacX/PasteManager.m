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

static char escapedCopyBuffer[64*1024];

void PasteManagerEscapeCopy(char *string)
{
    int inv = 0;
    
    escapedCopyBuffer[0] = 0;

    unsigned char *c = (unsigned char *) string;
    while(*c) {
        if (*c == 0x9B) {
            strcat(escapedCopyBuffer,"\n");
            c++;
            continue;
        }
        
        if ((*c ^ inv) & 0x80) {
            inv ^= 0x80;

            strcat(escapedCopyBuffer,"{inv}");
        }

        *c &= 0x7F;

        if (*c == 0x00) {
            strcat(escapedCopyBuffer,"{^},");
        } else if (*c >= 0x01 && *c < 0x1B) {
            unsigned char cprime;
            strcat(escapedCopyBuffer,"{^}");
            cprime = (char)('a' + (*c - 0x01));
            strncat(escapedCopyBuffer,(char *)&cprime,1);
        } else if (*c == 0x1B) {
            strcat(escapedCopyBuffer,"{esc}{esc}");
        } else if (*c == 0x1C) {
            if (inv)
                strcat(escapedCopyBuffer,"{esc}{+delete}");
            else
                strcat(escapedCopyBuffer,"{esc}{up}");
        } else if (*c == 0x1D) {
            if (inv)
                strcat(escapedCopyBuffer,"{esc}{+insert}");
            else
                strcat(escapedCopyBuffer,"{esc}{down}");
        } else if (*c == 0x1E) {
            if (inv)
                strcat(escapedCopyBuffer,"{esc}{^tab}");
            else
                strcat(escapedCopyBuffer,"{esc}{left}");
        } else if (*c == 0x1F) {
            if (inv)
                strcat(escapedCopyBuffer,"{esc}{+tab}");
            else
                strcat(escapedCopyBuffer,"{esc}{right}");
        } else if (*c >= 0x20 && *c < 0x60) {
            strncat(escapedCopyBuffer,(char *)c,1);
        } else if (*c == 0x60) {
            strcat(escapedCopyBuffer,"{^}.");
        } else if (*c >= 0x61 && *c < 0x7B) {
            strncat(escapedCopyBuffer,(char *)c,1);
        } else if (*c == 0x7B) {
            strcat(escapedCopyBuffer,"{^};");
        } else if (*c == 0x7C) {
            strncat(escapedCopyBuffer,(char *)c,1);
        } else if (*c == 0x7D) {
            if (inv)
                strcat(escapedCopyBuffer,"{esc}{^}2");
            else
                strcat(escapedCopyBuffer,"{esc}{clear}");
        } else if (*c == 0x7E) {
            if (inv)
                strcat(escapedCopyBuffer,"{esc}{del}");
            else
                strcat(escapedCopyBuffer,"{esc}{back}");
        } else if (*c == 0x7F) {
            if (inv)
                strcat(escapedCopyBuffer,"{esc}{ins}");
            else
                strcat(escapedCopyBuffer,"{esc}{tab}");
        }
        c++;
    }

    if (inv)
        strcat(escapedCopyBuffer,"{inv}");

}

void PasteManagerStartCopy(char *string)
{
    PasteManagerEscapeCopy(string);
	[[PasteManager sharedInstance] startCopy:escapedCopyBuffer];
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
