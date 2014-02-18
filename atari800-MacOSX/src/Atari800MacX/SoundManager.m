/* SoundManager.m - Menu suppor class to 
   the Sound menu functions for the 
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimac@kc.rr.com>
   
   Based on the Preferences pane of the
   TextEdit application.

*/
#import "SoundManager.h"

extern int sound_enabled;
extern double sound_volume;
extern int POKEYSND_stereo_enabled;
extern int requestSoundEnabledChange;
extern int requestSoundRecordingChange;
extern int requestSoundStereoChange;

/* Functions which provide an interface for C code to call this object's shared Instance functions */
void SetSoundManagerEnable(int soundEnabled) {
    [[SoundManager sharedInstance] setSoundEnableMenu:(soundEnabled)];
    }

void SetSoundManagerStereo(int soundStereo) {
    [[SoundManager sharedInstance] setSoundStereoMenu:(soundStereo)];
    }

void SetSoundManagerRecording(int soundRecording) {
    [[SoundManager sharedInstance] setSoundRecordingMenu:(soundRecording)];
    }

void SoundManagerStereoDo() {
    [[SoundManager sharedInstance] stereoSound:nil];
}

void SoundManagerIncreaseVolume() {
    [[SoundManager sharedInstance] increaseVolume:nil];
}

void SoundManagerDecreaseVolume() {
    [[SoundManager sharedInstance] decreaseVolume:nil];
}

void SoundManagerRecordingDo() {
    [[SoundManager sharedInstance] soundRecording:nil];
}

@implementation SoundManager
static SoundManager *sharedInstance = nil;

+ (SoundManager *)sharedInstance {
    return sharedInstance ? sharedInstance : [[self alloc] init];
}

- (id)init {
    if (sharedInstance) {
	[self dealloc];
    } else {
        [super init];
        sharedInstance = self;
    }
    return sharedInstance;
}

/*------------------------------------------------------------------------------
*  setSoundEnableMenu - This method is used to set the menu text for the 
*     sound enable/disable menu item.
*-----------------------------------------------------------------------------*/
- (void)setSoundEnableMenu:(int)soundEnabled
{
    static NSString *enabledStr = nil;
    static NSString *disabledStr = nil;
    
    if (enabledStr == nil) {
        enabledStr = [[NSString alloc] initWithString:@"Disable Sound"];
        disabledStr = [[NSString alloc] initWithString:@"Enable Sound"];
        }

    if (soundEnabled)	
        [enableSoundItem setTitle:enabledStr];
    else
        [enableSoundItem setTitle:disabledStr];

}

/*------------------------------------------------------------------------------
 *  setSoundStereoMenu - This method is used to set the menu text for the 
 *     sound stereo/mono menu item.
 *-----------------------------------------------------------------------------*/
- (void)setSoundStereoMenu:(int)soundStereo
{
    if (soundStereo)	
        [stereoSoundItem setState:NSOnState];
    else
        [stereoSoundItem setState:NSOffState];
}

/*------------------------------------------------------------------------------
 *  setSoundVolumeMenu - This method is used to set the menu text for the 
 *     sound volume menu item.
 *-----------------------------------------------------------------------------*/
- (void)setSoundStereoMenu
{
    NSString *volumeStr;
	int volPercent;
	
	volPercent = sound_volume * 100;
	volumeStr = [NSString stringWithFormat:@"Volume Level - %u%%",volPercent];
	[volumeItem setTitle:volumeStr];
}

/*------------------------------------------------------------------------------
*  setSoundRecordingMenu - This method is used to set the menu text for the 
*     sound recording menu item.
*-----------------------------------------------------------------------------*/
- (void)setSoundRecordingMenu:(int)soundRecording;
{
    static NSString *enabledStr = nil;
    static NSString *disabledStr = nil;
    
    if (enabledStr == nil) {
        enabledStr = [[NSString alloc] initWithString:@"Stop Sound Recording"];
        disabledStr = [[NSString alloc] initWithString:@"Start Sound Recording"];
        }

    if (soundRecording)	
        [soundRecordingItem setTitle:enabledStr];
    else
        [soundRecordingItem setTitle:disabledStr];
}

/*------------------------------------------------------------------------------
*  enableSound - This method handles the enable/disable sound menu selection.
*-----------------------------------------------------------------------------*/
- (IBAction)enableSound:(id)sender
{
    [self setSoundEnableMenu:(1-sound_enabled)];
    requestSoundEnabledChange = 1;
}

/*------------------------------------------------------------------------------
*  soundRecording - This method handles the sound recording menu selection.
*-----------------------------------------------------------------------------*/
- (IBAction)soundRecording:(id)sender
{
    requestSoundRecordingChange = 1;
}

/*------------------------------------------------------------------------------
*  stereoSound - This method handles the stero sound menu selection.
*-----------------------------------------------------------------------------*/
- (IBAction)stereoSound:(id)sender
{
    [self setSoundStereoMenu:(1-POKEYSND_stereo_enabled)];
    requestSoundStereoChange = 1;
}

/*------------------------------------------------------------------------------
 *  increaseVolume - This method handles the increase volume menu selection.
 *-----------------------------------------------------------------------------*/
- (IBAction)increaseVolume:(id)sender
{
	if (sound_volume < 1.0)
		sound_volume += 0.05;
	[self setSoundStereoMenu];
}

/*------------------------------------------------------------------------------
 *  decreaseVolume - This method handles the decrease volume menu selection.
 *-----------------------------------------------------------------------------*/
- (IBAction)decreaseVolume:(id)sender;
{
	if (sound_volume > 0.0)
		sound_volume -= 0.05;
	[self setSoundStereoMenu];
}


@end
