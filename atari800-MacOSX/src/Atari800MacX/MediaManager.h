/* MediaManager.h - Window and menu support
   class to handle disk, cartridge, cassette,
   and executable file management and support 
   functions for the Macintosh OS X SDL port 
   of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   
   Based on the Preferences pane of the
   TextEdit application.

*/

#import <Cocoa/Cocoa.h>

@interface MediaManager : NSObject
{
    IBOutlet id d1DiskField;
    IBOutlet id d1DriveStatusPulldown;
    IBOutlet id d2DiskField;
    IBOutlet id d2DriveStatusPulldown;
    IBOutlet id d3DiskField;
    IBOutlet id d3DriveStatusPulldown;
    IBOutlet id d4DiskField;
    IBOutlet id d4DriveStatusPulldown;
    IBOutlet id d5DiskField;
    IBOutlet id d5DriveStatusPulldown;
    IBOutlet id d6DiskField;
    IBOutlet id d6DriveStatusPulldown;
    IBOutlet id d7DiskField;
    IBOutlet id d7DriveStatusPulldown;
    IBOutlet id d8DiskField;
    IBOutlet id d8DriveStatusPulldown;
	IBOutlet id d1SwitchButton;
	IBOutlet id d2SwitchButton;
	IBOutlet id d3SwitchButton;
	IBOutlet id d4SwitchButton;
	IBOutlet id d5SwitchButton;
	IBOutlet id d6SwitchButton;
	IBOutlet id d7SwitchButton;
	IBOutlet id d8SwitchButton;
    IBOutlet id diskFmtCusBytesPulldown;
    IBOutlet id diskFmtCusSecField;
    IBOutlet id diskFmtDDBytesPulldown;
    IBOutlet id diskFmtInsertDrivePulldown;
    IBOutlet id diskFmtInsertNewButton;
    IBOutlet id diskFmtMatrix;
    IBOutlet id hardDiskFmtCusMBField;
    IBOutlet id hardDiskFmtCusSecField;
    IBOutlet id hardDiskFmtInsertNewButton;
    IBOutlet id hardDiskFmtMatrix;
	IBOutlet id insertSecondCartItem;
    IBOutlet id removeMenu;
    IBOutlet id removeD1Item;
    IBOutlet id removeD2Item;
    IBOutlet id removeD3Item;
    IBOutlet id removeD4Item;
    IBOutlet id removeD5Item;
    IBOutlet id removeD6Item;
    IBOutlet id removeD7Item;
    IBOutlet id removeD8Item;
    IBOutlet id removeCartItem;
    IBOutlet id removeSecondCartItem;
    IBOutlet id protectCassItem;
    IBOutlet id recordCassItem;
    IBOutlet id removeCassItem;
    IBOutlet id rewindCassItem;
	IBOutlet id selectPrinterPulldown;
	IBOutlet id selectPrinterMenu;
	IBOutlet id selectTextItem;
	IBOutlet id selectTextMenuItem;
	IBOutlet id selectAtari825Item;
	IBOutlet id selectAtari825MenuItem;
	IBOutlet id selectAtari1020Item;
	IBOutlet id selectAtari1020MenuItem;
	IBOutlet id selectEpsonItem;
	IBOutlet id selectEpsonMenuItem;
    IBOutlet id selectAtasciiItem;
    IBOutlet id selectAtasciiMenuItem;
	IBOutlet id resetPrinterItem;
	IBOutlet id resetPrinterMenuItem;
    IBOutlet id errorButton;
    IBOutlet id errorField;
    IBOutlet id error2Button;
    IBOutlet id error2Field1;
    IBOutlet id error2Field2;
    IBOutlet id cart2KMatrix;
    IBOutlet id cart4KMatrix;
    IBOutlet id cart8KMatrix;
    IBOutlet id cart16KMatrix;
    IBOutlet id cart32KMatrix;
    IBOutlet id cart40KMatrix;
    IBOutlet id cart64KMatrix;
    IBOutlet id cart128KMatrix;
    IBOutlet id cart256KMatrix;
    IBOutlet id cart512KMatrix;
    IBOutlet id cart1024KMatrix;
    IBOutlet id cart2048KMatrix;
    IBOutlet id cart4096KMatrix;
    IBOutlet id cart32MMatrix;
    IBOutlet id cart64MMatrix;
    IBOutlet id cart128MMatrix;
	IBOutlet id d1DiskImageInsertButton;
	IBOutlet id d1DiskImageNameField;
	IBOutlet id d1DiskImageNumberField;
	IBOutlet id d1DiskImageSectorField;
	IBOutlet id d1DiskImagePowerButton;
	IBOutlet id d1DiskImageProtectButton;
	IBOutlet id d1DiskImageView;
	IBOutlet id d1DiskImageLockView;
	IBOutlet id d2DiskImageInsertButton;
	IBOutlet id d2DiskImageNameField;
	IBOutlet id d2DiskImageNumberField;
	IBOutlet id d2DiskImageSectorField;
	IBOutlet id d2DiskImagePowerButton;
	IBOutlet id d2DiskImageProtectButton;
	IBOutlet id d2DiskImageView;
	IBOutlet id d2DiskImageLockView;
	IBOutlet id d3DiskImageInsertButton;
	IBOutlet id d3DiskImageNameField;
	IBOutlet id d3DiskImageNumberField;
	IBOutlet id d3DiskImageSectorField;
	IBOutlet id d3DiskImagePowerButton;
	IBOutlet id d3DiskImageProtectButton;
	IBOutlet id d3DiskImageView;
	IBOutlet id d3DiskImageLockView;
	IBOutlet id d4DiskImageInsertButton;
	IBOutlet id d4DiskImageNameField;
	IBOutlet id d4DiskImageNumberField;
	IBOutlet id d4DiskImageSectorField;
	IBOutlet id d4DiskImagePowerButton;
	IBOutlet id d4DiskImageProtectButton;
	IBOutlet id d4DiskImageView;
	IBOutlet id d4DiskImageLockView;
	IBOutlet id d8DiskImageLockView;
	IBOutlet id driveSelectMatrix;
	IBOutlet id cassImageInsertButton;
	IBOutlet id cassImageNameField;
	IBOutlet id cassImageView;
	IBOutlet id cassImageSlider;
	IBOutlet id cassImageSliderCurrField;
	IBOutlet id cassImageSliderMaxField;
    IBOutlet id cassImageLockView;
    IBOutlet id cassImageProtectButton;
    IBOutlet id cassImageRecordButton;
	IBOutlet id cartImageInsertButton;
	IBOutlet id cartImageSecondInsertButton;
	IBOutlet id cartImageNameField;
	IBOutlet id cartImageSecondNameField;
	IBOutlet id cartImageView;
    IBOutlet id cartImageSIDEButton;
    IBOutlet id cartImageSDXButton;
    IBOutlet id cartImageRomInsertButton;
    IBOutlet id printerImageView;
	IBOutlet id printerImageNameField;
	IBOutlet id printerPreviewButton;
	IBOutlet id printerPreviewItem;
	IBOutlet id speedLimitButton;
	IBOutlet id disBasicButton;
    IBOutlet id insertBasicItem;
    IBOutlet id insertSIDE2Item;
    IBOutlet id changeSIDE2RomItem;
    IBOutlet id saveSIDE2RomItem;
    IBOutlet id saveUltimateRomItem;
    IBOutlet id attachSIDE2CFItem;
    IBOutlet id removeSIDE2CFItem;
    IBOutlet id slideSIDE2ButtonSDXItem;
    IBOutlet id slideSIDE2ButtonLoaderItem;
    IBOutlet id pressSIDE2ButtonItem;
    IBOutlet id xep80Button;
    IBOutlet id xep80Pulldown;
	IBOutlet id machineTypePulldown;
	IBOutlet id scaleModePulldown;
	IBOutlet id widthModePulldown;
	IBOutlet id artifactModePulldown;
	int numChecked;
	int checks[8];
}
+ (MediaManager *)sharedInstance;
- (void)displayError:(NSString *)errorMsg;
- (void)updateInfo;
- (NSString *) browseFileInDirectory:(NSString *)directory;
- (NSString *) browseFileTypeInDirectory:(NSString *)directory:(NSArray *) filetypes;
- (NSString *) saveFileInDirectory:(NSString *)directory:(NSString *)type;
- (IBAction)cancelDisk:(id)sender;
- (IBAction)cancelHardDisk:(id)sender;
- (IBAction)cartInsert:(id)sender;
- (IBAction)basicInsert:(id)sender;
- (IBAction)side2Insert:(id)sender;
- (IBAction)cartSecondInsert:(id)sender;
- (void)cartInsertFile:(NSString *)filename;
- (IBAction)cartRemove:(id)sender;
- (IBAction)cartSecondRemove:(id)sender;
- (int)cartSelect:(int)cartSize;
- (IBAction)cartSelectOK:(id)sender;
- (IBAction)cartSelectCancel:(id)sender;
- (IBAction)cassInsert:(id)sender;
- (void)cassInsertFile:(NSString *)filename;
- (IBAction)cassRecord:(id)sender;
- (IBAction)cassRemove:(id)sender;
- (IBAction)cassRewind:(id)sender;
- (IBAction)cassStatusProtect:(id)sender;
- (void)changeToComputer;
- (IBAction)convertCartRom:(id)sender;
- (IBAction)convertRomCart:(id)sender;
- (IBAction)createDisk:(id)sender;
- (IBAction)createHardDisk:(id)sender;
- (IBAction)createCassette:(id)sender;
- (IBAction)diskInsert:(id)sender;
- (IBAction)diskRotate:(id)sender;
- (void)diskInsertFile:(NSString *)filename;
- (void)diskNoInsertFile:(NSString *)filename:(int) driveNo;
- (IBAction)diskRemove:(id)sender;
- (IBAction)diskInsertKey:(int)diskNum;
- (IBAction)diskSetSave:(id)sender;
- (IBAction)diskSetLoad:(id)sender;
- (IBAction)diskSetLoadFile:(NSString *)filename;
- (IBAction)diskRemoveKey:(int)diskNum;
- (IBAction)diskRemoveAll:(id)sender;
- (IBAction)driveStatusChange:(id)sender;
- (IBAction)loadExeFile:(id)sender;
- (void)loadExeFileFile:(NSString *)filename;
- (IBAction)convertXFDtoATR:(id)sender;
- (IBAction)convertATRtoXFD:(id)sender;
- (IBAction)convertATRtoDCM:(id)sender;
- (IBAction)convertDCMtoATR:(id)sender;
- (IBAction)convertATRtoSCP:(id)sender;
- (IBAction)convertSCPtoATR:(id)sender;
- (IBAction)miscUpdate:(id)sender;
- (IBAction)hardSecUpdate:(id)sender;
- (IBAction)hardMBUpdate:(id)sender;
- (IBAction)ok:(id)sender;
- (IBAction)showCreatePanel:(id)sender;
- (IBAction)showHardCreatePanel:(id)sender;
- (IBAction)showManagementPanel:(id)sender;
- (IBAction)showSectorPanel:(id)sender;
- (IBAction)showEditorPanel:(id)sender;
- (IBAction)errorOK:(id)sender;
- (IBAction)error2OK:(id)sender;
- (void) mediaStatusWindowShow:(id)sender;
- (NSPoint)mediaStatusOriginSave;
- (IBAction)diskDisplayChange:(id)sender;
- (IBAction)diskStatusChange:(id)sender;
- (IBAction)diskStatusPower:(id)sender;
- (IBAction)diskStatusProtect:(id)sender;
- (IBAction)cartStatusChange:(id)sender;
- (IBAction)cartSecondStatusChange:(id)sender;
- (IBAction)cassStatusChange:(id)sender;
- (IBAction)cassSliderChange:(id)sender;
- (void)cassSliderUpdate:(int)block;
- (void) updateMediaStatusWindow;
- (void) statusLed:(int)diskNo:(int)on:(int)read;
- (void) sectorLed:(int)diskNo:(int)sectorNo:(int)on;
- (NSImageView *) getDiskImageView:(int)tag;
- (IBAction)coldStart:(id)sender;
- (IBAction)warmStart:(id)sender;
- (IBAction)limit:(id)sender;
- (void)setLimitButton:(int)limit;
- (IBAction)disableBasic:(id)sender;
- (IBAction)changeXEP80:(id)sender;
- (void)setDisableBasicButton:(int)mode:(int)onoff;
- (void)closeKeyWindow:(id)sender;
- (IBAction)machineTypeChange:(id)sender;
- (IBAction)scaleModeChange:(id)sender;
- (IBAction)widthModeChange:(id)sender;
- (IBAction)artifactModeChange:(id)sender;
- (IBAction)checkDisk:(id)sender;
- (void)switchDisks;
- (void)enable80ColMode:(int)machineType;
- (void)set80ColMode:(int)xep80Enabled:(int)af80Enabled:(int)bit3Enabled:(int)xep80;
- (IBAction)xep80Mode:(id)sender;
- (IBAction)side2SaveRom:(id)sender;
- (IBAction)ultimateSaveRom:(id)sender;
- (IBAction)side2ChangeRom:(id)sender;
- (IBAction)side2AttachCF:(id)sender;
- (IBAction)side2AttachCFFile:(NSString *)filename;
- (IBAction)side2RemoveCF:(id)sender;
- (IBAction)side2SlideSwitch:(id)sender;
- (IBAction)side2Button:(id)sender;

@end
