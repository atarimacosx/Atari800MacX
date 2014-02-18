/* DisplayManager.h - Menu suppor class to 
   the Display menu functions for the 
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimac@kc.rr.com>
   
   Based on the Preferences pane of the
   TextEdit application.

*/
#import <Cocoa/Cocoa.h>

@interface DisplayManager : NSObject
{
    IBOutlet id doubleSizeItem;
	IBOutlet id scale1xItem;
	IBOutlet id scale2xItem;
	IBOutlet id scale3xItem;
	IBOutlet id scale4xItem;
    IBOutlet id fullscreenItem;
    IBOutlet id xep80Item;
    IBOutlet id xep80Mode0Item;
    IBOutlet id xep80Mode1Item;
    IBOutlet id xep80Mode2Item;
    IBOutlet id widthModeNarrowItem;
    IBOutlet id widthModeDefaultItem;
    IBOutlet id widthModeFullItem;
    IBOutlet id displayFpsItem;
	IBOutlet id scaleModeNormalItem;
	IBOutlet id scaleModeScanlineItem;
	IBOutlet id scaleModeSmoothItem;
    IBOutlet id grabMouseItem;
	IBOutlet id NoArtifactItem;
	IBOutlet id revXLXEGTIAItem;
	IBOutlet id XLXEGTIAItem;
	IBOutlet id GTIA400800Item;
	IBOutlet id CTIA400800Item;

}
+ (DisplayManager *)sharedInstance;
- (void)setDoublesizeMenu:(int)scale;
- (void)setWidthmodeMenu:(int)widthMode;
- (void)setGrabmouseMenu:(int)mouseOn;
- (void)setFpsMenu:(int)fpsOn;
- (void)setScaleModeMenu:(int)scaleMode;
- (void)setArtifactModeMenu:(int)artifactMode;
- (void)setXEP80ModeMenu:(int)xep80Enabled:(int)xep80Port:(int)xep80;
- (IBAction)doubleSize:(id)sender;
- (IBAction)fullScreen:(id)sender;
- (IBAction)xep80:(id)sender;
- (IBAction)xep80Mode:(id)sender;
- (IBAction)grabMouse:(id)sender;
- (IBAction)displayFps:(id)sender;
- (IBAction)scaleMode:(id)sender;
- (IBAction)screenshot:(id)sender;
- (IBAction)widthMode:(id)sender;
- (IBAction)artifactModeChange:(id)sender;
- (void) paste:(id) sender;
- (void) selectAll:(id) sender;
- (void) copy:(id) sender;
@end
