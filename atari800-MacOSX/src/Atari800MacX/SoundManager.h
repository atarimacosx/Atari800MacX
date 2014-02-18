/* SoundManager.h - Menu suppor class to 
   the Sound menu functions for the 
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimac@kc.rr.com>
   
   Based on the Preferences pane of the
   TextEdit application.

*/
#import <Cocoa/Cocoa.h>

@interface SoundManager : NSObject
{
    IBOutlet id enableSoundItem;
    IBOutlet id soundRecordingItem;
    IBOutlet id stereoSoundItem;
	IBOutlet id volumeItem;
}
+ (SoundManager *)sharedInstance;
- (void)setSoundEnableMenu:(int)soundEnabled;
- (void)setSoundStereoMenu:(int)soundStereo;
- (void)setSoundRecordingMenu:(int)soundRecording;
- (IBAction)enableSound:(id)sender;
- (IBAction)soundRecording:(id)sender;
- (IBAction)stereoSound:(id)sender;
- (IBAction)increaseVolume:(id)sender;
- (IBAction)decreaseVolume:(id)sender;
@end
