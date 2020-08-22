/* ControlManager.h - Menu suppor class to 
   the Control menu functions for the 
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   
   Based on the Preferences pane of the
   TextEdit application.

*/
#import <Cocoa/Cocoa.h>
#import "MemoryEditorDataSource.h"
#import "DisasmDataSource.h"
#import "BreakpointDataSource.h"
#import "BreakpointTableView.h"
#import "StackDataSource.h"
#import "WatchDataSource.h"
#import "LabelDataSource.h"
#import "AnticDataSource.h"
#import "GtiaDataSource.h"
#import "PiaDataSource.h"
#import "PokeyDataSource.h"

#define kHistorySize 25

@interface ControlManager : NSObject
{
    NSString *history[kHistorySize];
    int historySize;
    int historyIndex;
    int historyLine;
	NSColor *black;
	NSColor *red;
    NSColor *white;
	NSDictionary *blackDict;
	NSDictionary *redDict;
	NSAttributedString *redNString;
	NSAttributedString *blackNString;
	NSAttributedString *redVString;
	NSAttributedString *blackVString;
	NSAttributedString *redBString;
	NSAttributedString *blackBString;
	NSAttributedString *redDString;
	NSAttributedString *blackDString;
	NSAttributedString *redIString;
	NSAttributedString *blackIString;
	NSAttributedString *redZString;
	NSAttributedString *blackZString;
	NSAttributedString *redCString;
	NSAttributedString *blackCString;
	MemoryEditorDataSource *monitorMemoryDataSource;
	DisasmDataSource *monitorDisasmDataSource;
	BreakpointDataSource *breakpointDataSource;
	StackDataSource *monitorStackDataSource;
	WatchDataSource *monitorWatchDataSource;
	LabelDataSource *monitorLabelDataSource;
	PiaDataSource *monitorPiaDataSource;
	PokeyDataSource *monitorPokeyDataSource;
	AnticDataSource *monitorAnticDataSource;
	GtiaDataSource *monitorGtiaDataSource;
	NSMutableArray *foundMemoryLocations;
	int foundMemoryIndex;
	int memorySearchLength;
    IBOutlet id coldResetItem;
    IBOutlet id limitItem;
    IBOutlet id disableBasicItem;
    IBOutlet id cx85EnableItem;
    IBOutlet id keyjoyEnableItem;
    IBOutlet id loadStateItem;
    IBOutlet id pauseItem;
    IBOutlet id saveStateItem;
    IBOutlet id warmResetItem;
    IBOutlet id errorTextField;
    IBOutlet id normalErrorTextField;
    IBOutlet id dualErrorTextField1;
    IBOutlet id dualErrorTextField2;
    IBOutlet id messageOutputView;
    IBOutlet id monitorOutputView;
    IBOutlet id monitorInputField;
    IBOutlet id monitorExeButton;
    IBOutlet id monitorDoneButton;
	IBOutlet id monitorTabView;
	IBOutlet id monitorPCRegField;
	IBOutlet id monitorARegField;
	IBOutlet id monitorXRegField;
	IBOutlet id monitorYRegField;
	IBOutlet id monitorSRegField;
	IBOutlet id monitorPRegField;
	IBOutlet id monitorMemoryTableView;
	IBOutlet id monitorMemoryAddressField;
	IBOutlet id monitorMemoryFindField;
	IBOutlet id monitorMemoryStepper;
	IBOutlet id monitorBreakpointTableView;
	IBOutlet id monitorBreakpointEditorTableView;
	IBOutlet id monitorDisasmAddressField;
	IBOutlet id monitorDisasmTableView;
	IBOutlet id monitorStackTableView;
	IBOutlet id monitorWatchTableView;
	IBOutlet id monitorLabelTableView;
	IBOutlet id monitorPiaTableView;
	IBOutlet id monitorPokeyTableView;
	IBOutlet id monitorAnticTableView;
	IBOutlet id monitorGtiaTableView;
	IBOutlet id monitorBreakValueField;
	IBOutlet id monitorNFlagCheckBox;
	IBOutlet id monitorVFlagCheckBox;
	IBOutlet id monitorBFlagCheckBox;
	IBOutlet id monitorDFlagCheckBox;
	IBOutlet id monitorIFlagCheckBox;
	IBOutlet id monitorZFlagCheckBox;
	IBOutlet id monitorCFlagCheckBox;
	IBOutlet id monitorBreakpointEditorValField;
	IBOutlet id monitorBuiltinLabelCheckBox;
	IBOutlet id monitorUserLabelCountField;
	IBOutlet id monitorUserLabelField;
	IBOutlet id monitorUserValueField;
	IBOutlet id machineTypeMenu;
	IBOutlet id pbiExpansionMenu;
	IBOutlet id arrowKeysMenu;
	IBOutlet id ctrlArrowKeysItem;
	IBOutlet id arrowKeysOnlyItem;
	IBOutlet id arrowKeysF1F4Item;
	IBOutlet id startButton;
}
+ (ControlManager *)sharedInstance;
- (void)setLimitMenu:(int)limit;
- (void)setDisableBasicMenu:(int)mode:(int)disableBasic;
- (void)setKeyjoyEnableMenu:(int)keyjoyEnable;
- (void)setCX85EnableMenu:(int)cx85Enable;
- (NSString *) browseFileInDirectory:(NSString *)directory;
- (NSString *) saveFileInDirectory:(NSString *)directory:(NSString *)type;
- (IBAction)coldReset:(id)sender;
- (IBAction)limit:(id)sender;
- (IBAction)disableBasic:(id)sender;
- (IBAction)keyjoyEnable:(id)sender;
- (IBAction)cx85Enable:(id)sender;
- (IBAction)loadState:(id)sender;
- (void)loadStateFile:(NSString *)filename;
- (IBAction)pause:(id)sender;
- (IBAction)saveState:(id)sender;
- (IBAction)warmReset:(id)sender;
- (int)fatalError;
- (void)error:(NSString *)errorString;
- (void)error2:(char *)error1:(char *)error2;
- (IBAction)ejectCartridgeSelected:(id)sender;
- (IBAction)ejectDiskSelected:(id)sender;
- (IBAction)coldStartSelected:(id)sender;
- (IBAction)warmStartSelected:(id)sender;
- (IBAction)monitorSelected:(id)sender;
- (IBAction)quitSelected:(id)sender;
- (IBAction)errorOK:(id)sender;
- (IBAction)changeMachineType:(id)sender;
- (IBAction)changePBIExpansion:(id)sender;
- (IBAction)changeArrowKeys:(id)sender;
- (void)setMachineTypeMenu:(int)machineType:(int)ramSize;
- (void)setPBIExpansionMenu:(int)type;
- (void)setArrowKeysMenu:(int)keys;
- (void) releaseKey:(int)keyCode;
- (IBAction)monitorExecute:(id)sender;
- (void)messagePrint:(char *)printString;
- (void)messageWindowShow:(id)sender;
- (NSPoint)messagesOriginSave;
- (NSPoint)functionKeysOriginSave;
- (NSPoint)monitorOriginSave;
- (BOOL)monitorGUIVisableSave;
- (void)monitorPrint:(char *)string;
- (IBAction)monitorMenuRun:(id)sender;
- (int)monitorRun;
- (void)addToHistory:(NSString *)str;
- (void)historyScroll:(int)direction;
- (NSString *)getHistoryString:(int)direction;
- (void)monitorUpArrow;
- (void)monitorDownArrow;
- (void)updateMonitorGUI;
- (int)runBreakpointEditor:(id)sender;
- (IBAction)okBreakpointEditor:(id)sender;
- (IBAction)cancelBreakpointEditor:(id)sender;
- (void) reloadBreakpointEditor;
- (IBAction) addBreakpointCondition:(id)sender;
- (void)updateMonitorGUIRegisters;
- (void)updateMonitorGUIBreakpoints;
- (void)updateMonitorGUIStack;
- (void)updateMonitorStackTableData;
- (void)updateMonitorGUIWatch;
- (void)updateMonitorWatchTableData;
- (void)updateMonitorBreakpointTableData;
- (void)updateMonitorGUIMemory;
- (void)updateMonitorGUIDisasm:(BOOL) usePc: (unsigned short) altAddress;
- (void)updateMonitorGUILabels;
- (void)updateMonitorGUIHardware;
- (IBAction) monitorToggleTableBreakpoint:(id)sender;
- (IBAction) monitorAddByteWatch:(id)sender;
- (IBAction) monitorAddWordWatch:(id)sender;
- (void) runButtonAction:(char *)action;
- (IBAction) monitorGoButton:(id)sender;
- (IBAction) monitorStepButton:(id)sender;
- (IBAction) monitorStepOverButton:(id)sender;
- (IBAction) monitorStepOutButton:(id)sender;
- (IBAction) monitorQuitButton:(id)sender;
- (IBAction) monitorCPURegChanged:(id)sender;
- (IBAction) monitorCPUFlagChanged:(id)sender;
- (IBAction) monitorMemoryAddressChanged:(id)sender;
- (void) monitorMemoryAddress:(unsigned short)addr;
- (IBAction) monitorDisasmAddressChanged:(id)sender;
- (IBAction) monitorMemoryStepperChanged:(id)sender;
- (IBAction) monitorMemoryFindFirst:(id)sender;
- (IBAction) monitorMemoryFindNext:(id)sender;
- (void) showMemoryAtFoundIndex;
- (IBAction) monitorDisasmRunToHere:(id)sender;
- (IBAction) monitorDoneBreakValue:(id)sender;
- (unsigned short) monitorGetBreakValue;
- (IBAction) monitorToggleBuiltinLabels:(id)sender;
- (IBAction) monitorLoadLabelFile:(id)sender;
- (IBAction) monitorDeleteUserLabels:(id)sender;
- (IBAction) monitorAddUserlabel:(id)sender;
- (void) monitorSetLabelsDirty;
- (IBAction)showAboutBox:(id)sender;
- (IBAction)showDonation:(id)sender;
- (IBAction)breakPressed:(id)sender;
- (IBAction)startPressed:(id)sender;
- (IBAction)optionPressed:(id)sender;
- (IBAction)selectPressed:(id)sender;
- (IBAction)inversePressed:(id)sender;
- (IBAction)clearPressed:(id)sender;
- (IBAction)helpPressed:(id)sender;
- (IBAction)insertCharPressed:(id)sender;
- (IBAction)insertLinePressed:(id)sender;
- (IBAction)deleteCharPressed:(id)sender;
- (IBAction)deleteLinePressed:(id)sender;
- (IBAction)f1Pressed:(id)sender;
- (IBAction)f2Pressed:(id)sender;
- (IBAction)f3Pressed:(id)sender;
- (IBAction)f4Pressed:(id)sender;
- (IBAction)functionKeyWindowShow:(id)sender;
- (NSString *) hexStringFromShort:(unsigned short)a;
- (NSMutableAttributedString *) colorFromShort:(unsigned short)a:(unsigned short)old_a;
- (NSString *) hexStringFromByte:(unsigned char)b;
- (NSAttributedString *) colorFromByte:(unsigned char)b:(unsigned char)old_b;
- (void) setRegFlag:(id)outlet:(unsigned char)old_reg:(unsigned char)mask;
@end
