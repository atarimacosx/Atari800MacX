/* Atari800FunctionKeysWindow.h - Atari800FunctionKeysWindow 
 header for the Macintosh OS X SDL port 
 of Atari800 Mark Grebe <atarimacosx@gmail.com>
 */


#import <Cocoa/Cocoa.h>
#import <PastePanel.h>


@interface Atari800FunctionKeysWindow : PastePanel {
}

- (BOOL)windowShouldClose:(id)sender;

@end

