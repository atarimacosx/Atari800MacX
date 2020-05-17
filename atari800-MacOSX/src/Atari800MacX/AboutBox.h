/* AboutBox.h - Header for About Box 
   window class and support functions for the
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   
*/

#import <Cocoa/Cocoa.h>

@interface AboutBox : NSObject
{
    IBOutlet id appNameField;
    IBOutlet id creditsField;
    IBOutlet id versionField;
    NSTimer *scrollTimer;
    float currentPosition;
    float maxScrollHeight;
    NSTimeInterval startTime;
    BOOL restartAtTop;
}

+ (AboutBox *)sharedInstance;
- (IBAction)showPanel:(id)sender;
- (IBAction)showDonation:(id)sender;
- (void)scrollCredits;
- (void)clicked;
- (void)doubleClicked;

@end
