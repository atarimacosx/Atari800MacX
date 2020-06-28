/* KeyMapper.m - 
 KeyMapper class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "KeyMapper.h"
#import "SDL.h"


@implementation KeyMapper
static KeyMapper *sharedInstance = nil;

+ (KeyMapper *)sharedInstance {
    return sharedInstance ? sharedInstance : [[self alloc] init];
}

- (id)init {
    BOOL saw_layout = NO;
    Uint32 state;
    UInt32 value;
    Uint16 i;
    int world = 128; // TBD SDLK_WORLD_0;

    if (sharedInstance) {
	    [self dealloc];
    } else {
        [super init];
        sharedInstance = self;
		
		for ( i=0; i<128; ++i )
			keymap[i] = SDLK_UNKNOWN;
		
        TISInputSourceRef src = TISCopyCurrentKeyboardLayoutInputSource();
        if (src != NULL) {
            CFDataRef data = (CFDataRef)
                TISGetInputSourceProperty(src,
                    kTISPropertyUnicodeKeyLayoutData);
            if (data != NULL) {
                const UCKeyboardLayout *layout = (const UCKeyboardLayout *)
                    CFDataGetBytePtr(data);
                if (layout != NULL) {
                    const UInt32 kbdtype = LMGetKbdType();
                    saw_layout = YES;

                    /* Loop over all 127 possible scan codes */
                    for (i = 0; i < 0x7F; i++) {
                        UniChar buf[16];
                        UniCharCount count = 0;

                        /* We pretend a clean start to begin with (i.e. no dead keys active */
                        state = 0;

                        if (UCKeyTranslate(layout, i, kUCKeyActionDown, 0, kbdtype,
                                           0, &state, 16, &count, buf) != noErr) {
                            continue;
                        }

                        /* If the state become 0, it was a dead key. We need to
                           translate again, passing in the new state, to get
                           the actual key value */
                        if (state != 0) {
                            if (UCKeyTranslate(layout, i, kUCKeyActionDown, 0, kbdtype,
                                               0, &state, 16, &count, buf) != noErr) {
                                continue;
                            }
                        }

                        if (count != 1) {
                            continue;  /* no multi-char. Use SDL 1.3 instead. :) */
                        }

                        value = (UInt32) buf[0];
                        if (value >= 128) {
                            /* Some non-ASCII char, map it to SDLK_WORLD_* */
                            if (world < 0xFF) {
                                keymap[i] = world++;
                            }
                        } else if (value >= 32) {     /* non-control ASCII char */
                            keymap[i] = value;
                        }
                    }
                }
            }
            CFRelease(src);
        }
 	}
    return sharedInstance;
}

/*------------------------------------------------------------------------------
*  releaseCmdKeys - This method fixes an issue when modal windows are used with
*     the Mac OSX version of the SDL library.
*     As the SDL normally captures all keystrokes, but we need to type in some 
*     Mac windows, all of the control menu windows run in modal mode.  However, 
*     when this happens, the release of the command key and the shortcut key 
*     are not sent to SDL.  We have to manually cause these events to happen 
*     to keep the SDL library in a sane state, otherwise only everyother shortcut
*     keypress will work.
*-----------------------------------------------------------------------------*/
- (void) releaseCmdKeys:(NSString *)character
{
    NSEvent *event1, *event2;
    NSPoint point = {0.0,0.0};
    SDL_Event event;
	int keyCode;
	unsigned int charCode;
	
	charCode = [character characterAtIndex:0];
	keyCode = [self getQuartzKey:charCode];
    
    event1 = [NSEvent keyEventWithType:NSEventTypeKeyUp location:point modifierFlags:0
                    timestamp:0.0 windowNumber:0 context:nil characters:character
                    charactersIgnoringModifiers:character isARepeat:NO keyCode:keyCode];
    [NSApp postEvent:event1 atStart:NO];
    
    event2 = [NSEvent keyEventWithType:NSEventTypeFlagsChanged location:point modifierFlags:0
                    timestamp:0.0 windowNumber:0 context:nil characters:@""
                    charactersIgnoringModifiers:@"" isARepeat:NO keyCode:0];
    [NSApp postEvent:event2 atStart:NO];
	
	SDL_PollEvent(&event);
}

-(unsigned int)getQuartzKey:(unsigned int) character;
{
	int i;
	
	for (i=0;i<128;i++) {
		if (keymap[i] == character)
			return(i);
	}
	
	return 0;
}


@end
