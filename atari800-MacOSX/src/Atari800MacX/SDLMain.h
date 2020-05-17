/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>

       Macintosh OS X SDL port of Atari800
       Mark Grebe <atarimacosx@gmail.com>

    Feel free to customize this file to suit your needs
*/

#import <Cocoa/Cocoa.h>

@interface SDLMain : NSObject
+(void)loadFile:(NSString *)filename;
@end
