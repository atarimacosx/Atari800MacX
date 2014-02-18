//
//  PrinterView.m
//  Atari800MacX
//
//  Created by Mark Grebe on Sun Mar 13 2005.
//  Copyright (c) 2005 __MyCompanyName__. All rights reserved.
//

#import "PrintOutputController.h"
#import "PrinterView.h"


@implementation PrinterView

- (id)initWithFrame:(NSRect)frame:(PrintOutputController *)owner {
    self = [super initWithFrame:frame];
    if (self) {
	    controller = owner;
        // Initialization code here.
    }
    return self;
}

- (void)drawRect:(NSRect)rect {
    // Drawing code here.
	float aFontMatrix [6];
	NSFont *lNewFont;
	NSFont *pNewFont;
	NSAttributedString *text;
//	NSRect r = NSMakeRect(10, 10, 50, 60);
//    NSColor *color = [NSColor blueColor];
	NSPoint textLoc;
//	NSPoint textLoc2 = NSMakePoint(10,90);
    char num[80];
	int i;
	
//    [color set];
//    NSRectFill(r);
	aFontMatrix [1] = aFontMatrix [2] = aFontMatrix [4] = aFontMatrix [5] = 0.0;  // Always
	aFontMatrix [3] = 12.0;
	aFontMatrix [0] = 12.0;

	lNewFont = [NSFont fontWithName : @"Courier" matrix : aFontMatrix];
	pNewFont = [NSFont fontWithName : @"Helvetica" matrix : aFontMatrix];

	text = [NSAttributedString alloc];
	textLoc = NSMakePoint(10,0);
	[text initWithString:@"01234567890123456789012345678901234567890123456789012345678901234567890123456789" 
	                     attributes:[NSDictionary dictionaryWithObject:lNewFont forKey:NSFontAttributeName]];
	[text drawAtPoint:textLoc];
	[text release];
	
	text = [NSAttributedString alloc];
	textLoc = NSMakePoint(10,12);
	[text initWithString:@"abcdefghijklmnopqrstuvwxyz" 
	                     attributes:[NSDictionary dictionaryWithObject:pNewFont forKey:NSFontAttributeName]];
	[text drawAtPoint:textLoc];
	[text release];
	
	for (i=2;i<67;i++)
	{
		textLoc=NSMakePoint(10,i*12);
		sprintf(num,"%d",i);
		text = [NSAttributedString alloc];
		[text initWithString:[NSString stringWithCString:num] 
	                     attributes:[NSDictionary dictionaryWithObject:lNewFont forKey:NSFontAttributeName]];
		[text drawAtPoint:textLoc];
		[text release];
	}

}

- (BOOL)isFlipped
{
	return YES;
}

@end
