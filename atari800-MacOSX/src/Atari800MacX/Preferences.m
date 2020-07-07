/* Preferences.m - Preferences window 
   class and support functions for the
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   
   Based on the Preferences pane of the
   TextEdit application.

*/
#import <Cocoa/Cocoa.h>
#import <SDL.h>
#import "Preferences.h"
#import "MediaManager.h"
#import "ControlManager.h"
#import "Atari800Window.h"
#import "KeyMapper.h"
#import "log.h"
#import "config.h"

#import <IOKit/IOKitLib.h>
#import <IOKit/serial/IOSerialKeys.h>
#import <IOKit/serial/ioss.h>
#import <IOKit/IOBSD.h>
#import <IOKit/IOCFPlugIn.h>
#import <IOKit/usb/IOUSBLib.h>
#import <mach/mach.h>

#define QZ_COMMA		0x2B
#define UI_MENU_SAVECFG          30
#define UI_MENU_LOADCFG          31
#define NUM_JOYSTICK_BUTTONS     24

extern SDL_Joystick *joystick0, *joystick1;
extern SDL_Joystick *joystick2, *joystick3;
extern int joystick0_nbuttons, joystick1_nbuttons;
extern int joystick2_nbuttons, joystick3_nbuttons;
extern int joystick0_nsticks, joystick0_nhats;
extern int joystick1_nsticks, joystick1_nhats;
extern int joystick2_nsticks, joystick2_nhats;
extern int joystick3_nsticks, joystick3_nhats;

extern void PauseAudio(int pause);
extern int requestPrefsChange;
extern int configurationChanged;
extern void Reinit_Joysticks(void);
extern void checkForNewJoysticks(void);

extern ATARI825_PREF prefs825;
extern ATARI1020_PREF prefs1020;
extern EPSON_PREF prefsEpson;
extern ATASCII_PREF prefsAtascii;
extern int FULLSCREEN;
extern int UI_alt_function;
extern int requestFullScreenUI;
extern int diskDriveSound;

extern char atari_config_dir[FILENAME_MAX];

static char workingDirectory[FILENAME_MAX], osromsDir[FILENAME_MAX], paletteDir[FILENAME_MAX];
static char imageDirStr[FILENAME_MAX],printDirStr[FILENAME_MAX];
static char hardDiskDir1Str[FILENAME_MAX], hardDiskDir2Str[FILENAME_MAX], hardDiskDir3Str[FILENAME_MAX];
static char hardDiskDir4Str[FILENAME_MAX], osARomFileStr[FILENAME_MAX], osBRomFileStr[FILENAME_MAX];
static char xlRomFileStr[FILENAME_MAX], basicRomFileStr[FILENAME_MAX], a5200RomFileStr[FILENAME_MAX];
static char diskImageDirStr[FILENAME_MAX],diskSetDirStr[FILENAME_MAX], cartImageDirStr[FILENAME_MAX], cassImageDirStr[FILENAME_MAX];
static char exeFileDirStr[FILENAME_MAX], savedStateDirStr[FILENAME_MAX], configDirStr[FILENAME_MAX];
static char paletteStr[FILENAME_MAX];

/* In version 4, an additional Atari type variable was introduced,
 so that the new types introduced in version 4 would not cause
 preference files to be incompatible with previous versions. 
 These arrays map the position of a machine type in a pulldown
 menu to the type variables and visa versa */
#define NUM_ORIG_TYPES	14
#define NUM_V4_TYPES	5
#define NUM_TOTAL_TYPES NUM_ORIG_TYPES+NUM_V4_TYPES

static int types[NUM_TOTAL_TYPES] =
{ 0, 1, 2, 0, 0, 3, 4, 5, 0, 0, 6, 7, 8, 0, 9,10,11,12,13};
static int v4types[NUM_TOTAL_TYPES] =
{-1,-1,-1, 0, 1,-1,-1,-1, 2, 3,-1,-1,-1, 4,-1,-1,-1,-1,-1};
static int indicies[NUM_TOTAL_TYPES] =
{0,1,2,5,6,7,10,11,12,14,15,16,17,18,3,4,8,9,13};
static int axlonBankMasks[] = {3,7,15,31,63,127,255,0};
static int mosaicBankMaxs[] = {3,19,35,0};

void RunPreferences() {

    [[Preferences sharedInstance] showPanel:[Preferences sharedInstance]];
    [[KeyMapper sharedInstance] releaseCmdKeys:@","];
}

void UpdatePreferencesJoysticks() {
    [[Preferences sharedInstance] updateJoyNumMenus];
}

void ReturnPreferences(struct ATARI800MACX_PREFSAVE *prefssave) {
    [[Preferences sharedInstance] transferValuesFromEmulator:prefssave];
} 

void SaveMedia(char disk_filename[][FILENAME_MAX], 
			   char cassette_filename[FILENAME_MAX],
			   char cart_filename[FILENAME_MAX],
			   char cart2_filename[FILENAME_MAX]) {
    [[Preferences sharedInstance] saveCurrentMedia:disk_filename:cassette_filename:cart_filename:cart2_filename];
}

void PreferencesSaveDefaults(void) {
    [[Preferences sharedInstance] saveDefaults];
}

int PreferencesTypeFromIndex(int index, int *ver4type) {
	return [[Preferences sharedInstance] typeFromIndex:index :ver4type];
}

void PreferencesSaveConfiguration() {
    [[Preferences sharedInstance] saveConfigurationMenu:nil];
}

void PreferencesSaveConfigurationUI(char *filename) {
    [[Preferences sharedInstance] saveConfigurationUI:filename];
}

void PreferencesLoadConfigurationUI(char *filename) {
    [[Preferences sharedInstance] loadConfigurationUI:filename];
}

void PreferencesLoadConfiguration() {
    [[Preferences sharedInstance] loadConfigurationMenu:nil];
}

void PreferencesIdentifyGamepadNew() {
    [[Preferences sharedInstance]
     identifyGamepadNew:0];
}

/*------------------------------------------------------------------------------
*  defaultValues - This method sets up the default values for the preferences
*-----------------------------------------------------------------------------*/
static NSDictionary *defaultValues() {
    static NSDictionary *dict = nil;
    
    strcpy(paletteStr, workingDirectory);
    strcat(paletteStr, "/Palettes/Real.act");    
    strcpy(paletteDir, workingDirectory);
    strcat(paletteDir, "/Palettes");    
    strcpy(imageDirStr, workingDirectory);
    strcpy(printDirStr, workingDirectory);
    strcpy(hardDiskDir1Str, workingDirectory);
    strcat(hardDiskDir1Str, "/HardDrive1");
    strcpy(hardDiskDir2Str, workingDirectory);
    strcat(hardDiskDir2Str, "/HardDrive2");
    strcpy(hardDiskDir3Str, workingDirectory);
    strcat(hardDiskDir3Str, "/HardDrive3");
    strcpy(hardDiskDir4Str, workingDirectory);
    strcat(hardDiskDir4Str, "/HardDrive4");
    strcpy(osromsDir, workingDirectory);
    strcat(osromsDir, "/OSRoms");
    strcpy(osARomFileStr, workingDirectory);
    strcat(osARomFileStr, "/OSRoms/atariosa.rom");
    strcpy(osBRomFileStr, workingDirectory);
    strcat(osBRomFileStr, "/OSRoms/atariosb.rom");
    strcpy(xlRomFileStr, workingDirectory);
    strcat(xlRomFileStr, "/OSRoms/atarixl.rom");
    strcpy(basicRomFileStr, workingDirectory);
    strcat(basicRomFileStr, "/OSRoms/ataribas.rom");
    strcpy(a5200RomFileStr, workingDirectory);
    strcat(a5200RomFileStr, "/OSRoms/a5200.rom");
    strcpy(diskImageDirStr, workingDirectory);
    strcat(diskImageDirStr, "/Disks");
    strcpy(diskSetDirStr, workingDirectory);
    strcat(diskSetDirStr, "/Disks/Sets");
    strcpy(cartImageDirStr, workingDirectory);
    strcat(cartImageDirStr, "/Carts");
    strcpy(cassImageDirStr, workingDirectory);
    strcat(cassImageDirStr, "/AtariExeFiles");
    strcpy(exeFileDirStr, workingDirectory);
    strcat(exeFileDirStr, "/AtariExeFiles");
    strcpy(savedStateDirStr, workingDirectory);
    strcat(savedStateDirStr, "/SavedState");
    strcpy(configDirStr, workingDirectory);
    
    if (!dict) {
        dict = [[NSDictionary alloc] initWithObjectsAndKeys:
                [NSNumber numberWithBool:NO], FullScreen, 
                [NSNumber numberWithBool:YES], OpenGl, 
                [NSNumber numberWithBool:YES], LockFullscreenSize, 
                [NSNumber numberWithBool:YES], FullscreenMonitor, 
                [NSNumber numberWithFloat:0.0],FullscreenForeRed,
                [NSNumber numberWithFloat:0.0],FullscreenForeBlue,
                [NSNumber numberWithFloat:0.8],FullscreenForeGreen,
                [NSNumber numberWithFloat:1.0],FullscreenForeAlpha,
                [NSNumber numberWithFloat:0.0],FullscreenBackRed,
                [NSNumber numberWithFloat:0.0],FullscreenBackBlue,
                [NSNumber numberWithFloat:0.0],FullscreenBackGreen,
                [NSNumber numberWithFloat:1.0],FullscreenBackAlpha,
                [NSNumber numberWithBool:NO], DoubleSize, 
                [NSNumber numberWithInt:0], ScaleMode, 
                [NSNumber numberWithInt:2], ScaleFactor, 
                [NSNumber numberWithInt:1], WidthMode, 
                [NSNumber numberWithInt:0], TvMode, 
                [NSNumber numberWithFloat:1.0], EmulationSpeed,
                [NSNumber numberWithInt:1], RefreshRatio, 
                [NSNumber numberWithInt:1], SpriteCollisions, 
                [NSNumber numberWithInt:0], ArtifactingMode, 
                [NSNumber numberWithBool:YES], ArtifactNew, 
                [NSNumber numberWithBool:NO], UseBuiltinPalette, 
                [NSNumber numberWithBool:YES], AdjustPalette,
                [NSNumber numberWithInt:0], BlackLevel, 
                [NSNumber numberWithInt:224], WhiteLevel, 
                [NSNumber numberWithInt:100], Intensity, 
                [NSNumber numberWithInt:40], ColorShift, 
                [NSString stringWithCString:paletteStr encoding:NSASCIIStringEncoding], PaletteFile, 
                [NSNumber numberWithBool:NO], ShowFPS,
                [NSNumber numberWithBool:YES], LedStatus,
                [NSNumber numberWithBool:YES], LedSector,
                [NSNumber numberWithBool:YES], LedStatusMedia,
                [NSNumber numberWithBool:YES], LedSectorMedia,
                [NSNumber numberWithBool:NO], XEP80Enabled,
                [NSNumber numberWithInt:0], XEP80Port,
                [NSNumber numberWithBool:NO], XEP80,
                [NSNumber numberWithInt:15], XEP80OnColor,
                [NSNumber numberWithInt:0], XEP80OffColor,
                [NSNumber numberWithInt:4], AtariType, 
                [NSNumber numberWithInt:-1], AtariTypeVer4, 
                [NSNumber numberWithInt:7], AtariSwitchType, 
                [NSNumber numberWithInt:-1], AtariSwitchTypeVer4, 
                [NSNumber numberWithInt:7],AxlonBankMask,
                [NSNumber numberWithInt:3],MosaicMaxBank,
                [NSNumber numberWithBool:NO],MioEnabled,
                [NSNumber numberWithBool:NO],BlackBoxEnabled,
                @"",MioRomFile,
                @"",BlackBoxRomFile,
                @"",BlackBoxScsiDiskFile,
                @"",MioScsiDiskFile,
                [NSNumber numberWithBool:YES], DisableBasic, 
                [NSNumber numberWithBool:NO], DisableAllBasic, 
                [NSNumber numberWithBool:YES], EnableSioPatch, 
                [NSNumber numberWithBool:YES], EnableHPatch,
                [NSNumber numberWithBool:NO], EnableDPatch,
                [NSNumber numberWithInt:0], UseAtariCursorKeys,
                @"open %s",PrintCommand,
                [NSNumber numberWithInt:0],PrinterType,
                [NSNumber numberWithInt:0],Atari825CharSet,
                [NSNumber numberWithInt:11],Atari825FormLength,
                [NSNumber numberWithBool:YES],Atari825AutoLinefeed,
                [NSNumber numberWithInt:0],Atari1020PrintWidth,
                [NSNumber numberWithInt:11],Atari1020FormLength,
                [NSNumber numberWithBool:YES],Atari1020AutoLinefeed,
                [NSNumber numberWithBool:YES],Atari1020AutoPageAdjust,
                [NSNumber numberWithFloat:0.0],Atari1020Pen1Red,
                [NSNumber numberWithFloat:0.0],Atari1020Pen1Blue,
                [NSNumber numberWithFloat:0.0],Atari1020Pen1Green,
                [NSNumber numberWithFloat:1.0],Atari1020Pen1Alpha,
                [NSNumber numberWithFloat:0.0],Atari1020Pen2Red,
                [NSNumber numberWithFloat:1.0],Atari1020Pen2Blue,
                [NSNumber numberWithFloat:0.0],Atari1020Pen2Green,
                [NSNumber numberWithFloat:1.0],Atari1020Pen2Alpha,
                [NSNumber numberWithFloat:0.0],Atari1020Pen3Red,
                [NSNumber numberWithFloat:0.0],Atari1020Pen3Blue,
                [NSNumber numberWithFloat:1.0],Atari1020Pen3Green,
                [NSNumber numberWithFloat:1.0],Atari1020Pen3Alpha,
                [NSNumber numberWithFloat:1.0],Atari1020Pen4Red,
                [NSNumber numberWithFloat:0.0],Atari1020Pen4Blue,
                [NSNumber numberWithFloat:0.0],Atari1020Pen4Green,
                [NSNumber numberWithFloat:1.0],Atari1020Pen4Alpha,
                [NSNumber numberWithInt:11],AtasciiFormLength,
                [NSNumber numberWithInt:0],AtasciiCharSet,
                [NSNumber numberWithInt:7],AtasciiCharSize,
                [NSNumber numberWithInt:0],AtasciiLineGap,
                [NSNumber numberWithInt:0],EpsonCharSet,
                [NSNumber numberWithInt:0],EpsonPrintPitch,
                [NSNumber numberWithInt:0],EpsonPrintWeight,
                [NSNumber numberWithInt:11],EpsonFormLength,
                [NSNumber numberWithBool:YES],EpsonAutoLinefeed,
                [NSNumber numberWithInt:NO],EpsonPrintSlashedZeros,
                [NSNumber numberWithInt:NO],EpsonAutoSkip,
                [NSNumber numberWithInt:NO],EpsonSplitSkip,
                [NSNumber numberWithBool:YES], EnablePPatch,
                [NSNumber numberWithBool:NO], EnableRPatch, 
                [NSNumber numberWithInt:8888], RPatchPort,
                [NSNumber numberWithBool:NO], RPatchSerialEnabled,
                @"", RPatchSerialPort,
                [NSNumber numberWithBool:NO], BootFromCassette, 
                [NSNumber numberWithBool:YES], SpeedLimit, 
                [NSNumber numberWithBool:YES], EnableSound, 
                [NSNumber numberWithFloat:1.0], SoundVolume, 
                [NSNumber numberWithBool:NO], EnableStereo, 
#if 0 /* enableHifiSound is deprecated from 4.2.2 on */    				
                [NSNumber numberWithBool:YES], EnableHifiSound, 
#endif	
#ifdef WORDS_BIGENDIAN				
                [NSNumber numberWithBool:NO], Enable16BitSound, 
#else
                [NSNumber numberWithBool:YES], Enable16BitSound, 
#endif
                [NSNumber numberWithBool:YES], EnableConsoleSound, 
                [NSNumber numberWithBool:YES], EnableSerioSound, 
                [NSNumber numberWithBool:NO], DontMuteAudio,
                [NSNumber numberWithBool:YES], DiskDriveSound,
                [NSNumber numberWithBool:NO], EnableInternational,
                [NSNumber numberWithBool:NO], EnableMultijoy, 
                [NSNumber numberWithBool:NO], IgnoreHeaderWriteprotect,
                [NSString stringWithCString:imageDirStr encoding:NSASCIIStringEncoding], ImageDir, 
                [NSString stringWithCString:printDirStr encoding:NSASCIIStringEncoding], PrintDir, 
                [NSString stringWithCString:hardDiskDir1Str encoding:NSASCIIStringEncoding], HardDiskDir1, 
                [NSString stringWithCString:hardDiskDir2Str encoding:NSASCIIStringEncoding], HardDiskDir2, 
                [NSString stringWithCString:hardDiskDir3Str encoding:NSASCIIStringEncoding], HardDiskDir3, 
                [NSString stringWithCString:hardDiskDir4Str encoding:NSASCIIStringEncoding], HardDiskDir4, 
                [NSNumber numberWithBool:YES], HardDrivesReadOnly, 
                @"H1:>DOS;>DOS",HPath,
                [NSString stringWithCString:osARomFileStr encoding:NSASCIIStringEncoding], OsARomFile, 
                [NSString stringWithCString:osBRomFileStr encoding:NSASCIIStringEncoding], OsBRomFile, 
                [NSString stringWithCString:xlRomFileStr encoding:NSASCIIStringEncoding], XlRomFile, 
                [NSString stringWithCString:basicRomFileStr encoding:NSASCIIStringEncoding], BasicRomFile, 
                [NSString stringWithCString:a5200RomFileStr encoding:NSASCIIStringEncoding], A5200RomFile, 
                [NSString stringWithCString:diskImageDirStr encoding:NSASCIIStringEncoding], DiskImageDir, 
                [NSString stringWithCString:diskSetDirStr encoding:NSASCIIStringEncoding], DiskSetDir, 
                [NSString stringWithCString:cartImageDirStr encoding:NSASCIIStringEncoding], CartImageDir, 
                [NSString stringWithCString:cassImageDirStr encoding:NSASCIIStringEncoding], CassImageDir, 
                [NSString stringWithCString:exeFileDirStr encoding:NSASCIIStringEncoding], ExeFileDir, 
                [NSString stringWithCString:savedStateDirStr encoding:NSASCIIStringEncoding], SavedStateDir, 
                [NSString stringWithCString:configDirStr encoding:NSASCIIStringEncoding], ConfigDir, 
                @"", D1File,
                @"", D2File,
                @"", D3File,
                @"", D4File,
                @"", D5File,
                @"", D6File,
                @"", D7File,
                @"", D8File,
                @"", CartFile,
                @"", Cart2File,
                @"", ExeFile,
                @"", CassFile,
                [NSNumber numberWithBool:NO], D1FileEnabled, 
                [NSNumber numberWithBool:NO], D2FileEnabled, 
                [NSNumber numberWithBool:NO], D3FileEnabled, 
                [NSNumber numberWithBool:NO], D4FileEnabled, 
                [NSNumber numberWithBool:NO], D5FileEnabled, 
                [NSNumber numberWithBool:NO], D6FileEnabled, 
                [NSNumber numberWithBool:NO], D7FileEnabled, 
                [NSNumber numberWithBool:NO], D8FileEnabled, 
                [NSNumber numberWithBool:NO], CartFileEnabled, 
                [NSNumber numberWithBool:NO], Cart2FileEnabled, 
                [NSNumber numberWithBool:NO], ExeFileEnabled,
                [NSNumber numberWithBool:NO], CassFileEnabled,
                [NSNumber numberWithInt:0], Joystick1Mode, 
                [NSNumber numberWithInt:0], Joystick2Mode, 
                [NSNumber numberWithInt:0], Joystick3Mode, 
                [NSNumber numberWithInt:0], Joystick4Mode, 
                [NSNumber numberWithBool:NO], Joystick1MultiMode, 
                [NSNumber numberWithBool:NO], Joystick2MultiMode, 
                [NSNumber numberWithBool:NO], Joystick3MultiMode, 
                [NSNumber numberWithBool:NO], Joystick4MultiMode, 
                [NSNumber numberWithInt:0], Joystick1Autofire, 
                [NSNumber numberWithInt:0], Joystick2Autofire, 
                [NSNumber numberWithInt:0], Joystick3Autofire, 
                [NSNumber numberWithInt:0], Joystick4Autofire, 
                [NSNumber numberWithInt:0], MouseDevice, 
                [NSNumber numberWithInt:3], MouseSpeed, 
                [NSNumber numberWithInt:0], MouseMinVal, 
                [NSNumber numberWithInt:228], MouseMaxVal, 
                [NSNumber numberWithInt:0], MouseHOffset, 
                [NSNumber numberWithInt:0], MouseVOffset, 
                [NSNumber numberWithInt:10], MouseInertia, 
                StandardConfigString, GamepadConfigCurrent,
                StandardConfigString, Gamepad1ConfigCurrent,
                StandardConfigString, Gamepad2ConfigCurrent,
                StandardConfigString, Gamepad3ConfigCurrent,
                StandardConfigString, Gamepad4ConfigCurrent,
                [NSMutableArray new], GamepadConfigArray,
                [NSMutableArray arrayWithObjects:[NSNumber numberWithInt:0], [NSNumber numberWithInt:0],
                [NSNumber numberWithInt:0],[NSNumber numberWithInt:0],[NSNumber numberWithInt:0],
                [NSNumber numberWithInt:0],[NSNumber numberWithInt:0],[NSNumber numberWithInt:0],
                [NSNumber numberWithInt:0],[NSNumber numberWithInt:0],[NSNumber numberWithInt:0],
                [NSNumber numberWithInt:0],[NSNumber numberWithInt:0],[NSNumber numberWithInt:0],
                [NSNumber numberWithInt:0],[NSNumber numberWithInt:0],[NSNumber numberWithInt:0],
                [NSNumber numberWithInt:0],[NSNumber numberWithInt:0],[NSNumber numberWithInt:0],
                [NSNumber numberWithInt:0],[NSNumber numberWithInt:0],[NSNumber numberWithInt:0],
                [NSNumber numberWithInt:0],nil], ButtonAssignment,
                [NSMutableArray arrayWithObjects:[NSNumber numberWithInt:0], [NSNumber numberWithInt:0],
                [NSNumber numberWithInt:0],[NSNumber numberWithInt:0],[NSNumber numberWithInt:0],
                [NSNumber numberWithInt:0],[NSNumber numberWithInt:0],[NSNumber numberWithInt:0],
                [NSNumber numberWithInt:0],[NSNumber numberWithInt:0],[NSNumber numberWithInt:0],
                [NSNumber numberWithInt:0],[NSNumber numberWithInt:0],[NSNumber numberWithInt:0],
                [NSNumber numberWithInt:0],[NSNumber numberWithInt:0],[NSNumber numberWithInt:0],
                [NSNumber numberWithInt:0],[NSNumber numberWithInt:0],[NSNumber numberWithInt:0],
                [NSNumber numberWithInt:0],[NSNumber numberWithInt:0],[NSNumber numberWithInt:0],
                [NSNumber numberWithInt:0],nil], Button5200Assignment,
                [NSNumber numberWithInt:0], Joystick1Type,
                [NSNumber numberWithInt:0], Joystick2Type,
                [NSNumber numberWithInt:0], Joystick3Type,
                [NSNumber numberWithInt:0], Joystick4Type,
                [NSNumber numberWithInt:0], Joystick1Num,
                [NSNumber numberWithInt:0], Joystick2Num,
                [NSNumber numberWithInt:0], Joystick3Num,
                [NSNumber numberWithInt:0], Joystick4Num,
                [NSNumber numberWithBool:NO], CX85Enabled,
                [NSNumber numberWithInt:1], CX85Port,
                [NSNumber numberWithInt:0], PaddlesXAxisOnly,
                [NSNumber numberWithInt:22], LeftJoyUp,
                [NSNumber numberWithInt:23], LeftJoyDown,
                [NSNumber numberWithInt:0], LeftJoyLeft,
                [NSNumber numberWithInt:3], LeftJoyRight,
                [NSNumber numberWithInt:16], LeftJoyUpLeft,
                [NSNumber numberWithInt:4], LeftJoyUpRight,
                [NSNumber numberWithInt:25], LeftJoyDownLeft,
                [NSNumber numberWithInt:2], LeftJoyDownRight,
                [NSNumber numberWithInt:40], LeftJoyFire,
                [NSNumber numberWithInt:48], LeftJoyAltFire,
                [NSNumber numberWithInt:70], PadJoyUp,
                [NSNumber numberWithInt:64], PadJoyDown,
                [NSNumber numberWithInt:66], PadJoyLeft,
                [NSNumber numberWithInt:68], PadJoyRight,
                [NSNumber numberWithInt:69], PadJoyUpLeft,
                [NSNumber numberWithInt:71], PadJoyUpRight,
                [NSNumber numberWithInt:63], PadJoyDownLeft,
                [NSNumber numberWithInt:65], PadJoyDownRight,
                [NSNumber numberWithInt:49], PadJoyFire,
                [NSNumber numberWithInt:62], PadJoyAltFire,
                [NSNumber numberWithBool:YES], MediaStatusDisplayed, 
                [NSNumber numberWithBool:NO], FunctionKeysDisplayed, 
                [NSNumber numberWithInt:0], MediaStatusX,
                [NSNumber numberWithInt:0], MediaStatusY,
                [NSNumber numberWithInt:0], MessagesX,
                [NSNumber numberWithInt:0], MessagesY,
                [NSNumber numberWithInt:0], MonitorX,
                [NSNumber numberWithInt:0], MonitorY,
                [NSNumber numberWithBool:NO], MonitorGUIVisable,
                [NSNumber numberWithInt:0], FunctionKeysX,
                [NSNumber numberWithInt:0], FunctionKeysY,
                [NSNumber numberWithInt:59999], ApplicationWindowX,
                [NSNumber numberWithInt:59999], ApplicationWindowY,
                [NSNumber numberWithBool:NO], SaveCurrentMedia,
                [NSNumber numberWithBool:YES], ClearCurrentMedia,
                [NSNumber numberWithBool:YES], KeyjoyEnable,
                nil];
    }
    return dict;
}

@implementation Preferences

static Preferences *sharedInstance = nil;

+ (Preferences *)sharedInstance {
    return sharedInstance ? sharedInstance : [[self alloc] init];
}

/* The next few factory methods are conveniences, working on the shared instance
*/
+ (id)objectForKey:(id)key {
    return [[[self sharedInstance] preferences] objectForKey:key];
}

+ (void)saveDefaults {
    [[self sharedInstance] saveDefaults];
}

/*------------------------------------------------------------------------------
*  setWorkingDirectory - Sets the working directory to the folder containing the
*     app.
*-----------------------------------------------------------------------------*/
+ (void)setWorkingDirectory:(char *)dir {
    char *c = workingDirectory;

    strncpy ( workingDirectory, dir, sizeof(workingDirectory) );
    
    while (*c != '\0')     /* go to end */
        c++;
    
    while (*c != '/')      /* back up to parent */
        c--;
    c--;
    while (*c != '/')      /* And three more times... */
        c--;
    c--;
    while (*c != '/')      
        c--;
    c--;
    while (*c != '/')      
        c--;
        
    *c = '\0';             /* cut off last part  */
    
    }

/*------------------------------------------------------------------------------
*  getWorkingDirectory - Gets the working directory which is the folder 
*     containing the app.
*-----------------------------------------------------------------------------*/
+ (char *)getWorkingDirectory {
	return(workingDirectory);
    }
        
/*------------------------------------------------------------------------------
*  saveDefaults - Called by the main app class to save the preferences when the
*     program exits.
*-----------------------------------------------------------------------------*/
- (void)saveDefaults {
    NSDictionary *prefs;
	NSPoint origin;
	BOOL guiVisable;

	// Save the window frames 
	origin = [[MediaManager sharedInstance] mediaStatusOriginSave];
	[displayedValues setObject:[NSNumber numberWithFloat:origin.x] forKey:MediaStatusX];
	[displayedValues setObject:[NSNumber numberWithFloat:origin.y] forKey:MediaStatusY];
	origin = [[ControlManager sharedInstance] messagesOriginSave];
	[displayedValues setObject:[NSNumber numberWithFloat:origin.x] forKey:MessagesX];
	[displayedValues setObject:[NSNumber numberWithFloat:origin.y] forKey:MessagesY];
	origin = [[ControlManager sharedInstance] functionKeysOriginSave];
	[displayedValues setObject:[NSNumber numberWithFloat:origin.x] forKey:FunctionKeysX];
	[displayedValues setObject:[NSNumber numberWithFloat:origin.y] forKey:FunctionKeysY];
	guiVisable = [[ControlManager sharedInstance] monitorGUIVisableSave];
	[displayedValues setObject:[NSNumber numberWithBool:guiVisable] forKey:MonitorGUIVisable];
	origin = [[ControlManager sharedInstance] monitorOriginSave];
	[displayedValues setObject:[NSNumber numberWithFloat:origin.x] forKey:MonitorX];
	if (guiVisable)
		origin.y -= kGraphicalDrawerSize;
	[displayedValues setObject:[NSNumber numberWithFloat:origin.y] forKey:MonitorY];
	origin = [Atari800Window applicationWindowOriginSave];
	[displayedValues setObject:[NSNumber numberWithFloat:origin.x] forKey:ApplicationWindowX];
	[displayedValues setObject:[NSNumber numberWithFloat:origin.y] forKey:ApplicationWindowY];


	// Get the changed prefs back from emulator
	savePrefs();
	[self commitDisplayedValues];
	prefs = [self preferences];
	
    if (![origValues isEqual:prefs]) [Preferences savePreferencesToDefaults:prefs];
}

/*------------------------------------------------------------------------------
*  Constructor
*-----------------------------------------------------------------------------*/
- (id)init {
    if (sharedInstance) {
	[self dealloc];
    } else {
        [super init];
        curValues = [[[self class] preferencesFromDefaults] copyWithZone:[self zone]];
        origValues = [curValues retain];
        [self transferValuesToEmulator];
        [self transferValuesToAtari825];
        [self transferValuesToAtari1020];
        [self transferValuesToAtascii];
        [self transferValuesToEpson];
        commitPrefs();
        [self discardDisplayedValues];
        sharedInstance = self;
		modems = [NSMutableArray array];
		[modems retain];
    }
    return sharedInstance;
}

/*------------------------------------------------------------------------------
*  Destructor
*-----------------------------------------------------------------------------*/
- (void)dealloc {
	[super dealloc];
}

/*------------------------------------------------------------------------------
* preferences - Method to return pointer to current preferences.
*-----------------------------------------------------------------------------*/
- (NSDictionary *)preferences {
    return curValues;
}

/*------------------------------------------------------------------------------
* preferences - Method to return current brushed steel state.
*-----------------------------------------------------------------------------*/
- (int)getBrushed {
    // Starting in version 5.0 of Atari800MacX, we no longer support
    // brushed metal.
    return 0;
}

/*------------------------------------------------------------------------------
* showPanel - Method to display the preferences window.
*-----------------------------------------------------------------------------*/
- (void)showPanel:(id)sender {
    NSMutableArray *configArray;
    NSArray *top;
    int i,numberGamepadConfigs;
    int currNumConfigs;
  
    if (FULLSCREEN)
        return;
	/* Transfer the changed prefs values back from emulator */
	savePrefs();
	[self commitDisplayedValues];
	[self generateModemList];
	[self updateUI];
	[self updateJoyNumMenus];
	
    PauseAudio(1);

    if (!prefTabView) {
        if (![[NSBundle mainBundle] loadNibNamed:@"Preferences" owner:self topLevelObjects:&top])  {
				NSLog(@"Failed to load Preferences.nib");
				NSBeep();
				return;
		}
    [top retain];
	[[prefTabView window] setExcludedFromWindowsMenu:YES];
	[[prefTabView window] setMenu:nil];
	[[gamepadButton1 window] setExcludedFromWindowsMenu:YES];
	[[gamepadButton1 window] setMenu:nil];
	[[errorOKButton window] setExcludedFromWindowsMenu:YES];
	[[errorOKButton window] setMenu:nil];
	[[configNameField window] setExcludedFromWindowsMenu:YES];
	[[configNameField window] setMenu:nil];
	[[leftJoyUpPulldown window] setExcludedFromWindowsMenu:YES];
	[[leftJoyUpPulldown window] setMenu:nil];
	[[padJoyUpPulldown window] setExcludedFromWindowsMenu:YES];
	[[padJoyUpPulldown window] setMenu:nil];
        [self updateUI];
        [self miscChanged:self];
        [self gamepadButtonChange:self];
        [[prefTabView window] center];
        [[gamepadButton1 window] center];
        [[errorOKButton window] center];
        [[configNameField window] center];
        [[leftJoyUpPulldown window] center];
        [[padJoyUpPulldown window] center];
        
        /* Get the current gamepad config and values */
        configArray = [curValues objectForKey:GamepadConfigArray];
        numberGamepadConfigs = [configArray count];
        for (i=0;i<numberGamepadConfigs;i++) {
            [gamepadConfigPulldown insertItemWithTitle:[configArray objectAtIndex:i] atIndex: (2+i)];
            }
        for (i=0;i<numberGamepadConfigs;i++) {
            [gamepad1ConfigPulldown insertItemWithTitle:[configArray objectAtIndex:i] atIndex: (1+i)];
            }
        for (i=0;i<numberGamepadConfigs;i++) {
            [gamepad2ConfigPulldown insertItemWithTitle:[configArray objectAtIndex:i] atIndex: (1+i)];
            }
        for (i=0;i<numberGamepadConfigs;i++) {
            [gamepad3ConfigPulldown insertItemWithTitle:[configArray objectAtIndex:i] atIndex: (1+i)];
            }
        for (i=0;i<numberGamepadConfigs;i++) {
            [gamepad4ConfigPulldown insertItemWithTitle:[configArray objectAtIndex:i] atIndex: (1+i)];
            }
        [gamepadConfigPulldown selectItemWithTitle:[curValues objectForKey:GamepadConfigCurrent]];
        [gamepad1ConfigPulldown selectItemWithTitle:[curValues objectForKey:Gamepad1ConfigCurrent]];
        [gamepad2ConfigPulldown selectItemWithTitle:[curValues objectForKey:Gamepad2ConfigCurrent]];
        [gamepad3ConfigPulldown selectItemWithTitle:[curValues objectForKey:Gamepad3ConfigCurrent]];
        [gamepad4ConfigPulldown selectItemWithTitle:[curValues objectForKey:Gamepad4ConfigCurrent]];
        [[gamepadConfigPulldown menu] setAutoenablesItems:NO];
        /* If we have default selected, then turn off Save, Rename, and Delete */
        if ([gamepadConfigPulldown indexOfSelectedItem] == 0) {
            currNumConfigs = [[curValues objectForKey:GamepadConfigArray] count];
            [[gamepadConfigPulldown itemAtIndex:(3+currNumConfigs)] setEnabled:NO];
            [[gamepadConfigPulldown itemAtIndex:(5+currNumConfigs)] setEnabled:NO];
            [[gamepadConfigPulldown itemAtIndex:(6+currNumConfigs)] setEnabled:NO];
            }

    }
    
    [[joystick1TypePulldown menu] setAutoenablesItems:NO];
    [[joystick1NumPulldown menu] setAutoenablesItems:NO];
    [[joystick1Pulldown menu] setAutoenablesItems:NO];
    [[joystick2TypePulldown menu] setAutoenablesItems:NO];
    [[joystick2Pulldown menu] setAutoenablesItems:NO];
    [[joystick2NumPulldown menu] setAutoenablesItems:NO];
    [[joystick3TypePulldown menu] setAutoenablesItems:NO];
    [[joystick3NumPulldown menu] setAutoenablesItems:NO];
    [[joystick3Pulldown menu] setAutoenablesItems:NO];
    [[joystick4TypePulldown menu] setAutoenablesItems:NO];
    [[joystick4NumPulldown menu] setAutoenablesItems:NO];
    [[joystick4Pulldown menu] setAutoenablesItems:NO];

    theTopTimer = [NSTimer timerWithTimeInterval: 0.1
                                      target: self
                                    selector: @selector(checkNewGamepads:)
                                    userInfo: nil
                                     repeats:YES];

    [[NSRunLoop currentRunLoop] addTimer:theTopTimer  forMode: NSModalPanelRunLoopMode];
    
    [NSApp runModalForWindow:[prefTabView window]];
}


/*------------------------------------------------------------------------------
* updateUI - Method to update the display, based on the stored values.
*-----------------------------------------------------------------------------*/
- (void)updateUI {
    int index;
    int i,j,foundMatch;
    NSString *portName;

    if (!prefTabView) return;	/* UI hasn't been loaded... */

    [fullScreenMatrix selectCellWithTag:[[displayedValues objectForKey:FullScreen] boolValue] ? 1 : 0];
    [openglMatrix selectCellWithTag:[[displayedValues objectForKey:OpenGl] boolValue] ? 1 : 0];
    [openglMatrix setEnabled:NO];
    [lockFullscreenSizeButton setState:[[displayedValues objectForKey:LockFullscreenSize] boolValue] ? NSOnState : NSOffState];
    [fullscreenMonitorButton setState:[[displayedValues objectForKey:FullscreenMonitor] boolValue] ? NSOnState : NSOffState];
    [fullscreenForegroundRed setIntValue:[[displayedValues objectForKey:FullscreenForeRed] floatValue]*255];
    [fullscreenForegroundGreen setIntValue:[[displayedValues objectForKey:FullscreenForeGreen] floatValue]*255];
    [fullscreenForegroundBlue setIntValue:[[displayedValues objectForKey:FullscreenForeBlue] floatValue]*255];
    [fullscreenBackgroundRed setIntValue:[[displayedValues objectForKey:FullscreenBackRed] floatValue]*255];
    [fullscreenBackgroundGreen setIntValue:[[displayedValues objectForKey:FullscreenBackGreen] floatValue]*255];
    [fullscreenBackgroundBlue setIntValue:[[displayedValues objectForKey:FullscreenBackBlue] floatValue]*255];
	if ([[displayedValues objectForKey:DoubleSize] boolValue] == 0)
		[scaleFactorMatrix  selectCellWithTag:0];
	else
		[scaleFactorMatrix  selectCellWithTag:[[displayedValues objectForKey:ScaleFactor] intValue]];
    [scaleModeMatrix  selectCellWithTag:[[displayedValues objectForKey:ScaleMode] intValue]];
    [widthModeMatrix  selectCellWithTag:[[displayedValues objectForKey:WidthMode] intValue]];
    [tvModeMatrix  selectCellWithTag:[[displayedValues objectForKey:TvMode] intValue]];
    [spriteCollisionsButton setState:[[displayedValues objectForKey:SpriteCollisions] boolValue] ? NSOnState : NSOffState];
    index = [[displayedValues objectForKey:RefreshRatio] intValue] - 1;
    [refreshRatioPulldown  selectItemAtIndex:index];
    [artifactingPulldown  selectItemAtIndex:[[displayedValues objectForKey:ArtifactingMode] intValue]];
    [artifactNewButton setState:[[displayedValues objectForKey:ArtifactNew] boolValue] ? NSOnState : NSOffState];
    [blackLevelField setIntValue:[[displayedValues objectForKey:BlackLevel] intValue]];
    [whiteLevelField setIntValue:[[displayedValues objectForKey:WhiteLevel] intValue]];
    [intensityField setIntValue:[[displayedValues objectForKey:Intensity] intValue]];
    [colorShiftField setIntValue:[[displayedValues objectForKey:ColorShift] intValue]];
    [paletteField setStringValue:[displayedValues objectForKey:PaletteFile]];
    [externalPaletteButton setState:[[displayedValues objectForKey:UseBuiltinPalette] boolValue] ? NSOffState : NSOnState];
    [adjustPaletteButton setState:[[displayedValues objectForKey:AdjustPalette] boolValue] ? NSOnState : NSOffState];
    [fpsButton setState:[[displayedValues objectForKey:ShowFPS] boolValue] ? NSOnState : NSOffState];
    [ledStatusButton setState:[[displayedValues objectForKey:LedStatus] boolValue] ? NSOnState : NSOffState];
    [ledSectorButton setState:[[displayedValues objectForKey:LedSector] boolValue] ? NSOnState : NSOffState];
    [ledStatusMediaButton setState:[[displayedValues objectForKey:LedStatusMedia] boolValue] ? NSOnState : NSOffState];
    [ledSectorMediaButton setState:[[displayedValues objectForKey:LedSectorMedia] boolValue] ? NSOnState : NSOffState];

    [atariTypePulldown  selectItemAtIndex:[self indexFromType:[[displayedValues objectForKey:AtariType] intValue] :
										   [[displayedValues objectForKey:AtariTypeVer4] intValue]]];
    [atariSwitchTypePulldown  selectItemAtIndex:[self indexFromType:[[displayedValues objectForKey:AtariSwitchType] intValue] :
										   [[displayedValues objectForKey:AtariSwitchTypeVer4] intValue]]];
    [disableBasicButton setState:[[displayedValues objectForKey:DisableBasic] boolValue] ? NSOnState : NSOffState];
    [disableAllBasicButton setState:[[displayedValues objectForKey:DisableAllBasic] boolValue] ? NSOnState : NSOffState];
	[emulationSpeedSlider setFloatValue:[[displayedValues objectForKey:EmulationSpeed] floatValue]];
    [enableSioPatchButton setState:[[displayedValues objectForKey:EnableSioPatch] boolValue] ? NSOnState : NSOffState];
    [enableHPatchButton setState:[[displayedValues objectForKey:EnableHPatch] boolValue] ? NSOnState : NSOffState];
    [enableDPatchButton setState:[[displayedValues objectForKey:EnableDPatch] boolValue] ? NSOnState : NSOffState];
    [enablePPatchButton setState:[[displayedValues objectForKey:EnablePPatch] boolValue] ? NSOnState : NSOffState];
    [enableRPatchButton setState:[[displayedValues objectForKey:EnableRPatch] boolValue] ? NSOnState : NSOffState];
    [rPatchPortField setStringValue:[displayedValues objectForKey:RPatchPort]];
	portName = [displayedValues objectForKey:RPatchSerialPort];	
	if ([[displayedValues objectForKey:RPatchSerialEnabled] boolValue] == YES) {
		[rPatchSerialMatrix selectCellWithTag:0];
  }
	else {
		[rPatchSerialMatrix selectCellWithTag:1];
  }
	
	[rPatchSerialPulldown removeAllItems];
	[rPatchSerialPulldown addItemWithTitle:@"No Connection"];
  if ([portName isEqual:@""]) {
		[rPatchSerialPulldown selectItemAtIndex:0];
  }
	for (j=0;j<[modems count];j++) {
		NSString *modem = [modems objectAtIndex:j];
		NSString *portSubName;

		[rPatchSerialPulldown addItemWithTitle:modem];
		
		if ([portName length] != 0) {
			portSubName = [portName substringFromIndex:([portName length] - [modem length])];
			if ([modem isEqual:portSubName]) {
				[rPatchSerialPulldown  selectItemAtIndex:j+1];
			}
		}
	}
	
	[useAtariCursorKeysPulldown selectItemAtIndex:[[displayedValues objectForKey:UseAtariCursorKeys] intValue]];
	
    [printCommandField setStringValue:[displayedValues objectForKey:PrintCommand]];
    [bootFromCassetteButton setState:[[displayedValues objectForKey:BootFromCassette] boolValue] ? NSOnState : NSOffState];
    [speedLimitButton setState:[[displayedValues objectForKey:SpeedLimit] boolValue] ? NSOnState : NSOffState];
    
	[xep80EnabledButton setState:[[displayedValues objectForKey:XEP80Enabled] boolValue] ? NSOnState : NSOffState];
	[xep80PortPulldown selectItemAtIndex:[[displayedValues objectForKey:XEP80Port] intValue]];
    [xep80ForegroundField setIntValue:[[displayedValues objectForKey:XEP80OnColor] intValue]];
    [xep80BackgroundField setIntValue:[[displayedValues objectForKey:XEP80OffColor] intValue]];
	[enableSoundButton setState:[[displayedValues objectForKey:EnableSound] boolValue] ? NSOnState : NSOffState];
#if 0 /* enableHifiSound is deprecated from 4.2.2 on */    
    [enableHifiSoundButton setState:[[displayedValues objectForKey:EnableHifiSound] boolValue] ? NSOnState : NSOffState];
#endif
#ifdef WORDS_BIGENDIAN
	[enable16BitSoundPulldown setEnabled:NO];
	[enable16BitSoundPulldown selectItemAtIndex:1];
#else	
	if ([[displayedValues objectForKey:Enable16BitSound] boolValue])
		[enable16BitSoundPulldown selectItemAtIndex:0];
	else
		[enable16BitSoundPulldown selectItemAtIndex:1];
#endif
    [consoleSoundEnableButton setState:[[displayedValues objectForKey:EnableConsoleSound] boolValue] ? NSOnState : NSOffState];
    [serioSoundEnableButton setState:[[displayedValues objectForKey:EnableSerioSound] boolValue] ? NSOnState : NSOffState];
    [muteAudioButton setState:[[displayedValues objectForKey:DontMuteAudio] boolValue] ? NSOffState : NSOnState];
    [diskDriveSoundButton setState:[[displayedValues objectForKey:DiskDriveSound] boolValue] ? NSOnState : NSOffState];
    [internationalKeyboardButton setState:[[displayedValues objectForKey:EnableInternational] boolValue] ? NSOnState : NSOffState];
    [internationalKeyboardButton setEnabled:NO]; // Always enabled on libSDL 2.0
    [enableMultijoyButton setState:[[displayedValues objectForKey:EnableMultijoy] boolValue] ? NSOnState : NSOffState];
    [ignoreHeaderWriteprotectButton setState:[[displayedValues objectForKey:IgnoreHeaderWriteprotect] boolValue] ? NSOnState : NSOffState];
	foundMatch = FALSE;
	for (i=0;axlonBankMasks[i] != 0;i++) {
		if (axlonBankMasks[i] == [[displayedValues objectForKey:AxlonBankMask] intValue]) {
			foundMatch = TRUE;
			[axlonMemSizePulldown selectItemAtIndex:i];
		}
	}
	if (!foundMatch)
		[axlonMemSizePulldown selectItemAtIndex:0];
	foundMatch = FALSE;
	for (i=0;mosaicBankMaxs[i] != 0;i++) {
		if (mosaicBankMaxs[i] == [[displayedValues objectForKey:MosaicMaxBank] intValue]) { 
			foundMatch = TRUE;
			[mosaicMemSizePulldown selectItemAtIndex:i];
		}
	}
	if (!foundMatch)
		[mosaicMemSizePulldown selectItemAtIndex:0];
	if ([[displayedValues objectForKey:BlackBoxEnabled] boolValue] == YES)
		[pbiExpansionMatrix selectCellWithTag:1];
	else if ([[displayedValues objectForKey:MioEnabled] boolValue] == YES)
		[pbiExpansionMatrix selectCellWithTag:2];
	else
		[pbiExpansionMatrix selectCellWithTag:0];		
    [blackBoxRomFileField setStringValue:[displayedValues objectForKey:BlackBoxRomFile]];
    [mioRomFileField setStringValue:[displayedValues objectForKey:MioRomFile]];
    [blackBoxScsiDiskFileField setStringValue:[displayedValues objectForKey:BlackBoxScsiDiskFile]];
    [mioScsiDiskFileField setStringValue:[displayedValues objectForKey:MioScsiDiskFile]];
	
    [imageDirField setStringValue:[displayedValues objectForKey:ImageDir]];
    [printDirField setStringValue:[displayedValues objectForKey:PrintDir]];
    [hardDiskDir1Field setStringValue:[displayedValues objectForKey:HardDiskDir1]];
    [hardDiskDir2Field setStringValue:[displayedValues objectForKey:HardDiskDir2]];
    [hardDiskDir3Field setStringValue:[displayedValues objectForKey:HardDiskDir3]];
    [hardDiskDir4Field setStringValue:[displayedValues objectForKey:HardDiskDir4]];
    [hardDrivesReadOnlyButton setState:[[displayedValues objectForKey:HardDrivesReadOnly] boolValue] ? NSOnState : NSOffState];
    [hPathField setStringValue:[displayedValues objectForKey:HPath]];

    [osARomFileField setStringValue:[displayedValues objectForKey:OsARomFile]];
    [osBRomFileField setStringValue:[displayedValues objectForKey:OsBRomFile]];
    [xlRomFileField setStringValue:[displayedValues objectForKey:XlRomFile]];
    [basicRomFileField setStringValue:[displayedValues objectForKey:BasicRomFile]];
    [a5200RomFileField setStringValue:[displayedValues objectForKey:A5200RomFile]];

    [diskImageDirField setStringValue:[displayedValues objectForKey:DiskImageDir]];
    [diskSetDirField setStringValue:[displayedValues objectForKey:DiskSetDir]];
    [cartImageDirField setStringValue:[displayedValues objectForKey:CartImageDir]];
    [cassImageDirField setStringValue:[displayedValues objectForKey:CassImageDir]];
    [exeFileDirField setStringValue:[displayedValues objectForKey:ExeFileDir]];
    [savedStateDirField setStringValue:[displayedValues objectForKey:SavedStateDir]];
    [configDirField setStringValue:[displayedValues objectForKey:ConfigDir]];
   
    [d1FileField setStringValue:[displayedValues objectForKey:D1File]];
    [d2FileField setStringValue:[displayedValues objectForKey:D2File]];
    [d3FileField setStringValue:[displayedValues objectForKey:D3File]];
    [d4FileField setStringValue:[displayedValues objectForKey:D4File]];
    [d5FileField setStringValue:[displayedValues objectForKey:D5File]];
    [d6FileField setStringValue:[displayedValues objectForKey:D6File]];
    [d7FileField setStringValue:[displayedValues objectForKey:D7File]];
    [d8FileField setStringValue:[displayedValues objectForKey:D8File]];
    [cartFileField setStringValue:[displayedValues objectForKey:CartFile]];
    [cart2FileField setStringValue:[displayedValues objectForKey:Cart2File]];
    [exeFileField setStringValue:[displayedValues objectForKey:ExeFile]];
    [cassFileField setStringValue:[displayedValues objectForKey:CassFile]];
    [d1FileEnabledButton setState:[[displayedValues objectForKey:D1FileEnabled] boolValue] ? NSOnState : NSOffState];
    [d2FileEnabledButton setState:[[displayedValues objectForKey:D2FileEnabled] boolValue] ? NSOnState : NSOffState];
    [d3FileEnabledButton setState:[[displayedValues objectForKey:D3FileEnabled] boolValue] ? NSOnState : NSOffState];
    [d4FileEnabledButton setState:[[displayedValues objectForKey:D4FileEnabled] boolValue] ? NSOnState : NSOffState];
    [d5FileEnabledButton setState:[[displayedValues objectForKey:D5FileEnabled] boolValue] ? NSOnState : NSOffState];
    [d6FileEnabledButton setState:[[displayedValues objectForKey:D6FileEnabled] boolValue] ? NSOnState : NSOffState];
    [d7FileEnabledButton setState:[[displayedValues objectForKey:D7FileEnabled] boolValue] ? NSOnState : NSOffState];
    [d8FileEnabledButton setState:[[displayedValues objectForKey:D8FileEnabled] boolValue] ? NSOnState : NSOffState];
    [cartFileEnabledButton setState:[[displayedValues objectForKey:CartFileEnabled] boolValue] ? NSOnState : NSOffState];
    [cart2FileEnabledButton setState:[[displayedValues objectForKey:Cart2FileEnabled] boolValue] ? NSOnState : NSOffState];
    [exeFileEnabledButton setState:[[displayedValues objectForKey:ExeFileEnabled] boolValue] ? NSOnState : NSOffState];
    [cassFileEnabledButton setState:[[displayedValues objectForKey:CassFileEnabled] boolValue] ? NSOnState : NSOffState];
    [saveCurrentMediaButton setState:[[displayedValues objectForKey:SaveCurrentMedia] boolValue] ? NSOnState : NSOffState];
    [clearCurrentMediaButton setState:[[displayedValues objectForKey:ClearCurrentMedia] boolValue] ? NSOnState : NSOffState];
	[self setBootMediaActive:![[displayedValues objectForKey:SaveCurrentMedia] boolValue]];

	[joystick1Pulldown  selectItemAtIndex:[[displayedValues objectForKey:Joystick1Mode] intValue]];
	[joystick2Pulldown  selectItemAtIndex:[[displayedValues objectForKey:Joystick2Mode] intValue]];
	[joystick3Pulldown  selectItemAtIndex:[[displayedValues objectForKey:Joystick3Mode] intValue]];
	[joystick4Pulldown  selectItemAtIndex:[[displayedValues objectForKey:Joystick4Mode] intValue]];
    [joy1AutofirePulldown  selectItemAtIndex:[[displayedValues objectForKey:Joystick1Autofire] intValue]];
    [joy2AutofirePulldown  selectItemAtIndex:[[displayedValues objectForKey:Joystick2Autofire] intValue]];
    [joy3AutofirePulldown  selectItemAtIndex:[[displayedValues objectForKey:Joystick3Autofire] intValue]];
    [joy4AutofirePulldown  selectItemAtIndex:[[displayedValues objectForKey:Joystick4Autofire] intValue]];
    [mouseDevicePulldown  selectItemAtIndex:[[displayedValues objectForKey:MouseDevice] intValue]];
    [mouseSpeedField setIntValue:[[displayedValues objectForKey:MouseSpeed] intValue]];
    [mouseMinValField setIntValue:[[displayedValues objectForKey:MouseMinVal] intValue]];
    [mouseMaxValField setIntValue:[[displayedValues objectForKey:MouseMaxVal] intValue]];
    [mouseHOffsetField setIntValue:[[displayedValues objectForKey:MouseHOffset] intValue]];
    [mouseVOffsetField setIntValue:[[displayedValues objectForKey:MouseVOffset] intValue]];
    [mouseInertiaField setIntValue:[[displayedValues objectForKey:MouseInertia] intValue]];

    [joystick1TypePulldown  selectItemAtIndex:[[displayedValues objectForKey:Joystick1Type] intValue]];
    [joystick2TypePulldown  selectItemAtIndex:[[displayedValues objectForKey:Joystick2Type] intValue]];
    [joystick3TypePulldown  selectItemAtIndex:[[displayedValues objectForKey:Joystick3Type] intValue]];
    [joystick4TypePulldown  selectItemAtIndex:[[displayedValues objectForKey:Joystick4Type] intValue]];
    [self updateJoyNumMenus];
	if ([[displayedValues objectForKey:Joystick1MultiMode] boolValue] == NO)
		[joystick1NumPulldown  selectItemAtIndex:[[displayedValues objectForKey:Joystick1Num] intValue]];
	else
		[joystick1NumPulldown  selectItemAtIndex:3];
	if ([[displayedValues objectForKey:Joystick2MultiMode] boolValue] == NO)
		[joystick2NumPulldown  selectItemAtIndex:[[displayedValues objectForKey:Joystick2Num] intValue]];
	else
		[joystick2NumPulldown  selectItemAtIndex:3];
	if ([[displayedValues objectForKey:Joystick3MultiMode] boolValue] == NO)
		[joystick3NumPulldown  selectItemAtIndex:[[displayedValues objectForKey:Joystick3Num] intValue]];
	else
		[joystick3NumPulldown  selectItemAtIndex:3];
	if ([[displayedValues objectForKey:Joystick4MultiMode] boolValue] == NO)
		[joystick4NumPulldown  selectItemAtIndex:[[displayedValues objectForKey:Joystick4Num] intValue]];
	else
		[joystick4NumPulldown  selectItemAtIndex:3];
    [paddlesXAxisOnlyButton setState:[[displayedValues objectForKey:PaddlesXAxisOnly] boolValue] ? NSOnState : NSOffState];
    [cx85EnabledButton setState:[[displayedValues objectForKey:CX85Enabled] boolValue] ? NSOnState : NSOffState];
	[cx85PortPulldown selectItemAtIndex:[[displayedValues objectForKey:CX85Port] intValue]];

    [gamepad1ConfigPulldown selectItemWithTitle:[curValues objectForKey:Gamepad1ConfigCurrent]];
    [gamepad2ConfigPulldown selectItemWithTitle:[curValues objectForKey:Gamepad2ConfigCurrent]];
    [gamepad3ConfigPulldown selectItemWithTitle:[curValues objectForKey:Gamepad3ConfigCurrent]];
    [gamepad4ConfigPulldown selectItemWithTitle:[curValues objectForKey:Gamepad4ConfigCurrent]];
	
	[printerTypePulldown selectItemAtIndex:[[displayedValues objectForKey:PrinterType] intValue]];
	[printerTabView selectTabViewItemAtIndex:[[displayedValues objectForKey:PrinterType] intValue]];
	[atari825CharSetPulldown selectItemAtIndex:[[displayedValues objectForKey:Atari825CharSet] intValue]];
	[atari825FormLengthField setIntValue:[[displayedValues objectForKey:Atari825FormLength] intValue]];
	[atari825FormLengthStepper setIntValue:[[displayedValues objectForKey:Atari825FormLength] intValue]];
	[atari825AutoLinefeedButton setState:[[displayedValues objectForKey:Atari825AutoLinefeed] boolValue] ? NSOnState : NSOffState];
	[atari1020PrintWidthPulldown selectItemAtIndex:[[displayedValues objectForKey:Atari1020PrintWidth] intValue]];
	[atari1020FormLengthField setIntValue:[[displayedValues objectForKey:Atari1020FormLength] intValue]];
	[atari1020FormLengthStepper setIntValue:[[displayedValues objectForKey:Atari1020FormLength] intValue]];
	[atari1020AutoLinefeedButton setState:[[displayedValues objectForKey:Atari1020AutoLinefeed] boolValue] ? NSOnState : NSOffState];
	[atari1020AutoPageAdjustButton setState:[[displayedValues objectForKey:Atari1020AutoPageAdjust] boolValue] ? NSOnState : NSOffState];
    [atari1020Pen1Red setIntValue:[[displayedValues objectForKey:Atari1020Pen1Red] floatValue]*255];
    [atari1020Pen1Green setIntValue:[[displayedValues objectForKey:Atari1020Pen1Green] floatValue]*255];
    [atari1020Pen1Blue setIntValue:[[displayedValues objectForKey:Atari1020Pen1Blue] floatValue]*255];
    [atari1020Pen2Red setIntValue:[[displayedValues objectForKey:Atari1020Pen2Red] floatValue]*255];
    [atari1020Pen2Green setIntValue:[[displayedValues objectForKey:Atari1020Pen2Green] floatValue]*255];
    [atari1020Pen2Blue setIntValue:[[displayedValues objectForKey:Atari1020Pen2Blue] floatValue]*255];
    [atari1020Pen3Red setIntValue:[[displayedValues objectForKey:Atari1020Pen3Red] floatValue]*255];
    [atari1020Pen3Green setIntValue:[[displayedValues objectForKey:Atari1020Pen3Green] floatValue]*255];
    [atari1020Pen3Blue setIntValue:[[displayedValues objectForKey:Atari1020Pen3Blue] floatValue]*255];
    [atari1020Pen4Red setIntValue:[[displayedValues objectForKey:Atari1020Pen4Red] floatValue]*255];
    [atari1020Pen4Green setIntValue:[[displayedValues objectForKey:Atari1020Pen4Green] floatValue]*255];
    [atari1020Pen4Blue setIntValue:[[displayedValues objectForKey:Atari1020Pen4Blue] floatValue]*255];
    [atasciiFormLengthField setIntValue:[[displayedValues objectForKey:AtasciiFormLength] intValue]];
    [atasciiFormLengthStepper setIntValue:[[displayedValues objectForKey:AtasciiFormLength] intValue]];
    [atasciiCharSetPulldown selectItemAtIndex:[[displayedValues objectForKey:AtasciiCharSet] intValue]];
    [atasciiCharSizeField setIntValue:[[displayedValues objectForKey:AtasciiCharSize] intValue]];
    [atasciiCharSizeStepper setIntValue:[[displayedValues objectForKey:AtasciiCharSize] intValue]];
    [atasciiLineGapField setIntValue:[[displayedValues objectForKey:AtasciiLineGap] intValue]];
    [atasciiLineGapStepper setIntValue:[[displayedValues objectForKey:AtasciiLineGap] intValue]];
	[epsonCharSetPulldown selectItemAtIndex:[[displayedValues objectForKey:EpsonCharSet] intValue]];
	[epsonPrintPitchPulldown selectItemAtIndex:[[displayedValues objectForKey:EpsonPrintPitch] intValue]];
	[epsonPrintWeightPulldown selectItemAtIndex:[[displayedValues objectForKey:EpsonPrintWeight] intValue]];
	[epsonFormLengthField setIntValue:[[displayedValues objectForKey:EpsonFormLength] intValue]];
	[epsonFormLengthStepper setIntValue:[[displayedValues objectForKey:EpsonFormLength] intValue]];
	[epsonAutoLinefeedButton setState:[[displayedValues objectForKey:EpsonAutoLinefeed] boolValue] ? NSOnState : NSOffState];
	[epsonPrintSlashedZerosButton setState:[[displayedValues objectForKey:EpsonPrintSlashedZeros] boolValue] ? NSOnState : NSOffState];
	[epsonAutoSkipButton setState:[[displayedValues objectForKey:EpsonAutoSkip] boolValue] ? NSOnState : NSOffState];
	[epsonSplitSkipButton setState:[[displayedValues objectForKey:EpsonSplitSkip] boolValue] ? NSOnState : NSOffState];
}

- (void)setBootMediaActive:(bool) active
{
	[d1FileEnabledButton setEnabled:active];
	[d2FileEnabledButton setEnabled:active];
	[d3FileEnabledButton setEnabled:active];
	[d4FileEnabledButton setEnabled:active];
	[d5FileEnabledButton setEnabled:active];
	[d6FileEnabledButton setEnabled:active];
	[d7FileEnabledButton setEnabled:active];
	[d8FileEnabledButton setEnabled:active];
	[cartFileEnabledButton setEnabled:active]; 
	[cart2FileEnabledButton setEnabled:active]; 
	[exeFileEnabledButton setEnabled:active];
	[cassFileEnabledButton setEnabled:active];
	[d1FileSelectButton setEnabled:active];
	[d2FileSelectButton setEnabled:active];
	[d3FileSelectButton setEnabled:active];
	[d4FileSelectButton setEnabled:active];
	[d5FileSelectButton setEnabled:active];
	[d6FileSelectButton setEnabled:active];
	[d7FileSelectButton setEnabled:active];
	[d8FileSelectButton setEnabled:active];
	[cartFileSelectButton setEnabled:active]; 
	[cart2FileSelectButton setEnabled:active]; 
	[exeFileSelectButton setEnabled:active];
	[cassFileSelectButton setEnabled:active];
}
- (void)clearBootMedia
{
    [d1FileField setStringValue:@""];
    [d2FileField setStringValue:@""];
    [d3FileField setStringValue:@""];
    [d4FileField setStringValue:@""];
    [d5FileField setStringValue:@""];
    [d6FileField setStringValue:@""];
    [d7FileField setStringValue:@""];
    [d8FileField setStringValue:@""];
    [cartFileField setStringValue:@""];
    [cart2FileField setStringValue:@""];
    [exeFileField setStringValue:@""];
    [cassFileField setStringValue:@""];
}

/*------------------------------------------------------------------------------
* updateJoyNumMenus - Method to update the joystick number menus, based on 
*                     joystick type.
*-----------------------------------------------------------------------------*/
- (void)updateJoyNumMenus {
    if (joystick0 == NULL) {
        [joystick1TypePulldown setEnabled:NO];
        [joystick1NumPulldown setEnabled:NO];
        [gamepad1ConfigPulldown setEnabled:NO];
        [gamepad1IdentifyButton setEnabled:NO];
        [[joystick1Pulldown itemAtIndex:3] setEnabled:NO];
        [[joystick2Pulldown itemAtIndex:3] setEnabled:NO];
        [[joystick3Pulldown itemAtIndex:3] setEnabled:NO];
        [[joystick4Pulldown itemAtIndex:3] setEnabled:NO];
    } else {
        [[joystick1Pulldown itemAtIndex:3] setEnabled:YES];
        [[joystick2Pulldown itemAtIndex:3] setEnabled:YES];
        [[joystick3Pulldown itemAtIndex:3] setEnabled:YES];
        [[joystick4Pulldown itemAtIndex:3] setEnabled:YES];
        [joystick1TypePulldown setEnabled:YES];
        [gamepad1ConfigPulldown setEnabled:YES];
        [gamepad1IdentifyButton setEnabled:YES];
        if (joystick0_nsticks == 0)
            [[joystick1TypePulldown itemAtIndex:0] setEnabled:NO];
        else
            [[joystick1TypePulldown itemAtIndex:0] setEnabled:YES];
        if (joystick0_nhats == 0)
            [[joystick1TypePulldown itemAtIndex:1] setEnabled:NO];
        else
            [[joystick1TypePulldown itemAtIndex:1] setEnabled:YES];
        switch([[displayedValues objectForKey:Joystick1Type] intValue])
        {
            case 0:
                [[joystick1NumPulldown itemAtIndex:0] setTitle:@"Stick 1"];
                [[joystick1NumPulldown itemAtIndex:1] setTitle:@"Stick 2"];
                [[joystick1NumPulldown itemAtIndex:2] setTitle:@"Stick 3"];
                [[joystick1NumPulldown itemAtIndex:3] setTitle:@"MultiStick"];
                [joystick1NumPulldown setEnabled:YES];
                if (joystick0_nsticks == 0) {
                    [[joystick1NumPulldown itemAtIndex:0] setEnabled:NO];
                    [[joystick1NumPulldown itemAtIndex:1] setEnabled:NO];
                    [[joystick1NumPulldown itemAtIndex:2] setEnabled:NO];
                    [[joystick1NumPulldown itemAtIndex:3] setEnabled:NO];
                    }
                else if (joystick0_nsticks == 1) {
                    [[joystick1NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick1NumPulldown itemAtIndex:1] setEnabled:NO];
                    [[joystick1NumPulldown itemAtIndex:2] setEnabled:NO];
                    [[joystick1NumPulldown itemAtIndex:3] setEnabled:NO];
                    }
                else if (joystick0_nsticks == 2) {
                    [[joystick1NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick1NumPulldown itemAtIndex:1] setEnabled:YES];
                    [[joystick1NumPulldown itemAtIndex:2] setEnabled:NO];
                    [[joystick1NumPulldown itemAtIndex:3] setEnabled:YES];
                    }
                else {
                    [[joystick1NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick1NumPulldown itemAtIndex:1] setEnabled:YES];
                    [[joystick1NumPulldown itemAtIndex:2] setEnabled:YES];
                    [[joystick1NumPulldown itemAtIndex:3] setEnabled:YES];
                    }
                break;
            case 1:
                [[joystick1NumPulldown itemAtIndex:0] setTitle:@"Hat 1"];
                [[joystick1NumPulldown itemAtIndex:1] setTitle:@"Hat 2"];
                [[joystick1NumPulldown itemAtIndex:2] setTitle:@"Hat 3"];
                [[joystick1NumPulldown itemAtIndex:3] setTitle:@"MultiStick"];
                [joystick1NumPulldown setEnabled:YES];
                if (joystick0_nhats == 0) {
                    [[joystick1NumPulldown itemAtIndex:0] setEnabled:NO];
                    [[joystick1NumPulldown itemAtIndex:1] setEnabled:NO];
                    [[joystick1NumPulldown itemAtIndex:2] setEnabled:NO];
					[[joystick1NumPulldown itemAtIndex:3] setEnabled:NO];
                    }
                else if (joystick0_nhats == 1) {
                    [[joystick1NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick1NumPulldown itemAtIndex:1] setEnabled:NO];
                    [[joystick1NumPulldown itemAtIndex:2] setEnabled:NO];
					[[joystick1NumPulldown itemAtIndex:3] setEnabled:NO];
                    }
                else if (joystick0_nhats == 2) {
                    [[joystick1NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick1NumPulldown itemAtIndex:1] setEnabled:YES];
                    [[joystick1NumPulldown itemAtIndex:2] setEnabled:NO];
					[[joystick1NumPulldown itemAtIndex:3] setEnabled:YES];
                    }
                else {
                    [[joystick1NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick1NumPulldown itemAtIndex:1] setEnabled:YES];
                    [[joystick1NumPulldown itemAtIndex:2] setEnabled:YES];
					[[joystick1NumPulldown itemAtIndex:3] setEnabled:YES];
                    }
                break;
            case 2:
                [[joystick1NumPulldown itemAtIndex:0] setTitle:@"-----"];
                [[joystick1NumPulldown itemAtIndex:1] setTitle:@"-----"];
                [[joystick1NumPulldown itemAtIndex:2] setTitle:@"-----"];
                [joystick1NumPulldown setEnabled:NO];
                break;
        }
    }
    if (joystick1 == NULL) {
        [joystick2TypePulldown setEnabled:NO];
        [joystick2NumPulldown setEnabled:NO];
        [gamepad2ConfigPulldown setEnabled:NO];
        [gamepad2IdentifyButton setEnabled:NO];
        [[joystick1Pulldown itemAtIndex:4] setEnabled:NO];
        [[joystick2Pulldown itemAtIndex:4] setEnabled:NO];
        [[joystick3Pulldown itemAtIndex:4] setEnabled:NO];
        [[joystick4Pulldown itemAtIndex:4] setEnabled:NO];
    } else {
        [[joystick1Pulldown itemAtIndex:4] setEnabled:YES];
        [[joystick2Pulldown itemAtIndex:4] setEnabled:YES];
        [[joystick3Pulldown itemAtIndex:4] setEnabled:YES];
        [[joystick4Pulldown itemAtIndex:4] setEnabled:YES];
        [joystick2TypePulldown setEnabled:YES];
        [gamepad2ConfigPulldown setEnabled:YES];
        [gamepad2IdentifyButton setEnabled:YES];
        if (joystick1_nsticks == 0)
            [[joystick2TypePulldown itemAtIndex:0] setEnabled:NO];
        else
            [[joystick2TypePulldown itemAtIndex:0] setEnabled:YES];
        if (joystick1_nhats == 0)
            [[joystick2TypePulldown itemAtIndex:1] setEnabled:NO];
        else
            [[joystick2TypePulldown itemAtIndex:1] setEnabled:YES];
        switch([[displayedValues objectForKey:Joystick2Type] intValue])
        {
            case 0:
                [[joystick2NumPulldown itemAtIndex:0] setTitle:@"Stick 1"];
                [[joystick2NumPulldown itemAtIndex:1] setTitle:@"Stick 2"];
                [[joystick2NumPulldown itemAtIndex:2] setTitle:@"Stick 3"];
                [[joystick2NumPulldown itemAtIndex:3] setTitle:@"MultiStick"];
                [joystick2NumPulldown setEnabled:YES];
                if (joystick1_nsticks == 0) {
                    [[joystick2NumPulldown itemAtIndex:0] setEnabled:NO];
                    [[joystick2NumPulldown itemAtIndex:1] setEnabled:NO];
                    [[joystick2NumPulldown itemAtIndex:2] setEnabled:NO];
                    [[joystick2NumPulldown itemAtIndex:3] setEnabled:NO];
                    }
                else if (joystick1_nsticks == 1) {
                    [[joystick2NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick2NumPulldown itemAtIndex:1] setEnabled:NO];
                    [[joystick2NumPulldown itemAtIndex:2] setEnabled:NO];
                    [[joystick2NumPulldown itemAtIndex:3] setEnabled:NO];
                    }
                else if (joystick1_nsticks == 2) {
                    [[joystick2NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick2NumPulldown itemAtIndex:1] setEnabled:YES];
                    [[joystick2NumPulldown itemAtIndex:2] setEnabled:NO];
                    [[joystick2NumPulldown itemAtIndex:3] setEnabled:YES];
                    }
                else {
                    [[joystick2NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick2NumPulldown itemAtIndex:1] setEnabled:YES];
                    [[joystick2NumPulldown itemAtIndex:2] setEnabled:YES];
                    [[joystick2NumPulldown itemAtIndex:3] setEnabled:YES];
                    }
                break;
            case 1:	
                [[joystick2NumPulldown itemAtIndex:0] setTitle:@"Hat 1"];
                [[joystick2NumPulldown itemAtIndex:1] setTitle:@"Hat 2"];
                [[joystick2NumPulldown itemAtIndex:2] setTitle:@"Hat 3"];
                [[joystick2NumPulldown itemAtIndex:2] setTitle:@"MultiStick"];
                [joystick2NumPulldown setEnabled:YES];
                if (joystick1_nhats == 0) {
                    [[joystick2NumPulldown itemAtIndex:0] setEnabled:NO];
                    [[joystick2NumPulldown itemAtIndex:1] setEnabled:NO];
                    [[joystick2NumPulldown itemAtIndex:2] setEnabled:NO];
					[[joystick2NumPulldown itemAtIndex:3] setEnabled:NO];
                    }
                else if (joystick1_nhats == 1) {
                    [[joystick2NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick2NumPulldown itemAtIndex:1] setEnabled:NO];
                    [[joystick2NumPulldown itemAtIndex:2] setEnabled:NO];
					[[joystick2NumPulldown itemAtIndex:3] setEnabled:NO];
                    }
                else if (joystick1_nhats == 2) {
                    [[joystick2NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick2NumPulldown itemAtIndex:1] setEnabled:YES];
                    [[joystick2NumPulldown itemAtIndex:2] setEnabled:NO];
					[[joystick2NumPulldown itemAtIndex:3] setEnabled:YES];
                    }
                else {
                    [[joystick2NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick2NumPulldown itemAtIndex:1] setEnabled:YES];
                    [[joystick2NumPulldown itemAtIndex:2] setEnabled:YES];
					[[joystick2NumPulldown itemAtIndex:3] setEnabled:YES];
                    }
                break;
            case 2:
                [[joystick2NumPulldown itemAtIndex:0] setTitle:@"-----"];
                [[joystick2NumPulldown itemAtIndex:1] setTitle:@"-----"];
                [[joystick2NumPulldown itemAtIndex:2] setTitle:@"-----"];
                [[joystick2NumPulldown itemAtIndex:3] setTitle:@"-----"];
                [joystick2NumPulldown setEnabled:NO];
                break;
        }
    }
    if (joystick2 == NULL) {
        [joystick3TypePulldown setEnabled:NO];
        [joystick3NumPulldown setEnabled:NO];
        [gamepad3ConfigPulldown setEnabled:NO];
        [gamepad3IdentifyButton setEnabled:NO];
        [[joystick1Pulldown itemAtIndex:5] setEnabled:NO];
        [[joystick2Pulldown itemAtIndex:5] setEnabled:NO];
        [[joystick3Pulldown itemAtIndex:5] setEnabled:NO];
        [[joystick4Pulldown itemAtIndex:5] setEnabled:NO];
    } else {
        [[joystick1Pulldown itemAtIndex:5] setEnabled:YES];
        [[joystick2Pulldown itemAtIndex:5] setEnabled:YES];
        [[joystick3Pulldown itemAtIndex:5] setEnabled:YES];
        [[joystick4Pulldown itemAtIndex:5] setEnabled:YES];
        [joystick3TypePulldown setEnabled:YES];
        [gamepad3ConfigPulldown setEnabled:YES];
        [gamepad3IdentifyButton setEnabled:YES];
        if (joystick2_nsticks == 0)
            [[joystick3TypePulldown itemAtIndex:0] setEnabled:NO];
        else
            [[joystick3TypePulldown itemAtIndex:0] setEnabled:YES];
        if (joystick2_nhats == 0)
            [[joystick3TypePulldown itemAtIndex:1] setEnabled:NO];
        else
            [[joystick3TypePulldown itemAtIndex:1] setEnabled:YES];
        switch([[displayedValues objectForKey:Joystick3Type] intValue])
        {
            case 0:
                [[joystick3NumPulldown itemAtIndex:0] setTitle:@"Stick 1"];
                [[joystick3NumPulldown itemAtIndex:1] setTitle:@"Stick 2"];
                [[joystick3NumPulldown itemAtIndex:2] setTitle:@"Stick 3"];
                [[joystick3NumPulldown itemAtIndex:3] setTitle:@"MultiStick"];
                [joystick3NumPulldown setEnabled:YES];
                if (joystick2_nsticks == 0) {
                    [[joystick3NumPulldown itemAtIndex:0] setEnabled:NO];
                    [[joystick3NumPulldown itemAtIndex:1] setEnabled:NO];
                    [[joystick3NumPulldown itemAtIndex:2] setEnabled:NO];
                    [[joystick3NumPulldown itemAtIndex:3] setEnabled:NO];
                    }
                else if (joystick2_nsticks == 1) {
                    [[joystick3NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick3NumPulldown itemAtIndex:1] setEnabled:NO];
                    [[joystick3NumPulldown itemAtIndex:2] setEnabled:NO];
                    [[joystick3NumPulldown itemAtIndex:3] setEnabled:NO];
                    }
                else if (joystick2_nsticks == 2) {
                    [[joystick3NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick3NumPulldown itemAtIndex:1] setEnabled:YES];
                    [[joystick3NumPulldown itemAtIndex:2] setEnabled:NO];
                    [[joystick3NumPulldown itemAtIndex:3] setEnabled:YES];
                    }
                else {
                    [[joystick3NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick3NumPulldown itemAtIndex:1] setEnabled:YES];
                    [[joystick3NumPulldown itemAtIndex:2] setEnabled:YES];
                    [[joystick3NumPulldown itemAtIndex:3] setEnabled:YES];
                    }
                break;
            case 1:
                [[joystick3NumPulldown itemAtIndex:0] setTitle:@"Hat 1"];
                [[joystick3NumPulldown itemAtIndex:1] setTitle:@"Hat 2"];
                [[joystick3NumPulldown itemAtIndex:2] setTitle:@"Hat 3"];
                [[joystick3NumPulldown itemAtIndex:3] setTitle:@"MultiStick"];
                [joystick3NumPulldown setEnabled:YES];
				if (joystick2_nhats == 0) {
                    [[joystick3NumPulldown itemAtIndex:0] setEnabled:NO];
                    [[joystick3NumPulldown itemAtIndex:1] setEnabled:NO];
                    [[joystick3NumPulldown itemAtIndex:2] setEnabled:NO];
					[[joystick3NumPulldown itemAtIndex:3] setEnabled:NO];
                    }
                else if (joystick2_nhats == 1) {
                    [[joystick3NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick3NumPulldown itemAtIndex:1] setEnabled:NO];
                    [[joystick3NumPulldown itemAtIndex:2] setEnabled:NO];
					[[joystick3NumPulldown itemAtIndex:3] setEnabled:NO];
                    }
                else if (joystick2_nhats == 2) {
                    [[joystick3NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick3NumPulldown itemAtIndex:1] setEnabled:YES];
                    [[joystick3NumPulldown itemAtIndex:2] setEnabled:NO];
					[[joystick3NumPulldown itemAtIndex:3] setEnabled:YES];
                    }
                else {
                    [[joystick3NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick3NumPulldown itemAtIndex:1] setEnabled:YES];
                    [[joystick3NumPulldown itemAtIndex:2] setEnabled:YES];
					[[joystick3NumPulldown itemAtIndex:3] setEnabled:YES];
                    }
                break;
            case 2:
                [[joystick3NumPulldown itemAtIndex:0] setTitle:@"-----"];
                [[joystick3NumPulldown itemAtIndex:1] setTitle:@"-----"];
                [[joystick3NumPulldown itemAtIndex:2] setTitle:@"-----"];
                [[joystick3NumPulldown itemAtIndex:3] setTitle:@"-----"];
                [joystick3NumPulldown setEnabled:NO];
                break;
        }
    }
    if (joystick3 == NULL) {
        [joystick4TypePulldown setEnabled:NO];
        [joystick4NumPulldown setEnabled:NO];
        [gamepad4ConfigPulldown setEnabled:NO];
        [gamepad4IdentifyButton setEnabled:NO];
        [[joystick1Pulldown itemAtIndex:6] setEnabled:NO];
        [[joystick2Pulldown itemAtIndex:6] setEnabled:NO];
        [[joystick3Pulldown itemAtIndex:6] setEnabled:NO];
        [[joystick4Pulldown itemAtIndex:6] setEnabled:NO];
    } else {
        [[joystick1Pulldown itemAtIndex:6] setEnabled:YES];
        [[joystick2Pulldown itemAtIndex:6] setEnabled:YES];
        [[joystick3Pulldown itemAtIndex:6] setEnabled:YES];
        [[joystick4Pulldown itemAtIndex:6] setEnabled:YES];
        [joystick4TypePulldown setEnabled:YES];
        [gamepad4ConfigPulldown setEnabled:YES];
        [gamepad4IdentifyButton setEnabled:YES];
        if (joystick3_nsticks == 0)
            [[joystick4TypePulldown itemAtIndex:0] setEnabled:NO];
        else
            [[joystick4TypePulldown itemAtIndex:0] setEnabled:YES];
        if (joystick3_nhats == 0)
            [[joystick4TypePulldown itemAtIndex:1] setEnabled:NO];
        else
            [[joystick4TypePulldown itemAtIndex:1] setEnabled:YES];
        switch([[displayedValues objectForKey:Joystick4Type] intValue])
        {
            case 0:
                [[joystick4NumPulldown itemAtIndex:0] setTitle:@"Stick 1"];
                [[joystick4NumPulldown itemAtIndex:1] setTitle:@"Stick 2"];
                [[joystick4NumPulldown itemAtIndex:2] setTitle:@"Stick 2"];
                [[joystick4NumPulldown itemAtIndex:3] setTitle:@"MultiStick"];
                [joystick4NumPulldown setEnabled:YES];
                if (joystick3_nsticks == 0) {
                    [[joystick4NumPulldown itemAtIndex:0] setEnabled:NO];
                    [[joystick4NumPulldown itemAtIndex:1] setEnabled:NO];
                    [[joystick4NumPulldown itemAtIndex:2] setEnabled:NO];
                    [[joystick4NumPulldown itemAtIndex:3] setEnabled:NO];
                    }
                else if (joystick3_nsticks == 1) {
                    [[joystick4NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick4NumPulldown itemAtIndex:1] setEnabled:NO];
                    [[joystick4NumPulldown itemAtIndex:2] setEnabled:NO];
                    [[joystick4NumPulldown itemAtIndex:3] setEnabled:NO];
                    }
                else if (joystick3_nsticks == 2) {
                    [[joystick4NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick4NumPulldown itemAtIndex:1] setEnabled:YES];
                    [[joystick4NumPulldown itemAtIndex:2] setEnabled:NO];
                    [[joystick4NumPulldown itemAtIndex:3] setEnabled:YES];
                    }
                else {
                    [[joystick4NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick4NumPulldown itemAtIndex:1] setEnabled:YES];
                    [[joystick4NumPulldown itemAtIndex:2] setEnabled:YES];
                    [[joystick4NumPulldown itemAtIndex:3] setEnabled:YES];
                    }
                break;
            case 1:
                [[joystick4NumPulldown itemAtIndex:0] setTitle:@"Hat 1"];
                [[joystick4NumPulldown itemAtIndex:1] setTitle:@"Hat 2"];
                [[joystick4NumPulldown itemAtIndex:2] setTitle:@"Hat 3"];
                [[joystick4NumPulldown itemAtIndex:3] setTitle:@"MultiStick"];
                [joystick4NumPulldown setEnabled:YES];
                if (joystick3_nhats == 0) {
                    [[joystick4NumPulldown itemAtIndex:0] setEnabled:NO];
                    [[joystick4NumPulldown itemAtIndex:1] setEnabled:NO];
                    [[joystick4NumPulldown itemAtIndex:2] setEnabled:NO];
					[[joystick4NumPulldown itemAtIndex:3] setEnabled:NO];
                    }
                else if (joystick3_nhats == 1) {
                    [[joystick4NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick4NumPulldown itemAtIndex:1] setEnabled:NO];
                    [[joystick4NumPulldown itemAtIndex:2] setEnabled:NO];
					[[joystick4NumPulldown itemAtIndex:3] setEnabled:NO];
                    }
                else if (joystick3_nhats == 2) {
                    [[joystick4NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick4NumPulldown itemAtIndex:1] setEnabled:YES];
                    [[joystick4NumPulldown itemAtIndex:2] setEnabled:NO];
					[[joystick4NumPulldown itemAtIndex:3] setEnabled:YES];
                    }
                else {
                    [[joystick4NumPulldown itemAtIndex:0] setEnabled:YES];
                    [[joystick4NumPulldown itemAtIndex:1] setEnabled:YES];
                    [[joystick4NumPulldown itemAtIndex:2] setEnabled:YES];
					[[joystick4NumPulldown itemAtIndex:3] setEnabled:YES];
                    }
                break;
            case 2:
                [[joystick4NumPulldown itemAtIndex:0] setTitle:@"-----"];
                [[joystick4NumPulldown itemAtIndex:1] setTitle:@"-----"];
                [[joystick4NumPulldown itemAtIndex:2] setTitle:@"-----"];
                [[joystick4NumPulldown itemAtIndex:3] setTitle:@"-----"];
                [joystick4NumPulldown setEnabled:NO];
                break;
        }
    }
}

/*------------------------------------------------------------------------------
* miscChanged - Method to get everything from User Interface when an event 
*        occurs.  Should probably be broke up by tab, since it is so huge.
*-----------------------------------------------------------------------------*/
- (void)miscChanged:(id)sender {
    int anInt;
    double aFloat;
    int mouseCount = 0;
    int firstMouse = 0;
    float penRed, penBlue, penGreen, penAlpha;
    int type, typever4;
    
    static NSNumber *yes = nil;
    static NSNumber *no = nil;
    static NSNumber *zero = nil;
    static NSNumber *one = nil;
    static NSNumber *two = nil;
    static NSNumber *three = nil;
    static NSNumber *four = nil;
    static NSNumber *five = nil;
    static NSNumber *six = nil;
    static NSNumber *seven = nil;
    static NSNumber *eight = nil;
    static NSNumber *nine = nil;
    static NSNumber *ten = nil;
    static NSNumber *eleven = nil;
    static NSNumber *twelve = nil;
    static NSNumber *thirteen = nil;
    static NSNumber *fourteen = nil;
   
    if (!yes) {
        yes = [[NSNumber alloc] initWithBool:YES];
        no = [[NSNumber alloc] initWithBool:NO];
        zero = [[NSNumber alloc] initWithInt:0];
        one = [[NSNumber alloc] initWithInt:1];
        two = [[NSNumber alloc] initWithInt:2];
        three = [[NSNumber alloc] initWithInt:3];
        four = [[NSNumber alloc] initWithInt:4];
        five = [[NSNumber alloc] initWithInt:5];
        six = [[NSNumber alloc] initWithInt:6];
        seven = [[NSNumber alloc] initWithInt:7];
        eight = [[NSNumber alloc] initWithInt:8];
        nine = [[NSNumber alloc] initWithInt:9];
        ten = [[NSNumber alloc] initWithInt:10];
        eleven = [[NSNumber alloc] initWithInt:11];
        twelve = [[NSNumber alloc] initWithInt:12];
        thirteen = [[NSNumber alloc] initWithInt:13];
        fourteen = [[NSNumber alloc] initWithInt:14];
    }

    [displayedValues setObject:[[fullScreenMatrix selectedCell] tag] ? yes : no forKey:FullScreen];
    [displayedValues setObject:[[openglMatrix selectedCell] tag] ? yes : no forKey:OpenGl];
    if ([lockFullscreenSizeButton state] == NSOnState) {
        [displayedValues setObject:yes forKey:LockFullscreenSize];
		[fullscreenMonitorButton setEnabled:YES];
        [fullscreenForegroundRed setEnabled:YES];
        [fullscreenForegroundGreen setEnabled:YES];
        [fullscreenForegroundBlue setEnabled:YES];
        [fullscreenBackgroundRed setEnabled:YES];
        [fullscreenBackgroundBlue setEnabled:YES];
        [fullscreenBackgroundGreen setEnabled:YES];
		}
    else {
        [displayedValues setObject:no forKey:LockFullscreenSize];
 		[fullscreenMonitorButton setEnabled:NO];
        [fullscreenForegroundRed setEnabled:NO];
        [fullscreenForegroundGreen setEnabled:NO];
        [fullscreenForegroundBlue setEnabled:NO];
        [fullscreenBackgroundRed setEnabled:NO];
        [fullscreenBackgroundGreen setEnabled:NO];
        [fullscreenBackgroundBlue setEnabled:NO];
		}
    if ([fullscreenMonitorButton state] == NSOnState)
        [displayedValues setObject:yes forKey:FullscreenMonitor];
    else
        [displayedValues setObject:no forKey:FullscreenMonitor];
    penRed = ((float) [fullscreenForegroundRed intValue])/255.0;
    penGreen = ((float) [fullscreenForegroundGreen intValue])/255.0;
    penBlue = ((float) [fullscreenForegroundBlue intValue])/255.0;
    penAlpha = 0.0;
	[displayedValues setObject:[NSNumber numberWithFloat:penRed] forKey:FullscreenForeRed];
	[displayedValues setObject:[NSNumber numberWithFloat:penBlue] forKey:FullscreenForeBlue];
	[displayedValues setObject:[NSNumber numberWithFloat:penGreen] forKey:FullscreenForeGreen];
	[displayedValues setObject:[NSNumber numberWithFloat:penAlpha] forKey:FullscreenForeAlpha];
    penRed = ((float) [fullscreenBackgroundRed intValue])/255.0;
    penGreen = ((float) [fullscreenBackgroundGreen intValue])/255.0;
    penBlue = ((float) [fullscreenBackgroundBlue intValue])/255.0;
    penAlpha = 0.0;
	[displayedValues setObject:[NSNumber numberWithFloat:penRed] forKey:FullscreenBackRed];
	[displayedValues setObject:[NSNumber numberWithFloat:penBlue] forKey:FullscreenBackBlue];
	[displayedValues setObject:[NSNumber numberWithFloat:penGreen] forKey:FullscreenBackGreen];
	[displayedValues setObject:[NSNumber numberWithFloat:penAlpha] forKey:FullscreenBackAlpha];
    switch([[scaleFactorMatrix selectedCell] tag]) {
        case 1:
		default:
            [displayedValues setObject:one forKey:ScaleFactor];
			[displayedValues setObject:no forKey:DoubleSize];
            break;
        case 2:
            [displayedValues setObject:two forKey:ScaleFactor];
			[displayedValues setObject:yes forKey:DoubleSize];
            break;
        case 3:
            [displayedValues setObject:three forKey:ScaleFactor];
			[displayedValues setObject:yes forKey:DoubleSize];
            break;
        case 4:
            [displayedValues setObject:four forKey:ScaleFactor];
			[displayedValues setObject:yes forKey:DoubleSize];
            break;
    }
	switch([[scaleModeMatrix selectedCell] tag]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:ScaleMode];
            break;
        case 1:
            [displayedValues setObject:one forKey:ScaleMode];
            break;
        case 2:
            [displayedValues setObject:two forKey:ScaleMode];
            break;
    }
	switch([[widthModeMatrix selectedCell] tag]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:WidthMode];
            break;
        case 1:
            [displayedValues setObject:one forKey:WidthMode];
            break;
        case 2:
            [displayedValues setObject:two forKey:WidthMode];
            break;
    }
    switch([[tvModeMatrix selectedCell] tag]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:TvMode];
            break;
        case 1:
            [displayedValues setObject:one forKey:TvMode];
            break;
    }
    switch([refreshRatioPulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:one forKey:RefreshRatio];
            break;
        case 1:
            [displayedValues setObject:two forKey:RefreshRatio];
            break;
        case 2:
            [displayedValues setObject:three forKey:RefreshRatio];
            break;
        case 3:
            [displayedValues setObject:four forKey:RefreshRatio];
            break;
    }
    if ([spriteCollisionsButton state] == NSOnState)
        [displayedValues setObject:yes forKey:SpriteCollisions];
    else
        [displayedValues setObject:no forKey:SpriteCollisions];
    switch([artifactingPulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:ArtifactingMode];
            break;
        case 1:
            [displayedValues setObject:one forKey:ArtifactingMode];
            break;
        case 2:
            [displayedValues setObject:two forKey:ArtifactingMode];
            break;
        case 3:
            [displayedValues setObject:three forKey:ArtifactingMode];
            break;
        case 4:
            [displayedValues setObject:four forKey:ArtifactingMode];
            break;
    }
    if ([artifactNewButton state] == NSOnState)
        [displayedValues setObject:yes forKey:ArtifactNew];
    else
        [displayedValues setObject:no forKey:ArtifactNew];
    anInt = [whiteLevelField intValue];
    [displayedValues setObject:[NSNumber numberWithInt:anInt] forKey:WhiteLevel];
    anInt = [blackLevelField intValue];
    [displayedValues setObject:[NSNumber numberWithInt:anInt] forKey:BlackLevel];
    anInt = [intensityField intValue];
    [displayedValues setObject:[NSNumber numberWithInt:anInt] forKey:Intensity];
    anInt = [colorShiftField intValue];
    [displayedValues setObject:[NSNumber numberWithInt:anInt] forKey:ColorShift];
    if ([externalPaletteButton state] == NSOnState) {
        [adjustPaletteButton setEnabled:YES];
        [displayedValues setObject:no forKey:UseBuiltinPalette];
        [colorShiftField setEnabled:NO];
        [paletteField setEnabled:YES];
        [paletteChooseButton setEnabled:YES];
        if ([adjustPaletteButton state] == NSOnState) {
            [displayedValues setObject:yes forKey:AdjustPalette];
            [blackLevelField setEnabled:YES];
            [whiteLevelField setEnabled:YES];
            [intensityField setEnabled:YES];
            }
        else {
            [displayedValues setObject:no forKey:AdjustPalette];
            [blackLevelField setEnabled:NO];
            [whiteLevelField setEnabled:NO];
            [intensityField setEnabled:NO];
            }
        }
    else {
        [displayedValues setObject:yes forKey:UseBuiltinPalette];
        [displayedValues setObject:no forKey:AdjustPalette];
        [blackLevelField setEnabled:YES];
        [whiteLevelField setEnabled:YES];
        [intensityField setEnabled:YES];
        [colorShiftField setEnabled:YES];
        [paletteField setEnabled:NO];
        [paletteChooseButton setEnabled:NO];
        [adjustPaletteButton setEnabled:NO];
        }
    [displayedValues setObject:[paletteField stringValue] forKey:PaletteFile];
    if ([fpsButton state] == NSOnState)
        [displayedValues setObject:yes forKey:ShowFPS];
    else
        [displayedValues setObject:no forKey:ShowFPS];
    if ([ledSectorButton state] == NSOnState)
        [displayedValues setObject:yes forKey:LedSector];
    else
        [displayedValues setObject:no forKey:LedSector];
    if ([ledStatusButton state] == NSOnState) {
        [displayedValues setObject:yes forKey:LedStatus];
		[ledSectorButton setEnabled:YES];
		}
    else {
        [displayedValues setObject:no forKey:LedStatus];
		[ledSectorButton setState:NSOffState];
		[ledSectorButton setEnabled:NO];
		}
    if ([ledSectorMediaButton state] == NSOnState)
        [displayedValues setObject:yes forKey:LedSectorMedia];
    else
        [displayedValues setObject:no forKey:LedSectorMedia];
    if ([ledStatusMediaButton state] == NSOnState) {
        [displayedValues setObject:yes forKey:LedStatusMedia];
		[ledSectorMediaButton setEnabled:YES];
		}
    else {
        [displayedValues setObject:no forKey:LedStatusMedia];
		[ledSectorMediaButton setState:NSOffState];
		[ledSectorMediaButton setEnabled:NO];
		}
 
	type = [self typeFromIndex:[atariTypePulldown indexOfSelectedItem] :&typever4];
	[displayedValues setObject:[NSNumber numberWithInt:type] forKey:AtariType];
	[displayedValues setObject:[NSNumber numberWithInt:typever4] forKey:AtariTypeVer4];
	type = [self typeFromIndex:[atariSwitchTypePulldown indexOfSelectedItem] :&typever4];
	[displayedValues setObject:[NSNumber numberWithInt:type] forKey:AtariSwitchType];
	[displayedValues setObject:[NSNumber numberWithInt:typever4] forKey:AtariSwitchTypeVer4];

    if ([disableBasicButton state] == NSOnState)
        [displayedValues setObject:yes forKey:DisableBasic];
    else
        [displayedValues setObject:no forKey:DisableBasic];
    if ([disableAllBasicButton state] == NSOnState)
        [displayedValues setObject:yes forKey:DisableAllBasic];
    else
        [displayedValues setObject:no forKey:DisableAllBasic];
	aFloat = [emulationSpeedSlider floatValue];
	if (aFloat < 0.1) {
		aFloat = 0.1;
		[emulationSpeedSlider setFloatValue:aFloat];
		}
    [displayedValues setObject:[NSNumber numberWithFloat:aFloat] 
		forKey:EmulationSpeed];
    if ([enableSioPatchButton state] == NSOnState)
        [displayedValues setObject:yes forKey:EnableSioPatch];
    else
        [displayedValues setObject:no forKey:EnableSioPatch];
    if ([enableHPatchButton state] == NSOnState)
        [displayedValues setObject:yes forKey:EnableHPatch];
    else
        [displayedValues setObject:no forKey:EnableHPatch];
    if ([enableDPatchButton state] == NSOnState)
        [displayedValues setObject:yes forKey:EnableDPatch];
    else
        [displayedValues setObject:no forKey:EnableDPatch];
    [displayedValues setObject:[printCommandField stringValue] forKey:PrintCommand];   
    if ([enablePPatchButton state] == NSOnState) {
        [displayedValues setObject:yes forKey:EnablePPatch];
        [printCommandField setEnabled:YES];
        [printerTypePulldown setEnabled:YES];
        [atari825CharSetPulldown setEnabled:YES];
        [atari825FormLengthField setEnabled:YES];
        [atari825FormLengthStepper setEnabled:YES];
        [atari825AutoLinefeedButton setEnabled:YES];
        [atari1020PrintWidthPulldown setEnabled:YES];
        [atari1020FormLengthField setEnabled:YES];
        [atari1020FormLengthStepper setEnabled:YES];
        [atari1020AutoLinefeedButton setEnabled:YES];
        [atari1020AutoPageAdjustButton setEnabled:YES];
        [atari1020Pen1Red setEnabled:YES];
        [atari1020Pen1Green setEnabled:YES];
        [atari1020Pen1Blue setEnabled:YES];
        [atari1020Pen2Red setEnabled:YES];
        [atari1020Pen2Green setEnabled:YES];
        [atari1020Pen2Blue setEnabled:YES];
        [atari1020Pen3Red setEnabled:YES];
        [atari1020Pen3Green setEnabled:YES];
        [atari1020Pen3Blue setEnabled:YES];
        [atari1020Pen4Red setEnabled:YES];
        [atari1020Pen4Green setEnabled:YES];
        [atari1020Pen4Blue setEnabled:YES];
        [atasciiFormLengthField setEnabled:YES];
        [atasciiFormLengthStepper setEnabled:YES];
        [atasciiCharSetPulldown setEnabled:YES];
        [atasciiCharSizeField setEnabled:YES];
        [atasciiCharSizeStepper setEnabled:YES];
        [atasciiLineGapField setEnabled:YES];
        [atasciiLineGapStepper setEnabled:YES];
        [epsonCharSetPulldown setEnabled:YES];
        [epsonPrintPitchPulldown setEnabled:YES];
        [epsonPrintWeightPulldown setEnabled:YES];
        [epsonFormLengthField setEnabled:YES];
        [epsonFormLengthStepper setEnabled:YES];
        [epsonAutoLinefeedButton setEnabled:YES];
        [epsonPrintSlashedZerosButton setEnabled:YES];
        [epsonAutoSkipButton setEnabled:YES];
        }
    else {
        [displayedValues setObject:no forKey:EnablePPatch];
        [printCommandField setEnabled:NO];
        [printerTypePulldown setEnabled:NO];
        [atari825CharSetPulldown setEnabled:NO];
        [atari825FormLengthField setEnabled:NO];
        [atari825FormLengthStepper setEnabled:NO];
        [atari825AutoLinefeedButton setEnabled:NO];
        [atari1020PrintWidthPulldown setEnabled:NO];
        [atari1020FormLengthField setEnabled:NO];
        [atari1020FormLengthStepper setEnabled:NO];
        [atari1020AutoLinefeedButton setEnabled:NO];
        [atari1020AutoPageAdjustButton setEnabled:NO];
        [atari1020Pen1Red setEnabled:NO];
        [atari1020Pen1Green setEnabled:NO];
        [atari1020Pen1Blue setEnabled:NO];
        [atari1020Pen2Red setEnabled:NO];
        [atari1020Pen2Green setEnabled:NO];
        [atari1020Pen2Blue setEnabled:NO];
        [atari1020Pen3Red setEnabled:NO];
        [atari1020Pen3Green setEnabled:NO];
        [atari1020Pen3Blue setEnabled:NO];
        [atari1020Pen4Red setEnabled:NO];
        [atari1020Pen4Green setEnabled:NO];
        [atari1020Pen4Blue setEnabled:NO];
        [atasciiFormLengthField setEnabled:NO];
        [atasciiFormLengthStepper setEnabled:NO];
        [atasciiCharSetPulldown setEnabled:NO];
        [atasciiCharSizeField setEnabled:NO];
        [atasciiCharSizeStepper setEnabled:NO];
        [atasciiLineGapField setEnabled:NO];
        [atasciiLineGapStepper setEnabled:NO];
        [epsonCharSetPulldown setEnabled:NO];
        [epsonPrintPitchPulldown setEnabled:NO];
        [epsonPrintWeightPulldown setEnabled:NO];
        [epsonFormLengthField setEnabled:NO];
        [epsonFormLengthStepper setEnabled:NO];
        [epsonAutoLinefeedButton setEnabled:NO];
        [epsonPrintSlashedZerosButton setEnabled:NO];
        [epsonAutoSkipButton setEnabled:NO];
        }
    switch([printerTypePulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:PrinterType];
            break;
        case 1:
            [displayedValues setObject:one forKey:PrinterType];
            break;
        case 2:
            [displayedValues setObject:two forKey:PrinterType];
            break;
        case 3:
            [displayedValues setObject:three forKey:PrinterType];
            break;
        case 4:
            [displayedValues setObject:four forKey:PrinterType];
            break;
		}
	[printerTabView selectTabViewItemAtIndex:[printerTypePulldown indexOfSelectedItem]];
    switch([atari825CharSetPulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:Atari825CharSet];
            break;
        case 1:
            [displayedValues setObject:one forKey:Atari825CharSet];
            break;
        case 2:
            [displayedValues setObject:two forKey:Atari825CharSet];
            break;
        case 3:
            [displayedValues setObject:three forKey:Atari825CharSet];
            break;
        case 4:
            [displayedValues setObject:four forKey:Atari825CharSet];
            break;
        case 5:
            [displayedValues setObject:five forKey:Atari825CharSet];
            break;
		}
    anInt = [atari825FormLengthStepper intValue];
    [displayedValues setObject:[NSNumber numberWithInt:anInt] forKey:Atari825FormLength];
	[atari825FormLengthField setIntValue:anInt];
    if ([atari825AutoLinefeedButton state] == NSOnState)
        [displayedValues setObject:yes forKey:Atari825AutoLinefeed];
    else
        [displayedValues setObject:no forKey:Atari825AutoLinefeed];

    anInt = [atari1020FormLengthStepper intValue];
    [displayedValues setObject:[NSNumber numberWithInt:anInt] forKey:Atari1020FormLength];
	[atari1020FormLengthField setIntValue:anInt];
    if ([atari1020AutoLinefeedButton state] == NSOnState)
        [displayedValues setObject:yes forKey:Atari1020AutoLinefeed];
    else
        [displayedValues setObject:no forKey:Atari1020AutoLinefeed];
    if ([atari1020AutoPageAdjustButton state] == NSOnState)
        [displayedValues setObject:yes forKey:Atari1020AutoPageAdjust];
    else
        [displayedValues setObject:no forKey:Atari1020AutoPageAdjust];
    switch([atari1020PrintWidthPulldown indexOfSelectedItem]) {
        case 0:
            [displayedValues setObject:zero forKey:Atari1020PrintWidth];
            break;
        case 1:
            [displayedValues setObject:one forKey:Atari1020PrintWidth];
            break;
		}
    penRed = ((float) [atari1020Pen1Red intValue])/255.0;
    penGreen = ((float) [atari1020Pen1Green intValue])/255.0;
    penBlue = ((float) [atari1020Pen1Blue intValue])/255.0;
    [displayedValues setObject:[NSNumber numberWithFloat:penRed] forKey:Atari1020Pen1Red];
    [displayedValues setObject:[NSNumber numberWithFloat:penBlue] forKey:Atari1020Pen1Blue];
    [displayedValues setObject:[NSNumber numberWithFloat:penGreen] forKey:Atari1020Pen1Green];
    penRed = ((float) [atari1020Pen2Red intValue])/255.0;
    penGreen = ((float) [atari1020Pen2Green intValue])/255.0;
    penBlue = ((float) [atari1020Pen2Blue intValue])/255.0;
    [displayedValues setObject:[NSNumber numberWithFloat:penRed] forKey:Atari1020Pen2Red];
    [displayedValues setObject:[NSNumber numberWithFloat:penBlue] forKey:Atari1020Pen2Blue];
    [displayedValues setObject:[NSNumber numberWithFloat:penGreen] forKey:Atari1020Pen2Green];
    penRed = ((float) [atari1020Pen3Red intValue])/255.0;
    penGreen = ((float) [atari1020Pen3Green intValue])/255.0;
    penBlue = ((float) [atari1020Pen3Blue intValue])/255.0;
    [displayedValues setObject:[NSNumber numberWithFloat:penRed] forKey:Atari1020Pen3Red];
    [displayedValues setObject:[NSNumber numberWithFloat:penBlue] forKey:Atari1020Pen3Blue];
    [displayedValues setObject:[NSNumber numberWithFloat:penGreen] forKey:Atari1020Pen3Green];
    penRed = ((float) [atari1020Pen4Red intValue])/255.0;
    penGreen = ((float) [atari1020Pen4Green intValue])/255.0;
    penBlue = ((float) [atari1020Pen4Blue intValue])/255.0;
    [displayedValues setObject:[NSNumber numberWithFloat:penRed] forKey:Atari1020Pen4Red];
    [displayedValues setObject:[NSNumber numberWithFloat:penBlue] forKey:Atari1020Pen4Blue];
    [displayedValues setObject:[NSNumber numberWithFloat:penGreen] forKey:Atari1020Pen4Green];
    anInt = [atasciiFormLengthStepper intValue];
    [displayedValues setObject:[NSNumber numberWithInt:anInt] forKey:AtasciiFormLength];
    [atasciiFormLengthField setIntValue:anInt];
    anInt = [atasciiCharSizeStepper intValue];
    [displayedValues setObject:[NSNumber numberWithInt:anInt] forKey:AtasciiCharSize];
    [atasciiCharSizeField setIntValue:anInt];
    anInt = [atasciiLineGapStepper intValue];
    [displayedValues setObject:[NSNumber numberWithInt:anInt] forKey:AtasciiLineGap];
    [atasciiLineGapField setIntValue:anInt];
    switch([atasciiCharSetPulldown indexOfSelectedItem]) {
        case 0:
        default:
            [displayedValues setObject:zero forKey:AtasciiCharSet];
            break;
        case 1:
            [displayedValues setObject:one forKey:AtasciiCharSet];
            break;
        }

    switch([epsonCharSetPulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:EpsonCharSet];
            break;
        case 1:
            [displayedValues setObject:one forKey:EpsonCharSet];
            break;
        case 2:
            [displayedValues setObject:two forKey:EpsonCharSet];
            break;
        case 3:
            [displayedValues setObject:three forKey:EpsonCharSet];
            break;
        case 4:
            [displayedValues setObject:four forKey:EpsonCharSet];
            break;
        case 5:
            [displayedValues setObject:five forKey:EpsonCharSet];
            break;
        case 6:
            [displayedValues setObject:six forKey:EpsonCharSet];
            break;
        case 7:
            [displayedValues setObject:seven forKey:EpsonCharSet];
            break;
        case 8:
            [displayedValues setObject:eight forKey:EpsonCharSet];
            break;
		}	
	anInt = [epsonFormLengthStepper intValue];
    [displayedValues setObject:[NSNumber numberWithInt:anInt] forKey:EpsonFormLength];
	[epsonFormLengthField setIntValue:anInt];
    if ([epsonAutoLinefeedButton state] == NSOnState)
        [displayedValues setObject:yes forKey:EpsonAutoLinefeed];
    else
        [displayedValues setObject:no forKey:EpsonAutoLinefeed];
    switch([epsonPrintPitchPulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:EpsonPrintPitch];
            break;
        case 1:
            [displayedValues setObject:one forKey:EpsonPrintPitch];
            break;
		}
    switch([epsonPrintWeightPulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:EpsonPrintWeight];
            break;
        case 1:
            [displayedValues setObject:one forKey:EpsonPrintWeight];
            break;
		}
    if ([epsonAutoLinefeedButton state] == NSOnState)
        [displayedValues setObject:yes forKey:EpsonAutoLinefeed];
    else
        [displayedValues setObject:no forKey:EpsonAutoLinefeed];
    if ([epsonPrintSlashedZerosButton state] == NSOnState)
        [displayedValues setObject:yes forKey:EpsonPrintSlashedZeros];
    else
        [displayedValues setObject:no forKey:EpsonPrintSlashedZeros];
    if ([epsonAutoSkipButton state] == NSOnState)
		{
        [displayedValues setObject:yes forKey:EpsonAutoSkip];
		[epsonSplitSkipButton setEnabled:YES];
		}
    else
		{
        [displayedValues setObject:no forKey:EpsonAutoSkip];
		[epsonSplitSkipButton setEnabled:NO];
		}
    if ([epsonSplitSkipButton state] == NSOnState)
        [displayedValues setObject:yes forKey:EpsonSplitSkip];
    else
        [displayedValues setObject:no forKey:EpsonSplitSkip];

    if ([enableRPatchButton state] == NSOnState)
        [displayedValues setObject:yes forKey:EnableRPatch];
    else
        [displayedValues setObject:no forKey:EnableRPatch];
    [displayedValues setObject:[rPatchPortField stringValue] forKey:RPatchPort];
	switch([[rPatchSerialMatrix selectedCell] tag]) {
        case 0:
		default:
            [displayedValues setObject:yes forKey:RPatchSerialEnabled];
			[rPatchSerialPulldown setEnabled:YES];
			[rPatchPortField setEnabled:NO];
            break;
        case 1:
            [displayedValues setObject:no forKey:RPatchSerialEnabled];
			[rPatchSerialPulldown setEnabled:NO];
			[rPatchPortField setEnabled:YES];
            break;
    }
	if ([rPatchSerialPulldown indexOfSelectedItem] == 0)
		[displayedValues setObject:@"" forKey:RPatchSerialPort];
	else
		[displayedValues 
			setObject:[NSString stringWithCString:bsdPaths[[rPatchSerialPulldown indexOfSelectedItem]-1] encoding:NSASCIIStringEncoding] 
			forKey:RPatchSerialPort];

    switch([useAtariCursorKeysPulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:UseAtariCursorKeys];
            break;
        case 1:
            [displayedValues setObject:one forKey:UseAtariCursorKeys];
            break;
        case 2:
            [displayedValues setObject:two forKey:UseAtariCursorKeys];
            break;
	}
		
    if ([bootFromCassetteButton state] == NSOnState)
        [displayedValues setObject:yes forKey:BootFromCassette];
    else
        [displayedValues setObject:no forKey:BootFromCassette];
    if ([speedLimitButton state] == NSOnState)
        [displayedValues setObject:yes forKey:SpeedLimit];
    else
        [displayedValues setObject:no forKey:SpeedLimit];
    if ([xep80EnabledButton state] == NSOnState)
        [displayedValues setObject:yes forKey:XEP80Enabled];
    else
        [displayedValues setObject:no forKey:XEP80Enabled];
    switch([xep80PortPulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:XEP80Port];
            break;
        case 1:
            [displayedValues setObject:one forKey:XEP80Port];
            break;
		}
    anInt = [xep80ForegroundField intValue];
    [displayedValues setObject:[NSNumber numberWithInt:anInt] forKey:XEP80OnColor];
    anInt = [xep80BackgroundField intValue];
    [displayedValues setObject:[NSNumber numberWithInt:anInt] forKey:XEP80OffColor];
    if ([enableSoundButton state] == NSOnState)
        [displayedValues setObject:yes forKey:EnableSound];
    else
        [displayedValues setObject:no forKey:EnableSound];
#if 0 /* enableHifiSound is deprecated from 4.2.2 on */    
    if ([enableHifiSoundButton state] == NSOnState)
        [displayedValues setObject:yes forKey:EnableHifiSound];
    else
        [displayedValues setObject:no forKey:EnableHifiSound];
#endif	
#ifdef WORDS_BIGENDIAN
	[displayedValues setObject:no forKey:Enable16BitSound];
	[enable16BitSoundPulldown selectItemAtIndex:1];
#else
	if ([enable16BitSoundPulldown indexOfSelectedItem] == 0)
        [displayedValues setObject:yes forKey:Enable16BitSound];
	else 
        [displayedValues setObject:no forKey:Enable16BitSound];		
#endif	
    if ([consoleSoundEnableButton state] == NSOnState)
        [displayedValues setObject:yes forKey:EnableConsoleSound];
    else
        [displayedValues setObject:no forKey:EnableConsoleSound];
    if ([serioSoundEnableButton state] == NSOnState)
        [displayedValues setObject:yes forKey:EnableSerioSound];
    else
        [displayedValues setObject:no forKey:EnableSerioSound];
    if ([muteAudioButton state] == NSOffState)
        [displayedValues setObject:yes forKey:DontMuteAudio];
    else
        [displayedValues setObject:no forKey:DontMuteAudio];
    if ([diskDriveSoundButton state] == NSOnState)
        [displayedValues setObject:yes forKey:DiskDriveSound];
    else
        [displayedValues setObject:no forKey:DiskDriveSound];
    if ([internationalKeyboardButton state] == NSOnState)
        [displayedValues setObject:yes forKey:EnableInternational];
    else
        [displayedValues setObject:no forKey:EnableInternational];
    if ([enableMultijoyButton state] == NSOnState)
        [displayedValues setObject:yes forKey:EnableMultijoy];
    else
        [displayedValues setObject:no forKey:EnableMultijoy];
    if ([ignoreHeaderWriteprotectButton state] == NSOnState)
        [displayedValues setObject:yes forKey:IgnoreHeaderWriteprotect];
    else
        [displayedValues setObject:no forKey:IgnoreHeaderWriteprotect];
	
	[displayedValues setObject:[NSNumber numberWithInt:axlonBankMasks[[axlonMemSizePulldown indexOfSelectedItem]]] forKey:AxlonBankMask];
	[displayedValues setObject:[NSNumber numberWithInt:mosaicBankMaxs[[mosaicMemSizePulldown indexOfSelectedItem]]] forKey:MosaicMaxBank];
	switch([[pbiExpansionMatrix selectedCell] tag]) {
        case 0:
		default:
            [displayedValues setObject:no forKey:BlackBoxEnabled];
            [displayedValues setObject:no forKey:MioEnabled];
            break;
        case 1:
            [displayedValues setObject:yes forKey:BlackBoxEnabled];
            [displayedValues setObject:no forKey:MioEnabled];
            break;
        case 2:
            [displayedValues setObject:no forKey:BlackBoxEnabled];
            [displayedValues setObject:yes forKey:MioEnabled];
            break;
    }
	[displayedValues setObject:[blackBoxRomFileField stringValue] forKey:BlackBoxRomFile];
	[displayedValues setObject:[mioRomFileField stringValue] forKey:MioRomFile];
	[displayedValues setObject:[blackBoxScsiDiskFileField stringValue] forKey:BlackBoxScsiDiskFile];
	[displayedValues setObject:[mioScsiDiskFileField stringValue] forKey:MioScsiDiskFile];
	 
	[displayedValues setObject:[imageDirField stringValue] forKey:ImageDir];
    [displayedValues setObject:[printDirField stringValue] forKey:PrintDir];
    [displayedValues setObject:[hardDiskDir1Field stringValue] forKey:HardDiskDir1];
    [displayedValues setObject:[hardDiskDir2Field stringValue] forKey:HardDiskDir2];
    [displayedValues setObject:[hardDiskDir3Field stringValue] forKey:HardDiskDir3];
    [displayedValues setObject:[hardDiskDir4Field stringValue] forKey:HardDiskDir4];
    if ([hardDrivesReadOnlyButton state] == NSOnState)
        [displayedValues setObject:yes forKey:HardDrivesReadOnly];
    else
        [displayedValues setObject:no forKey:HardDrivesReadOnly];
    [displayedValues setObject:[hPathField stringValue] forKey:HPath];

    [displayedValues setObject:[osARomFileField stringValue] forKey:OsARomFile];
    [displayedValues setObject:[osBRomFileField stringValue] forKey:OsBRomFile];
    [displayedValues setObject:[xlRomFileField stringValue] forKey:XlRomFile];
    [displayedValues setObject:[basicRomFileField stringValue] forKey:BasicRomFile];
    [displayedValues setObject:[a5200RomFileField stringValue] forKey:A5200RomFile];

    [displayedValues setObject:[diskImageDirField stringValue] forKey:DiskImageDir];
    [displayedValues setObject:[diskSetDirField stringValue] forKey:DiskSetDir];
    [displayedValues setObject:[cartImageDirField stringValue] forKey:CartImageDir];
    [displayedValues setObject:[cassImageDirField stringValue] forKey:CassImageDir];
    [displayedValues setObject:[exeFileDirField stringValue] forKey:ExeFileDir];
    [displayedValues setObject:[savedStateDirField stringValue] forKey:SavedStateDir];
    [displayedValues setObject:[configDirField stringValue] forKey:ConfigDir];

	if (([saveCurrentMediaButton state] == NSOnState) && ([[displayedValues objectForKey:SaveCurrentMedia] boolValue] == NO))
		[self clearBootMedia];
    [displayedValues setObject:[d1FileField stringValue] forKey:D1File];
    [displayedValues setObject:[d2FileField stringValue] forKey:D2File];
    [displayedValues setObject:[d3FileField stringValue] forKey:D3File];
    [displayedValues setObject:[d4FileField stringValue] forKey:D4File];
    [displayedValues setObject:[d5FileField stringValue] forKey:D5File];
    [displayedValues setObject:[d6FileField stringValue] forKey:D6File];
    [displayedValues setObject:[d7FileField stringValue] forKey:D7File];
    [displayedValues setObject:[d8FileField stringValue] forKey:D8File];
    [displayedValues setObject:[cartFileField stringValue] forKey:CartFile];
    [displayedValues setObject:[cart2FileField stringValue] forKey:Cart2File];
    [displayedValues setObject:[exeFileField stringValue] forKey:ExeFile];
    [displayedValues setObject:[cassFileField stringValue] forKey:CassFile];
    if ([d1FileEnabledButton state] == NSOnState)
        [displayedValues setObject:yes forKey:D1FileEnabled];
    else
        [displayedValues setObject:no forKey:D1FileEnabled];
    if ([d2FileEnabledButton state] == NSOnState)
        [displayedValues setObject:yes forKey:D2FileEnabled];
    else
        [displayedValues setObject:no forKey:D2FileEnabled];
    if ([d3FileEnabledButton state] == NSOnState)
        [displayedValues setObject:yes forKey:D3FileEnabled];
    else
        [displayedValues setObject:no forKey:D3FileEnabled];
    if ([d4FileEnabledButton state] == NSOnState)
        [displayedValues setObject:yes forKey:D4FileEnabled];
    else
        [displayedValues setObject:no forKey:D4FileEnabled];
    if ([d5FileEnabledButton state] == NSOnState)
        [displayedValues setObject:yes forKey:D5FileEnabled];
    else
        [displayedValues setObject:no forKey:D5FileEnabled];
    if ([d6FileEnabledButton state] == NSOnState)
        [displayedValues setObject:yes forKey:D6FileEnabled];
    else
        [displayedValues setObject:no forKey:D6FileEnabled];
    if ([d7FileEnabledButton state] == NSOnState)
        [displayedValues setObject:yes forKey:D7FileEnabled];
    else
        [displayedValues setObject:no forKey:D7FileEnabled];
    if ([d8FileEnabledButton state] == NSOnState)
        [displayedValues setObject:yes forKey:D8FileEnabled];
    else
        [displayedValues setObject:no forKey:D8FileEnabled];
    if ([cartFileEnabledButton state] == NSOnState)
        [displayedValues setObject:yes forKey:CartFileEnabled];
    else
        [displayedValues setObject:no forKey:CartFileEnabled];
    if ([cart2FileEnabledButton state] == NSOnState)
        [displayedValues setObject:yes forKey:Cart2FileEnabled];
    else
        [displayedValues setObject:no forKey:Cart2FileEnabled];
    if ([exeFileEnabledButton state] == NSOnState)
        [displayedValues setObject:yes forKey:ExeFileEnabled];
    else
        [displayedValues setObject:no forKey:ExeFileEnabled];
    if ([cassFileEnabledButton state] == NSOnState)
        [displayedValues setObject:yes forKey:CassFileEnabled];
    else
        [displayedValues setObject:no forKey:CassFileEnabled];
    if ([saveCurrentMediaButton state] == NSOnState)
        [displayedValues setObject:yes forKey:SaveCurrentMedia];
    else
        [displayedValues setObject:no forKey:SaveCurrentMedia];
    if ([clearCurrentMediaButton state] == NSOnState)
        [displayedValues setObject:yes forKey:ClearCurrentMedia];
    else
        [displayedValues setObject:no forKey:ClearCurrentMedia];
	[self setBootMediaActive:![[displayedValues objectForKey:SaveCurrentMedia] boolValue]];

    switch([joystick1Pulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:Joystick1Mode];
            break;
        case 1:
            [displayedValues setObject:one forKey:Joystick1Mode];
            break;
        case 2:
            [displayedValues setObject:two forKey:Joystick1Mode];
            break;
        case 3:
            [displayedValues setObject:three forKey:Joystick1Mode];
            break;
        case 4:
            [displayedValues setObject:four forKey:Joystick1Mode];
            break;
        case 5:
            [displayedValues setObject:five forKey:Joystick1Mode];
            break;
        case 6:
            [displayedValues setObject:six forKey:Joystick1Mode];
            break;
        case 7:
            [displayedValues setObject:seven forKey:Joystick1Mode];
            mouseCount++;
            break;
        }
    switch([joystick2Pulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:Joystick2Mode];
            break;
        case 1:
            [displayedValues setObject:one forKey:Joystick2Mode];
            break;
        case 2:
            [displayedValues setObject:two forKey:Joystick2Mode];
            break;
        case 3:
            [displayedValues setObject:three forKey:Joystick2Mode];
            break;
        case 4:
            [displayedValues setObject:four forKey:Joystick2Mode];
            break;
        case 5:
            [displayedValues setObject:five forKey:Joystick2Mode];
            break;
        case 6:
            [displayedValues setObject:six forKey:Joystick2Mode];
            break;
        case 7:
            [displayedValues setObject:seven forKey:Joystick2Mode];
            mouseCount++;
            break;
        }
    switch([joystick3Pulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:Joystick3Mode];
            break;
        case 1:
            [displayedValues setObject:one forKey:Joystick3Mode];
            break;
        case 2:
            [displayedValues setObject:two forKey:Joystick3Mode];
            break;
        case 3:
            [displayedValues setObject:three forKey:Joystick3Mode];
            break;
        case 4:
            [displayedValues setObject:four forKey:Joystick3Mode];
            break;
        case 5:
            [displayedValues setObject:five forKey:Joystick3Mode];
            break;
        case 6:
            [displayedValues setObject:six forKey:Joystick3Mode];
            break;
        case 7:
            [displayedValues setObject:seven forKey:Joystick3Mode];
            mouseCount++;
            break;
        }
    switch([joystick4Pulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:Joystick4Mode];
            break;
        case 1:
            [displayedValues setObject:one forKey:Joystick4Mode];
            break;
        case 2:
            [displayedValues setObject:two forKey:Joystick4Mode];
            break;
        case 3:
            [displayedValues setObject:three forKey:Joystick4Mode];
            break;
        case 4:
            [displayedValues setObject:four forKey:Joystick4Mode];
            break;
        case 5:
            [displayedValues setObject:five forKey:Joystick4Mode];
            break;
        case 6:
            [displayedValues setObject:six forKey:Joystick4Mode];
            break;
        case 7:
            [displayedValues setObject:seven forKey:Joystick4Mode];
            mouseCount++;
            break;
        }
    if (mouseCount >1) {
        if ([joystick1Pulldown indexOfSelectedItem] == 5)
            firstMouse = 1;
        else if ([joystick2Pulldown indexOfSelectedItem] == 5) 
            firstMouse = 2;
        else if ([joystick2Pulldown indexOfSelectedItem] == 5)
            firstMouse = 3;
            
        if (firstMouse == 1) {
            if ([joystick2Pulldown indexOfSelectedItem] == 5) {
                [joystick2Pulldown  selectItemAtIndex:0];
                [displayedValues setObject:zero forKey:Joystick2Mode];
                }
            else if ([joystick3Pulldown indexOfSelectedItem] == 5) {
                [joystick3Pulldown  selectItemAtIndex:0];
                [displayedValues setObject:zero forKey:Joystick3Mode];
                }
            else {
                [joystick4Pulldown  selectItemAtIndex:0];
                [displayedValues setObject:zero forKey:Joystick4Mode];
                }
            }
        else if (firstMouse == 2) {
            if ([joystick3Pulldown indexOfSelectedItem] == 5) {
                [joystick3Pulldown  selectItemAtIndex:0];
                [displayedValues setObject:zero forKey:Joystick3Mode];
                }
            else {
                [joystick4Pulldown  selectItemAtIndex:0];
                [displayedValues setObject:zero forKey:Joystick4Mode];
                }
            }
        else {
            [joystick4Pulldown  selectItemAtIndex:0];
            [displayedValues setObject:zero forKey:Joystick4Mode];
            }
        mouseCount = 1;              
        }
    switch([joy1AutofirePulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:Joystick1Autofire];
            break;
        case 1:
            [displayedValues setObject:one forKey:Joystick1Autofire];
            break;
        case 2:
            [displayedValues setObject:two forKey:Joystick1Autofire];
            break;
        }
    switch([joy2AutofirePulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:Joystick2Autofire];
            break;
        case 1:
            [displayedValues setObject:one forKey:Joystick2Autofire];
            break;
        case 2:
            [displayedValues setObject:two forKey:Joystick2Autofire];
            break;
        }
    switch([joy3AutofirePulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:Joystick3Autofire];
            break;
        case 1:
            [displayedValues setObject:one forKey:Joystick3Autofire];
            break;
        case 2:
            [displayedValues setObject:two forKey:Joystick3Autofire];
            break;
        }
    switch([joy4AutofirePulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:Joystick4Autofire];
            break;
        case 1:
            [displayedValues setObject:one forKey:Joystick4Autofire];
            break;
        case 2:
            [displayedValues setObject:two forKey:Joystick4Autofire];
            break;
        }
    if (mouseCount == 0) {
        [mouseDevicePulldown  selectItemAtIndex:0];
        [mouseDevicePulldown setEnabled:NO];
        [mouseSpeedField setEnabled:NO];
        [mouseMinValField setEnabled:NO];
        [mouseMaxValField setEnabled:NO];
        [mouseHOffsetField setEnabled:NO];
        [mouseVOffsetField setEnabled:NO];
        [mouseInertiaField setEnabled:NO];
        [displayedValues setObject:zero forKey:MouseDevice];
        }
    else {
        [mouseDevicePulldown setEnabled:YES];
        switch([mouseDevicePulldown indexOfSelectedItem]) {
            case 0:
                [displayedValues setObject:zero forKey:MouseDevice];
                [mouseSpeedField setEnabled:NO];
                [mouseMinValField setEnabled:NO];
                [mouseMaxValField setEnabled:NO];
                [mouseHOffsetField setEnabled:NO];
                [mouseVOffsetField setEnabled:NO];
                [mouseInertiaField setEnabled:NO];
                break;
            case 1:
                [displayedValues setObject:one forKey:MouseDevice];
                [mouseSpeedField setEnabled:YES];
                [mouseMinValField setEnabled:YES];
                [mouseMaxValField setEnabled:YES];
                [mouseHOffsetField setEnabled:NO];
                [mouseVOffsetField setEnabled:NO];
                [mouseInertiaField setEnabled:NO];
                break;
            case 2:
                [displayedValues setObject:two forKey:MouseDevice];
                [mouseSpeedField setEnabled:YES];
                [mouseMinValField setEnabled:YES];
            [mouseMaxValField setEnabled:YES];
            [mouseHOffsetField setEnabled:NO];
            [mouseVOffsetField setEnabled:NO];
            [mouseInertiaField setEnabled:NO];
            break;
        case 3:
            [displayedValues setObject:three forKey:MouseDevice];
            [mouseSpeedField setEnabled:YES];
            [mouseMinValField setEnabled:YES];
            [mouseMaxValField setEnabled:YES];
            [mouseHOffsetField setEnabled:NO];
            [mouseVOffsetField setEnabled:NO];
            [mouseInertiaField setEnabled:NO];
            break;
        case 4:
            [displayedValues setObject:four forKey:MouseDevice];
            [mouseSpeedField setEnabled:YES];
            [mouseMinValField setEnabled:NO];
            [mouseMaxValField setEnabled:NO];
            [mouseHOffsetField setEnabled:YES];
            [mouseVOffsetField setEnabled:YES];
            [mouseInertiaField setEnabled:NO];
            break;
        case 5:
            [displayedValues setObject:five forKey:MouseDevice];
            [mouseSpeedField setEnabled:YES];
            [mouseMinValField setEnabled:NO];
            [mouseMaxValField setEnabled:NO];
            [mouseHOffsetField setEnabled:YES];
            [mouseVOffsetField setEnabled:YES];
            [mouseInertiaField setEnabled:NO];
            break;
        case 6:
            [displayedValues setObject:six forKey:MouseDevice];
            [mouseSpeedField setEnabled:YES];
            [mouseMinValField setEnabled:NO];
            [mouseMaxValField setEnabled:NO];
            [mouseHOffsetField setEnabled:NO];
            [mouseVOffsetField setEnabled:NO];
            [mouseInertiaField setEnabled:NO];
            break;
        case 7:
            [displayedValues setObject:seven forKey:MouseDevice];
            [mouseSpeedField setEnabled:YES];
            [mouseMinValField setEnabled:NO];
            [mouseMaxValField setEnabled:NO];
            [mouseHOffsetField setEnabled:NO];
            [mouseVOffsetField setEnabled:NO];
            [mouseInertiaField setEnabled:NO];
            break;
        case 8:
            [displayedValues setObject:eight forKey:MouseDevice];
            [mouseSpeedField setEnabled:YES];
            [mouseMinValField setEnabled:NO];
            [mouseMaxValField setEnabled:NO];
            [mouseHOffsetField setEnabled:NO];
            [mouseVOffsetField setEnabled:NO];
            [mouseInertiaField setEnabled:NO];
            break;
        case 9:
            [displayedValues setObject:nine forKey:MouseDevice];
            [mouseSpeedField setEnabled:YES];
            [mouseMinValField setEnabled:NO];
            [mouseMaxValField setEnabled:NO];
            [mouseHOffsetField setEnabled:NO];
            [mouseVOffsetField setEnabled:NO];
            [mouseInertiaField setEnabled:YES];
            break;
        }
        }
    anInt = [mouseSpeedField intValue];
    [displayedValues setObject:[NSNumber numberWithInt:anInt] forKey:MouseSpeed];
    anInt = [mouseMinValField intValue];
    [displayedValues setObject:[NSNumber numberWithInt:anInt] forKey:MouseMinVal];
    anInt = [mouseMaxValField intValue];
    [displayedValues setObject:[NSNumber numberWithInt:anInt] forKey:MouseMaxVal];
    anInt = [mouseHOffsetField intValue];
    [displayedValues setObject:[NSNumber numberWithInt:anInt] forKey:MouseHOffset];
    anInt = [mouseVOffsetField intValue];
    [displayedValues setObject:[NSNumber numberWithInt:anInt] forKey:MouseVOffset];
    anInt = [mouseInertiaField intValue];
    [displayedValues setObject:[NSNumber numberWithInt:anInt] forKey:MouseInertia];

    switch([joystick1TypePulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:Joystick1Type];
            break;
        case 1:
            [displayedValues setObject:one forKey:Joystick1Type];
			[displayedValues setObject:no forKey:Joystick1MultiMode];
			break;
        case 2:
            [displayedValues setObject:two forKey:Joystick1Type];
			[displayedValues setObject:no forKey:Joystick1MultiMode];
            break;
        }
    switch([joystick2TypePulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:Joystick2Type];
            break;
        case 1:
            [displayedValues setObject:one forKey:Joystick2Type];
			[displayedValues setObject:no forKey:Joystick2MultiMode];
            break;
        case 2:
            [displayedValues setObject:two forKey:Joystick2Type];
			[displayedValues setObject:no forKey:Joystick2MultiMode];
            break;
        }
    switch([joystick3TypePulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:Joystick3Type];
            break;
        case 1:
            [displayedValues setObject:one forKey:Joystick3Type];
			[displayedValues setObject:no forKey:Joystick3MultiMode];
            break;
        case 2:
            [displayedValues setObject:two forKey:Joystick3Type];
			[displayedValues setObject:no forKey:Joystick3MultiMode];
            break;
        }
    switch([joystick4TypePulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:Joystick4Type];
            break;
        case 1:
            [displayedValues setObject:one forKey:Joystick4Type];
 			[displayedValues setObject:no forKey:Joystick4MultiMode];
           break;
        case 2:
            [displayedValues setObject:two forKey:Joystick4Type];
			[displayedValues setObject:no forKey:Joystick4MultiMode];
            break;
        }

    [self updateJoyNumMenus];

    switch([joystick1NumPulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:Joystick1Num];
            [displayedValues setObject:no forKey:Joystick1MultiMode];
          break;
        case 1:
            [displayedValues setObject:one forKey:Joystick1Num];
			[displayedValues setObject:no forKey:Joystick1MultiMode];
			break;
        case 2:
            [displayedValues setObject:two forKey:Joystick1Num];
			[displayedValues setObject:no forKey:Joystick1MultiMode];
			break;
        case 3:
            [displayedValues setObject:zero forKey:Joystick1Num];
			[displayedValues setObject:yes forKey:Joystick1MultiMode];
			break;
		}
    switch([joystick2NumPulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:Joystick2Num];
            [displayedValues setObject:no forKey:Joystick2MultiMode];
            break;
        case 1:
            [displayedValues setObject:one forKey:Joystick2Num];
            [displayedValues setObject:no forKey:Joystick2MultiMode];
            break;
        case 2:
            [displayedValues setObject:two forKey:Joystick2Num];
            [displayedValues setObject:no forKey:Joystick2MultiMode];
            break;
        case 3:
            [displayedValues setObject:zero forKey:Joystick2Num];
			[displayedValues setObject:yes forKey:Joystick2MultiMode];
			break;
	}
    switch([joystick3NumPulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:Joystick3Num];
            [displayedValues setObject:no forKey:Joystick3MultiMode];
            break;
        case 1:
            [displayedValues setObject:one forKey:Joystick3Num];
            [displayedValues setObject:no forKey:Joystick3MultiMode];
            break;
        case 2:
            [displayedValues setObject:two forKey:Joystick3Num];
            [displayedValues setObject:no forKey:Joystick3MultiMode];
            break;
        case 3:
            [displayedValues setObject:zero forKey:Joystick3Num];
			[displayedValues setObject:yes forKey:Joystick3MultiMode];
			break;
	}
    switch([joystick4NumPulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:Joystick4Num];
            [displayedValues setObject:no forKey:Joystick4MultiMode];
            break;
        case 1:
            [displayedValues setObject:one forKey:Joystick4Num];
            [displayedValues setObject:no forKey:Joystick4MultiMode];
            break;
        case 2:
            [displayedValues setObject:two forKey:Joystick4Num];
            [displayedValues setObject:no forKey:Joystick4MultiMode];
            break;
        case 3:
            [displayedValues setObject:zero forKey:Joystick4Num];
			[displayedValues setObject:yes forKey:Joystick4MultiMode];
			break;
	}
        
    if ([paddlesXAxisOnlyButton state] == NSOnState)
        [displayedValues setObject:yes forKey:PaddlesXAxisOnly];
    else
        [displayedValues setObject:no forKey:PaddlesXAxisOnly];
	
    if ([cx85EnabledButton state] == NSOnState) {
        [displayedValues setObject:yes forKey:CX85Enabled];
		[cx85PortPulldown setEnabled:YES];
	}
    else {
        [displayedValues setObject:no forKey:CX85Enabled];
		[cx85PortPulldown setEnabled:NO];
	}
    switch([cx85PortPulldown indexOfSelectedItem]) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:CX85Port];
            break;
        case 1:
            [displayedValues setObject:one forKey:CX85Port];
            break;
        case 2:
            [displayedValues setObject:two forKey:CX85Port];
            break;
        case 3:
            [displayedValues setObject:three forKey:CX85Port];
            break;
	}
	
    [displayedValues setObject:[gamepad1ConfigPulldown title] forKey:Gamepad1ConfigCurrent];
    [displayedValues setObject:[gamepad2ConfigPulldown title] forKey:Gamepad2ConfigCurrent];
    [displayedValues setObject:[gamepad3ConfigPulldown title] forKey:Gamepad3ConfigCurrent];
    [displayedValues setObject:[gamepad4ConfigPulldown title] forKey:Gamepad4ConfigCurrent];

}


/*------------------------------------------------------------------------------
* browseFile - Method which allows user to choose a file.
*-----------------------------------------------------------------------------*/
- (NSString *) browseFile {
    NSOpenPanel *openPanel;
    
    openPanel = [NSOpenPanel openPanel];
    [openPanel setCanChooseDirectories:NO];
    [openPanel setCanChooseFiles:YES];
    
    if ([openPanel runModal] == NSModalResponseOK)
        return([[[openPanel URLs] objectAtIndex:0] path]);
    else
        return nil;
    }

/*------------------------------------------------------------------------------
* browseFileInDirectory - Method which allows user to choose a file in a 
*     specific directory.
*-----------------------------------------------------------------------------*/
- (NSString *) browseFileInDirectory:(NSString *)directory {
    NSOpenPanel *openPanel;
    
    openPanel = [NSOpenPanel openPanel];
    [openPanel setCanChooseDirectories:NO];
    [openPanel setCanChooseFiles:YES];
    [openPanel setDirectoryURL:[NSURL fileURLWithPath:directory]];
    if ([openPanel runModal] == NSModalResponseOK)
        return([[[openPanel URLs] objectAtIndex:0] path]);
    else
        return nil;
    }

/*------------------------------------------------------------------------------
 *  saveFileInDirectory - This allows the user to chose a filename to save in from
 *     the specified directory.
 *-----------------------------------------------------------------------------*/
- (NSString *) saveFileInDirectory:(NSString *)directory:(NSString *)type {
    NSSavePanel *savePanel = nil;
    
    savePanel = [NSSavePanel savePanel];
    
    [savePanel setAllowedFileTypes:[NSArray arrayWithObject:type]];
    [savePanel setDirectoryURL:[NSURL fileURLWithPath:directory]];
    if ([savePanel runModal] == NSModalResponseOK)
        return([[savePanel URL] path]);
    else
        return nil;
}

/*------------------------------------------------------------------------------
* browseDir - Method which allows user to choose a directory.
*-----------------------------------------------------------------------------*/
- (NSString *) browseDir {
    NSOpenPanel *openPanel;
    
    openPanel = [NSOpenPanel openPanel];
    [openPanel setCanChooseDirectories:YES];
    [openPanel setCanChooseFiles:NO];
    
    if ([openPanel runModal] == NSModalResponseOK)
        return([[[openPanel URLs] objectAtIndex:0] path]);
    else
        return nil;
    }

/* The following methods allow the user to choose the color Palette file */
- (void)browsePalette:(id)sender {
    NSString *filename, *dir;
    
    dir = [[NSString alloc] initWithCString:paletteDir encoding:NSASCIIStringEncoding];
    filename = [self browseFileInDirectory:dir];
    if (filename != nil) {
        [paletteField setStringValue:filename];
        [self miscChanged:self];
        }
    [dir release];
    }

- (void)browseImage:(id)sender {
    NSString *dirname;
    
    dirname = [self browseDir];
    if (dirname != nil) {
        [imageDirField setStringValue:dirname];
        [self miscChanged:self];
        }
    }

- (void)browsePrint:(id)sender {
    NSString *dirname;
    
    dirname = [self browseDir];
    if (dirname != nil) {
        [printDirField setStringValue:dirname];
        [self miscChanged:self];
        }
    }

/* The following methods allow the user to choose the Hard Disk Drive 
   directories */
- (void)browseHardDisk1:(id)sender {
    NSString *dirname;
    
    dirname = [self browseDir];
    if (dirname != nil) {
        [hardDiskDir1Field setStringValue:dirname];
        [self miscChanged:self];
        }
    }

- (void)browseHardDisk2:(id)sender {
    NSString *dirname;
    
    dirname = [self browseDir];
    if (dirname != nil) {
        [hardDiskDir2Field setStringValue:dirname];
        [self miscChanged:self];
        }
    }

- (void)browseHardDisk3:(id)sender {
    NSString *dirname;
    
    dirname = [self browseDir];
    if (dirname != nil) {
        [hardDiskDir3Field setStringValue:dirname];
        [self miscChanged:self];
        }
    }

- (void)browseHardDisk4:(id)sender {
    NSString *dirname;
    
    dirname = [self browseDir];
    if (dirname != nil) {
        [hardDiskDir4Field setStringValue:dirname];
        [self miscChanged:self];
        }
    }

/* The following methods allow the user to choose the ROM files */
   
- (void)browseOsARom:(id)sender {
    NSString *filename, *dir;
    
    dir = [[NSString alloc] initWithCString:osromsDir encoding:NSASCIIStringEncoding];
    filename = [self browseFileInDirectory:dir];
    if (filename != nil) {
        [osARomFileField setStringValue:filename];
        [self miscChanged:self];
        }
    [dir release];
    }
    
- (void)browseOsBRom:(id)sender {
    NSString *filename, *dir;
    
    dir = [[NSString alloc] initWithCString:osromsDir encoding:NSASCIIStringEncoding];
    filename = [self browseFileInDirectory:dir];
    if (filename != nil) {
        [osBRomFileField setStringValue:filename];
        [self miscChanged:self];
        }
    [dir release];
    }
    
- (void)browseXlRom:(id)sender {
    NSString *filename, *dir;
    
    dir = [[NSString alloc] initWithCString:osromsDir encoding:NSASCIIStringEncoding];
    filename = [self browseFileInDirectory:dir];
    if (filename != nil) {
        [xlRomFileField setStringValue:filename];
        [self miscChanged:self];
        }
    [dir release];
    }
    
- (void)browseBasicRom:(id)sender {
    NSString *filename, *dir;
    
    dir = [[NSString alloc] initWithCString:osromsDir encoding:NSASCIIStringEncoding];
    filename = [self browseFileInDirectory:dir];
    if (filename != nil) {
        [basicRomFileField setStringValue:filename];
        [self miscChanged:self];
        }
    [dir release];
    }
    
- (void)browse5200Rom:(id)sender {
    NSString *filename, *dir;
    
    dir = [[NSString alloc] initWithCString:osromsDir encoding:NSASCIIStringEncoding];
    filename = [self browseFileInDirectory:dir];
    if (filename != nil) {
        [a5200RomFileField setStringValue:filename];
        [self miscChanged:self];
        }
    [dir release];
    }

- (void)browseMioRom:(id)sender {
    NSString *filename, *dir;
    
    dir = [[NSString alloc] initWithCString:osromsDir encoding:NSASCIIStringEncoding];
    filename = [self browseFileInDirectory:dir];
    if (filename != nil) {
        [mioRomFileField setStringValue:filename];
        [self miscChanged:self];
	}
    [dir release];
}

- (void)browseBlackBoxRom:(id)sender {
    NSString *filename, *dir;
    
    dir = [[NSString alloc] initWithCString:osromsDir encoding:NSASCIIStringEncoding];
    filename = [self browseFileInDirectory:dir];
    if (filename != nil) {
        [blackBoxRomFileField setStringValue:filename];
        [self miscChanged:self];
	}
    [dir release];
}

- (void)browseBlackBoxScsiDiskFile:(id)sender {
    NSString *filename, *dir;
    
    dir = [[NSString alloc] initWithCString:diskImageDirStr encoding:NSASCIIStringEncoding];
    filename = [self browseFileInDirectory:dir];
    if (filename != nil) {
        [blackBoxScsiDiskFileField setStringValue:filename];
        [self miscChanged:self];
	}
    [dir release];
}

- (void)browseMioScsiDiskFile:(id)sender {
    NSString *filename, *dir;
    
    dir = [[NSString alloc] initWithCString:diskImageDirStr encoding:NSASCIIStringEncoding];
    filename = [self browseFileInDirectory:dir];
    if (filename != nil) {
        [mioScsiDiskFileField setStringValue:filename];
        [self miscChanged:self];
	}
    [dir release];
}
/* The following methods allow the user to choose the default directories
    for files */
    
- (void)browseDiskDir:(id)sender {
    NSString *dirname;
    
    dirname = [self browseDir];
    if (dirname != nil) {
        [diskImageDirField setStringValue:dirname];
        [self miscChanged:self];
        }
    }

- (void)browseDiskSetDir:(id)sender {
    NSString *dirname;
    
    dirname = [self browseDir];
    if (dirname != nil) {
        [diskSetDirField setStringValue:dirname];
        [self miscChanged:self];
        }
    }

- (void)browseCartDir:(id)sender {
    NSString *dirname;
    
    dirname = [self browseDir];
    if (dirname != nil) {
        [cartImageDirField setStringValue:dirname];
        [self miscChanged:self];
        }
    }

- (void)browseCassDir:(id)sender {
    NSString *dirname;
    
    dirname = [self browseDir];
    if (dirname != nil) {
        [cassImageDirField setStringValue:dirname];
        [self miscChanged:self];
        }
    }

- (void)browseExeDir:(id)sender {
    NSString *dirname;
    
    dirname = [self browseDir];
    if (dirname != nil) {
        [exeFileDirField setStringValue:dirname];
        [self miscChanged:self];
        }
    }

- (void)browseStateDir:(id)sender {
    NSString *dirname;
    
    dirname = [self browseDir];
    if (dirname != nil) {
        [savedStateDirField setStringValue:dirname];
        [self miscChanged:self];
	}
}

- (void)browseConfigDir:(id)sender {
    NSString *dirname;
    
    dirname = [self browseDir];
    if (dirname != nil) {
        [configDirField setStringValue:dirname];
        [self miscChanged:self];
	}
}

/* The following methods allow the user to choose the files for disks, cartridges
   and cassettes which are inserted at the emulator startup */

- (void)browseD1File:(id)sender {
    NSString *filename;
    
    filename = [self browseFileInDirectory:[curValues objectForKey:DiskImageDir]];
    if (filename != nil) {
        [d1FileField setStringValue:filename];
        [self miscChanged:self];
        }
    }
    
- (void)browseD2File:(id)sender {
    NSString *filename;
    
    filename = [self browseFileInDirectory:[curValues objectForKey:DiskImageDir]];
    if (filename != nil) {
        [d2FileField setStringValue:filename];
        [self miscChanged:self];
        }
    }
    
- (void)browseD3File:(id)sender {
    NSString *filename;
    
    filename = [self browseFileInDirectory:[curValues objectForKey:DiskImageDir]];
    if (filename != nil) {
        [d3FileField setStringValue:filename];
        [self miscChanged:self];
        }
    }
    
- (void)browseD4File:(id)sender {
    NSString *filename;
    
    filename = [self browseFileInDirectory:[curValues objectForKey:DiskImageDir]];
    if (filename != nil) {
        [d4FileField setStringValue:filename];
        [self miscChanged:self];
        }
    }
    
- (void)browseD5File:(id)sender {
    NSString *filename;
    
    filename = [self browseFileInDirectory:[curValues objectForKey:DiskImageDir]];
    if (filename != nil) {
        [d5FileField setStringValue:filename];
        [self miscChanged:self];
        }
    }
    
- (void)browseD6File:(id)sender {
    NSString *filename;
    
    filename = [self browseFileInDirectory:[curValues objectForKey:DiskImageDir]];
    if (filename != nil) {
        [d6FileField setStringValue:filename];
        [self miscChanged:self];
        }
    }
    
- (void)browseD7File:(id)sender {
    NSString *filename;
    
    filename = [self browseFileInDirectory:[curValues objectForKey:DiskImageDir]];
    if (filename != nil) {
        [d7FileField setStringValue:filename];
        [self miscChanged:self];
        }
    }
    
- (void)browseD8File:(id)sender {
    NSString *filename;
    
    filename = [self browseFileInDirectory:[curValues objectForKey:DiskImageDir]];
    if (filename != nil) {
        [d8FileField setStringValue:filename];
        [self miscChanged:self];
        }
    }
    
- (void)browseCartFile:(id)sender {
    NSString *filename;
    
    filename = [self browseFileInDirectory:[curValues objectForKey:CartImageDir]];
    if (filename != nil) {
        [cartFileField setStringValue:filename];
        [self miscChanged:self];
        }
    }
    
- (void)browseCart2File:(id)sender {
    NSString *filename;
    
    filename = [self browseFileInDirectory:[curValues objectForKey:CartImageDir]];
    if (filename != nil) {
        [cart2FileField setStringValue:filename];
        [self miscChanged:self];
        }
    }
    
- (void)browseExeFile:(id)sender {
    NSString *filename;
    
    filename = [self browseFileInDirectory:[curValues objectForKey:ExeFileDir]];
    if (filename != nil) {
        [exeFileField setStringValue:filename];
        [self miscChanged:self];
        }
    }

- (void)browseCassFile:(id)sender {
    NSString *filename;
    
    filename = [self browseFileInDirectory:[curValues objectForKey:CassImageDir]];
    if (filename != nil) {
        [cassFileField setStringValue:filename];
        [self miscChanged:self];
        }
    }

/**** Commit/revert etc ****/

- (void)commitDisplayedValues {
    if (curValues != displayedValues) {
        [curValues release];
        curValues = [displayedValues copyWithZone:[self zone]];
    }
}

- (void)discardDisplayedValues {
    if (curValues != displayedValues) {
        [displayedValues release];
        displayedValues = [curValues mutableCopyWithZone:[self zone]];
        [self updateUI];
    }
}

- (void)transferValuesToAtari825
	{
	prefs825.charSet = [[curValues objectForKey:Atari825CharSet] intValue];
	prefs825.formLength = [[curValues objectForKey:Atari825FormLength] intValue];
	prefs825.autoLinefeed = [[curValues objectForKey:Atari825AutoLinefeed] intValue];
	}
	
- (void)transferValuesToAtari1020
	{
	prefs1020.printWidth = [[curValues objectForKey:Atari1020PrintWidth] intValue];
	prefs1020.formLength = [[curValues objectForKey:Atari1020FormLength] intValue];
	prefs1020.autoLinefeed = [[curValues objectForKey:Atari1020AutoLinefeed] intValue];
	prefs1020.autoPageAdjust = [[curValues objectForKey:Atari1020AutoPageAdjust] intValue];
	prefs1020.pen1Red = [[curValues objectForKey:Atari1020Pen1Red] floatValue];
	prefs1020.pen1Blue = [[curValues objectForKey:Atari1020Pen1Blue] floatValue];
	prefs1020.pen1Green = [[curValues objectForKey:Atari1020Pen1Green] floatValue];
	prefs1020.pen1Alpha = [[curValues objectForKey:Atari1020Pen1Alpha] floatValue];
	prefs1020.pen2Red = [[curValues objectForKey:Atari1020Pen2Red] floatValue];
	prefs1020.pen2Blue = [[curValues objectForKey:Atari1020Pen2Blue] floatValue];
	prefs1020.pen2Green = [[curValues objectForKey:Atari1020Pen2Green] floatValue];
	prefs1020.pen2Alpha = [[curValues objectForKey:Atari1020Pen2Alpha] floatValue];
	prefs1020.pen3Red = [[curValues objectForKey:Atari1020Pen3Red] floatValue];
	prefs1020.pen3Blue = [[curValues objectForKey:Atari1020Pen3Blue] floatValue];
	prefs1020.pen3Green = [[curValues objectForKey:Atari1020Pen3Green] floatValue];
	prefs1020.pen3Alpha = [[curValues objectForKey:Atari1020Pen3Alpha] floatValue];
	prefs1020.pen4Red = [[curValues objectForKey:Atari1020Pen4Red] floatValue];
	prefs1020.pen4Blue = [[curValues objectForKey:Atari1020Pen4Blue] floatValue];
	prefs1020.pen4Green = [[curValues objectForKey:Atari1020Pen4Green] floatValue];
	prefs1020.pen4Alpha = [[curValues objectForKey:Atari1020Pen4Alpha] floatValue];
	}
	
- (void)transferValuesToAtascii
    {
    prefsAtascii.formLength = [[curValues objectForKey:AtasciiFormLength] intValue];
    prefsAtascii.charSet = [[curValues objectForKey:AtasciiCharSet] intValue];
    prefsAtascii.charSize = [[curValues objectForKey:AtasciiCharSize] intValue];
    prefsAtascii.lineGap = [[curValues objectForKey:AtasciiLineGap] intValue];
    }

- (void)transferValuesToEpson
	{
	prefsEpson.charSet = [[curValues objectForKey:EpsonCharSet] intValue];
	prefsEpson.formLength = [[curValues objectForKey:EpsonFormLength] intValue];
	prefsEpson.printPitch = [[curValues objectForKey:EpsonPrintPitch] intValue];
	prefsEpson.printWeight = [[curValues objectForKey:EpsonPrintWeight] intValue];
	prefsEpson.autoLinefeed = [[curValues objectForKey:EpsonAutoLinefeed] intValue];
	prefsEpson.printSlashedZeros = [[curValues objectForKey:EpsonPrintSlashedZeros] intValue];
	prefsEpson.autoSkip = [[curValues objectForKey:EpsonAutoSkip] intValue];
	prefsEpson.splitSkip = [[curValues objectForKey:EpsonSplitSkip] intValue];
	}

/*------------------------------------------------------------------------------
* transferValuesToEmulator - Method which allows preferences to be transfered
*   to the 'C' structure which is a buffer between the emulator code and this
*   Cocoa code.
*-----------------------------------------------------------------------------*/
- (void)transferValuesToEmulator {
    ATARI800MACX_PREF *prefs;
    int i,j;
    NSString *configString = NULL;
    NSString *buttonKey, *button5200Key;
    
    prefs = getPrefStorage();
    prefs->fullScreen = [[curValues objectForKey:FullScreen] intValue]; 
    prefs->openGl = [[curValues objectForKey:OpenGl] intValue]; 
    prefs->lockFullscreenSize = [[curValues objectForKey:LockFullscreenSize] intValue]; 
    prefs->fullscreenMonitor = [[curValues objectForKey:FullscreenMonitor] intValue];
	prefs->fullForeRed = (int) ([[curValues objectForKey:FullscreenForeRed] floatValue] * 255);
	prefs->fullForeGreen = (int) ([[curValues objectForKey:FullscreenForeGreen] floatValue] * 255);
	prefs->fullForeBlue = (int) ([[curValues objectForKey:FullscreenForeBlue] floatValue] * 255);
	prefs->fullBackRed = (int) ([[curValues objectForKey:FullscreenBackRed] floatValue] * 255);
	prefs->fullBackGreen = (int) ([[curValues objectForKey:FullscreenBackGreen] floatValue] * 255);
	prefs->fullBackBlue = (int) ([[curValues objectForKey:FullscreenBackBlue] floatValue] * 255);
    prefs->spriteCollisions = [[curValues objectForKey:SpriteCollisions] intValue]; 
    prefs->doubleSize = [[curValues objectForKey:DoubleSize] intValue]; 
    prefs->scaleFactor = [[curValues objectForKey:ScaleFactor] intValue]; 
    prefs->widthMode = [[curValues objectForKey:WidthMode] intValue]; 
	prefs->scaleMode = [[curValues objectForKey:ScaleMode] intValue];
    prefs->tvMode = [[curValues objectForKey:TvMode] intValue]; 
    prefs->emulationSpeed = [[curValues objectForKey:EmulationSpeed] floatValue]; 
    prefs->refreshRatio = [[curValues objectForKey:RefreshRatio] intValue]; 
    prefs->artifactingMode = [[curValues objectForKey:ArtifactingMode] intValue]; 
    prefs->artifactNew = [[curValues objectForKey:ArtifactNew] intValue]; 
    prefs->useBuiltinPalette = [[curValues objectForKey:UseBuiltinPalette] intValue]; 
    prefs->adjustPalette = [[curValues objectForKey:AdjustPalette] intValue]; 
    prefs->blackLevel = [[curValues objectForKey:BlackLevel] intValue]; 
    prefs->whiteLevel = [[curValues objectForKey:WhiteLevel] intValue]; 
    prefs->intensity = [[curValues objectForKey:Intensity] intValue]; 
    prefs->colorShift = [[curValues objectForKey:ColorShift] intValue]; 
    [[curValues objectForKey:PaletteFile] getCString:prefs->paletteFile maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
    prefs->showFPS = [[curValues objectForKey:ShowFPS] intValue]; 
    prefs->ledStatus = [[curValues objectForKey:LedStatus] intValue]; 
    prefs->ledSector = [[curValues objectForKey:LedSector] intValue]; 
    prefs->ledStatusMedia = [[curValues objectForKey:LedStatusMedia] intValue]; 
    prefs->ledSectorMedia = [[curValues objectForKey:LedSectorMedia] intValue];
	if ([[curValues objectForKey:AtariTypeVer4] intValue] == -1)
		prefs->atariType = [[curValues objectForKey:AtariType] intValue]; 
	else
		prefs->atariType = [[curValues objectForKey:AtariTypeVer4] intValue] + NUM_ORIG_TYPES; 
	if ([[curValues objectForKey:AtariSwitchTypeVer4] intValue] == -1)
		prefs->atariSwitchType = [[curValues objectForKey:AtariSwitchType] intValue]; 
	else
		prefs->atariSwitchType = [[curValues objectForKey:AtariSwitchTypeVer4] intValue] + NUM_ORIG_TYPES; 
    prefs->disableBasic = [[curValues objectForKey:DisableBasic] intValue]; 
    prefs->disableAllBasic = [[curValues objectForKey:DisableAllBasic] intValue]; 
    prefs->enableSioPatch = [[curValues objectForKey:EnableSioPatch] intValue]; 
    prefs->enableHPatch = [[curValues objectForKey:EnableHPatch] intValue]; 
    prefs->enableDPatch = [[curValues objectForKey:EnableDPatch] intValue]; 
    prefs->enablePPatch = [[curValues objectForKey:EnablePPatch] intValue]; 
    prefs->enableRPatch = [[curValues objectForKey:EnableRPatch] intValue];
    prefs->rPatchPort = [[curValues objectForKey:RPatchPort] intValue];
	prefs->rPatchSerialEnabled = [[curValues objectForKey:RPatchSerialEnabled] intValue];
	prefs->useAtariCursorKeys = [[curValues objectForKey:UseAtariCursorKeys] intValue];
    [[curValues objectForKey:RPatchSerialPort]getCString:prefs->rPatchSerialPort maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
    [[curValues objectForKey:PrintCommand] getCString:prefs->printCommand maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding ];
    prefs->bootFromCassette = [[curValues objectForKey:BootFromCassette] intValue]; 
    prefs->speedLimit = [[curValues objectForKey:SpeedLimit] intValue]; 
    prefs->enableSound = [[curValues objectForKey:EnableSound] intValue]; 
	prefs->soundVolume = [[curValues objectForKey:SoundVolume] floatValue];
    prefs->xep80_enabled = [[curValues objectForKey:XEP80Enabled] intValue];
    prefs->xep80_port = [[curValues objectForKey:XEP80Port] intValue];
    prefs->xep80 = [[curValues objectForKey:XEP80] intValue];
    prefs->xep80_oncolor = [[curValues objectForKey:XEP80OnColor] intValue];
    prefs->xep80_offcolor = [[curValues objectForKey:XEP80OffColor] intValue];
    prefs->enableStereo = [[curValues objectForKey:EnableStereo] intValue]; 
#if 0 /* enableHifiSound is deprecated from 4.2.2 on */    	
    prefs->enableHifiSound = [[curValues objectForKey:EnableHifiSound] intValue]; 
#endif
#ifdef WORDS_BIGENDIAN	
	prefs->enable16BitSound = 0;
#else
	prefs->enable16BitSound = [[curValues objectForKey:Enable16BitSound] intValue];
#endif
  prefs->enableConsoleSound = [[curValues objectForKey:EnableConsoleSound] intValue];
  prefs->enableSerioSound = [[curValues objectForKey:EnableSerioSound] intValue];
  prefs->dontMuteAudio = [[curValues objectForKey:DontMuteAudio] intValue];
  prefs->diskDriveSound = [[curValues objectForKey:DiskDriveSound] intValue];
  prefs->enableInternational = [[curValues objectForKey:EnableInternational] intValue];
  prefs->enableMultijoy = [[curValues objectForKey:EnableMultijoy] intValue];
  prefs->ignoreHeaderWriteprotect = [[curValues objectForKey:IgnoreHeaderWriteprotect] intValue];
	prefs->axlonBankMask =  [[curValues objectForKey:AxlonBankMask] intValue];
	prefs->mosaicMaxBank =  [[curValues objectForKey:MosaicMaxBank] intValue];
	prefs->blackBoxEnabled = [[curValues objectForKey:BlackBoxEnabled] intValue];
	prefs->mioEnabled = [[curValues objectForKey:MioEnabled] intValue];
  [[curValues objectForKey:BlackBoxRomFile] getCString:prefs->blackBoxRomFile maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
	[[curValues objectForKey:MioRomFile] getCString:prefs->mioRomFile maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
	[[curValues objectForKey:BlackBoxScsiDiskFile] getCString:prefs->blackBoxScsiDiskFile maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
	[[curValues objectForKey:MioScsiDiskFile] getCString:prefs->mioScsiDiskFile maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:ImageDir] getCString:prefs->imageDir maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:PrintDir] getCString:prefs->printDir maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:HardDiskDir1] getCString:prefs->hardDiskDir[0] maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:HardDiskDir2] getCString:prefs->hardDiskDir[1] maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:HardDiskDir3] getCString:prefs->hardDiskDir[2] maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:HardDiskDir4] getCString:prefs->hardDiskDir[3] maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  prefs->hardDrivesReadOnly = [[curValues objectForKey:HardDrivesReadOnly] intValue];
  [[curValues objectForKey:HPath] getCString:prefs->hPath maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:OsARomFile] getCString:prefs->osARomFile maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:OsBRomFile] getCString:prefs->osBRomFile maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:XlRomFile] getCString:prefs->xlRomFile maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:BasicRomFile] getCString:prefs->basicRomFile maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:A5200RomFile] getCString:prefs->a5200RomFile maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:DiskImageDir] getCString:prefs->diskImageDir maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:DiskSetDir] getCString:prefs->diskSetDir maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:CartImageDir] getCString:prefs->cartImageDir maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:CassImageDir] getCString:prefs->cassImageDir maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:ExeFileDir] getCString:prefs->exeFileDir maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:SavedStateDir] getCString:prefs->savedStateDir maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:ConfigDir] getCString:prefs->configDir maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:D1File] getCString:prefs->dFile[0] maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:D2File] getCString:prefs->dFile[1] maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:D3File] getCString:prefs->dFile[2] maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:D4File] getCString:prefs->dFile[3] maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:D5File] getCString:prefs->dFile[4] maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:D6File] getCString:prefs->dFile[5] maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:D7File] getCString:prefs->dFile[6] maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:D8File] getCString:prefs->dFile[7] maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:CartFile] getCString:prefs->cartFile maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:Cart2File] getCString:prefs->cart2File maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:ExeFile] getCString:prefs->exeFile maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  [[curValues objectForKey:CassFile] getCString:prefs->cassFile maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
  prefs->saveCurrentMedia = [[curValues objectForKey:SaveCurrentMedia] intValue];
  prefs->clearCurrentMedia = [[curValues objectForKey:ClearCurrentMedia] intValue];
  prefs->dFileEnabled[0] = [[curValues objectForKey:D1FileEnabled] intValue];
  prefs->dFileEnabled[1] = [[curValues objectForKey:D2FileEnabled] intValue];
  prefs->dFileEnabled[2] = [[curValues objectForKey:D3FileEnabled] intValue];
  prefs->dFileEnabled[3] = [[curValues objectForKey:D4FileEnabled] intValue];
  prefs->dFileEnabled[4] = [[curValues objectForKey:D5FileEnabled] intValue];
  prefs->dFileEnabled[5] = [[curValues objectForKey:D6FileEnabled] intValue];
  prefs->dFileEnabled[6] = [[curValues objectForKey:D7FileEnabled] intValue];
  prefs->dFileEnabled[7] = [[curValues objectForKey:D8FileEnabled] intValue];
  prefs->cartFileEnabled = [[curValues objectForKey:CartFileEnabled] intValue];
  prefs->cart2FileEnabled = [[curValues objectForKey:Cart2FileEnabled] intValue];
  prefs->exeFileEnabled = [[curValues objectForKey:ExeFileEnabled] intValue];
  prefs->cassFileEnabled = [[curValues objectForKey:CassFileEnabled] intValue];
  prefs->keyjoyEnable = [[curValues objectForKey:KeyjoyEnable] intValue];
  prefs->joystickMode[0] = [[curValues objectForKey:Joystick1Mode] intValue];
  prefs->joystickMode[1] = [[curValues objectForKey:Joystick2Mode] intValue];
  prefs->joystickMode[2] = [[curValues objectForKey:Joystick3Mode] intValue];
  prefs->joystickMode[3] = [[curValues objectForKey:Joystick4Mode] intValue];
  prefs->joystickAutofire[0] = [[curValues objectForKey:Joystick1Autofire] intValue];
  prefs->joystickAutofire[1] = [[curValues objectForKey:Joystick2Autofire] intValue];
  prefs->joystickAutofire[2] = [[curValues objectForKey:Joystick3Autofire] intValue];
  prefs->joystickAutofire[3] = [[curValues objectForKey:Joystick4Autofire] intValue];
  prefs->mouseDevice = [[curValues objectForKey:MouseDevice] intValue];
  prefs->mouseSpeed = [[curValues objectForKey:MouseSpeed] intValue];
  prefs->mouseMinVal = [[curValues objectForKey:MouseMinVal] intValue];
  prefs->mouseMaxVal = [[curValues objectForKey:MouseMaxVal] intValue];
  prefs->mouseHOffset = [[curValues objectForKey:MouseHOffset] intValue];
  prefs->mouseVOffset = [[curValues objectForKey:MouseVOffset] intValue];
  prefs->mouseInertia = [[curValues objectForKey:MouseInertia] intValue];
  prefs->joystick1Type = [[curValues objectForKey:Joystick1Type] intValue];
  prefs->joystick2Type = [[curValues objectForKey:Joystick2Type] intValue];
  prefs->joystick3Type = [[curValues objectForKey:Joystick3Type] intValue];
  prefs->joystick4Type = [[curValues objectForKey:Joystick4Type] intValue];
  if ([[curValues objectForKey:Joystick1MultiMode] boolValue] == NO)
    prefs->joystick1Num = [[curValues objectForKey:Joystick1Num] intValue];
  else
    prefs->joystick1Num = 9999;
  if ([[curValues objectForKey:Joystick2MultiMode] boolValue] == NO)
    prefs->joystick2Num = [[curValues objectForKey:Joystick2Num] intValue];
  else
    prefs->joystick2Num = 9999;
  if ([[curValues objectForKey:Joystick3MultiMode] boolValue] == NO)
    prefs->joystick3Num = [[curValues objectForKey:Joystick3Num] intValue];
  else
    prefs->joystick3Num = 9999;
  if ([[curValues objectForKey:Joystick4MultiMode] boolValue] == NO)
    prefs->joystick4Num = [[curValues objectForKey:Joystick4Num] intValue];
  else
    prefs->joystick4Num = 9999;
    prefs->paddlesXAxisOnly = [[curValues objectForKey:PaddlesXAxisOnly] intValue];
  prefs->cx85enabled = [[curValues objectForKey:CX85Enabled] intValue];
  prefs->cx85port = [[curValues objectForKey:CX85Port] intValue];

  for (i=0;i<4;i++) {
    switch(i) {
      case 0:
          configString = [curValues objectForKey:Gamepad1ConfigCurrent];
          break;
      case 1:
          configString = [curValues objectForKey:Gamepad2ConfigCurrent];
          break;
      case 2:
          configString = [curValues objectForKey:Gamepad3ConfigCurrent];
          break;
      case 3:
          configString = [curValues objectForKey:Gamepad4ConfigCurrent];
          break;
    }
  
    if ([configString isEqual:StandardConfigString]) {
      for (j=0;j<24;j++) {
          prefs->buttonAssignment[i][j] = 0;
          prefs->button5200Assignment[i][j] = 0;
          }
      }
    else {
      buttonKey = [ButtonAssignmentPrefix stringByAppendingString:configString];
      button5200Key = [Button5200AssignmentPrefix stringByAppendingString:configString];
    
      for (j=0;j<24;j++) {
          prefs->buttonAssignment[i][j] =
              [[[[NSUserDefaults standardUserDefaults] objectForKey:buttonKey] objectAtIndex:j] intValue];
          prefs->button5200Assignment[i][j] =
              [[[[NSUserDefaults standardUserDefaults] objectForKey:button5200Key] objectAtIndex:j] intValue];
          }
      }
  }
        
  prefs->leftJoyUp = [[curValues objectForKey:LeftJoyUp] intValue];
  prefs->leftJoyDown = [[curValues objectForKey:LeftJoyDown] intValue];
  prefs->leftJoyLeft = [[curValues objectForKey:LeftJoyLeft] intValue];
  prefs->leftJoyRight = [[curValues objectForKey:LeftJoyRight] intValue];
  prefs->leftJoyUpLeft = [[curValues objectForKey:LeftJoyUpLeft] intValue];
  prefs->leftJoyUpRight = [[curValues objectForKey:LeftJoyUpRight] intValue];
  prefs->leftJoyDownLeft = [[curValues objectForKey:LeftJoyDownLeft] intValue];
  prefs->leftJoyDownRight = [[curValues objectForKey:LeftJoyDownRight] intValue];
  prefs->leftJoyFire = [[curValues objectForKey:LeftJoyFire] intValue];
  prefs->leftJoyAltFire = [[curValues objectForKey:LeftJoyAltFire] intValue];
  prefs->padJoyUp = [[curValues objectForKey:PadJoyUp] intValue];
  prefs->padJoyDown = [[curValues objectForKey:PadJoyDown] intValue];
  prefs->padJoyLeft = [[curValues objectForKey:PadJoyLeft] intValue];
  prefs->padJoyRight = [[curValues objectForKey:PadJoyRight] intValue];
  prefs->padJoyUpLeft = [[curValues objectForKey:PadJoyUpLeft] intValue];
  prefs->padJoyUpRight = [[curValues objectForKey:PadJoyUpRight] intValue];
  prefs->padJoyDownLeft = [[curValues objectForKey:PadJoyDownLeft] intValue];
  prefs->padJoyDownRight = [[curValues objectForKey:PadJoyDownRight] intValue];
  prefs->padJoyFire = [[curValues objectForKey:PadJoyFire] intValue];
  prefs->padJoyAltFire = [[curValues objectForKey:PadJoyAltFire] intValue];
  prefs->mediaStatusDisplayed = [[curValues objectForKey:MediaStatusDisplayed] intValue];
  prefs->functionKeysDisplayed = [[curValues objectForKey:FunctionKeysDisplayed] intValue];
	prefs->currPrinter =  [[curValues objectForKey:PrinterType] intValue];
}

/*------------------------------------------------------------------------------
*  transferValuesFromEmulator - This method transfers preference values back
*     from the emulator that may have been changed during operation.
*-----------------------------------------------------------------------------*/
- (void)transferValuesFromEmulator:(struct ATARI800MACX_PREFSAVE *)prefssave {
    static NSNumber *yes = nil;
    static NSNumber *no = nil;
    static NSNumber *zero = nil;
    static NSNumber *one = nil;
    static NSNumber *two = nil;
    static NSNumber *three = nil;
    static NSNumber *four = nil;
    static NSNumber *five = nil;
    static NSNumber *six = nil;
    static NSNumber *seven = nil;
    static NSNumber *eight = nil;
    static NSNumber *nine = nil;
    static NSNumber *ten = nil;
    static NSNumber *eleven = nil;
    static NSNumber *twelve = nil;
    static NSNumber *thirteen = nil;
   
    if (!yes) {
        yes = [[NSNumber alloc] initWithBool:YES];
        no = [[NSNumber alloc] initWithBool:NO];
        zero = [[NSNumber alloc] initWithInt:0];
        one = [[NSNumber alloc] initWithInt:1];
        two = [[NSNumber alloc] initWithInt:2];
        three = [[NSNumber alloc] initWithInt:3];
        four = [[NSNumber alloc] initWithInt:4];
        five = [[NSNumber alloc] initWithInt:5];
        six = [[NSNumber alloc] initWithInt:6];
        seven = [[NSNumber alloc] initWithInt:7];
        eight = [[NSNumber alloc] initWithInt:8];
        nine = [[NSNumber alloc] initWithInt:9];
        ten = [[NSNumber alloc] initWithInt:10];
        eleven = [[NSNumber alloc] initWithInt:11];
        twelve = [[NSNumber alloc] initWithInt:12];
        thirteen = [[NSNumber alloc] initWithInt:13];
    }

    [displayedValues setObject:prefssave->fullScreen ? yes : no forKey:FullScreen];
    [displayedValues setObject:prefssave->doubleSize ? yes : no forKey:DoubleSize];
    switch(prefssave->scaleFactor) {
        case 1:
            [displayedValues setObject:one forKey:ScaleFactor];
            break;
        case 2:
            [displayedValues setObject:two forKey:ScaleFactor];
            break;
        case 3:
            [displayedValues setObject:three forKey:ScaleFactor];
            break;
        case 4:
            [displayedValues setObject:four forKey:ScaleFactor];
            break;
		}
    switch(prefssave->widthMode) {
        case 0:
            [displayedValues setObject:zero forKey:WidthMode];
            break;
        case 1:
            [displayedValues setObject:one forKey:WidthMode];
            break;
        case 2:
            [displayedValues setObject:two forKey:WidthMode];
            break;
		}
    switch(prefssave->scaleMode) {
        case 0:
            [displayedValues setObject:zero forKey:ScaleMode];
            break;
        case 1:
            [displayedValues setObject:one forKey:ScaleMode];
            break;
        case 2:
            [displayedValues setObject:two forKey:ScaleMode];
            break;
		}
    [displayedValues setObject:prefssave->showFPS ? yes : no forKey:ShowFPS];
    [displayedValues setObject:prefssave->ledStatus ? yes : no forKey:LedStatus];
    [displayedValues setObject:prefssave->ledSector ? yes : no forKey:LedSector];
    [displayedValues setObject:prefssave->speedLimit ? yes : no forKey:SpeedLimit];
    [displayedValues setObject:prefssave->enableSound ? yes : no forKey:EnableSound];
    [displayedValues setObject:prefssave->xep80 ? yes : no forKey:XEP80];
    [displayedValues setObject:prefssave->xep80_enabled ? yes : no forKey:XEP80Enabled];
    switch(prefssave->xep80_port) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:XEP80Port];
            break;
        case 1:
            [displayedValues setObject:one forKey:XEP80Port];
            break;
		}
    [displayedValues setObject:prefssave->mediaStatusDisplayed ? yes : no forKey:MediaStatusDisplayed];
    [displayedValues setObject:prefssave->functionKeysDisplayed ? yes : no forKey:FunctionKeysDisplayed];
    [displayedValues setObject:prefssave->disableBasic ? yes : no forKey:DisableBasic];
    [displayedValues setObject:prefssave->keyjoyEnable ? yes : no forKey:KeyjoyEnable];
    [displayedValues setObject:prefssave->cx85Enable ? yes : no forKey:CX85Enabled];
    [displayedValues setObject:prefssave->bootFromCassette ? yes : no forKey:BootFromCassette];
    [displayedValues setObject:prefssave->enableSioPatch ? yes : no forKey:EnableSioPatch];
    [displayedValues setObject:prefssave->enableHPatch ? yes : no forKey:EnableHPatch];
    [displayedValues setObject:prefssave->enableDPatch ? yes : no forKey:EnableDPatch];
    [displayedValues setObject:prefssave->hardDrivesReadOnly ? yes : no forKey:HardDrivesReadOnly];
    [displayedValues setObject:prefssave->enablePPatch ? yes : no forKey:EnablePPatch];
    [displayedValues setObject:prefssave->enableRPatch ? yes : no forKey:EnableRPatch];
    switch(prefssave->useAtariCursorKeys) {
        case 0:
		default:
            [displayedValues setObject:zero forKey:UseAtariCursorKeys];
            break;
        case 1:
            [displayedValues setObject:one forKey:UseAtariCursorKeys];
            break;
        case 2:
            [displayedValues setObject:two forKey:UseAtariCursorKeys];
            break;
	}
	if (prefssave->atariType < 14) {
		[displayedValues setObject:[NSNumber numberWithInt:prefssave->atariType] forKey:AtariType];
		[displayedValues setObject:[NSNumber numberWithInt:-1] forKey:AtariTypeVer4];
	} else {
		[displayedValues setObject:zero forKey:AtariType];
		[displayedValues setObject:[NSNumber numberWithInt:(prefssave->atariType-14)] forKey:AtariTypeVer4];
	}
    switch(prefssave->atariType) {
        case 0:
            [displayedValues setObject:zero forKey:AtariType];
            break;
        case 1:
            [displayedValues setObject:one forKey:AtariType];
            break;
        case 2:
            [displayedValues setObject:two forKey:AtariType];
            break;
        case 3:
            [displayedValues setObject:three forKey:AtariType];
            break;
        case 4:
            [displayedValues setObject:four forKey:AtariType];
            break;
        case 5:
            [displayedValues setObject:five forKey:AtariType];
            break;
        case 6:
            [displayedValues setObject:six forKey:AtariType];
            break;
        case 7:
            [displayedValues setObject:seven forKey:AtariType];
            break;
        case 8:
            [displayedValues setObject:eight forKey:AtariType];
            break;
        case 9:
            [displayedValues setObject:nine forKey:AtariType];
            break;
        case 10:
            [displayedValues setObject:ten forKey:AtariType];
            break;
        case 11:
            [displayedValues setObject:eleven forKey:AtariType];
            break;
        case 12:
            [displayedValues setObject:twelve forKey:AtariType];
            break;
        case 13:
            [displayedValues setObject:thirteen forKey:AtariType];
            break;
		}
    switch(prefssave->currPrinter) {
        case 0:
            [displayedValues setObject:zero forKey:PrinterType];
            break;
        case 1:
            [displayedValues setObject:one forKey:PrinterType];
            break;
        case 2:
            [displayedValues setObject:two forKey:PrinterType];
            break;
        case 3:
            [displayedValues setObject:three forKey:PrinterType];
            break;
        case 4:
            [displayedValues setObject:four forKey:PrinterType];
            break;
		}
    switch(prefssave->artifactingMode) {
        case 0:
            [displayedValues setObject:zero forKey:ArtifactingMode];
            break;
        case 1:
            [displayedValues setObject:one forKey:ArtifactingMode];
            break;
        case 2:
            [displayedValues setObject:two forKey:ArtifactingMode];
            break;
        case 3:
            [displayedValues setObject:three forKey:ArtifactingMode];
            break;
        case 4:
            [displayedValues setObject:three forKey:ArtifactingMode];
            break;
		}
    [displayedValues setObject:prefssave->enableStereo ? yes : no forKey:EnableStereo];

	[displayedValues setObject:prefssave->blackBoxEnabled ? yes : no forKey:BlackBoxEnabled];
	[displayedValues setObject:prefssave->mioEnabled ? yes : no forKey:MioEnabled];
	}

- (void) saveCurrentMediaAction:(id)sender
{
	saveMediaPrefs();
	[self updateUI];
}
	
- (void) saveCurrentMedia:(char [][FILENAME_MAX]) disk_filename:(char *) cassette_filename:
						(char *) cart_filename:(char *) cart2_filename
{
    static NSNumber *yes = nil;
    static NSNumber *no = nil;
	
    if (!yes) {
        yes = [[NSNumber alloc] initWithBool:YES];
        no = [[NSNumber alloc] initWithBool:NO];
		}
	
	if ((strcmp(disk_filename[0],"Off") != 0) && (strcmp(disk_filename[0],"Empty") != 0)) {
		[displayedValues setObject:yes forKey:D1FileEnabled];
		[displayedValues setObject:[NSString stringWithCString:disk_filename[0] encoding:NSASCIIStringEncoding] forKey:D1File];
		}
	else {
		[displayedValues setObject:no forKey:D1FileEnabled];
		[displayedValues setObject:@"" forKey:D1File];
		}
	if ((strcmp(disk_filename[1],"Off") != 0) && (strcmp(disk_filename[1],"Empty") != 0)) {
		[displayedValues setObject:yes forKey:D2FileEnabled];
		[displayedValues setObject:[NSString stringWithCString:disk_filename[1] encoding:NSASCIIStringEncoding] forKey:D2File];
		}
	else {
		[displayedValues setObject:no forKey:D2FileEnabled];
		[displayedValues setObject:@"" forKey:D2File];
		}
	if ((strcmp(disk_filename[2],"Off") != 0) && (strcmp(disk_filename[2],"Empty") != 0)) {
		[displayedValues setObject:yes forKey:D3FileEnabled];
		[displayedValues setObject:[NSString stringWithCString:disk_filename[2] encoding:NSASCIIStringEncoding] forKey:D3File];
		}
	else {
		[displayedValues setObject:no forKey:D3FileEnabled];
		[displayedValues setObject:@"" forKey:D3File];
		}
	if ((strcmp(disk_filename[3],"Off") != 0) && (strcmp(disk_filename[3],"Empty") != 0)) {
		[displayedValues setObject:yes forKey:D4FileEnabled];
		[displayedValues setObject:[NSString stringWithCString:disk_filename[3] encoding:NSASCIIStringEncoding] forKey:D4File];
		}
	else {
		[displayedValues setObject:no forKey:D4FileEnabled];
		[displayedValues setObject:@"" forKey:D4File];
		}
	if ((strcmp(disk_filename[4],"Off") != 0) && (strcmp(disk_filename[4],"Empty") != 0)) {
		[displayedValues setObject:yes forKey:D5FileEnabled];
		[displayedValues setObject:[NSString stringWithCString:disk_filename[4] encoding:NSASCIIStringEncoding] forKey:D5File];
		}
	else {
		[displayedValues setObject:no forKey:D5FileEnabled];
		[displayedValues setObject:@"" forKey:D5File];
		}
	if ((strcmp(disk_filename[5],"Off") != 0) && (strcmp(disk_filename[5],"Empty") != 0)) {
		[displayedValues setObject:yes forKey:D6FileEnabled];
		[displayedValues setObject:[NSString stringWithCString:disk_filename[5] encoding:NSASCIIStringEncoding] forKey:D6File];
		}
	else {
		[displayedValues setObject:no forKey:D6FileEnabled];
		[displayedValues setObject:@"" forKey:D6File];
		}
	if ((strcmp(disk_filename[6],"Off") != 0) && (strcmp(disk_filename[6],"Empty") != 0)) {
		[displayedValues setObject:yes forKey:D7FileEnabled];
		[displayedValues setObject:[NSString stringWithCString:disk_filename[6] encoding:NSASCIIStringEncoding] forKey:D7File];
		}
	else {
		[displayedValues setObject:no forKey:D7FileEnabled];
		[displayedValues setObject:@"" forKey:D7File];
		}
	if ((strcmp(disk_filename[7],"Off") != 0) && (strcmp(disk_filename[7],"Empty") != 0)) {
		[displayedValues setObject:yes forKey:D8FileEnabled];
		[displayedValues setObject:[NSString stringWithCString:disk_filename[7] encoding:NSASCIIStringEncoding] forKey:D8File];
		}
	else {
		[displayedValues setObject:no forKey:D8FileEnabled];
		[displayedValues setObject:@"" forKey:D8File];
		}
	if ((strcmp(cassette_filename, "None") != 0) && (strlen(cassette_filename) != 0)) {
		[displayedValues setObject:yes forKey:CassFileEnabled];
		[displayedValues setObject:[NSString stringWithCString:cassette_filename encoding:NSASCIIStringEncoding] forKey:CassFile];
		}
	else {
		[displayedValues setObject:no forKey:CassFileEnabled];
		[displayedValues setObject:@"" forKey:CassFile];
		}
	if (strlen(cart_filename) != 0) {
		[displayedValues setObject:yes forKey:CartFileEnabled];
		[displayedValues setObject:[NSString stringWithCString:cart_filename encoding:NSASCIIStringEncoding] forKey:CartFile];
		}
	else {
		[displayedValues setObject:no forKey:CartFileEnabled];
		[displayedValues setObject:@"" forKey:CartFile];
		}
	if (strlen(cart2_filename) != 0) {
		[displayedValues setObject:yes forKey:Cart2FileEnabled];
		[displayedValues setObject:[NSString stringWithCString:cart2_filename encoding:NSASCIIStringEncoding] forKey:Cart2File];
		}
	else {
		[displayedValues setObject:no forKey:Cart2FileEnabled];
		[displayedValues setObject:@"" forKey:Cart2File];
		}

}
		
/*------------------------------------------------------------------------------
*  Origin functions which return the origin of windows stored in the 
*     preferences.
*-----------------------------------------------------------------------------*/

- (NSPoint)mediaStatusOrigin
{
   NSPoint origin;
   
   origin.x = [[displayedValues objectForKey:MediaStatusX] floatValue];
   origin.y = [[displayedValues objectForKey:MediaStatusY] floatValue];
   
   return(origin);
}
	
- (NSPoint)messagesOrigin
{
   NSPoint origin;
   
   origin.x = [[displayedValues objectForKey:MessagesX] floatValue];
   origin.y = [[displayedValues objectForKey:MessagesY] floatValue];
   
   return(origin);
}
	
- (NSPoint)functionKeysOrigin
{
   NSPoint origin;
   
   origin.x = [[displayedValues objectForKey:FunctionKeysX] floatValue];
   origin.y = [[displayedValues objectForKey:FunctionKeysY] floatValue];
   
   return(origin);
}
	
- (NSPoint)monitorOrigin
{
   NSPoint origin;
   
   origin.x = [[displayedValues objectForKey:MonitorX] floatValue];
   origin.y = [[displayedValues objectForKey:MonitorY] floatValue];
   
   return(origin);
}

- (BOOL)monitorGUIVisable
{
	return ([[displayedValues objectForKey:MonitorGUIVisable] boolValue]); 
}
	
- (NSPoint)applicationWindowOrigin
{
   NSPoint origin;
   
   origin.x = [[displayedValues objectForKey:ApplicationWindowX] floatValue];
   origin.y = [[displayedValues objectForKey:ApplicationWindowY] floatValue];
   
   return(origin);
}

/* Handle the OK, cancel, and Revert buttons */

- (void)ok:(id)sender {
    [self miscChanged:self];
    [self gamepadConfigChange:gamepadConfigPulldown];
    [self commitDisplayedValues];
    [theTopTimer invalidate];
    [NSApp stopModal];
    [[prefTabView window] close];
    [self transferValuesToEmulator];
    [self transferValuesToAtari825];
    [self transferValuesToAtari1020];
    [self transferValuesToEpson];
    requestPrefsChange = 1;
    PauseAudio(0);
}

- (void)revertToDefault:(id)sender {
    NSMutableArray *configArray;
    
    configArray = [[curValues objectForKey:GamepadConfigArray] mutableCopyWithZone:[self zone]];
    curValues = [defaultValues() mutableCopyWithZone:[self zone]];
    [curValues setObject:configArray forKey:GamepadConfigArray];
    
    [self discardDisplayedValues];
    [gamepadConfigPulldown selectItemAtIndex:0];
    [gamepad1ConfigPulldown selectItemAtIndex:0];
    [gamepad2ConfigPulldown selectItemAtIndex:0];
    [gamepad3ConfigPulldown selectItemAtIndex:0];
    [gamepad4ConfigPulldown selectItemAtIndex:0];
    [self gamepadButtonChange:self];
    [theTopTimer invalidate];
    [NSApp stopModal];
    [[prefTabView window] close];
    [self transferValuesToEmulator];
    [self transferValuesToAtari825];
    [self transferValuesToAtari1020];
    [self transferValuesToEpson];
    requestPrefsChange = 1;
    PauseAudio(0);
}

- (void)revert:(id)sender {
    [self discardDisplayedValues];
    [theTopTimer invalidate];
    [NSApp stopModal];
    [[prefTabView window] close];
    PauseAudio(0);
}

- (void)gamepadConfigChange:(id)sender {
    int numberOfConfigs, action;
    int nameTaken = FALSE;
    int i,currNumConfigs;
    NSString *buttonKey, *button5200Key;
    
    numberOfConfigs = [[curValues objectForKey:GamepadConfigArray] count];
    action = [sender indexOfSelectedItem];
    if (action == 0) {  /* Default Config */
        if (![[displayedValues objectForKey:GamepadConfigCurrent] isEqual:StandardConfigString]) {
            [displayedValues setObject:StandardConfigString forKey:GamepadConfigCurrent];
            for (i=0;i<24;i++) {
                [[displayedValues objectForKey:ButtonAssignment] replaceObjectAtIndex:i 
                    withObject:[[defaultValues() objectForKey:ButtonAssignment] objectAtIndex:i]];
                [[displayedValues objectForKey:Button5200Assignment] replaceObjectAtIndex:i 
                    withObject:[[defaultValues() objectForKey:Button5200Assignment] objectAtIndex:i]];
                }
            }
        }
    else if (action == (3 + numberOfConfigs)) { /* Save Config */
        buttonKey = [ButtonAssignmentPrefix stringByAppendingString:[displayedValues objectForKey:GamepadConfigCurrent]];
        button5200Key = [Button5200AssignmentPrefix 
                            stringByAppendingString:[displayedValues objectForKey:GamepadConfigCurrent]];
        [[NSUserDefaults standardUserDefaults] 
            setObject:[displayedValues objectForKey:ButtonAssignment] forKey:buttonKey];
        [[NSUserDefaults standardUserDefaults] 
            setObject:[displayedValues objectForKey:Button5200Assignment] forKey:button5200Key];
        }
    else if (action == (4 + numberOfConfigs)) { /* Save As....Config */
        /* Go get the deired configuration name */
        [NSApp runModalForWindow:[configNameField window]];
        /* Findout if the configuration name is in use */
        currNumConfigs = [[curValues objectForKey:GamepadConfigArray] count];
        for (i=0;i < currNumConfigs ;i++)
            if ([[configNameField stringValue] isEqual:[[curValues objectForKey:GamepadConfigArray] objectAtIndex:i]])
                nameTaken = TRUE;
        if ([[configNameField stringValue] isEqual:StandardConfigString])
            nameTaken = TRUE;
        /* If it is display an error....*/
        if (nameTaken) {
            [NSApp runModalForWindow:[errorOKButton window]];
            }
        /* Otherwise save the current config under that name...*/
        else {
            /* Add the name to the array */
            [[curValues objectForKey:GamepadConfigArray] 
                addObject:[NSString stringWithString:[configNameField stringValue]]];
            /* Set the current config to it */
            [displayedValues setObject:[configNameField stringValue] forKey:GamepadConfigCurrent];
            /* Add the name to the menu...*/
            [gamepadConfigPulldown insertItemWithTitle:[configNameField stringValue] atIndex: (2+currNumConfigs)];
            [gamepad1ConfigPulldown insertItemWithTitle:[configNameField stringValue] atIndex: (1+currNumConfigs)];
            [gamepad2ConfigPulldown insertItemWithTitle:[configNameField stringValue] atIndex: (1+currNumConfigs)];
            [gamepad3ConfigPulldown insertItemWithTitle:[configNameField stringValue] atIndex: (1+currNumConfigs)];
            [gamepad4ConfigPulldown insertItemWithTitle:[configNameField stringValue] atIndex: (1+currNumConfigs)];
            /* And save the config in Defaults ... */
            buttonKey = [ButtonAssignmentPrefix stringByAppendingString:[configNameField stringValue]];
            button5200Key = [Button5200AssignmentPrefix stringByAppendingString:[configNameField stringValue]];
            [[NSUserDefaults standardUserDefaults] 
                setObject:[displayedValues objectForKey:ButtonAssignment] forKey:buttonKey];
            [[NSUserDefaults standardUserDefaults] 
                setObject:[displayedValues objectForKey:Button5200Assignment] forKey:button5200Key];
            }
        }
    else if (action == (5 + numberOfConfigs)) { /* Rename....Config */
        /* Go get the deired configuration name */
        [NSApp runModalForWindow:[configNameField window]];
        /* Findout if the configuration name is in use */
        currNumConfigs = [[curValues objectForKey:GamepadConfigArray] count];
        for (i=0;i < currNumConfigs ;i++)
            if ([[configNameField stringValue] isEqual:[[curValues objectForKey:GamepadConfigArray] objectAtIndex:i]])
                nameTaken = TRUE;
        if ([[configNameField stringValue] isEqual:StandardConfigString])
            nameTaken = TRUE;
        if ([[configNameField stringValue] isEqual:[curValues objectForKey:GamepadConfigCurrent]])
            nameTaken = FALSE;
        /* If it is display an error....*/
        if (nameTaken) {
            [NSApp runModalForWindow:[errorOKButton window]];
            }
        /* Otherwise rename the current config under that name...*/
        else {
            /* Find the name in the current list */
            currNumConfigs = [[curValues objectForKey:GamepadConfigArray] count];
            for (i=0;i < currNumConfigs ;i++) 
                if ([[displayedValues objectForKey:GamepadConfigCurrent] 
                    isEqual:[[curValues objectForKey:GamepadConfigArray] objectAtIndex:i]])
                        break;
            /* Take out the old name in the list, and put in the new... */
            [[curValues objectForKey:GamepadConfigArray] removeObjectAtIndex:i];
            [[curValues objectForKey:GamepadConfigArray] 
                insertObject:[NSString stringWithString:[configNameField stringValue]] atIndex:i];
            /* Rename the menu item */
            [[gamepadConfigPulldown itemWithTitle:[displayedValues objectForKey:GamepadConfigCurrent]] 
                setTitle:[configNameField stringValue]];
            [[gamepad1ConfigPulldown itemWithTitle:[displayedValues objectForKey:GamepadConfigCurrent]] 
                setTitle:[configNameField stringValue]];
            [[gamepad2ConfigPulldown itemWithTitle:[displayedValues objectForKey:GamepadConfigCurrent]] 
                setTitle:[configNameField stringValue]];
            [[gamepad3ConfigPulldown itemWithTitle:[displayedValues objectForKey:GamepadConfigCurrent]] 
                setTitle:[configNameField stringValue]];
            [[gamepad4ConfigPulldown itemWithTitle:[displayedValues objectForKey:GamepadConfigCurrent]] 
                setTitle:[configNameField stringValue]];
            /* Remove the old configs from the defaults....*/
            buttonKey = [ButtonAssignmentPrefix stringByAppendingString:
                [displayedValues objectForKey:GamepadConfigCurrent]];
            button5200Key = [Button5200AssignmentPrefix stringByAppendingString:
                [displayedValues objectForKey:GamepadConfigCurrent]];
            [[NSUserDefaults standardUserDefaults] removeObjectForKey:buttonKey];
            [[NSUserDefaults standardUserDefaults] removeObjectForKey:button5200Key];
            /* ...And add the new ones */
            buttonKey = [ButtonAssignmentPrefix stringByAppendingString:[configNameField stringValue]];
            button5200Key = [Button5200AssignmentPrefix stringByAppendingString:[configNameField stringValue]];
            [[NSUserDefaults standardUserDefaults] 
                setObject:[displayedValues objectForKey:ButtonAssignment] forKey:buttonKey];
            [[NSUserDefaults standardUserDefaults] 
                setObject:[displayedValues objectForKey:Button5200Assignment] forKey:button5200Key];
            /* Set the current config to it */
            [displayedValues setObject:[configNameField stringValue] forKey:GamepadConfigCurrent];
            }
        }
    else if (action == (6 + numberOfConfigs)) { /* Delete Config */
        /* Find the name in the current list */
        currNumConfigs = [[curValues objectForKey:GamepadConfigArray] count];
        for (i=0;i < currNumConfigs ;i++) {
            if ([[displayedValues objectForKey:GamepadConfigCurrent] 
                    isEqual:[[curValues objectForKey:GamepadConfigArray] objectAtIndex:i]])
               break;
            }
        /* Take it out of the list */
        [[curValues objectForKey:GamepadConfigArray] removeObjectAtIndex:i];
        /* And the Menu */
        [gamepadConfigPulldown removeItemWithTitle:[displayedValues objectForKey:GamepadConfigCurrent]];
        [gamepad1ConfigPulldown removeItemWithTitle:[displayedValues objectForKey:GamepadConfigCurrent]];
        [gamepad2ConfigPulldown removeItemWithTitle:[displayedValues objectForKey:GamepadConfigCurrent]];
        [gamepad3ConfigPulldown removeItemWithTitle:[displayedValues objectForKey:GamepadConfigCurrent]];
        [gamepad4ConfigPulldown removeItemWithTitle:[displayedValues objectForKey:GamepadConfigCurrent]];
        /* And the defaults.... */
        buttonKey = [ButtonAssignmentPrefix stringByAppendingString:[displayedValues objectForKey:GamepadConfigCurrent]];
        button5200Key = [Button5200AssignmentPrefix 
                            stringByAppendingString:[displayedValues objectForKey:GamepadConfigCurrent]];
        [[NSUserDefaults standardUserDefaults] removeObjectForKey:buttonKey];
        [[NSUserDefaults standardUserDefaults] removeObjectForKey:button5200Key];

        /* Now set the default to the config before it, if it exists...*/
        /* If there was only one, we are back to Default */
        if (currNumConfigs == 1) {
            [displayedValues setObject:StandardConfigString forKey:GamepadConfigCurrent];
            }
        else {
            /* If it was the first one, select the new first one */
            if (i==0) {
                [displayedValues setObject:[[curValues objectForKey:GamepadConfigArray] objectAtIndex:0] 						forKey:GamepadConfigCurrent];
                }
            /* Otherwise select the one before it */
            else {
                [displayedValues setObject:[[curValues objectForKey:GamepadConfigArray] objectAtIndex:(i-1)] 						forKey:GamepadConfigCurrent];
                }
            /* and load in the button settings for that item ...*/
            buttonKey = [ButtonAssignmentPrefix stringByAppendingString:
                [displayedValues objectForKey:GamepadConfigCurrent]];
            button5200Key = [Button5200AssignmentPrefix stringByAppendingString:
                [displayedValues objectForKey:GamepadConfigCurrent]];
            [displayedValues setObject:[[[NSUserDefaults standardUserDefaults] objectForKey:buttonKey] 
                mutableCopyWithZone:[self zone]] forKey:ButtonAssignment];
            [displayedValues setObject:[[[NSUserDefaults standardUserDefaults] objectForKey:button5200Key]
                mutableCopyWithZone:[self zone]] forKey:Button5200Assignment];
            }
        }
    else {
        /* Set the current config to the selected one */
        [displayedValues setObject:
            [gamepadConfigPulldown itemTitleAtIndex:action] forKey:GamepadConfigCurrent];
        /* And get the button settings from Defaults */
        buttonKey = [ButtonAssignmentPrefix stringByAppendingString:[gamepadConfigPulldown itemTitleAtIndex:action]];
        button5200Key = [Button5200AssignmentPrefix stringByAppendingString:[gamepadConfigPulldown itemTitleAtIndex:action]];
        [displayedValues setObject:[[[NSUserDefaults standardUserDefaults] objectForKey:buttonKey] 
            mutableCopyWithZone:[self zone]] forKey:ButtonAssignment];
        [displayedValues setObject:[[[NSUserDefaults standardUserDefaults] objectForKey:button5200Key]
            mutableCopyWithZone:[self zone]] forKey:Button5200Assignment];
        }
    
    /* Set the menu to the selected configuration */
    if ([[displayedValues objectForKey:GamepadConfigCurrent] isEqual:StandardConfigString])
        [gamepadConfigPulldown selectItemAtIndex:0];
    else {
        /* Find the name in the current list */
        currNumConfigs = [[curValues objectForKey:GamepadConfigArray] count];
        for (i=0;i < currNumConfigs ;i++) {
            if ([[displayedValues objectForKey:GamepadConfigCurrent] 
                    isEqual:[[curValues objectForKey:GamepadConfigArray] objectAtIndex:i]])
               break;
            }
        [gamepadConfigPulldown selectItemAtIndex:(2+i)];
        }
    
    /* Turn on/off the Save, Delete, and Rename menu items based on if Default is selected... */
    currNumConfigs = [[curValues objectForKey:GamepadConfigArray] count];
    if ([gamepadConfigPulldown indexOfSelectedItem] == 0) {
        [[gamepadConfigPulldown itemAtIndex:(3+currNumConfigs)] setEnabled:NO];
        [[gamepadConfigPulldown itemAtIndex:(5+currNumConfigs)] setEnabled:NO];
        [[gamepadConfigPulldown itemAtIndex:(6+currNumConfigs)] setEnabled:NO];
        }
    else {
        [[gamepadConfigPulldown itemAtIndex:(3+currNumConfigs)] setEnabled:YES];
        [[gamepadConfigPulldown itemAtIndex:(5+currNumConfigs)] setEnabled:YES];
        [[gamepadConfigPulldown itemAtIndex:(6+currNumConfigs)] setEnabled:YES];
        }
    
    /* Tell the other menus to update... */
    [self gamepadButtonChange:self];
    [self miscChanged:self];
    }
    
- (void)configNameOK:(id)sender {
    [NSApp stopModal];
    [[configNameField window] close];
    }
    
- (void)errorOK:(id)sender {
    [NSApp stopModal];
    [[errorOKButton window] close];
    }

- (void)gamepadButtonChange:(id)sender {
    int buttonNum;

    buttonNum = [gamepadButtonPulldown indexOfSelectedItem];
    
    [gamepadAssignmentPulldown selectItemAtIndex:
        [[[displayedValues objectForKey:ButtonAssignment] objectAtIndex:buttonNum] intValue]];
    [gamepad5200AssignmentPulldown  selectItemAtIndex:
        [[[displayedValues objectForKey:Button5200Assignment] objectAtIndex:buttonNum] intValue]];
    }
    
- (void)buttonAssign:(id)sender {
    int buttonNum;
    int index;

    buttonNum = [gamepadButtonPulldown indexOfSelectedItem];
    index = [gamepadAssignmentPulldown indexOfSelectedItem];
    
    [[displayedValues objectForKey:ButtonAssignment] 
        replaceObjectAtIndex:buttonNum withObject:[NSNumber numberWithInt:index]];
    }
    
- (void)button5200Assign:(id)sender {
    int buttonNum;
    int index;

    buttonNum = [gamepadButtonPulldown indexOfSelectedItem];
    index = [gamepad5200AssignmentPulldown indexOfSelectedItem];
    
    [[displayedValues objectForKey:Button5200Assignment] 
        replaceObjectAtIndex:buttonNum withObject:[NSNumber numberWithInt:index]];
    }
    
- (IBAction)identifyGamepad:(id)sender{
    NSArray *top;
    if (!gamepadButton1) {
        if (![[NSBundle mainBundle] loadNibNamed:@"Preferences" owner:self topLevelObjects:&top ])  {
                NSLog(@"Failed to load Preferences.nib");
                NSBeep();
                return;
        }
        [top retain];
    }

    gamepadButtons[0] = gamepadButton1;
    gamepadButtons[1] = gamepadButton2;
    gamepadButtons[2] = gamepadButton3;
    gamepadButtons[3] = gamepadButton4;
    gamepadButtons[4] = gamepadButton5;
    gamepadButtons[5] = gamepadButton6;
    gamepadButtons[6] = gamepadButton7;
    gamepadButtons[7] = gamepadButton8;
    gamepadButtons[8] = gamepadButton9;
    gamepadButtons[9] = gamepadButton10;
    gamepadButtons[10] = gamepadButton11;
    gamepadButtons[11] = gamepadButton12;
    gamepadButtons[12] = gamepadButton13;
    gamepadButtons[13] = gamepadButton14;
    gamepadButtons[14] = gamepadButton15;
    gamepadButtons[15] = gamepadButton16;
    gamepadButtons[16] = gamepadButton17;
    gamepadButtons[17] = gamepadButton18;
    gamepadButtons[18] = gamepadButton19;
    gamepadButtons[19] = gamepadButton20;
    gamepadButtons[20] = gamepadButton21;
    gamepadButtons[21] = gamepadButton22;
    gamepadButtons[22] = gamepadButton23;
    gamepadButtons[23] = gamepadButton24;

    [self identifyGamepadNew:self];

    theIdentifyTimer = [NSTimer
            scheduledTimerWithTimeInterval:0.1 target:self  selector:@selector(identifyTest:) userInfo:nil repeats:YES];

    [[gamepadButton1 window] orderFront:self];
}

- (NSString *) removeUnicode:(NSString *) unicodeString {
    NSUInteger len = [unicodeString length];
    unichar buffer[len+1];

    [unicodeString getCharacters:buffer range:NSMakeRange(0, len)];

    unichar okBuffer[len+1];
    int index = 0;
    for(int i = 0; i < len; i++) {
        if(buffer[i] < 128) {
            okBuffer[index] = buffer[i];
            index = index + 1;
        }
    }

    NSString *removedUnicode = [[NSString alloc] initWithCharacters:okBuffer length:index];

    return removedUnicode;
}

- (IBAction)identifyGamepadNew:(id)sender {
    int numButtons, numSticks, numHats, i;
    SDL_Joystick *joystick;

    if (joystick0)
        [[gamepadSelector cellAtRow:0 column:0] setEnabled:YES];
    else
        [[gamepadSelector cellAtRow:0 column:0] setEnabled:NO];
    if (joystick1)
        [[gamepadSelector cellAtRow:0 column:1] setEnabled:YES];
    else
        [[gamepadSelector cellAtRow:0 column:1] setEnabled:NO];
    if (joystick2)
        [[gamepadSelector cellAtRow:0 column:2] setEnabled:YES];
    else
        [[gamepadSelector cellAtRow:0 column:2] setEnabled:NO];
    if (joystick3)
        [[gamepadSelector cellAtRow:0 column:3] setEnabled:YES];
    else
        [[gamepadSelector cellAtRow:0 column:3] setEnabled:NO];

    if (sender == self || sender == 0) {
        padNum = 0;
        [gamepadSelector selectCellWithTag:0];
    }
    else {
        padNum = [gamepadSelector selectedTag];
    }

    if (padNum == 0) {
        joystick = joystick0;
        numButtons = joystick0_nbuttons;
        numSticks = joystick0_nsticks;
        numHats = joystick0_nhats;
        }
    else if (padNum == 1) {
        joystick = joystick1;
        numButtons = joystick1_nbuttons;
        numSticks = joystick1_nsticks;
        numHats = joystick1_nhats;
        }
    else if (padNum == 2) {
        joystick = joystick2;
        numButtons = joystick2_nbuttons;
        numSticks = joystick2_nsticks;
        numHats = joystick2_nhats;
        }
    else {
        joystick = joystick3;
        numButtons = joystick3_nbuttons;
        numSticks = joystick3_nsticks;
        numHats = joystick3_nhats;
        }

    if (joystick == NULL) {
        [gamepadNameField setStringValue:@"No Joystick Connected"];
        [gamepadNumButtonsField setStringValue:@"0"];
        [gamepadNumSticksField setStringValue:@"0"];
        [gamepadNumHatsField setStringValue:@"0"];
        }
    else {
        NSString *name = [NSString stringWithCString:SDL_JoystickName(joystick) encoding:NSASCIIStringEncoding];
        [gamepadNameField setStringValue:[self  removeUnicode:name]];
        [gamepadNumButtonsField setIntValue:numButtons];
        [gamepadNumSticksField setIntValue:numSticks];
        [gamepadNumHatsField setIntValue:numHats];
        }

    for (i=0; i<NUM_JOYSTICK_BUTTONS; i++)
        [gamepadButtons[i] setState:NSOffState];

    for (i=0; i<numButtons; i++)
        [gamepadButtons[i] setEnabled:YES];
    for (i=numButtons; i<NUM_JOYSTICK_BUTTONS; i++)
        [gamepadButtons[i] setEnabled:NO];

}

- (void)checkNewGamepads:(id)sender {
    [self performSelectorOnMainThread:@selector(checkNewGamepadsMain:) withObject:self waitUntilDone:YES];
}

- (void)checkNewGamepadsMain:(id)sender {
    checkForNewJoysticks();
}

- (void)identifyOK:(id)sender {
    [theIdentifyTimer invalidate];
    [NSApp stopModal];
    [[gamepadButton1 window] close];
    }
    
- (void)identifyTest:(id)sender {
    int numButtons;
    SDL_Joystick *joystick;
    int i;
    int state;

    if (padNum == 0) {
        joystick = joystick0;
        numButtons = joystick0_nbuttons;
        }
    else if (padNum == 1) {
        joystick = joystick1;
        numButtons = joystick1_nbuttons;
        }
    else if (padNum == 2) {
        joystick = joystick2;
        numButtons = joystick2_nbuttons;
        }
    else {
        joystick = joystick3;
        numButtons = joystick3_nbuttons;
        }

    SDL_JoystickUpdate();
    for (i=0;i<numButtons;i++) {
        state = SDL_JoystickGetButton(joystick, i);
        if (state)
            [gamepadButtons[i] setState:NSOnState];
        else
            [gamepadButtons[i] setState:NSOffState];
        }
    }

- (void)leftJoyConfigure:(id)sender
{
    [leftJoyUpPulldown selectItemAtIndex:[[displayedValues objectForKey:LeftJoyUp] intValue]];
    [leftJoyDownPulldown selectItemAtIndex:[[displayedValues objectForKey:LeftJoyDown] intValue]];
    [leftJoyLeftPulldown selectItemAtIndex:[[displayedValues objectForKey:LeftJoyLeft] intValue]];
    [leftJoyRightPulldown selectItemAtIndex:[[displayedValues objectForKey:LeftJoyRight] intValue]];
    [leftJoyUpLeftPulldown selectItemAtIndex:[[displayedValues objectForKey:LeftJoyUpLeft] intValue]];
    [leftJoyUpRightPulldown selectItemAtIndex:[[displayedValues objectForKey:LeftJoyUpRight] intValue]];
    [leftJoyDownLeftPulldown selectItemAtIndex:[[displayedValues objectForKey:LeftJoyDownLeft] intValue]];
    [leftJoyDownRightPulldown selectItemAtIndex:[[displayedValues objectForKey:LeftJoyDownRight] intValue]];
    [leftJoyFirePulldown selectItemAtIndex:[[displayedValues objectForKey:LeftJoyFire] intValue]];
    [leftJoyAltFirePulldown selectItemAtIndex:[[displayedValues objectForKey:LeftJoyAltFire] intValue]];
    [NSApp runModalForWindow:[leftJoyUpPulldown window]];
}

- (void)leftJoyOK:(id)sender
{
    [displayedValues setObject:[NSNumber numberWithInt:[leftJoyUpPulldown indexOfSelectedItem]]
        forKey:LeftJoyUp]; 
    [displayedValues setObject:[NSNumber numberWithInt:[leftJoyDownPulldown indexOfSelectedItem]]
        forKey:LeftJoyDown]; 
    [displayedValues setObject:[NSNumber numberWithInt:[leftJoyLeftPulldown indexOfSelectedItem]]
        forKey:LeftJoyLeft]; 
    [displayedValues setObject:[NSNumber numberWithInt:[leftJoyRightPulldown indexOfSelectedItem]]
        forKey:LeftJoyRight]; 
    [displayedValues setObject:[NSNumber numberWithInt:[leftJoyUpLeftPulldown indexOfSelectedItem]]
        forKey:LeftJoyUpLeft]; 
    [displayedValues setObject:[NSNumber numberWithInt:[leftJoyUpRightPulldown indexOfSelectedItem]]
        forKey:LeftJoyUpRight]; 
    [displayedValues setObject:[NSNumber numberWithInt:[leftJoyDownLeftPulldown indexOfSelectedItem]]
        forKey:LeftJoyDownLeft]; 
    [displayedValues setObject:[NSNumber numberWithInt:[leftJoyDownRightPulldown indexOfSelectedItem]]
        forKey:LeftJoyDownRight]; 
    [displayedValues setObject:[NSNumber numberWithInt:[leftJoyFirePulldown indexOfSelectedItem]]
        forKey:LeftJoyFire]; 
    [displayedValues setObject:[NSNumber numberWithInt:[leftJoyAltFirePulldown indexOfSelectedItem]]
        forKey:LeftJoyAltFire]; 
    [NSApp stopModal];
    [[leftJoyUpPulldown window] close];
}

- (void)padJoyConfigure:(id)sender
{
    [padJoyUpPulldown selectItemAtIndex:[[displayedValues objectForKey:PadJoyUp] intValue]];
    [padJoyDownPulldown selectItemAtIndex:[[displayedValues objectForKey:PadJoyDown] intValue]];
    [padJoyLeftPulldown selectItemAtIndex:[[displayedValues objectForKey:PadJoyLeft] intValue]];
    [padJoyRightPulldown selectItemAtIndex:[[displayedValues objectForKey:PadJoyRight] intValue]];
    [padJoyUpLeftPulldown selectItemAtIndex:[[displayedValues objectForKey:PadJoyUpLeft] intValue]];
    [padJoyUpRightPulldown selectItemAtIndex:[[displayedValues objectForKey:PadJoyUpRight] intValue]];
    [padJoyDownLeftPulldown selectItemAtIndex:[[displayedValues objectForKey:PadJoyDownLeft] intValue]];
    [padJoyDownRightPulldown selectItemAtIndex:[[displayedValues objectForKey:PadJoyDownRight] intValue]];
    [padJoyFirePulldown selectItemAtIndex:[[displayedValues objectForKey:PadJoyFire] intValue]];
    [padJoyAltFirePulldown selectItemAtIndex:[[displayedValues objectForKey:PadJoyAltFire] intValue]];
    [NSApp runModalForWindow:[padJoyUpPulldown window]];
}

- (void)padJoyOK:(id)sender
{
    [displayedValues setObject:[NSNumber numberWithInt:[padJoyUpPulldown indexOfSelectedItem]]
        forKey:PadJoyUp]; 
    [displayedValues setObject:[NSNumber numberWithInt:[padJoyDownPulldown indexOfSelectedItem]]
        forKey:PadJoyDown]; 
    [displayedValues setObject:[NSNumber numberWithInt:[padJoyLeftPulldown indexOfSelectedItem]]
        forKey:PadJoyLeft]; 
    [displayedValues setObject:[NSNumber numberWithInt:[padJoyRightPulldown indexOfSelectedItem]]
        forKey:PadJoyRight]; 
    [displayedValues setObject:[NSNumber numberWithInt:[padJoyUpLeftPulldown indexOfSelectedItem]]
        forKey:PadJoyUpLeft]; 
    [displayedValues setObject:[NSNumber numberWithInt:[padJoyUpRightPulldown indexOfSelectedItem]]
        forKey:PadJoyUpRight]; 
    [displayedValues setObject:[NSNumber numberWithInt:[padJoyDownLeftPulldown indexOfSelectedItem]]
        forKey:PadJoyDownLeft]; 
    [displayedValues setObject:[NSNumber numberWithInt:[padJoyDownRightPulldown indexOfSelectedItem]]
        forKey:PadJoyDownRight]; 
    [displayedValues setObject:[NSNumber numberWithInt:[padJoyFirePulldown indexOfSelectedItem]]
        forKey:PadJoyFire]; 
    [displayedValues setObject:[NSNumber numberWithInt:[padJoyAltFirePulldown indexOfSelectedItem]]
        forKey:PadJoyAltFire]; 
    [NSApp stopModal];
    [[padJoyUpPulldown window] close];
}

/* Handle serial port preferences */
	
// Returns an iterator across all known modems. Caller is responsible for
// releasing the iterator when iteration is complete.
- (kern_return_t) findModems:(io_iterator_t *) matchingServices
{
	kern_return_t		kernResult; 
	mach_port_t			masterPort;
	CFMutableDictionaryRef	classesToMatch;
		
	kernResult = IOMasterPort(MACH_PORT_NULL, &masterPort);
	if (KERN_SUCCESS != kernResult)
		Log_print("IOMasterPort returned %d\n", kernResult);
		
	// Serial devices are instances of class IOSerialBSDClient
	classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
	if (classesToMatch == NULL)
		Log_print("IOServiceMatching returned a NULL dictionary.\n");
	else {
		CFDictionarySetValue(classesToMatch,
							 CFSTR(kIOSerialBSDTypeKey),
							 CFSTR(kIOSerialBSDRS232Type));
	}
		
	kernResult = IOServiceGetMatchingServices(masterPort, classesToMatch, matchingServices);    
	if (KERN_SUCCESS != kernResult)
		Log_print("IOServiceGetMatchingServices returned %d\n", kernResult);
		
	return kernResult;
}
	
// Given an iterator across a set of modems, return a list of the
// modems BSD paths.
// If no modems are found the path name is set to an empty string.
- (int) getModemPaths:(io_iterator_t )serialPortIterator 
{
	io_object_t		modemService;
	int modem_cnt = 0;
		
	while ((modemService = IOIteratorNext(serialPortIterator)) && modem_cnt < MAX_MODEMS)
	{
		CFTypeRef	modemNameAsCFString;
		CFTypeRef	bsdPathAsCFString;
			
			
		modem_cnt++;
		
		modemNameAsCFString = IORegistryEntryCreateCFProperty(modemService,
															  CFSTR(kIOTTYDeviceKey),
															  kCFAllocatorDefault,
															  0);
		if (modemNameAsCFString)
		{
			Boolean result;
			
			result = CFStringGetCString(modemNameAsCFString,
										modemNames[modem_cnt-1],
										FILENAME_MAX, 
										kCFStringEncodingASCII);
			CFRelease(modemNameAsCFString);
				
		}
			
		bsdPathAsCFString = IORegistryEntryCreateCFProperty(modemService,
															CFSTR(kIOCalloutDeviceKey),
															kCFAllocatorDefault,
															0);
		if (bsdPathAsCFString)
		{
			Boolean result;
			
			result = CFStringGetCString(bsdPathAsCFString,
										bsdPaths[modem_cnt-1],
										FILENAME_MAX, 
										kCFStringEncodingASCII);
			CFRelease(bsdPathAsCFString);
		}
					
		(void) IOObjectRelease(modemService);
	}
		
	return modem_cnt;
}

/* In version 4, an additional Atari type variable was introduced,
   so that the new types introduced in version 4 would not cause
   preference files to be incompatible with previous versions. 
   These functions map the position of a machine type in a pulldown
   menu to the type variables and visa versa */

- (int)indexFromType:(int) type:(int) ver4type
	{
	int compositeType;
		
	if (ver4type == -1)
		compositeType = type;
	else
		compositeType = NUM_ORIG_TYPES + ver4type;
	
	if (compositeType >= NUM_TOTAL_TYPES)
		return(0);
	else
		return(indicies[compositeType]);
	}

- (int)typeFromIndex:(int) index:(int *)ver4type
	{
	if (index >= NUM_TOTAL_TYPES)
		{
		*ver4type = -1;
		return(0);
		}
	*ver4type = v4types[index];
	return(types[index]);
	}
	
- (void)generateModemList
{
	kern_return_t	kernResult; // on PowerPC this is an int (4 bytes)
	io_iterator_t	serialPortIterator;
	int i;
	
	kernResult = [self findModems:&serialPortIterator];
	modemCount = [self getModemPaths:serialPortIterator];
	IOObjectRelease(serialPortIterator);	// Release the iterator.
		
	[modems removeAllObjects];
	for (i=0;i<modemCount;i++) {
		[modems addObject:[NSString stringWithCString:modemNames[i] encoding:NSASCIIStringEncoding]];
	}
}
	
	
/**** Code to deal with defaults ****/
   
#define getBoolDefault(name) \
  {id obj = [defaults objectForKey:name]; \
      [dict setObject:obj ? [NSNumber numberWithBool:[defaults boolForKey:name]] : [defaultValues() objectForKey:name] forKey:name];}

#define getIntDefault(name) \
  {id obj = [defaults objectForKey:name]; \
      [dict setObject:obj ? [NSNumber numberWithInt:[defaults integerForKey:name]] : [defaultValues() objectForKey:name] forKey:name];}

#define getFloatDefault(name) \
  {id obj = [defaults objectForKey:name]; \
      [dict setObject:obj ? [NSNumber numberWithFloat:[defaults floatForKey:name]] : [defaultValues() objectForKey:name] forKey:name];}

#define getStringDefault(name) \
  {id obj = [defaults objectForKey:name]; \
      [dict setObject:obj ? [NSString stringWithString:[defaults stringForKey:name]] : [defaultValues() objectForKey:name] forKey:name];}
      
#define getArrayDefault(name) \
  {id obj = [defaults objectForKey:name]; \
      [dict setObject:obj ? [NSMutableArray arrayWithArray:[defaults arrayForKey:name]] : [[defaultValues() objectForKey:name] mutableCopyWithZone:[self zone]] forKey:name];}
      
/* Read prefs from system defaults */
+ (NSDictionary *)preferencesFromDefaults {
    unsigned i,count;
    
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithCapacity:10];
    getBoolDefault(FullScreen);
    getBoolDefault(OpenGl);
    getBoolDefault(LockFullscreenSize);
    getBoolDefault(FullscreenMonitor);
	getFloatDefault(FullscreenForeRed); 
	getFloatDefault(FullscreenForeBlue); 
	getFloatDefault(FullscreenForeGreen); 
	getFloatDefault(FullscreenForeAlpha); 
	getFloatDefault(FullscreenBackRed); 
	getFloatDefault(FullscreenBackBlue); 
	getFloatDefault(FullscreenBackGreen); 
	getFloatDefault(FullscreenBackAlpha); 
    getBoolDefault(DoubleSize);
    getIntDefault(ScaleMode);
    getIntDefault(ScaleFactor);
    getIntDefault(WidthMode);
    getIntDefault(TvMode);
    getFloatDefault(EmulationSpeed);
    getIntDefault(RefreshRatio);
    getBoolDefault(SpriteCollisions);
    getIntDefault(ArtifactingMode);
    getBoolDefault(ArtifactNew);
    getStringDefault(PaletteFile);
    getBoolDefault(UseBuiltinPalette);
    getIntDefault(BlackLevel);
    getIntDefault(WhiteLevel);
    getIntDefault(Intensity);
    getIntDefault(ColorShift);
    getBoolDefault(AdjustPalette);
    getBoolDefault(ShowFPS);
    getBoolDefault(LedStatus);
    getBoolDefault(LedSector);
    getBoolDefault(LedStatusMedia);
    getBoolDefault(LedSectorMedia);
	getBoolDefault(XEP80Enabled);
	getIntDefault(XEP80Port);
	getBoolDefault(XEP80);
	getIntDefault(XEP80OnColor);
	getIntDefault(XEP80OffColor);
    getIntDefault(AtariType);
    getIntDefault(AtariSwitchType);
    getIntDefault(AtariTypeVer4);
    getIntDefault(AtariSwitchTypeVer4);
	getIntDefault(AxlonBankMask);
	getIntDefault(MosaicMaxBank);
	getBoolDefault(MioEnabled);
	getBoolDefault(BlackBoxEnabled);
	getStringDefault(MioRomFile);
	getStringDefault(BlackBoxRomFile);
	getStringDefault(BlackBoxScsiDiskFile);
	getStringDefault(MioScsiDiskFile);
    getBoolDefault(DisableBasic);
    getBoolDefault(DisableAllBasic);
    getBoolDefault(EnableSioPatch);
    getBoolDefault(EnableHPatch);
    getBoolDefault(EnableDPatch);
    getBoolDefault(EnablePPatch);
    getBoolDefault(EnableRPatch);
    getIntDefault(RPatchPort);
    getBoolDefault(RPatchSerialEnabled);
	getStringDefault(RPatchSerialPort);
	getIntDefault(UseAtariCursorKeys);
    getStringDefault(PrintCommand);
	getIntDefault(PrinterType);
	getIntDefault(Atari825CharSet); 
	getIntDefault(Atari825FormLength); 
	getBoolDefault(Atari825AutoLinefeed); 
	getIntDefault(Atari1020PrintWidth); 
	getIntDefault(Atari1020FormLength); 
	getBoolDefault(Atari1020AutoLinefeed); 
	getBoolDefault(Atari1020AutoPageAdjust); 
	getFloatDefault(Atari1020Pen1Red); 
	getFloatDefault(Atari1020Pen1Blue); 
	getFloatDefault(Atari1020Pen1Green); 
	getFloatDefault(Atari1020Pen1Alpha); 
	getFloatDefault(Atari1020Pen2Red); 
	getFloatDefault(Atari1020Pen2Blue); 
	getFloatDefault(Atari1020Pen2Green); 
	getFloatDefault(Atari1020Pen2Alpha); 
	getFloatDefault(Atari1020Pen3Red); 
	getFloatDefault(Atari1020Pen3Blue); 
	getFloatDefault(Atari1020Pen3Green); 
	getFloatDefault(Atari1020Pen3Alpha); 
	getFloatDefault(Atari1020Pen4Red); 
	getFloatDefault(Atari1020Pen4Blue);
	getFloatDefault(Atari1020Pen4Green); 
	getFloatDefault(Atari1020Pen4Alpha); 
    getIntDefault(AtasciiFormLength);
    getIntDefault(AtasciiCharSet);
    getIntDefault(AtasciiCharSize);
    getIntDefault(AtasciiLineGap);
	getIntDefault(EpsonCharSet);
	getIntDefault(EpsonPrintPitch); 
	getIntDefault(EpsonPrintWeight); 
	getIntDefault(EpsonFormLength); 
	getBoolDefault(EpsonAutoLinefeed); 
	getBoolDefault(EpsonPrintSlashedZeros); 
	getBoolDefault(EpsonAutoSkip); 
	getBoolDefault(EpsonSplitSkip); 
    getBoolDefault(BootFromCassette);
    getBoolDefault(SpeedLimit);
    getBoolDefault(EnableSound);
	getFloatDefault(SoundVolume);
    getBoolDefault(EnableStereo);
#if 0 /* enableHifiSound is deprecated from 4.2.2 on */    	
    getBoolDefault(EnableHifiSound);
#endif	
#ifdef WORDS_BIGENDIAN	
	[dict setObject:[defaultValues() objectForKey:Enable16BitSound] forKey:Enable16BitSound];
#else
    getBoolDefault(Enable16BitSound);
#endif
    getBoolDefault(EnableConsoleSound);
    getBoolDefault(EnableSerioSound);
    getBoolDefault(DontMuteAudio);
    getBoolDefault(DiskDriveSound);
    getBoolDefault(EnableInternational);
    getBoolDefault(EnableMultijoy);
    getBoolDefault(IgnoreHeaderWriteprotect);
    getStringDefault(ImageDir);
    getStringDefault(PrintDir);
    getStringDefault(HardDiskDir1);
    getStringDefault(HardDiskDir2);
    getStringDefault(HardDiskDir3);
    getStringDefault(HardDiskDir4);
    getBoolDefault(HardDrivesReadOnly);
    getStringDefault(HPath);
    getStringDefault(OsARomFile);
    getStringDefault(OsBRomFile);
    getStringDefault(XlRomFile);
    getStringDefault(BasicRomFile);
    getStringDefault(A5200RomFile);
    getStringDefault(DiskImageDir);
    getStringDefault(DiskSetDir);
    getStringDefault(CartImageDir);
    getStringDefault(CassImageDir);
    getStringDefault(ExeFileDir);
    getStringDefault(SavedStateDir);
    getStringDefault(ConfigDir);
    getStringDefault(D1File);
    getStringDefault(D2File);
    getStringDefault(D3File);
    getStringDefault(D4File);
    getStringDefault(D5File);
    getStringDefault(D6File);
    getStringDefault(D7File);
    getStringDefault(D8File);
    getStringDefault(CartFile);
    getStringDefault(Cart2File);
    getStringDefault(ExeFile);
    getStringDefault(CassFile);
    getBoolDefault(SaveCurrentMedia);
    getBoolDefault(ClearCurrentMedia);
    getBoolDefault(D1FileEnabled);
    getBoolDefault(D2FileEnabled);
    getBoolDefault(D3FileEnabled);
    getBoolDefault(D4FileEnabled);
    getBoolDefault(D5FileEnabled);
    getBoolDefault(D6FileEnabled);
    getBoolDefault(D7FileEnabled);
    getBoolDefault(D8FileEnabled);
    getBoolDefault(CartFileEnabled);
    getBoolDefault(Cart2FileEnabled);
    getBoolDefault(ExeFileEnabled);
    getBoolDefault(CassFileEnabled);
    getBoolDefault(KeyjoyEnable);
    getIntDefault(Joystick1Mode);
    getIntDefault(Joystick2Mode);
    getIntDefault(Joystick3Mode);
    getIntDefault(Joystick4Mode);
	getBoolDefault(Joystick1MultiMode);
    getBoolDefault(Joystick2MultiMode);
    getBoolDefault(Joystick3MultiMode);
    getBoolDefault(Joystick4MultiMode);
    getIntDefault(Joystick1Autofire);
    getIntDefault(Joystick2Autofire);
    getIntDefault(Joystick3Autofire);
    getIntDefault(Joystick4Autofire);
    getIntDefault(MouseDevice);
    getIntDefault(MouseSpeed);
    getIntDefault(MouseMinVal);
    getIntDefault(MouseMaxVal);
    getIntDefault(MouseHOffset);
    getIntDefault(MouseVOffset);
    getIntDefault(MouseInertia);
    getIntDefault(Joystick1Type);
    getIntDefault(Joystick2Type);
    getIntDefault(Joystick3Type);
    getIntDefault(Joystick4Type);
    getIntDefault(Joystick1Num);
    getIntDefault(Joystick2Num);
    getIntDefault(Joystick3Num);
    getIntDefault(Joystick4Num);
    getBoolDefault(CX85Enabled);
    getIntDefault(CX85Port);
    getArrayDefault(GamepadConfigArray);
    getStringDefault(GamepadConfigCurrent);
    getStringDefault(Gamepad1ConfigCurrent);
    getStringDefault(Gamepad2ConfigCurrent);
    getStringDefault(Gamepad3ConfigCurrent);
    getStringDefault(Gamepad4ConfigCurrent);
    getArrayDefault(ButtonAssignment);
    count = [[dict objectForKey:ButtonAssignment] count];
    if (count < 24) {
        for (i=count; i<24; i++)
            [[dict objectForKey:ButtonAssignment] addObject:[NSNumber numberWithInt:0]];
        }
    getArrayDefault(Button5200Assignment);
    count = [[dict objectForKey:Button5200Assignment] count];
    if (count < 24) {
        for (i=count; i<24; i++)
            [[dict objectForKey:Button5200Assignment] addObject:[NSNumber numberWithInt:0]];
        }
    getIntDefault(PaddlesXAxisOnly);
    getIntDefault(LeftJoyUp);
    getIntDefault(LeftJoyDown);
    getIntDefault(LeftJoyLeft);
    getIntDefault(LeftJoyRight);
    getIntDefault(LeftJoyUpLeft);
    getIntDefault(LeftJoyUpRight);
    getIntDefault(LeftJoyDownLeft);
    getIntDefault(LeftJoyDownRight);
    getIntDefault(LeftJoyFire);
    getIntDefault(LeftJoyAltFire);
    getIntDefault(PadJoyUp);
    getIntDefault(PadJoyDown);
    getIntDefault(PadJoyLeft);
    getIntDefault(PadJoyRight);
    getIntDefault(PadJoyUpLeft);
    getIntDefault(PadJoyUpRight);
    getIntDefault(PadJoyDownLeft);
    getIntDefault(PadJoyDownRight);
    getIntDefault(PadJoyFire);
    getIntDefault(PadJoyAltFire);
	getBoolDefault(MediaStatusDisplayed);
	getBoolDefault(FunctionKeysDisplayed);
    getIntDefault(MediaStatusX);
    getIntDefault(MediaStatusY);
    getIntDefault(MessagesX);
    getIntDefault(MessagesY);
    getIntDefault(MonitorX);
    getIntDefault(MonitorY);
	getBoolDefault(MonitorGUIVisable);
    getIntDefault(FunctionKeysX);
    getIntDefault(FunctionKeysY);
    getIntDefault(ApplicationWindowX);
    getIntDefault(ApplicationWindowY);

    return dict;
}

#define setBoolDefault(name) \
  {if ([[defaultValues() objectForKey:name] isEqual:[dict objectForKey:name]]) [defaults removeObjectForKey:name]; else [defaults setBool:[[dict objectForKey:name] boolValue] forKey:name];}

#define setIntDefault(name) \
  {if ([[defaultValues() objectForKey:name] isEqual:[dict objectForKey:name]]) [defaults removeObjectForKey:name]; else [defaults setInteger:[[dict objectForKey:name] intValue] forKey:name];}

#define setFloatDefault(name) \
  {if ([[defaultValues() objectForKey:name] isEqual:[dict objectForKey:name]]) [defaults removeObjectForKey:name]; else [defaults setFloat:[[dict objectForKey:name] floatValue] forKey:name];}

#define setStringDefault(name) \
  {if ([[defaultValues() objectForKey:name] isEqual:[dict objectForKey:name]]) [defaults removeObjectForKey:name]; else [defaults setObject:[dict objectForKey:name] forKey:name];}

#define setArrayDefault(name) \
  {if ([[defaultValues() objectForKey:name] isEqual:[dict objectForKey:name]]) [defaults removeObjectForKey:name]; else [defaults setObject:[dict objectForKey:name] forKey:name];}

/* Save preferences to system defaults */
+ (void)savePreferencesToDefaults:(NSDictionary *)dict {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    setBoolDefault(FullScreen);
    setBoolDefault(OpenGl);
    setBoolDefault(LockFullscreenSize);
    setBoolDefault(FullscreenMonitor);
	setFloatDefault(FullscreenForeRed); 
	setFloatDefault(FullscreenForeBlue); 
	setFloatDefault(FullscreenForeGreen); 
	setFloatDefault(FullscreenForeAlpha); 
	setFloatDefault(FullscreenBackRed); 
	setFloatDefault(FullscreenBackBlue); 
	setFloatDefault(FullscreenBackGreen); 
	setFloatDefault(FullscreenBackAlpha); 
    setBoolDefault(DoubleSize);
    setIntDefault(ScaleMode);
    setIntDefault(ScaleFactor);
    setIntDefault(WidthMode);
    setIntDefault(TvMode);
    setFloatDefault(EmulationSpeed);
    setIntDefault(RefreshRatio);
    setBoolDefault(SpriteCollisions);
    setIntDefault(ArtifactingMode);
    setBoolDefault(ArtifactNew);
    setStringDefault(PaletteFile);
    setBoolDefault(UseBuiltinPalette);
    setIntDefault(BlackLevel);
    setIntDefault(WhiteLevel);
    setIntDefault(Intensity);
    setIntDefault(ColorShift);
    setBoolDefault(AdjustPalette);
    setBoolDefault(ShowFPS);
    setBoolDefault(LedStatus);
    setBoolDefault(LedSector);
    setBoolDefault(LedStatusMedia);
    setBoolDefault(LedSectorMedia);
	setBoolDefault(XEP80Enabled);
	setIntDefault(XEP80Port);
	setBoolDefault(XEP80);
	setIntDefault(XEP80OnColor);
	setIntDefault(XEP80OffColor);
    setIntDefault(AtariType);
    setIntDefault(AtariSwitchType);
    setIntDefault(AtariTypeVer4);
    setIntDefault(AtariSwitchTypeVer4);
	setIntDefault(AxlonBankMask);
	setIntDefault(MosaicMaxBank);
	setBoolDefault(MioEnabled);
	setBoolDefault(BlackBoxEnabled);
	setStringDefault(MioRomFile);
	setStringDefault(BlackBoxRomFile);
	setStringDefault(BlackBoxScsiDiskFile);
	setStringDefault(MioScsiDiskFile);
    setBoolDefault(DisableBasic);
    setBoolDefault(DisableAllBasic);
    setBoolDefault(EnableSioPatch);
    setBoolDefault(EnableHPatch);
    setBoolDefault(EnableDPatch);
    setBoolDefault(EnablePPatch);
    setBoolDefault(EnableRPatch);
    setIntDefault(RPatchPort);
    setBoolDefault(RPatchSerialEnabled);
	setStringDefault(RPatchSerialPort);
	setIntDefault(UseAtariCursorKeys);
    setStringDefault(PrintCommand);
	setIntDefault(PrinterType);
	setIntDefault(Atari825CharSet); 
	setIntDefault(Atari825FormLength); 
	setBoolDefault(Atari825AutoLinefeed); 
	setIntDefault(Atari1020PrintWidth); 
	setIntDefault(Atari1020FormLength); 
	setBoolDefault(Atari1020AutoLinefeed); 
	setBoolDefault(Atari1020AutoPageAdjust); 
	setFloatDefault(Atari1020Pen1Red); 
	setFloatDefault(Atari1020Pen1Blue); 
	setFloatDefault(Atari1020Pen1Green); 
	setFloatDefault(Atari1020Pen1Alpha); 
	setFloatDefault(Atari1020Pen2Red); 
	setFloatDefault(Atari1020Pen2Blue); 
	setFloatDefault(Atari1020Pen2Green); 
	setFloatDefault(Atari1020Pen2Alpha); 
	setFloatDefault(Atari1020Pen3Red); 
	setFloatDefault(Atari1020Pen3Blue); 
	setFloatDefault(Atari1020Pen3Green); 
	setFloatDefault(Atari1020Pen3Alpha); 
	setFloatDefault(Atari1020Pen4Red); 
	setFloatDefault(Atari1020Pen4Blue);
	setFloatDefault(Atari1020Pen4Green); 
	setFloatDefault(Atari1020Pen4Alpha); 
    setIntDefault(AtasciiFormLength);
    setIntDefault(AtasciiCharSet);
    setIntDefault(AtasciiCharSize);
    setIntDefault(AtasciiLineGap);
	setIntDefault(EpsonCharSet);
	setIntDefault(EpsonPrintPitch); 
	setIntDefault(EpsonPrintWeight); 
	setIntDefault(EpsonFormLength); 
	setBoolDefault(EpsonAutoLinefeed); 
	setBoolDefault(EpsonPrintSlashedZeros); 
	setBoolDefault(EpsonAutoSkip); 
	setBoolDefault(EpsonSplitSkip); 
    setBoolDefault(BootFromCassette);
    setBoolDefault(SpeedLimit);
    setBoolDefault(EnableSound);
	setFloatDefault(SoundVolume);
    setBoolDefault(EnableStereo);
#if 0 /* enableHifiSound is deprecated from 4.2.2 on */    	
    setBoolDefault(EnableHifiSound);
#endif	
    setBoolDefault(Enable16BitSound);
    setBoolDefault(EnableConsoleSound);
    setBoolDefault(EnableSerioSound);
    setBoolDefault(DontMuteAudio);
    setBoolDefault(DiskDriveSound);
    setBoolDefault(EnableInternational);
    setBoolDefault(EnableMultijoy);
    setBoolDefault(IgnoreHeaderWriteprotect);
    setStringDefault(ImageDir);
    setStringDefault(PrintDir);
    setStringDefault(HardDiskDir1);
    setStringDefault(HardDiskDir2);
    setStringDefault(HardDiskDir3);
    setStringDefault(HardDiskDir4);
    setBoolDefault(HardDrivesReadOnly);
    setStringDefault(HPath);
    setStringDefault(OsARomFile);
    setStringDefault(OsBRomFile);
    setStringDefault(XlRomFile);
    setStringDefault(BasicRomFile);
    setStringDefault(A5200RomFile);
    setStringDefault(DiskImageDir);
    setStringDefault(DiskSetDir);
    setStringDefault(CartImageDir);
    setStringDefault(CassImageDir);
    setStringDefault(ExeFileDir);
    setStringDefault(SavedStateDir);
    setStringDefault(ConfigDir);
    setStringDefault(D1File);
    setStringDefault(D2File);
    setStringDefault(D3File);
    setStringDefault(D4File);
    setStringDefault(D5File);
    setStringDefault(D6File);
    setStringDefault(D7File);
    setStringDefault(D8File);
    setStringDefault(CartFile);
    setStringDefault(Cart2File);
    setStringDefault(ExeFile);
    setStringDefault(CassFile);
    setBoolDefault(SaveCurrentMedia);
    setBoolDefault(ClearCurrentMedia);
    setBoolDefault(D1FileEnabled);
    setBoolDefault(D2FileEnabled);
    setBoolDefault(D3FileEnabled);
    setBoolDefault(D4FileEnabled);
    setBoolDefault(D5FileEnabled);
    setBoolDefault(D6FileEnabled);
    setBoolDefault(D7FileEnabled);
    setBoolDefault(D8FileEnabled);
    setBoolDefault(CartFileEnabled);
    setBoolDefault(Cart2FileEnabled);
    setBoolDefault(ExeFileEnabled);
    setBoolDefault(CassFileEnabled);
    setBoolDefault(KeyjoyEnable);
    setIntDefault(Joystick1Mode);
    setIntDefault(Joystick2Mode);
    setIntDefault(Joystick3Mode);
    setIntDefault(Joystick4Mode);
	setBoolDefault(Joystick1MultiMode);
    setBoolDefault(Joystick2MultiMode);
    setBoolDefault(Joystick3MultiMode);
    setBoolDefault(Joystick4MultiMode);
	setIntDefault(Joystick1Autofire);
    setIntDefault(Joystick2Autofire);
    setIntDefault(Joystick3Autofire);
    setIntDefault(Joystick4Autofire);
    setIntDefault(MouseDevice);
    setIntDefault(MouseSpeed);
    setIntDefault(MouseMinVal);
    setIntDefault(MouseMaxVal);
    setIntDefault(MouseHOffset);
    setIntDefault(MouseVOffset);
    setIntDefault(MouseInertia);
    setIntDefault(Joystick1Type);
    setIntDefault(Joystick2Type);
    setIntDefault(Joystick3Type);
    setIntDefault(Joystick4Type);
    setIntDefault(Joystick1Num);
    setIntDefault(Joystick2Num);
    setIntDefault(Joystick3Num);
    setIntDefault(Joystick4Num);
    setBoolDefault(CX85Enabled);
    setIntDefault(CX85Port);
    setArrayDefault(GamepadConfigArray);
    setStringDefault(GamepadConfigCurrent);
    setStringDefault(Gamepad1ConfigCurrent);
    setStringDefault(Gamepad2ConfigCurrent);
    setStringDefault(Gamepad3ConfigCurrent);
    setStringDefault(Gamepad4ConfigCurrent);
    setArrayDefault(ButtonAssignment);
    setArrayDefault(Button5200Assignment);
    setIntDefault(PaddlesXAxisOnly);
    setIntDefault(LeftJoyUp);
    setIntDefault(LeftJoyDown);
    setIntDefault(LeftJoyLeft);
    setIntDefault(LeftJoyRight);
    setIntDefault(LeftJoyUpLeft);
    setIntDefault(LeftJoyUpRight);
    setIntDefault(LeftJoyDownLeft);
    setIntDefault(LeftJoyDownRight);
    setIntDefault(LeftJoyFire);
    setIntDefault(LeftJoyAltFire);
    setIntDefault(PadJoyUp);
    setIntDefault(PadJoyDown);
    setIntDefault(PadJoyLeft);
    setIntDefault(PadJoyRight);
    setIntDefault(PadJoyUpLeft);
    setIntDefault(PadJoyUpRight);
    setIntDefault(PadJoyDownLeft);
    setIntDefault(PadJoyDownRight);
    setIntDefault(PadJoyFire);
    setIntDefault(PadJoyAltFire);
	setBoolDefault(MediaStatusDisplayed);
	setBoolDefault(FunctionKeysDisplayed);
    setIntDefault(MediaStatusX);
    setIntDefault(MediaStatusY);
    setIntDefault(MessagesX);
    setIntDefault(MessagesY);
    setIntDefault(FunctionKeysX);
    setIntDefault(FunctionKeysY);
    setIntDefault(MonitorX);
    setIntDefault(MonitorY);
	setBoolDefault(MonitorGUIVisable);
    setIntDefault(ApplicationWindowX);
    setIntDefault(ApplicationWindowY);

    [defaults synchronize];
}

#define setConfig(name) \
{if ([[defaultValues() objectForKey:name] isEqual:[dict objectForKey:name]]) [dict removeObjectForKey:name];}

- (void)saveConfigurationData:(NSString *)filename {
	NSData *xmlData;
	NSError *error;
	NSMutableDictionary *dict;

	if ([[displayedValues objectForKey:SaveCurrentMedia] boolValue] == YES)
		saveMediaPrefs();
	[self commitDisplayedValues];
	dict = [[NSMutableDictionary alloc] initWithCapacity:100];
	[dict setDictionary:curValues];
    setConfig(FullScreen);
    setConfig(OpenGl);
    setConfig(LockFullscreenSize);
    setConfig(FullscreenMonitor);
	setConfig(FullscreenForeRed); 
	setConfig(FullscreenForeBlue); 
	setConfig(FullscreenForeGreen); 
	setConfig(FullscreenForeAlpha); 
	setConfig(FullscreenBackRed); 
	setConfig(FullscreenBackBlue); 
	setConfig(FullscreenBackGreen); 
	setConfig(FullscreenBackAlpha); 
    setConfig(DoubleSize);
    setConfig(ScaleMode);
    setConfig(ScaleFactor);
    setConfig(WidthMode);
    setConfig(TvMode);
    setConfig(EmulationSpeed);
    setConfig(RefreshRatio);
    setConfig(SpriteCollisions);
    setConfig(ArtifactingMode);
    setConfig(ArtifactNew);
    setConfig(PaletteFile);
    setConfig(UseBuiltinPalette);
    setConfig(BlackLevel);
    setConfig(WhiteLevel);
    setConfig(Intensity);
    setConfig(ColorShift);
    setConfig(AdjustPalette);
    setConfig(ShowFPS);
    setConfig(LedStatus);
    setConfig(LedSector);
    setConfig(LedStatusMedia);
    setConfig(LedSectorMedia);
	setConfig(XEP80Enabled);
	setConfig(XEP80Port);
	setConfig(XEP80);
	setConfig(XEP80OnColor);
	setConfig(XEP80OffColor);
    setConfig(AtariType);
    setConfig(AtariSwitchType);
    setConfig(AtariTypeVer4);
    setConfig(AtariSwitchTypeVer4);
	setConfig(AxlonBankMask);
	setConfig(MosaicMaxBank);
	setConfig(MioEnabled);
	setConfig(BlackBoxEnabled);
	setConfig(MioRomFile);
	setConfig(BlackBoxRomFile);
	setConfig(BlackBoxScsiDiskFile);
	setConfig(MioScsiDiskFile);
    setConfig(DisableBasic);
    setConfig(DisableAllBasic);
    setConfig(EnableSioPatch);
    setConfig(EnableHPatch);
    setConfig(EnableDPatch);
    setConfig(EnablePPatch);
    setConfig(EnableRPatch);
    setConfig(RPatchPort);
    setConfig(RPatchSerialEnabled);
	setConfig(RPatchSerialPort);
	setConfig(UseAtariCursorKeys);
    setConfig(PrintCommand);
	setConfig(PrinterType);
	setConfig(Atari825CharSet); 
	setConfig(Atari825FormLength); 
	setConfig(Atari825AutoLinefeed); 
	setConfig(Atari1020PrintWidth); 
	setConfig(Atari1020FormLength); 
	setConfig(Atari1020AutoLinefeed); 
	setConfig(Atari1020AutoPageAdjust); 
	setConfig(Atari1020Pen1Red); 
	setConfig(Atari1020Pen1Blue); 
	setConfig(Atari1020Pen1Green); 
	setConfig(Atari1020Pen1Alpha); 
	setConfig(Atari1020Pen2Red); 
	setConfig(Atari1020Pen2Blue); 
	setConfig(Atari1020Pen2Green); 
	setConfig(Atari1020Pen2Alpha); 
	setConfig(Atari1020Pen3Red); 
	setConfig(Atari1020Pen3Blue); 
	setConfig(Atari1020Pen3Green); 
	setConfig(Atari1020Pen3Alpha); 
	setConfig(Atari1020Pen4Red); 
	setConfig(Atari1020Pen4Blue);
	setConfig(Atari1020Pen4Green); 
	setConfig(Atari1020Pen4Alpha); 
    setConfig(AtasciiFormLength);
    setConfig(AtasciiCharSet);
    setConfig(AtasciiCharSize);
    setConfig(AtasciiLineGap);
	setConfig(EpsonCharSet);
	setConfig(EpsonPrintPitch); 
	setConfig(EpsonPrintWeight); 
	setConfig(EpsonFormLength); 
	setConfig(EpsonAutoLinefeed); 
	setConfig(EpsonPrintSlashedZeros); 
	setConfig(EpsonAutoSkip); 
	setConfig(EpsonSplitSkip); 
    setConfig(BootFromCassette);
    setConfig(SpeedLimit);
    setConfig(EnableSound);
	setConfig(SoundVolume);
    setConfig(EnableStereo);
#if 0 /* enableHifiSound is deprecated from 4.2.2 on */    	
    setConfig(EnableHifiSound);
#endif	
    setConfig(Enable16BitSound);
    setConfig(EnableConsoleSound);
    setConfig(EnableSerioSound);
    setConfig(DontMuteAudio);
    setConfig(DiskDriveSound);
    setConfig(EnableInternational);
    setConfig(EnableMultijoy);
    setConfig(IgnoreHeaderWriteprotect);
    setConfig(ImageDir);
    setConfig(PrintDir);
    setConfig(HardDiskDir1);
    setConfig(HardDiskDir2);
    setConfig(HardDiskDir3);
    setConfig(HardDiskDir4);
    setConfig(HardDrivesReadOnly);
    setConfig(HPath);
    setConfig(OsARomFile);
    setConfig(OsBRomFile);
    setConfig(XlRomFile);
    setConfig(BasicRomFile);
    setConfig(A5200RomFile);
    setConfig(DiskImageDir);
    setConfig(DiskSetDir);
    setConfig(CartImageDir);
    setConfig(CassImageDir);
    setConfig(ExeFileDir);
    setConfig(SavedStateDir);
    setConfig(ConfigDir);
    setConfig(D1File);
    setConfig(D2File);
    setConfig(D3File);
    setConfig(D4File);
    setConfig(D5File);
    setConfig(D6File);
    setConfig(D7File);
    setConfig(D8File);
    setConfig(CartFile);
    setConfig(Cart2File);
    setConfig(ExeFile);
    setConfig(CassFile);
    setConfig(SaveCurrentMedia);
    setConfig(ClearCurrentMedia);
    setConfig(D1FileEnabled);
    setConfig(D2FileEnabled);
    setConfig(D3FileEnabled);
    setConfig(D4FileEnabled);
    setConfig(D5FileEnabled);
    setConfig(D6FileEnabled);
    setConfig(D7FileEnabled);
    setConfig(D8FileEnabled);
    setConfig(CartFileEnabled);
    setConfig(Cart2FileEnabled);
    setConfig(ExeFileEnabled);
    setConfig(CassFileEnabled);
    setConfig(KeyjoyEnable);
    setConfig(Joystick1Mode);
    setConfig(Joystick2Mode);
    setConfig(Joystick3Mode);
    setConfig(Joystick4Mode);
    setConfig(Joystick1Autofire);
    setConfig(Joystick2Autofire);
    setConfig(Joystick3Autofire);
    setConfig(Joystick4Autofire);
    setConfig(MouseDevice);
    setConfig(MouseSpeed);
    setConfig(MouseMinVal);
    setConfig(MouseMaxVal);
    setConfig(MouseHOffset);
    setConfig(MouseVOffset);
    setConfig(MouseInertia);
    setConfig(Joystick1Type);
    setConfig(Joystick2Type);
    setConfig(Joystick3Type);
    setConfig(Joystick4Type);
    setConfig(Joystick1Num);
    setConfig(Joystick2Num);
    setConfig(Joystick3Num);
    setConfig(Joystick4Num);
    setConfig(CX85Enabled);
    setConfig(CX85Port);
    setConfig(GamepadConfigArray);
    setConfig(GamepadConfigCurrent);
    setConfig(Gamepad1ConfigCurrent);
    setConfig(Gamepad2ConfigCurrent);
    setConfig(Gamepad3ConfigCurrent);
    setConfig(Gamepad4ConfigCurrent);
    setConfig(ButtonAssignment);
    setConfig(Button5200Assignment);
    setConfig(PaddlesXAxisOnly);
    setConfig(LeftJoyUp);
    setConfig(LeftJoyDown);
    setConfig(LeftJoyLeft);
    setConfig(LeftJoyRight);
    setConfig(LeftJoyUpLeft);
    setConfig(LeftJoyUpRight);
    setConfig(LeftJoyDownLeft);
    setConfig(LeftJoyDownRight);
    setConfig(LeftJoyFire);
    setConfig(LeftJoyAltFire);
    setConfig(PadJoyUp);
    setConfig(PadJoyDown);
    setConfig(PadJoyLeft);
    setConfig(PadJoyRight);
    setConfig(PadJoyUpLeft);
    setConfig(PadJoyUpRight);
    setConfig(PadJoyDownLeft);
    setConfig(PadJoyDownRight);
    setConfig(PadJoyFire);
    setConfig(PadJoyAltFire);
	setConfig(MediaStatusDisplayed);
	setConfig(FunctionKeysDisplayed);
    setConfig(MediaStatusX);
    setConfig(MediaStatusY);
    setConfig(MessagesX);
    setConfig(MessagesY);
    setConfig(FunctionKeysX);
    setConfig(FunctionKeysY);
    setConfig(MonitorX);
    setConfig(MonitorY);
	setConfig(MonitorGUIVisable);
    setConfig(ApplicationWindowX);
    setConfig(ApplicationWindowY);
	
    xmlData = [NSPropertyListSerialization dataWithPropertyList:dict
        format:NSPropertyListXMLFormat_v1_0 options:0
                                           error:&error];
	[dict release];
	
	if(xmlData) {
		[xmlData writeToFile:filename atomically:YES];
	}
	else {
		NSLog(@"%@",error);
		[error release];
	}
}

- (void)saveConfiguration:(id)sender {
	NSString *filename;
	
    if (FULLSCREEN) {
        UI_alt_function = UI_MENU_SAVECFG;
        requestFullScreenUI = 1;
        return;
    }
    
	filename = [self saveFileInDirectory:[NSString stringWithCString:atari_config_dir  encoding:NSASCIIStringEncoding]:@"a8c"];
	if (filename != nil)
		[self saveConfigurationData:filename];
    [[KeyMapper sharedInstance] releaseCmdKeys:@"s"];
}

- (void)saveConfigurationMenu:(id)sender {
	/* Transfer the changed prefs values back from emulator */
	savePrefs();
	[self commitDisplayedValues];
	[self saveConfiguration:sender];
}

- (void)saveConfigurationUI:(char *)filename {
	/* Transfer the changed prefs values back from emulator */
	savePrefs();
	[self commitDisplayedValues];
	[self saveConfigurationData:[NSString stringWithCString:filename encoding:NSASCIIStringEncoding]];
}

#define getConfig(name) \
{id obj = [configDict objectForKey:name]; \
[dict setObject:obj ? [configDict objectForKey:name] : [defaultValues() objectForKey:name] forKey:name];}

- (int)loadConfiguration:(id)sender {
    NSString *filename;
	NSOpenPanel *openPanel;

    if (FULLSCREEN) {
        UI_alt_function = UI_MENU_LOADCFG;
        requestFullScreenUI = 1;
        return 0;
    }

	openPanel = [NSOpenPanel openPanel];
	[openPanel setCanChooseDirectories:NO];
	[openPanel setCanChooseFiles:YES];
    [openPanel setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithCString:atari_config_dir encoding:NSASCIIStringEncoding]]];
    [openPanel setAllowedFileTypes:[NSArray arrayWithObjects:@"a8c",@"A8C",nil]];

	if ([openPanel runModal] != NSModalResponseOK) { 
		[[KeyMapper sharedInstance] releaseCmdKeys:@"l"];
        return 0;
		}
	filename = [[[openPanel URLs] objectAtIndex:0] path];
	[self loadConfigFile:filename];
    [[KeyMapper sharedInstance] releaseCmdKeys:@"l"];
	configurationChanged = 1;
	return 1;
}

- (void)loadConfigurationMenu:(id)sender {
	/* Transfer the changed prefs values back from emulator */
	savePrefs();
	[self commitDisplayedValues];
	
	/* Load the config file */
	if (![self loadConfiguration:sender])
		return;
	
	/* Transfer the prefs back to the emulator */
    [self transferValuesToEmulator];
    [self transferValuesToAtari825];
    [self transferValuesToAtari1020];
    [self transferValuesToEpson];
    requestPrefsChange = 1;
}

- (void)loadConfigurationUI:(char *)filename {
	/* Transfer the changed prefs values back from emulator */
	savePrefs();
	[self commitDisplayedValues];
	
	/* Load the config file */
	[self loadConfigFile:[NSString stringWithCString:filename encoding:NSASCIIStringEncoding]];
	
	/* Transfer the prefs back to the emulator */
    [self transferValuesToEmulator];
    [self transferValuesToAtari825];
    [self transferValuesToAtari1020];
    [self transferValuesToEpson];
    requestPrefsChange = 1;
	configurationChanged = 1;
}

- (void) loadConfigFile:(NSString *) filename {
	NSData *plistData;
	int i, count;
	NSError *error;
	NSPropertyListFormat format;
    NSMutableDictionary *configDict;
    NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithCapacity:100];
	
	plistData = [NSData dataWithContentsOfFile:filename];
	
    configDict = [NSPropertyListSerialization propertyListWithData:plistData
        options:NSPropertyListMutableContainersAndLeaves
        format:&format
        error:&error];
	if(!configDict){
		NSLog(@"%@",error);
		[error release];
		return;
	}

    getConfig(FullScreen);
    getConfig(OpenGl);
    getConfig(LockFullscreenSize);
    getConfig(FullscreenMonitor);
	getConfig(FullscreenForeRed); 
	getConfig(FullscreenForeBlue); 
	getConfig(FullscreenForeGreen); 
	getConfig(FullscreenForeAlpha); 
	getConfig(FullscreenBackRed); 
	getConfig(FullscreenBackBlue); 
	getConfig(FullscreenBackGreen); 
	getConfig(FullscreenBackAlpha); 
    getConfig(DoubleSize);
    getConfig(ScaleMode);
    getConfig(ScaleFactor);
    getConfig(WidthMode);
    getConfig(TvMode);
    getConfig(EmulationSpeed);
    getConfig(RefreshRatio);
    getConfig(SpriteCollisions);
    getConfig(ArtifactingMode);
    getConfig(ArtifactNew);
    getConfig(PaletteFile);
    getConfig(UseBuiltinPalette);
    getConfig(BlackLevel);
    getConfig(WhiteLevel);
    getConfig(Intensity);
    getConfig(ColorShift);
    getConfig(AdjustPalette);
    getConfig(ShowFPS);
    getConfig(LedStatus);
    getConfig(LedSector);
    getConfig(LedStatusMedia);
    getConfig(LedSectorMedia);
	getConfig(XEP80Enabled);
	getConfig(XEP80Port);
	getConfig(XEP80);
	getConfig(XEP80OnColor);
	getConfig(XEP80OffColor);
    getConfig(AtariType);
    getConfig(AtariSwitchType);
    getConfig(AtariTypeVer4);
    getConfig(AtariSwitchTypeVer4);
	getConfig(AxlonBankMask);
	getConfig(MosaicMaxBank);
	getConfig(MioEnabled);
	getConfig(BlackBoxEnabled);
	getConfig(MioRomFile);
	getConfig(BlackBoxRomFile);
	getConfig(BlackBoxScsiDiskFile);
	getConfig(MioScsiDiskFile);
    getConfig(DisableBasic);
    getConfig(DisableAllBasic);
    getConfig(EnableSioPatch);
    getConfig(EnableHPatch);
    getConfig(EnableDPatch);
	getConfig(UseAtariCursorKeys);
    getConfig(EnablePPatch);
    getConfig(EnableRPatch);
    getConfig(RPatchPort);
    getConfig(RPatchSerialEnabled);
	getConfig(RPatchSerialPort);
    getConfig(PrintCommand);
	getConfig(PrinterType);
	getConfig(Atari825CharSet); 
	getConfig(Atari825FormLength); 
	getConfig(Atari825AutoLinefeed); 
	getConfig(Atari1020PrintWidth); 
	getConfig(Atari1020FormLength); 
	getConfig(Atari1020AutoLinefeed); 
	getConfig(Atari1020AutoPageAdjust); 
	getConfig(Atari1020Pen1Red); 
	getConfig(Atari1020Pen1Blue); 
	getConfig(Atari1020Pen1Green); 
	getConfig(Atari1020Pen1Alpha); 
	getConfig(Atari1020Pen2Red); 
	getConfig(Atari1020Pen2Blue); 
	getConfig(Atari1020Pen2Green); 
	getConfig(Atari1020Pen2Alpha); 
	getConfig(Atari1020Pen3Red); 
	getConfig(Atari1020Pen3Blue); 
	getConfig(Atari1020Pen3Green); 
	getConfig(Atari1020Pen3Alpha); 
	getConfig(Atari1020Pen4Red); 
	getConfig(Atari1020Pen4Blue);
	getConfig(Atari1020Pen4Green); 
	getConfig(Atari1020Pen4Alpha); 
    getConfig(AtasciiFormLength);
    getConfig(AtasciiCharSet);
    getConfig(AtasciiCharSize);
    getConfig(AtasciiLineGap);
	getConfig(EpsonCharSet);
	getConfig(EpsonPrintPitch); 
	getConfig(EpsonPrintWeight); 
	getConfig(EpsonFormLength); 
	getConfig(EpsonAutoLinefeed); 
	getConfig(EpsonPrintSlashedZeros); 
	getConfig(EpsonAutoSkip); 
	getConfig(EpsonSplitSkip); 
    getConfig(BootFromCassette);
    getConfig(SpeedLimit);
    getConfig(EnableSound);
	getConfig(SoundVolume);
    getConfig(EnableStereo);
#if 0 /* enableHifiSound is deprecated from 4.2.2 on */    	
    getConfig(EnableHifiSound);
#endif	
	getConfig(Enable16BitSound);
    getConfig(EnableConsoleSound);
    getConfig(EnableSerioSound);
    getConfig(DontMuteAudio);
    getConfig(DiskDriveSound);
    getConfig(EnableInternational);
    getConfig(EnableMultijoy);
    getConfig(IgnoreHeaderWriteprotect);
    getConfig(ImageDir);
    getConfig(PrintDir);
    getConfig(HardDiskDir1);
    getConfig(HardDiskDir2);
    getConfig(HardDiskDir3);
    getConfig(HardDiskDir4);
    getConfig(HardDrivesReadOnly);
    getConfig(HPath);
    getConfig(OsARomFile);
    getConfig(OsBRomFile);
    getConfig(XlRomFile);
    getConfig(BasicRomFile);
    getConfig(A5200RomFile);
    getConfig(DiskImageDir);
    getConfig(DiskSetDir);
    getConfig(CartImageDir);
    getConfig(CassImageDir);
    getConfig(ExeFileDir);
    getConfig(SavedStateDir);
    getConfig(ConfigDir);
    getConfig(D1File);
    getConfig(D2File);
    getConfig(D3File);
    getConfig(D4File);
    getConfig(D5File);
    getConfig(D6File);
    getConfig(D7File);
    getConfig(D8File);
    getConfig(CartFile);
    getConfig(Cart2File);
    getConfig(ExeFile);
    getConfig(CassFile);
    getConfig(SaveCurrentMedia);
    getConfig(ClearCurrentMedia);
    getConfig(D1FileEnabled);
    getConfig(D2FileEnabled);
    getConfig(D3FileEnabled);
    getConfig(D4FileEnabled);
    getConfig(D5FileEnabled);
    getConfig(D6FileEnabled);
    getConfig(D7FileEnabled);
    getConfig(D8FileEnabled);
    getConfig(CartFileEnabled);
    getConfig(Cart2FileEnabled);
    getConfig(ExeFileEnabled);
    getConfig(CassFileEnabled);
    getConfig(KeyjoyEnable);
    getConfig(Joystick1Mode);
    getConfig(Joystick2Mode);
    getConfig(Joystick3Mode);
    getConfig(Joystick4Mode);
    getConfig(Joystick1Autofire);
    getConfig(Joystick2Autofire);
    getConfig(Joystick3Autofire);
    getConfig(Joystick4Autofire);
    getConfig(MouseDevice);
    getConfig(MouseSpeed);
    getConfig(MouseMinVal);
    getConfig(MouseMaxVal);
    getConfig(MouseHOffset);
    getConfig(MouseVOffset);
    getConfig(MouseInertia);
    getConfig(Joystick1Type);
    getConfig(Joystick2Type);
    getConfig(Joystick3Type);
    getConfig(Joystick4Type);
    getConfig(Joystick1Num);
    getConfig(Joystick2Num);
    getConfig(Joystick3Num);
    getConfig(Joystick4Num);
    getConfig(CX85Enabled);
    getConfig(CX85Port);
    getConfig(GamepadConfigArray);
    getConfig(GamepadConfigCurrent);
    getConfig(Gamepad1ConfigCurrent);
    getConfig(Gamepad2ConfigCurrent);
    getConfig(Gamepad3ConfigCurrent);
    getConfig(Gamepad4ConfigCurrent);
    getConfig(ButtonAssignment);
    count = [[dict objectForKey:ButtonAssignment] count];
    if (count < 24) {
        for (i=count; i<24; i++)
            [[dict objectForKey:ButtonAssignment] addObject:[NSNumber numberWithInt:0]];
	}
    getConfig(Button5200Assignment);
    count = [[dict objectForKey:Button5200Assignment] count];
    if (count < 24) {
        for (i=count; i<24; i++)
            [[dict objectForKey:Button5200Assignment] addObject:[NSNumber numberWithInt:0]];
	}
    getConfig(PaddlesXAxisOnly);
    getConfig(LeftJoyUp);
    getConfig(LeftJoyDown);
    getConfig(LeftJoyLeft);
    getConfig(LeftJoyRight);
    getConfig(LeftJoyUpLeft);
    getConfig(LeftJoyUpRight);
    getConfig(LeftJoyDownLeft);
    getConfig(LeftJoyDownRight);
    getConfig(LeftJoyFire);
    getConfig(LeftJoyAltFire);
    getConfig(PadJoyUp);
    getConfig(PadJoyDown);
    getConfig(PadJoyLeft);
    getConfig(PadJoyRight);
    getConfig(PadJoyUpLeft);
    getConfig(PadJoyUpRight);
    getConfig(PadJoyDownLeft);
    getConfig(PadJoyDownRight);
    getConfig(PadJoyFire);
    getConfig(PadJoyAltFire);
	getConfig(MediaStatusDisplayed);
	getConfig(FunctionKeysDisplayed);
    getConfig(MediaStatusX);
    getConfig(MediaStatusY);
    getConfig(MessagesX);
    getConfig(MessagesY);
    getConfig(MonitorX);
    getConfig(MonitorY);
	getConfig(MonitorGUIVisable);
    getConfig(FunctionKeysX);
    getConfig(FunctionKeysY);
    getConfig(ApplicationWindowX);
    getConfig(ApplicationWindowY);

	[curValues release];
	curValues = [dict mutableCopyWithZone:[self zone]];
	[self discardDisplayedValues];
}

/**** Window delegation ****/

// We do this to catch the case where the user enters a value into one of the text fields but closes the window without hitting enter or tab.

- (void)windowWillClose:(NSNotification *)notification {
    NSWindow *window = [notification object];
    (void)[window makeFirstResponder:window];
}

@end
