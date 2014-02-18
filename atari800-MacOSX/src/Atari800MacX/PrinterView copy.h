//
//  PrinterView.h
//  Atari800MacX
//
//  Created by Mark Grebe on Sun Mar 13 2005.
//  Copyright (c) 2005 __MyCompanyName__. All rights reserved.
//

#import <AppKit/AppKit.h>


@interface PrinterView : NSView {
	PrintOutputController *controller;
}
- (id)initWithFrame:(NSRect)frame:(PrintOutputController *)owner;

@end
