/* DisplayManager.h - Menu suppor class to 
   the Display menu functions for the 
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   
   Based on the Preferences pane of the
   TextEdit application.

*/
#import <Cocoa/Cocoa.h>

@interface DisplayManager : NSObject
{

    IBOutlet id fullscreenItem;
    IBOutlet id xep80Item;
    IBOutlet id xep80AutoswitchItem;
    IBOutlet id xep80Mode0Item;
    IBOutlet id xep80Mode1Item;
    IBOutlet id xep80Mode2Item;
    IBOutlet id af80ModeItem;
    IBOutlet id bit3ModeItem;
    IBOutlet id widthModeNarrowItem;
    IBOutlet id widthModeDefaultItem;
    IBOutlet id widthModeFullItem;
    IBOutlet id displayFpsItem;
	IBOutlet id scaleModeNormalItem;
	IBOutlet id scaleModeScanlineItem;
    IBOutlet id grabMouseItem;
	IBOutlet id NoArtifactItem;
	IBOutlet id revXLXEGTIAItem;
	IBOutlet id XLXEGTIAItem;
	IBOutlet id GTIA400800Item;
	IBOutlet id CTIA400800Item;
    IBOutlet id copyMenu;
    IBOutlet id pasteMenu;
    IBOutlet id selectAllMenu;
}
+ (DisplayManager *)sharedInstance;
- (void)setWidthmodeMenu:(int)widthMode;
- (void)setGrabmouseMenu:(int)mouseOn;
- (void)setFpsMenu:(int)fpsOn;
- (void)setScaleModeMenu:(int)scaleMode;
- (void)setArtifactModeMenu:(int)artifactMode;
- (bool)enable80ColModeMenu:(int)machineType:(int)xep80Enabled:(int)af80Enabled:(int)bit3Enabled;
- (void)set80ColModeMenu:(int)xep80Enabled:(int)xep80Port:(int)af80Enabled:(int)bit3Enabled:(int)col80;

- (void)setXEP80AutoswitchMenu:(int)autoswitchOn;
- (void)disableAF80;
- (void)disableBit3;
- (IBAction)fullScreen:(id)sender;
- (IBAction)xep80:(id)sender;
- (IBAction)xep80Autoswitch:(id)sender;
- (IBAction)xep80Mode:(id)sender;
- (IBAction)grabMouse:(id)sender;
- (IBAction)displayFps:(id)sender;
- (IBAction)scaleMode:(id)sender;
- (IBAction)screenshot:(id)sender;
- (IBAction)widthMode:(id)sender;
- (IBAction)artifactModeChange:(id)sender;
- (IBAction) paste:(id) sender;
- (IBAction) selectAll:(id) sender;
- (IBAction) copy:(id) sender;
- (void) enableMacCopyPaste;
- (void) enableAtariCopyPaste;
@end
