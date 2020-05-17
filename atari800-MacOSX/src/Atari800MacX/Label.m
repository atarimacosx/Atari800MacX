/* Label.m - 
 Label class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "Label.h"


@implementation Label

+(Label *) labelWithName:(char *) name Addr:(unsigned short) addr Builtin:(BOOL) builtin Read:(BOOL) read Write:(BOOL) write
{
	Label *theLabel;
	
	theLabel = [[self alloc] initWithName:name Addr:addr Builtin:builtin Read:read Write:write];
	[theLabel autorelease];
	return(theLabel);
}

-(Label *) initWithName:(char *) name Addr:(unsigned short) addr Builtin:(BOOL) builtin Read:(BOOL) read Write:(BOOL) write
{
	myName = name;
	myAddr = addr;
	myBuiltin = builtin;
	myRead = read;
	myWrite = write;
	
	return self;
}

-(char *)labelName
{
	return myName;
}

-(unsigned short)addr
{
	return myAddr;
}

-(BOOL)builtin
{
	return myBuiltin;
}

-(BOOL)read
{
	return myRead;
}

-(BOOL)write
{
	return myWrite;
}

-(int)compareLabels:(id) other
{
	int cmp = strcmp((const char *) [self labelName], (const char *) [other labelName]);
	if (cmp == 0)
		return NSOrderedSame;
	else if (cmp < 0)
		return NSOrderedAscending;
	else
		return NSOrderedDescending;
}

-(int)compareValues:(id) other
{
	if ([self addr] < [(Label *) other addr])
		return NSOrderedAscending;
	else if ([self addr] > [(Label *) other addr])
		return NSOrderedDescending;
	else
		return NSOrderedSame;
}

-(int)compareBuiltins:(id) other
{
	if ([self builtin]) {
		if ([other builtin])
			return [self compareLabels:other];
		else
			return NSOrderedAscending;
	}
	else
	{
		if ([other builtin])
			return NSOrderedDescending;
		else
			return [self compareLabels:other];
	}
	
}

-(int)compareReadWrites:(id) other
{
	if ([self read]) {
		if ([other read])
			return [self compareLabels:other];
		else if ([other write])
			return NSOrderedAscending;
		else
			return NSOrderedDescending;
	}
	else if ([self write]) {
		if ([other read])
			return NSOrderedDescending;
		else if ([other write])
			return [self compareLabels:other];
		else
			return NSOrderedDescending;
	}
	else {
		if ([other read])
			return NSOrderedAscending;
		else if ([other write])
			return NSOrderedAscending;
		else
			return [self compareLabels:other];
	}
	
}


@end
