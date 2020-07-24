/* DisplayManager.m - Menu suppor class to 
   the Display menu functions for the 
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   
   Based on the Preferences pane of the
   TextEdit application.

*/
#import "DisplayManager.h"
#import "MediaManager.h"
#import "af80.h"
#import "bit3.h"
#import "xep80.h"

extern void SwitchFullscreen(void);
extern void PLATFORM_Switch80Col(void);
extern int SCALE_MODE;
extern int scaleFactor;
extern int requestWidthModeChange;
extern int WIDTH_MODE;
extern int Screen_show_atari_speed;
extern int requestFullScreenChange;
extern int request80ColChange;
extern int requestFpsChange;
extern int requestScaleModeChange;
extern int requestGrabMouse;
extern int requestScreenshot;
extern int ANTIC_artif_mode;
extern int requestArtifChange;
extern int PLATFORM_80col;
extern int requestPaste;
extern int request80ColModeChange;
extern int requestCopy;
extern int requestSelectAll;

/* Functions which provide an interface for C code to call this object's shared Instance functions */
void SetDisplayManagerWidthMode(int widthMode) {
    [[DisplayManager sharedInstance] setWidthmodeMenu:(widthMode)];
    }

void SetDisplayManagerFps(int fpsOn) {
    [[DisplayManager sharedInstance] setFpsMenu:(fpsOn)];
    }

void SetDisplayManagerScaleMode(int scaleMode) {
    [[DisplayManager sharedInstance] setScaleModeMenu:(scaleMode)];
    }

void SetDisplayManagerArtifactMode(int artifactMode) {
    [[DisplayManager sharedInstance] setArtifactModeMenu:(artifactMode)];
    }

void SetDisplayManagerGrabMouse(int mouseOn) {
    [[DisplayManager sharedInstance] setGrabmouseMenu:(mouseOn)];
    }

void SetDisplayManager80ColMode(int xep80Enabled, int xep80Port, int af80Enabled, int bit3Enabled, int col80) {
    [[DisplayManager sharedInstance] set80ColModeMenu:(xep80Enabled):(xep80Port):(af80Enabled):(bit3Enabled):(col80)];
    }

void SetDisplayManagerXEP80Autoswitch(int autoswitchOn) {
    [[DisplayManager sharedInstance] setXEP80AutoswitchMenu:autoswitchOn];
    }

void SetDisplayManagerDisableAF80() {
    [[DisplayManager sharedInstance] disableAF80];
    }

void SetDisplayManagerDisableBit3() {
    [[DisplayManager sharedInstance] disableBit3];
    }

@implementation DisplayManager

static DisplayManager *sharedInstance = nil;

+ (DisplayManager *)sharedInstance {
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

- (void)dealloc {
	[super dealloc];
}

/*------------------------------------------------------------------------------
*  setWidthmodeMenu - This method is used to set the menu text for the 
*     short/default/full width menu item.
*-----------------------------------------------------------------------------*/
- (void)setWidthmodeMenu:(int)widthMode
{
	switch(widthMode)
	{
		case 0:
			[widthModeNarrowItem setState:NSOnState];
			[widthModeDefaultItem setState:NSOffState];
			[widthModeFullItem setState:NSOffState];
			break;
		case 1:
			[widthModeNarrowItem setState:NSOffState];
			[widthModeDefaultItem setState:NSOnState];
			[widthModeFullItem setState:NSOffState];
			break;
		case 2:
			[widthModeNarrowItem setState:NSOffState];
			[widthModeDefaultItem setState:NSOffState];
			[widthModeFullItem setState:NSOnState];
			break;
	}

}

/*------------------------------------------------------------------------------
*  setGrabmouseMenu - This method is used to set/clear the enabled check for the 
*     Grab mouse menu item.
*-----------------------------------------------------------------------------*/
- (void)setGrabmouseMenu:(int)mouseOn
{
    if (mouseOn)
        [grabMouseItem setTarget:self];
    else
        [grabMouseItem setTarget:nil];
}

/*------------------------------------------------------------------------------
*  setFpsMenu - This method is used to set/clear the enabled check for the
*     display frames per second menu item.
*-----------------------------------------------------------------------------*/
- (void)setFpsMenu:(int)fpsOn
{
    if (fpsOn)
        [displayFpsItem setState:NSOnState];
    else
        [displayFpsItem setState:NSOffState];
}

/*------------------------------------------------------------------------------
*  setXEP80AutoswitchMenu - This method is used to set/clear the enabled check for the
*     XEP80 Autoswitch menu item.
*-----------------------------------------------------------------------------*/
- (void)setXEP80AutoswitchMenu:(int)autoswitchOn
{
    if (autoswitchOn)
        [xep80AutoswitchItem setState:NSOnState];
    else
        [xep80AutoswitchItem setState:NSOffState];
}

/*------------------------------------------------------------------------------
*  setScaleModeMenu - This method is used to set the menu text for the
*     normal/scanline/smooth scale mode menu item.
*-----------------------------------------------------------------------------*/
- (void)setScaleModeMenu:(int)scaleMode
{
	switch(scaleMode)
	{
		case 0:
			[scaleModeNormalItem setState:NSOnState];
			[scaleModeScanlineItem setState:NSOffState];
			[scaleModeSmoothItem setState:NSOffState];
			break;
		case 1:
			[scaleModeNormalItem setState:NSOffState];
			[scaleModeScanlineItem setState:NSOnState];
			[scaleModeSmoothItem setState:NSOffState];
			break;
		case 2:
			[scaleModeNormalItem setState:NSOffState];
			[scaleModeScanlineItem setState:NSOffState];
			[scaleModeSmoothItem setState:NSOnState];
			break;
	}
}

/*------------------------------------------------------------------------------
*  setArtifactModeMenu - This method is used to set the menu text for the
*     artifact mode menu item.
*-----------------------------------------------------------------------------*/
- (void)setArtifactModeMenu:(int)artifactMode
{
	switch(artifactMode)
	{
		case 0:
			[NoArtifactItem setState:NSOnState];
			[revXLXEGTIAItem setState:NSOffState];
			[XLXEGTIAItem setState:NSOffState];
			[GTIA400800Item setState:NSOffState];
			[CTIA400800Item setState:NSOffState];
			break;
		case 1:
			[NoArtifactItem setState:NSOffState];
			[revXLXEGTIAItem setState:NSOnState];
			[XLXEGTIAItem setState:NSOffState];
			[GTIA400800Item setState:NSOffState];
			[CTIA400800Item setState:NSOffState];
			break;
		case 2:
			[NoArtifactItem setState:NSOffState];
			[revXLXEGTIAItem setState:NSOffState];
			[XLXEGTIAItem setState:NSOnState];
			[GTIA400800Item setState:NSOffState];
			[CTIA400800Item setState:NSOffState];
			break;
		case 3:
			[NoArtifactItem setState:NSOffState];
			[revXLXEGTIAItem setState:NSOffState];
			[XLXEGTIAItem setState:NSOffState];
			[GTIA400800Item setState:NSOnState];
			[CTIA400800Item setState:NSOffState];
			break;
		case 4:
			[NoArtifactItem setState:NSOffState];
			[revXLXEGTIAItem setState:NSOffState];
			[XLXEGTIAItem setState:NSOffState];
			[GTIA400800Item setState:NSOffState];
			[CTIA400800Item setState:NSOnState];
			break;
	}
}

/*------------------------------------------------------------------------------
*  setXEP80ModeMenu - This method is used to set the menu text for the
*     80 Col mode menu items.
*-----------------------------------------------------------------------------*/
- (void)set80ColModeMenu:(int)xep80Enabled:(int)xep80Port:(int)af80Enabled:(int)bit3Enabled:(int)col80;
{
	if (xep80Enabled) {
		[xep80Item setTarget:self];
		[xep80Mode0Item setState:NSOffState];
        [af80ModeItem setState:NSOffState];
        [bit3ModeItem setState:NSOffState];
		if (xep80Port == 0) {
			[xep80Mode1Item setState:NSOnState];
			[xep80Mode2Item setState:NSOffState];
			}
		else {
			[xep80Mode1Item setState:NSOffState];
			[xep80Mode2Item setState:NSOnState];
			}
		if (col80)
			[xep80Item setState:NSOnState];
		else
			[xep80Item setState:NSOffState];
		}
    else if (af80Enabled) {
        [xep80Item setTarget:self];
        [xep80Mode0Item setState:NSOffState];
        [xep80Mode1Item setState:NSOffState];
        [xep80Mode2Item setState:NSOffState];
        [af80ModeItem setState:NSOnState];
        [bit3ModeItem setState:NSOffState];
    }
    else if (bit3Enabled) {
        [xep80Item setTarget:self];
        [xep80Mode0Item setState:NSOffState];
        [xep80Mode1Item setState:NSOffState];
        [xep80Mode2Item setState:NSOffState];
        [af80ModeItem setState:NSOffState];
        [bit3ModeItem setState:NSOnState];
    }
	else {
		[xep80Item setState:NSOffState];
		[xep80Item setTarget:nil];
		[xep80Mode0Item setState:NSOnState];
		[xep80Mode1Item setState:NSOffState];
		[xep80Mode2Item setState:NSOffState];
        [af80ModeItem setState:NSOffState];
        [bit3ModeItem setState:NSOffState];
		}
}

/*------------------------------------------------------------------------------
*  disableAF80 - This method disables the AF80 menu itmes.
*-----------------------------------------------------------------------------*/
- (void) disableAF80
{
    [af80ModeItem setTarget:nil];
}

/*------------------------------------------------------------------------------
*  disableBit3 - This method disables the Bit3 menu itmes.
*-----------------------------------------------------------------------------*/
- (void) disableBit3
{
    [bit3ModeItem setTarget:nil];
}

/*------------------------------------------------------------------------------
*  fullScreen - This method handles the windowed/fullscreen menu selection.
*-----------------------------------------------------------------------------*/
- (IBAction)fullScreen:(id)sender
{
   requestFullScreenChange = 1;
}

/*------------------------------------------------------------------------------
*  xep80 - This method handles the normal/XEP80 menu selection.
*-----------------------------------------------------------------------------*/
- (IBAction)xep80:(id)sender
{
    request80ColChange = 1;
}

/*------------------------------------------------------------------------------
*  xep80 - This method handles the normal/XEP80 menu selection.
*-----------------------------------------------------------------------------*/
- (IBAction)xep80Autoswitch:(id)sender
{
    COL80_autoswitch = 1 - COL80_autoswitch;
    [self setXEP80AutoswitchMenu:COL80_autoswitch];
}

/*------------------------------------------------------------------------------
*  xep80Mode - This method handles the normal/XEP80 menu selection.
*-----------------------------------------------------------------------------*/
- (IBAction)xep80Mode:(id)sender
{
    int modeChange = 1;
    
    if (AF80_enabled)
        modeChange = 2;
	switch ([sender tag]) {
		case 0:
			XEP80_enabled = FALSE;
            AF80_enabled = FALSE;
            BIT3_enabled = FALSE;
			if (PLATFORM_80col)
				PLATFORM_Switch80Col();
			break;
		case 1:
			XEP80_enabled = TRUE;
			XEP80_port = 0;
            AF80_enabled = FALSE;
            BIT3_enabled = FALSE;
			break;
        case 2:
            XEP80_enabled = TRUE;
            XEP80_port = 1;
            AF80_enabled = FALSE;
            BIT3_enabled = FALSE;
            break;
        case 3:
            XEP80_enabled = FALSE;
            AF80_enabled = TRUE;
            BIT3_enabled = FALSE;
            break;
        case 4:
            XEP80_enabled = FALSE;
            AF80_enabled = FALSE;
            BIT3_enabled = TRUE;
            break;
	}
    request80ColModeChange = modeChange;
    [self set80ColModeMenu:XEP80_enabled:XEP80_port:AF80_enabled:BIT3_enabled:PLATFORM_80col];
    [[MediaManager sharedInstance]  set80ColMode:XEP80_enabled:AF80_enabled:BIT3_enabled:PLATFORM_80col];
}

/*------------------------------------------------------------------------------
*  displayFps - This method handles the display frames per second menu 
*     selection.
*-----------------------------------------------------------------------------*/
- (IBAction)displayFps:(id)sender
{
    [self setFpsMenu:(1-Screen_show_atari_speed)];
    requestFpsChange = 1;
}

/*------------------------------------------------------------------------------
*  scaleMode - This method handles the normal/scanline/smooth scale mode menu
*       selection.
*-----------------------------------------------------------------------------*/
- (IBAction)scaleMode:(id)sender
{
    requestScaleModeChange = [sender tag];
}

/*------------------------------------------------------------------------------
*  displayFps - This method handles the grab mouse menu selection.
*-----------------------------------------------------------------------------*/
- (IBAction)grabMouse:(id)sender;
{
    requestGrabMouse = 1;
}

/*------------------------------------------------------------------------------
*  screenshot - This method handles the take screenshot menu selection.
*-----------------------------------------------------------------------------*/
- (IBAction)screenshot:(id)sender
{
    requestScreenshot = 1;
}

/*------------------------------------------------------------------------------
*  widthMode - This method handles the short/default/full width mode menu 
*       selection.
*-----------------------------------------------------------------------------*/
- (IBAction)widthMode:(id)sender
{
    requestWidthModeChange = [sender tag];
}

- (IBAction)artifactModeChange:(id)sender
{
	ANTIC_artif_mode = [sender tag];
	requestArtifChange = 1;
}

/*------------------------------------------------------------------------------
 *  paste - Starts Paste from Mac to Atari
 *-----------------------------------------------------------------------------*/
- (IBAction) paste:(id) sender
{
    requestPaste = 1;
}

/*------------------------------------------------------------------------------
 *  selectAll - Starts selectAll from Mac to Atari
 *-----------------------------------------------------------------------------*/
- (IBAction) selectAll:(id) sender
{
    requestSelectAll = 1;
}

/*------------------------------------------------------------------------------
 *  copy - Starts copy from Mac to Atari
 *-----------------------------------------------------------------------------*/
- (IBAction) copy:(id) sender
{
    requestCopy = 1;
}

@end
