/* PasteManager.m - 
 PasteManager class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "PasteManager.h"
#import "MacTypes.h"
#import "akey.h"

int PasteManagerStartPaste(void)
{
	return([[PasteManager sharedInstance] startPaste]);
}

int PasteManagerGetScancode(unsigned char *code)
{
    return([[PasteManager sharedInstance] getScancode:code]);
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
        parseDict = @{
            @"tab" : [NSNumber numberWithInt:0x2C],
            @"back" : [NSNumber numberWithInt:0x34],
            @"backspace" : [NSNumber numberWithInt:0x34],
            @"bksp" : [NSNumber numberWithInt:0x34],
            @"enter" : [NSNumber numberWithInt:0x0C],
            @"return" : [NSNumber numberWithInt:0x0C],
            @"esc" : [NSNumber numberWithInt:0x1C],
            @"escape" : [NSNumber numberWithInt:0x1C],
            @"fuji" : [NSNumber numberWithInt:0x27],
            @"inv" : [NSNumber numberWithInt:0x27],
            @"invert" : [NSNumber numberWithInt:0x27],
            @"help" : [NSNumber numberWithInt:0x11],
            @"clear" : [NSNumber numberWithInt:0x76],
            @"del" : [NSNumber numberWithInt:0xB4],
            @"delete" : [NSNumber numberWithInt:0xB4],
            @"ins" :[NSNumber numberWithInt:@0xB7],
            @"insert" : [NSNumber numberWithInt:0xB7],
            @"caps" : [NSNumber numberWithInt:0x3C],
            @"left" : [NSNumber numberWithInt:0x86],
            @"right" : [NSNumber numberWithInt:0x87],
            @"up" : [NSNumber numberWithInt:0x8E],
            @"down" : [NSNumber numberWithInt:0x8F],
            @"f1" : [NSNumber numberWithInt:0x03],
            @"f2" : [NSNumber numberWithInt:0x04],
            @"f3" : [NSNumber numberWithInt:0x13],
            @"f4" : [NSNumber numberWithInt:0x14]
        };
        [parseDict retain];
    }
    return sharedInstance;
}

-(void) nonEscapeCopy: (char *) string
{
    unsigned char *c = (unsigned char *) string;
    int count = 0;
    
    while(*c) {
        if (*c == 0x9B) {
            copyBuffer[count] = '\n';
            c++;
            count++;
            continue;
        }
        
        *c &= 0x7F;
        if (*c >= 0x20 && *c <= 0x7C && *c != 0x7B && *c != 0x60)
            copyBuffer[count] = *c;
        else
            copyBuffer[count] = ' ';
        c++;
        count++;
        if (count == COPY_BUFFER_SIZE)
            break;
    }
    
    copyBuffer[count] = 0;
}

-(void) escapeCopy: (char *) string
{
    int inv = 0;
    
    copyBuffer[0] = 0;

    unsigned char *c = (unsigned char *) string;
    while(*c) {
        if (*c == 0x9B) {
            strcat(copyBuffer,"\n");
            c++;
            continue;
        }
        
        if ((*c ^ inv) & 0x80) {
            inv ^= 0x80;

            strcat(copyBuffer,"{inv}");
        }

        *c &= 0x7F;

        if (*c == 0x00) {
            strcat(copyBuffer,"{^},");
        } else if (*c >= 0x01 && *c < 0x1B) {
            unsigned char cprime;
            strcat(copyBuffer,"{^}");
            cprime = (char)('a' + (*c - 0x01));
            strncat(copyBuffer,(char *)&cprime,1);
        } else if (*c == 0x1B) {
            strcat(copyBuffer,"{esc}{esc}");
        } else if (*c == 0x1C) {
            if (inv)
                strcat(copyBuffer,"{esc}{+delete}");
            else
                strcat(copyBuffer,"{esc}{up}");
        } else if (*c == 0x1D) {
            if (inv)
                strcat(copyBuffer,"{esc}{+insert}");
            else
                strcat(copyBuffer,"{esc}{down}");
        } else if (*c == 0x1E) {
            if (inv)
                strcat(copyBuffer,"{esc}{^tab}");
            else
                strcat(copyBuffer,"{esc}{left}");
        } else if (*c == 0x1F) {
            if (inv)
                strcat(copyBuffer,"{esc}{+tab}");
            else
                strcat(copyBuffer,"{esc}{right}");
        } else if (*c >= 0x20 && *c < 0x60) {
            strncat(copyBuffer,(char *)c,1);
        } else if (*c == 0x60) {
            strcat(copyBuffer,"{^}.");
        } else if (*c >= 0x61 && *c < 0x7B) {
            strncat(copyBuffer,(char *)c,1);
        } else if (*c == 0x7B) {
            strcat(copyBuffer,"{^};");
        } else if (*c == 0x7C) {
            strncat(copyBuffer,(char *)c,1);
        } else if (*c == 0x7D) {
            if (inv)
                strcat(copyBuffer,"{esc}{^}2");
            else
                strcat(copyBuffer,"{esc}{clear}");
        } else if (*c == 0x7E) {
            if (inv)
                strcat(copyBuffer,"{esc}{del}");
            else
                strcat(copyBuffer,"{esc}{back}");
        } else if (*c == 0x7F) {
            if (inv)
                strcat(copyBuffer,"{esc}{ins}");
            else
                strcat(copyBuffer,"{esc}{tab}");
        }
        c++;
    }

    if (inv)
        strcat(copyBuffer,"{inv}");

}

- (int)getScancode:(unsigned char *) code
{
    if (pasteIndex < charCount) {
        *code = pasteBuffer[pasteIndex];
        pasteIndex++;
        return(TRUE);
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
        charCount = 0;
        [self pasteStringToKeys];
        pasteIndex = 0;
		return TRUE;
		}
	else {
		pasteString = nil;
		charCount = 0;
		return FALSE;
		}
	}

-(BOOL) scancodeForCharacter: (char) c: (UInt8 *) ch {
    switch(c) {
#define CHAR_CASE(chval, sc) case chval: *ch = sc; return YES;
        CHAR_CASE('l', 0x00 )
        CHAR_CASE('L', 0x40 )

        CHAR_CASE('j', 0x01 )
        CHAR_CASE('J', 0x41 )

        CHAR_CASE(';', 0x02 )
        CHAR_CASE(':', 0x42 )

        CHAR_CASE('k', 0x05 )
        CHAR_CASE('K', 0x45 )

        CHAR_CASE('+', 0x06 )
        CHAR_CASE('\\', 0x46 )

        CHAR_CASE('*', 0x07 )
        CHAR_CASE('^', 0x47 )

        CHAR_CASE('o', 0x08 )
        CHAR_CASE('O', 0x48 )

        CHAR_CASE('p', 0x0A )
        CHAR_CASE('P', 0x4A )

        CHAR_CASE('u', 0x0B )
        CHAR_CASE('U', 0x4B )

        CHAR_CASE('i', 0x0D )
        CHAR_CASE('I', 0x4D )

        CHAR_CASE('-', 0x0E )
        CHAR_CASE('_', 0x4E )

        CHAR_CASE('=', 0x0F )
        CHAR_CASE('|', 0x4F )

        CHAR_CASE('v', 0x10 )
        CHAR_CASE('V', 0x50 )

        CHAR_CASE('c', 0x12 )
        CHAR_CASE('C', 0x52 )

        CHAR_CASE('b', 0x15 )
        CHAR_CASE('B', 0x55 )

        CHAR_CASE('x', 0x16 )
        CHAR_CASE('X', 0x56 )

        CHAR_CASE('z', 0x17 )
        CHAR_CASE('Z', 0x57 )

        CHAR_CASE('4', 0x18 )
        CHAR_CASE('$', 0x58 )

        CHAR_CASE('3', 0x1A )
        CHAR_CASE('#', 0x5A )

        CHAR_CASE('6', 0x1B )
        CHAR_CASE('&', 0x5B )

        CHAR_CASE('5', 0x1D )
        CHAR_CASE('%', 0x5D )

        CHAR_CASE('2', 0x1E )
        CHAR_CASE('"', 0x5E )

        CHAR_CASE('1', 0x1F )
        CHAR_CASE('!', 0x5F )

        CHAR_CASE(',', 0x20 )
        CHAR_CASE('[', 0x60 )

        CHAR_CASE(' ', 0x21 )

        CHAR_CASE('.', 0x22 )
        CHAR_CASE(']', 0x62 )

        CHAR_CASE('n', 0x23 )
        CHAR_CASE('N', 0x63 )

        CHAR_CASE('m', 0x25 )
        CHAR_CASE('M', 0x65 )

        CHAR_CASE('/', 0x26 )
        CHAR_CASE('?', 0x66 )

        CHAR_CASE('r', 0x28 )
        CHAR_CASE('R', 0x68 )

        CHAR_CASE('e', 0x2A )
        CHAR_CASE('E', 0x6A )

        CHAR_CASE('y', 0x2B )
        CHAR_CASE('Y', 0x6B )

        CHAR_CASE('t', 0x2D )
        CHAR_CASE('T', 0x6D )

        CHAR_CASE('w', 0x2E )
        CHAR_CASE('W', 0x6E )

        CHAR_CASE('q', 0x2F )
        CHAR_CASE('Q', 0x6F )

        CHAR_CASE('9', 0x30 )
        CHAR_CASE('(', 0x70 )

        CHAR_CASE('0', 0x32 )
        CHAR_CASE(')', 0x72 )

        CHAR_CASE('7', 0x33 )
        CHAR_CASE('\'', 0x73 )

        CHAR_CASE('8', 0x35 )
        CHAR_CASE('@', 0x75 )

        CHAR_CASE('<', 0x36 )
        CHAR_CASE('>', 0x37 )

        CHAR_CASE('f', 0x38 )
        CHAR_CASE('F', 0x78 )

        CHAR_CASE('h', 0x39 )
        CHAR_CASE('H', 0x79 )

        CHAR_CASE('d', 0x3A )
        CHAR_CASE('D', 0x7A )

        CHAR_CASE('g', 0x3D )
        CHAR_CASE('G', 0x7D )

        CHAR_CASE('s', 0x3E )
        CHAR_CASE('S', 0x7E )

        CHAR_CASE('a', 0x3F )
        CHAR_CASE('A', 0x7F )

        CHAR_CASE('`', 0x27 )
        CHAR_CASE('~', 0x67 )
#undef CHAR_CASE
        default:
            return NO;
    }

}


-(void) pasteStringToKeys {
    char  skipLT = 0;
    UInt8 scancodeModifier = 0;
    char  name[64];
    char  *pname;

    char *t = (char *)[pasteString UTF8String];
    char c;
    
    while ((c = *t++)) {
        if (c == skipLT) {
            skipLT = 0;
            continue;
        }

        skipLT = 0;

        if (c == '{') {
            char *start = t;

            while(*t && *t != '}')
                ++t;

            if (*t != '}')
                break;

            int spanlen = t - start;
            strncpy(name, start, spanlen);
            name[spanlen] = 0;
            pname = name;
            ++t;

            while(*pname) {
                if (pname[0] == '+') {
                    scancodeModifier |= 0x40;
                    pname++;
                    continue;
                }

                if ((strncmp(pname,"shift-",6) == 0) ||
                    (strncmp(pname,"shift+",6) == 0)) {
                    scancodeModifier |= 0x40;
                    pname += 6;
                    continue;
                }

                if (pname[0] == '^') {
                    scancodeModifier |= 0x80;
                    pname++;
                    continue;
                }

                if ((strncmp(pname,"ctrl-",5) == 0) ||
                    (strncmp(pname,"ctrl+",5) == 0)) {
                    scancodeModifier |= 0x80;
                    pname += 5;
                    continue;
                }
                
                if (strncmp(pname,"select",6) == 0) {
                    pasteBuffer[charCount] = AKEY_SELECT_PSEUDO;
                    charCount++;
                    break;
                }
                
                if (strncmp(pname,"start",5) == 0) {
                    pasteBuffer[charCount] = AKEY_START_PSEUDO;
                    charCount++;
                    break;
                }
                
                if (strncmp(pname,"option",6) == 0) {
                    pasteBuffer[charCount] = AKEY_OPTION_PSEUDO;
                    charCount++;
                    break;
                }
                
                if (strncmp(pname,"delay-",6) == 0) {
                    pname += 6;
                    int delayTicks = atoi(pname) & 0xFFFF;
                    if (charCount < 2046) {
                        pasteBuffer[charCount] = AKEY_DELAY_PSEUDO;
                        pasteBuffer[charCount+1] = (delayTicks & 0xFF00) >> 8;
                        pasteBuffer[charCount+2] = delayTicks & 0x00FF;
                        charCount += 3;
                    }
                    break;
                }
                
                NSString *keyword = [NSString stringWithCString:name encoding:NSASCIIStringEncoding];
                NSString *lowerKeyword = [keyword lowercaseString];
                NSNumber *scannumber = parseDict[lowerKeyword];
                if (scannumber != nil) {
                    UInt8 scancode = [scannumber intValue];
                    if (scancodeModifier)
                        scancode = (scancode & 0x3F) | scancodeModifier;

                    if (charCount < 2048) {
                        pasteBuffer[charCount] = scancode;
                        charCount++;
                    }
                }

                scancodeModifier = 0;
                break;
            }

            continue;
        }

        const UInt8 InvalidScancode = 0xFF;
        UInt8 scancode = InvalidScancode;

        switch(c) {
            case '\r':
            case '\n':
                skipLT = c ^ ('\r' ^ '\n');
                scancode = 0x0C;
                break;

            case '\t':         // Tab
                scancode = 0x2C;
                break;

            case '\x1B':     // Esc
                scancode = 0x1C;
                break;

            default:
                if(![self scancodeForCharacter:c:&scancode]) {
                    scancode = InvalidScancode;
                } else {
                    if (scancodeModifier)
                        scancode = (scancode & 0x3F) | scancodeModifier;
                }
                break;
        }

        if (scancode != InvalidScancode && charCount < 2048) {
            pasteBuffer[charCount] = scancode;
            charCount++;
        }
        
        scancodeModifier = 0;
    }
}

- (void)startCopy:(char *)string
{
    [self escapeCopy:string];
	NSPasteboard *pb = [NSPasteboard generalPasteboard];
	NSArray *types = [NSArray arrayWithObjects:
					  NSStringPboardType, nil];
	[pb declareTypes:types owner:self];
	[pb setString:[NSString stringWithCString:copyBuffer encoding:NSASCIIStringEncoding] forType:NSStringPboardType];
}

@end
