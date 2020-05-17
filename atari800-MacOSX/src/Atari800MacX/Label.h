/* Label.h - Label 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */


#import <Cocoa/Cocoa.h>
#import "monitor.h"


@interface Label : NSObject {
	char *myName;
	unsigned short myAddr;
	BOOL myBuiltin;
	BOOL myRead;
	BOOL myWrite;
}

+(Label *) labelWithName:(char *) name Addr:(unsigned short) addr Builtin:(BOOL) builtin Read:(BOOL) read Write:(BOOL) write;
-(Label *) initWithName:(char *) name Addr:(unsigned short) addr Builtin:(BOOL) builtin Read:(BOOL) read Write:(BOOL) write;
-(char *)labelName;
-(unsigned short)addr;
-(BOOL)builtin;
-(BOOL)read;
-(BOOL)write;
-(int)compareLabels:(id) other;
-(int)compareValues:(id) other;
-(int)compareBuiltins:(id) other;
-(int)compareReadWrites:(id) other;
	
@end
