/* Preferences.h - Header for Preferences 
   window class and support functions for the
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   
   Based on the Preferences pane of the
   TextEdit application.

*/
#import <Cocoa/Cocoa.h>
#import "Atari825Simulator.h"
#import "Atari1020Simulator.h"
#import "EpsonFx80Simulator.h"
#import "preferences_c.h"

/* Keys in the dictionary... */
#define FullScreen @"FullScreen"
#define OpenGl @"OpenGl"
#define LockFullscreenSize @"LockFullscreenSize"
#define FullscreenMonitor @"FullscreenMonitor"
#define FullscreenForeRed @"FullscreenForeRed"
#define FullscreenForeBlue @"FullscreenForeBlue"
#define FullscreenForeGreen @"FullscreenForeGreen"
#define FullscreenForeAlpha @"FullscreenForeAlpha"
#define FullscreenBackRed @"FullscreenBackRed"
#define FullscreenBackBlue @"FullscreenBackBlue"
#define FullscreenBackGreen @"FullscreenBackGreen"
#define FullscreenBackAlpha @"FullscreenBackAlpha"
#define ScaleFactor @"ScaleFactor"
#define DoubleSize @"DoubleSize"
#define ScaleMode @"ScaleMode"
#define WidthMode @"WidthMode"
#define TvMode @"TvMode"
#define EmulationSpeed @"EmulationSpeed"
#define RefreshRatio @"RefreshRatio"
#define SpriteCollisions @"SpriteCollisions"
#define ArtifactingMode @"ArtifactingMode"
#define ArtifactNew @"ArtifactNew"
#define UseBuiltinPalette @"UseBuiltinPalette"
#define AdjustPalette @"AdjustPalette"
#define BlackLevel @"BlackLevel"
#define WhiteLevel @"WhiteLevel"
#define Intensity @"Instensity"
#define ColorShift @"ColorShift"
#define PaletteFile @"PaletteFile"
#define ShowFPS @"ShowFPS"
#define LedStatus @"LedStatus"
#define LedSector @"LedSector"
#define LedStatusMedia @"LedStatusMedia"
#define LedSectorMedia @"LedSectorMedia"
#define XEP80Enabled @"XEP80Enabled"
#define XEP80Port @"XEP80Port"
#define XEP80OnColor @"XEP80OnColor"
#define XEP80OffColor @"XEP800OffColor"
#define XEP80 @"XEP80"
#define AtariType @"AtariType"
#define AtariTypeVer4 @"AtariTypeVer4"
#define AtariSwitchType @"AtariSwitchType"
#define AtariSwitchTypeVer4 @"AtariSwitchTypeVer4"
#define AxlonBankMask @"AxlonBankMask"
#define MosaicMaxBank @"MosaicMaxBank"
#define MioEnabled @"MioEnabled"
#define BlackBoxEnabled @"BlackBoxEnabled"
#define MioRomFile @"MioRomFile"
#define BlackBoxRomFile @"BlackBoxRomFile"
#define BlackBoxScsiDiskFile @"BlackBoxScsiDiskFile"
#define MioScsiDiskFile @"MioScsiDiskFile"
#define DisableBasic @"DisableBasic"
#define DisableAllBasic @"DisableAllBasic"
#define EnableSioPatch @"EnableSioPatch"
#define EnableHPatch @"EnableHPatch"
#define EnableDPatch @"EnableDPatch"
#define EnablePPatch @"EnablePPatch"
#define EnableRPatch @"EnableRPatch"
#define RPatchPort @"RPatchPort"
#define RPatchSerialEnabled @"RPatchSerialEnabled"
#define RPatchSerialPort @"RPatchSerialPort"
#define PrintCommand @"PrintCommand"
#define PrinterType @"PrinterType"
#define Atari825CharSet @"Atari825CharSet"
#define Atari825FormLength @"Atari825FormLength"
#define Atari825AutoLinefeed @"Atari825AutoLinefeed"
#define Atari1020PrintWidth @"Atari1020PrintWidth"
#define Atari1020FormLength @"Atari1020FormLength"
#define Atari1020AutoLinefeed @"Atari825AutoLinefeed"
#define Atari1020AutoPageAdjust @"Atari1020AutoPageAdjust"
#define Atari1020Pen1Red @"Atari1020Pen1Red"
#define Atari1020Pen1Blue @"Atari1020Pen1Blue"
#define Atari1020Pen1Green @"Atari1020Pen1Green"
#define Atari1020Pen1Alpha @"Atari1020Pen1Alpha"
#define Atari1020Pen2Red @"Atari1020Pen2Red"
#define Atari1020Pen2Blue @"Atari1020Pen2Blue"
#define Atari1020Pen2Green @"Atari1020Pen2Green"
#define Atari1020Pen2Alpha @"Atari1020Pen2Alpha"
#define Atari1020Pen3Red @"Atari1020Pen3Red"
#define Atari1020Pen3Blue @"Atari1020Pen3Blue"
#define Atari1020Pen3Green @"Atari1020Pen3Green"
#define Atari1020Pen3Alpha @"Atari1020Pen3Alpha"
#define Atari1020Pen4Red @"Atari1020Pen4Red"
#define Atari1020Pen4Blue @"Atari1020Pen4Blue"
#define Atari1020Pen4Green @"Atari1020Pen4Green"
#define Atari1020Pen4Alpha @"Atari1020Pen4Alpha"
#define EpsonCharSet @"EpsonCharSet"
#define EpsonPrintPitch @"EpsonPrintPitch"
#define EpsonPrintWeight @"EpsonPrintWeight"
#define EpsonFormLength @"EpsonFormLength"
#define EpsonAutoLinefeed @"EpsonAutoLinefeed"
#define EpsonPrintSlashedZeros @"EpsonPrintSlashedZeros"
#define EpsonAutoSkip @"EpsonAutoSkip"
#define EpsonSplitSkip @"EpsonSplitSkip"
#define BootFromCassette @"BootFromCassette"
#define SpeedLimit @"SpeedLimit"
#define EnableSound @"EnableSound"
#define SoundVolume @"SoundVolume"
#define EnableStereo @"EnableStereo"
#if 0 /* enableHifiSound is deprecated from 4.2.2 on */    
#define EnableHifiSound @"EnableHifiSound"
#endif
#define Enable16BitSound @"Enable16BitSound"
#define EnableConsoleSound @"EnableConsoleSound"
#define EnableSerioSound @"EnableSerioSound"
#define DontMuteAudio @"DontMuteAudio"
#define EnableMultijoy @"EnableMultijoy"
#define IgnoreHeaderWriteprotect @"IgnoreHeaderWriteprotect"
#define ImageDir @"ImageDir"
#define PrintDir @"PrintDir"
#define HardDiskDir1 @"HardDiskDir1"
#define HardDiskDir2 @"HardDiskDir2"
#define HardDiskDir3 @"HardDiskDir3"
#define HardDiskDir4 @"HardDiskDir4"
#define HardDrivesReadOnly @"HardDrivesReadOnly"
#define HPath @"HPath"
#define OsARomFile @"OsARomFile"
#define OsBRomFile @"OsBRomFile"
#define XlRomFile @"XlRomFile"
#define BasicRomFile @"BasicRomFile"
#define A5200RomFile @"A5200RomFile"
#define DiskImageDir @"DiskImageDir"
#define DiskSetDir @"DiskSetDir"
#define CartImageDir @"CartImageDir"
#define CassImageDir @"CassImageDir"
#define ExeFileDir @"ExeFileDir"
#define SavedStateDir @"SavedStateDir"
#define ConfigDir @"ConfigDir"
#define D1File @"D1File"
#define D2File @"D2File"
#define D3File @"D3File"
#define D4File @"D4File"
#define D5File @"D5File"
#define D6File @"D6File"
#define D7File @"D7File"
#define D8File @"D8File"
#define CartFile @"CartFile"
#define Cart2File @"Cart2File"
#define ExeFile @"ExeFile"
#define CassFile @"CassFile"
#define D1FileEnabled @"D1FileEnabled"
#define D2FileEnabled @"D2FileEnabled"
#define D3FileEnabled @"D3FileEnabled"
#define D4FileEnabled @"D4FileEnabled"
#define D5FileEnabled @"D5FileEnabled"
#define D6FileEnabled @"D6FileEnabled"
#define D7FileEnabled @"D7FileEnabled"
#define D8FileEnabled @"D8FileEnabled"
#define CartFileEnabled @"CartFileEnabled"
#define Cart2FileEnabled @"Cart2FileEnabled"
#define ExeFileEnabled @"ExeFileEnabled"
#define CassFileEnabled @"CassFileEnabled"
#define Joystick1Mode @"Joystick1Mode_v13"
#define Joystick2Mode @"Joystick2Mode_v13"
#define Joystick3Mode @"Joystick3Mode_v13"
#define Joystick4Mode @"Joystick4Mode_v13"
#define Joystick1MultiMode @"Joystick1MultiMode"
#define Joystick2MultiMode @"Joystick2MultiMode"
#define Joystick3MultiMode @"Joystick3MultiMode"
#define Joystick4MultiMode @"Joystick4MultiMode"
#define Joystick1Autofire @"Joystick1Autofire"
#define Joystick2Autofire @"Joystick2Autofire"
#define Joystick3Autofire @"Joystick3Autofire"
#define Joystick4Autofire @"Joystick4Autofire"
#define MouseDevice @"MouseDevice"
#define MouseSpeed @"MouseSpeed"
#define MouseMinVal @"MouseMinVal"
#define MouseMaxVal @"MouseMaxVal"
#define MouseHOffset @"MouseHOffset"
#define MouseVOffset @"MouseVOffset"
#define MouseInertia @"MouseInertia"
#define ButtonAssignment @"ButtonAssignment"
#define Button5200Assignment @"Button5200Assignment"
#define Joystick1Type @"Joystick1Type"
#define Joystick2Type @"Joystick2Type"
#define Joystick3Type @"Joystick3Type"
#define Joystick4Type @"Joystick4Type"
#define Joystick1Num @"Joystick1Num"
#define Joystick2Num @"Joystick2Num"
#define Joystick3Num @"Joystick3Num"
#define Joystick4Num @"Joystick4Num"
#define CX85Enabled @"CX85Enabled"
#define CX85Port @"CX85Port"
#define PaddlesXAxisOnly @"PaddlesXAxisOnly"
#define GamepadConfigArray @"GamepadConfigArray"
#define GamepadConfigCurrent @"GamepadConfigCurrent"
#define Gamepad1ConfigCurrent @"Gamepad1ConfigCurrent"
#define Gamepad2ConfigCurrent @"Gamepad2ConfigCurrent"
#define Gamepad3ConfigCurrent @"Gamepad3ConfigCurrent"
#define Gamepad4ConfigCurrent @"Gamepad4ConfigCurrent"
#define ButtonAssignmentPrefix @"ButtonAssignment."
#define Button5200AssignmentPrefix @"Button5200Assignment."
#define StandardConfigString @"Standard"
#define LeftJoyUp @"LeftJoyUp"
#define LeftJoyDown @"LeftJoyDown"
#define LeftJoyLeft @"LeftJoyLeft"
#define LeftJoyRight @"LeftJoyRight"
#define LeftJoyUpLeft @"LeftJoyUpLeft"
#define LeftJoyUpRight @"LeftJoyUpRight"
#define LeftJoyDownLeft @"LeftJoyDownLeft"
#define LeftJoyDownRight @"LeftJoyDownRight"
#define LeftJoyFire @"LeftJoyFire"
#define LeftJoyAltFire @"LeftJoyAltFire"
#define PadJoyUp @"PadJoyUp"
#define PadJoyDown @"PadJoyDown"
#define PadJoyLeft @"PadJoyLeft"
#define PadJoyRight @"PadJoyRight"
#define PadJoyUpLeft @"PadJoyUpLeft"
#define PadJoyUpRight @"PadJoyUpRight"
#define PadJoyDownLeft @"PadJoyDownLeft"
#define PadJoyDownRight @"PadJoyDownRight"
#define PadJoyFire @"PadJoyFire"
#define PadJoyAltFire @"PadJoyAltFire"
#define MediaStatusDisplayed @"MediaStatusDisplayed"
#define FunctionKeysDisplayed @"FunctionKeysDisplayed"
#define MediaStatusX @"MediaStatusX"
#define MediaStatusY @"MediaStatusY"
#define MessagesX @"MessagesX"
#define MessagesY @"MessagesY"
#define MonitorX @"MonitorX"
#define MonitorY @"MonitorY"
#define MonitorGUIVisable @"MonitorGUIVisable"
#define FunctionKeysX @"FunctionKeysX"
#define FunctionKeysY @"FunctionKeysY"
#define ApplicationWindowX @"ApplicationWindowX"
#define ApplicationWindowY @"ApplicationWindowY"
#define EnableInternational @"EnableInternational"
#define SaveCurrentMedia @"SaveCurrentMedia"
#define ClearCurrentMedia @"ClearCurrentMedia"
#define KeyjoyEnable @"KeyjoyEnable"
#define UseAtariCursorKeys @"UseAtariCursorKeys"

#define MAX_MODEMS				8

@interface Preferences : NSObject {
    IBOutlet id prefTabView;
    IBOutlet id a5200RomFileField;
    IBOutlet id adjustPaletteButton;
    IBOutlet id artifactingPulldown;
    IBOutlet id artifactNewButton;
    IBOutlet id atariTypePulldown;
    IBOutlet id atariSwitchTypePulldown;
    IBOutlet id basicRomFileField;
    IBOutlet id blackLevelField;
    IBOutlet id cartImageDirField;
    IBOutlet id cassImageDirField;
    IBOutlet id colorShiftField;
	IBOutlet id axlonMemSizePulldown;
	IBOutlet id mosaicMemSizePulldown;
	IBOutlet id pbiExpansionMatrix;
	IBOutlet id mioRomFileField;
	IBOutlet id blackBoxRomFileField;
	IBOutlet id blackBoxScsiDiskFileField;
	IBOutlet id mioScsiDiskFileField;
    IBOutlet id disableBasicButton;
    IBOutlet id disableAllBasicButton;
    IBOutlet id diskImageDirField;
    IBOutlet id diskSetDirField;
	IBOutlet id spriteCollisionsButton;
    IBOutlet id lockFullscreenSizeButton;
    IBOutlet id fullscreenMonitorButton;
	IBOutlet id fullscreenForegroundPot;
    IBOutlet id fullscreenForegroundRed;
    IBOutlet id fullscreenForegroundGreen;
    IBOutlet id fullscreenForegroundBlue;
	IBOutlet id fullscreenBackgroundPot;
    IBOutlet id fullscreenBackgroundRed;
    IBOutlet id fullscreenBackgroundGreen;
    IBOutlet id fullscreenBackgroundBlue;
    IBOutlet id scaleFactorMatrix;
	IBOutlet id scaleModeMatrix;
	IBOutlet id ledStatusButton;
	IBOutlet id ledSectorButton;
	IBOutlet id ledStatusMediaButton;
	IBOutlet id ledSectorMediaButton;
	IBOutlet id xep80EnabledButton;
	IBOutlet id xep80PortPulldown;
	IBOutlet id xep80ForegroundField;
	IBOutlet id xep80BackgroundField;
	IBOutlet id emulationSpeedSlider;
    IBOutlet id enableHPatchButton;
    IBOutlet id enableDPatchButton;
    IBOutlet id enablePPatchButton;
    IBOutlet id enableRPatchButton;
    IBOutlet id rPatchPortField;
	IBOutlet id rPatchSerialMatrix;
	IBOutlet id rPatchSerialPulldown;
	IBOutlet id useAtariCursorKeysPulldown;
    IBOutlet id externalPaletteButton;
    IBOutlet id printCommandField;
	IBOutlet id printerTypePulldown;
	IBOutlet id printerTabView;
	IBOutlet id atari825CharSetPulldown;
	IBOutlet id atari825FormLengthField;
	IBOutlet id atari825FormLengthStepper;
	IBOutlet id atari825AutoLinefeedButton;
	IBOutlet id atari1020PrintWidthPulldown;
	IBOutlet id atari1020FormLengthField;
	IBOutlet id atari1020FormLengthStepper;
	IBOutlet id atari1020AutoLinefeedButton;
	IBOutlet id atari1020AutoPageAdjustButton;
    IBOutlet id atari1020Pen1Red;
    IBOutlet id atari1020Pen1Green;
    IBOutlet id atari1020Pen1Blue;
    IBOutlet id atari1020Pen2Red;
    IBOutlet id atari1020Pen2Green;
    IBOutlet id atari1020Pen2Blue;
    IBOutlet id atari1020Pen3Red;
    IBOutlet id atari1020Pen3Green;
    IBOutlet id atari1020Pen3Blue;
    IBOutlet id atari1020Pen4Red;
    IBOutlet id atari1020Pen4Green;
    IBOutlet id atari1020Pen4Blue;
	IBOutlet id epsonCharSetPulldown;
	IBOutlet id epsonPrintPitchPulldown;
	IBOutlet id epsonPrintWeightPulldown;
	IBOutlet id epsonFormLengthField;
	IBOutlet id epsonFormLengthStepper;
	IBOutlet id epsonAutoLinefeedButton;
	IBOutlet id epsonPrintSlashedZerosButton;
	IBOutlet id epsonAutoSkipButton;
	IBOutlet id epsonSplitSkipButton;
    IBOutlet id enableSioPatchButton;
    IBOutlet id bootFromCassetteButton;
    IBOutlet id ignoreHeaderWriteprotectButton;
    IBOutlet id consoleSoundEnableButton;
    IBOutlet id serioSoundEnableButton;
	IBOutlet id muteAudioButton;
    IBOutlet id exeFileDirField;
    IBOutlet id fullScreenMatrix;
    IBOutlet id fpsButton;
    IBOutlet id imageDirField;
    IBOutlet id printDirField;
    IBOutlet id hardDiskDir1Field;
    IBOutlet id hardDiskDir2Field;
    IBOutlet id hardDiskDir3Field;
    IBOutlet id hardDiskDir4Field;
    IBOutlet id hardDrivesReadOnlyButton;
    IBOutlet id hPathField;
    IBOutlet id intensityField;
    IBOutlet id osARomFileField;
    IBOutlet id osBRomFileField;
    IBOutlet id refreshRatioPulldown;
    IBOutlet id savedStateDirField;
    IBOutlet id configDirField;
    IBOutlet id speedLimitButton;
    IBOutlet id enableSoundButton;
#if 0 /* enableHifiSound is deprecated from 4.2.2 on */    	
    IBOutlet id enableHifiSoundButton;
#endif	
	IBOutlet id enable16BitSoundPulldown;
	IBOutlet id internationalKeyboardButton;
    IBOutlet id enableMultijoyButton;
    IBOutlet id tvModeMatrix;
    IBOutlet id widthModeMatrix;
    IBOutlet id whiteLevelField;
    IBOutlet id xlRomFileField;
    IBOutlet id d1FileField;
    IBOutlet id d2FileField;
    IBOutlet id d3FileField;
    IBOutlet id d4FileField;
    IBOutlet id d5FileField;
    IBOutlet id d6FileField;
    IBOutlet id d7FileField;
    IBOutlet id d8FileField;
    IBOutlet id cartFileField;
    IBOutlet id cart2FileField;
    IBOutlet id exeFileField;
    IBOutlet id cassFileField;
    IBOutlet id d1FileEnabledButton;
    IBOutlet id d2FileEnabledButton;
    IBOutlet id d3FileEnabledButton;
    IBOutlet id d4FileEnabledButton;
    IBOutlet id d5FileEnabledButton;
    IBOutlet id d6FileEnabledButton;
    IBOutlet id d7FileEnabledButton;
    IBOutlet id d8FileEnabledButton;
    IBOutlet id cartFileEnabledButton;
    IBOutlet id cart2FileEnabledButton;
    IBOutlet id exeFileEnabledButton;
    IBOutlet id cassFileEnabledButton;
    IBOutlet id d1FileSelectButton;
    IBOutlet id d2FileSelectButton;
    IBOutlet id d3FileSelectButton;
    IBOutlet id d4FileSelectButton;
    IBOutlet id d5FileSelectButton;
    IBOutlet id d6FileSelectButton;
    IBOutlet id d7FileSelectButton;
    IBOutlet id d8FileSelectButton;
    IBOutlet id cartFileSelectButton;
    IBOutlet id cart2FileSelectButton;
    IBOutlet id exeFileSelectButton;
    IBOutlet id cassFileSelectButton;
    IBOutlet id joystick1Pulldown;
    IBOutlet id joystick2Pulldown;
    IBOutlet id joystick3Pulldown;
    IBOutlet id joystick4Pulldown;
    IBOutlet id joy1AutofirePulldown;
    IBOutlet id joy2AutofirePulldown;
    IBOutlet id joy3AutofirePulldown;
    IBOutlet id joy4AutofirePulldown;
    IBOutlet id mouseDevicePulldown;
    IBOutlet id mouseSpeedField;
    IBOutlet id mouseMinValField;
    IBOutlet id mouseMaxValField;
    IBOutlet id mouseHOffsetField;
    IBOutlet id mouseVOffsetField;
    IBOutlet id mouseInertiaField;
	IBOutlet id openglMatrix;
    IBOutlet id paletteChooseButton;
    IBOutlet id paletteField;
    IBOutlet id gamepadConfigPulldown;
    IBOutlet id gamepad1IdentifyButton;
    IBOutlet id gamepad2IdentifyButton;
    IBOutlet id gamepad3IdentifyButton;
    IBOutlet id gamepad4IdentifyButton;
    IBOutlet id gamepad1ConfigPulldown;
    IBOutlet id gamepad2ConfigPulldown;
    IBOutlet id gamepad3ConfigPulldown;
    IBOutlet id gamepad4ConfigPulldown;
    IBOutlet id gamepadButtonPulldown;
    IBOutlet id gamepadAssignmentPulldown;
    IBOutlet id gamepad5200AssignmentPulldown;
    IBOutlet id joystick1TypePulldown;
    IBOutlet id joystick2TypePulldown;
    IBOutlet id joystick3TypePulldown;
    IBOutlet id joystick4TypePulldown;
    IBOutlet id joystick1NumPulldown;
    IBOutlet id joystick2NumPulldown;
    IBOutlet id joystick3NumPulldown;
    IBOutlet id joystick4NumPulldown;
	IBOutlet id cx85EnabledButton;
	IBOutlet id cx85PortPulldown;
    IBOutlet id paddlesXAxisOnlyButton;
    IBOutlet id gamepadButton1;
    IBOutlet id gamepadButton2;
    IBOutlet id gamepadButton3;
    IBOutlet id gamepadButton4;
    IBOutlet id gamepadButton5;
    IBOutlet id gamepadButton6;
    IBOutlet id gamepadButton7;
    IBOutlet id gamepadButton8;
    IBOutlet id gamepadButton9;
    IBOutlet id gamepadButton10;
    IBOutlet id gamepadButton11;
    IBOutlet id gamepadButton12;
    IBOutlet id gamepadButton13;
    IBOutlet id gamepadButton14;
    IBOutlet id gamepadButton15;
    IBOutlet id gamepadButton16;
    IBOutlet id gamepadButton17;
    IBOutlet id gamepadButton18;
    IBOutlet id gamepadButton19;
    IBOutlet id gamepadButton20;
    IBOutlet id gamepadButton21;
    IBOutlet id gamepadButton22;
    IBOutlet id gamepadButton23;
    IBOutlet id gamepadButton24;
    IBOutlet id errorOKButton;
    IBOutlet id configNameField;
    IBOutlet id leftJoyUpPulldown;
    IBOutlet id leftJoyDownPulldown;
    IBOutlet id leftJoyLeftPulldown;
    IBOutlet id leftJoyRightPulldown;
    IBOutlet id leftJoyUpLeftPulldown;
    IBOutlet id leftJoyUpRightPulldown;
    IBOutlet id leftJoyDownLeftPulldown;
    IBOutlet id leftJoyDownRightPulldown;
    IBOutlet id leftJoyFirePulldown;
    IBOutlet id leftJoyAltFirePulldown;
    IBOutlet id padJoyUpPulldown;
    IBOutlet id padJoyDownPulldown;
    IBOutlet id padJoyLeftPulldown;
    IBOutlet id padJoyRightPulldown;
    IBOutlet id padJoyUpLeftPulldown;
    IBOutlet id padJoyUpRightPulldown;
    IBOutlet id padJoyDownLeftPulldown;
    IBOutlet id padJoyDownRightPulldown;
    IBOutlet id padJoyFirePulldown;
    IBOutlet id padJoyAltFirePulldown;
    IBOutlet id gamepadNameField;
    IBOutlet id gamepadNumButtonsField;
    IBOutlet id gamepadNumSticksField;
    IBOutlet id gamepadNumHatsField;
	IBOutlet id saveCurrentMediaButton;
	IBOutlet id clearCurrentMediaButton;

    int padNum; /* Remember number of gamepad for ID */
    char bsdPaths[MAX_MODEMS][FILENAME_MAX];
    char modemNames[MAX_MODEMS][FILENAME_MAX];
	int modemCount;
    NSMutableArray *modems;
    
    NSMutableDictionary *curValues;	// Current, confirmed values for the preferences
    NSDictionary *origValues;	// Values read from preferences at startup
    NSMutableDictionary *displayedValues;	// Values displayed in the UI
    NSTimer *theTimer;
}

+ (id)objectForKey:(id)key;	/* Convenience for getting global preferences */
+ (void)saveDefaults;		/* Convenience for saving global preferences */
- (void)saveDefaults;		/* Save the current preferences */

+ (Preferences *)sharedInstance;

+ (void)setWorkingDirectory:(char *)dir;  /* Save the base directory of the application */
+ (char *)getWorkingDirectory;  /* Get the base directory of the application */

- (NSDictionary *)preferences;	/* The current preferences; contains values for the documented keys */

- (void)showPanel:(id)sender;	/* Shows the panel */
- (int)getBrushed;

- (void)updateUI;		/* Updates the displayed values in the UI */
- (void)setBootMediaActive:(bool) active;
- (void)clearBootMedia;
- (void)updateJoyNumMenus;	/* Updates the joystick Number menus, based on joystick type */
- (void)commitDisplayedValues;	/* The displayed values are made current */
- (void)discardDisplayedValues;	/* The displayed values are replaced with current prefs and updateUI is called */

- (void)revert:(id)sender;	/* Reverts the displayed values to the current preferences */
- (void)ok:(id)sender;		/* Calls commitUI to commit the displayed values as current */
- (void)revertToDefault:(id)sender;    

- (void)miscChanged:(id)sender;	/* Action message for most of the misc items in the UI to get displayedValues  */
- (void)browsePalette:(id)sender; 
- (void)browseImage:(id)sender; 
- (void)browsePrint:(id)sender; 
- (void)browseHardDisk1:(id)sender; 
- (void)browseHardDisk2:(id)sender; 
- (void)browseHardDisk3:(id)sender; 
- (void)browseHardDisk4:(id)sender; 
- (void)browseOsARom:(id)sender; 
- (void)browseOsBRom:(id)sender; 
- (void)browseXlRom:(id)sender; 
- (void)browseBasicRom:(id)sender; 
- (void)browse5200Rom:(id)sender; 
- (void)browseMioRom:(id)sender; 
- (void)browseBlackBoxRom:(id)sender; 
- (void)browseBlackBoxScsiDiskFile:(id)sender; 
- (void)browseMioScsiDiskFile:(id)sender; 
- (void)browseDiskDir:(id)sender; 
- (void)browseDiskSetDir:(id)sender; 
- (void)browseCartDir:(id)sender; 
- (void)browseCassDir:(id)sender; 
- (void)browseExeDir:(id)sender; 
- (void)browseStateDir:(id)sender; 
- (void)browseConfigDir:(id)sender;
- (void)browseD1File:(id)sender; 
- (void)browseD2File:(id)sender; 
- (void)browseD3File:(id)sender; 
- (void)browseD4File:(id)sender; 
- (void)browseD5File:(id)sender; 
- (void)browseD6File:(id)sender; 
- (void)browseD7File:(id)sender; 
- (void)browseD8File:(id)sender; 
- (void)browseCartFile:(id)sender; 
- (void)browseExeFile:(id)sender; 
- (void)browseCassFile:(id)sender; 
- (void)transferValuesToEmulator;
- (void)transferValuesToAtari825;
- (void)transferValuesToAtari1020;
- (void)transferValuesToEpson;
- (NSPoint)mediaStatusOrigin;
- (NSPoint)messagesOrigin;
- (NSPoint)functionKeysOrigin;
- (NSPoint)monitorOrigin;
- (BOOL)monitorGUIVisable;
- (NSPoint)applicationWindowOrigin;
- (void)transferValuesFromEmulator:(struct ATARI800MACX_PREFSAVE *)prefssave;
- (void) saveCurrentMediaAction:(id)sender;
- (void) saveCurrentMedia:(char [][FILENAME_MAX]) disk_filename:(char *) cassette_filename:
						(char *) cart_filename:(char *) cart2_filename;
- (void)windowWillClose:(NSNotification *)notification;
- (void)gamepadConfigChange:(id)sender;
- (void)gamepadButtonChange:(id)sender;
- (void)buttonAssign:(id)sender; 
- (void)button5200Assign:(id)sender; 
- (IBAction)identifyGamepad:(id)sender; 
- (void)identifyOK:(id)sender;
- (void)identifyTest:(id)sender;
- (void)configNameOK:(id)sender;
- (void)errorOK:(id)sender;
- (void)leftJoyConfigure:(id)sender;
- (void)leftJoyOK:(id)sender;
- (void)padJoyConfigure:(id)sender;
- (void)padJoyOK:(id)sender;

- (int)indexFromType:(int) type:(int) ver4type;
- (int)typeFromIndex:(int) index:(int *)ver4type;

- (void)generateModemList;
- (int) getModemPaths:(io_iterator_t )serialPortIterator;
- (kern_return_t) findModems:(io_iterator_t *) matchingServices;

+ (NSDictionary *)preferencesFromDefaults;
+ (void)savePreferencesToDefaults:(NSDictionary *)dict;
- (void)saveConfigurationData:(NSString *)filename;
- (void)saveConfiguration:(id)sender;
- (void)saveConfigurationMenu:(id)sender;
- (void)saveConfigurationUI:(char *)filename;
- (int)loadConfiguration:(id)sender;
- (void)loadConfigurationMenu:(id)sender;
- (void)loadConfigurationUI:(char *)filename;
- (void)loadConfigFile:(NSString *)filename;

@end
