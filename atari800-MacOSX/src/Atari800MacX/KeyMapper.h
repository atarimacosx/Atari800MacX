/* KeyMapper.h - KeyMapper 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 
 */

#import <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

@interface KeyMapper : NSObject {
	unsigned int keymap[128];
}

+ (KeyMapper *)sharedInstance;
- (void) releaseCmdKeys:(NSString *)character;
-(unsigned int)getQuartzKey:(unsigned int) character;

@end
