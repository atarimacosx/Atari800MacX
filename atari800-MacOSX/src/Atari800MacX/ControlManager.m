/* ControlManager.m - Menu suppor class to 
   the Control menu functions for the 
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimac@kc.rr.com>
   
   Based on the Preferences pane of the
   TextEdit application.

*/
#import "ControlManager.h"
#import "Preferences.h"
#import "MediaManager.h"
#import "AboutBox.h"
#import "KeyMapper.h"
#import "BreakpointsController.h"
#import "BreakpointEditorDataSource.h"
#import "DisasmTableView.h"
#import "MemoryEditorTableView.h"
#import "memory.h"
#import "monitor.h"
#import "SDL.h"
#import "cpu.h"
#import "ui.h"
#import <stdarg.h>


/* Definition of Mac native keycodes for characters used as menu shortcuts the bring up a windo. */
#define QZ_F8			0x64

#define FUNCTION_KEY_PRESS_DURATION 5 /* Duration of holding the function key down in machine cycles */

extern void PauseAudio(int pause);
extern void MONITOR_monitorEnter(void);
extern int MONITOR_monitorCmd(char *input);
extern int CalcAtariType(int machineType, int ramSize, int axlon, int mosaic);


extern int pauseEmulator;
extern int requestPauseEmulator;
extern int requestColdReset;
extern int requestWarmReset;
extern int requestSaveState;
extern int requestLoadState;
extern int requestLimitChange;
extern int requestDisableBasicChange;
extern int requestKeyjoyEnableChange;
extern int requestCX85EnableChange;
extern int requestMachineTypeChange;
extern int requestPBIExpansionChange;
extern int requestMonitor;
extern int requestQuit;
extern int speed_limit;
extern char saveFilename[FILENAME_MAX];
extern char loadFilename[FILENAME_MAX];
extern char atari_state_dir[FILENAME_MAX];
extern char atari_exe_dir[FILENAME_MAX];
extern int MONITOR_assemblerMode;
extern int functionKeysWindowOpen;
extern int useAtariCursorKeys;
/* Function key window interface variables */
extern int breakFunctionPressed;
extern int startFunctionPressed;
extern int selectFunctionPressed;
extern int optionFunctionPressed;
extern int FULLSCREEN;
extern int MONITOR_break_run_to_here;
extern int UI_alt_function;
extern int requestFullScreenUI;

/* Functions which provide an interface for C code to call this object's shared Instance functions */
void SetControlManagerLimit(int limit) {
    [[ControlManager sharedInstance] setLimitMenu:(limit)];
    }
    
void SetControlManagerDisableBasic(int disableBasic) {
    [[ControlManager sharedInstance] setDisableBasicMenu:(disableBasic)];
    }
    
void SetControlManagerCX85Enable(int cx85Enable) {
    [[ControlManager sharedInstance] setCX85EnableMenu:(cx85Enable)];
}

void SetControlManagerKeyjoyEnable(int keyjoyEnable) {
    [[ControlManager sharedInstance] setKeyjoyEnableMenu:(keyjoyEnable)];
}

void SetControlManagerMachineType(int machineType, int ramSize) {
    [[ControlManager sharedInstance] setMachineTypeMenu:(machineType):(ramSize)];
}

void SetControlManagerPBIExpansion(int type) {
    [[ControlManager sharedInstance] setPBIExpansionMenu:(type)];
}

void SetControlManagerArrowKeys(int keys) {
    [[ControlManager sharedInstance] setArrowKeysMenu:(keys)];
}

int ControlManagerFatalError() {
    return([[ControlManager sharedInstance] fatalError]);
}

void ControlManagerDualError(char *error1, char *error2) {
	[[ControlManager sharedInstance] error2:error1:error2];
}

void ControlManagerSaveState() {
    [[ControlManager sharedInstance] saveState:nil];
}

void ControlManagerLoadState() {
    [[ControlManager sharedInstance] loadState:nil];
}

void ControlManagerPauseEmulator() {
    [[ControlManager sharedInstance] pause:nil];
}

void ControlManagerHideApp() {
    [NSApp hide:[ControlManager sharedInstance]];
    [[KeyMapper sharedInstance] releaseCmdKeys:@"h"];
}

void ControlManagerAboutApp() {
    [NSApp orderFrontStandardAboutPanel:[ControlManager sharedInstance]];
    [[KeyMapper sharedInstance] releaseCmdKeys:@"a"];
}

void ControlManagerShowHelp() {
    [NSApp showHelp:[ControlManager sharedInstance]];
    [[KeyMapper sharedInstance] releaseCmdKeys:@"?"];
}

void ControlManagerMiniturize() {
    [[NSApp keyWindow] performMiniaturize:[ControlManager sharedInstance]];
    [[KeyMapper sharedInstance] releaseCmdKeys:@"m"];
}

void ControlManagerMessagePrint(char *string)
{
[[ControlManager sharedInstance] messagePrint:string];
}

void ControlManagerMonitorPrintf(const char *format,...)
{
	static char string[512];
	va_list arguments;             
    va_start(arguments, format);  
    
	vsprintf(string, format, arguments);
    [[ControlManager sharedInstance] monitorPrint:string];
}

void ControlManagerMonitorPuts(const char *string)
{
    [[ControlManager sharedInstance] monitorPrint:(char *)string];
}

int ControlManagerMonitorRun()
{
    return([[ControlManager sharedInstance] monitorRun]);
}

void ControlManagerFunctionKeysWindowShow(void) 
{
	[[ControlManager sharedInstance] functionKeyWindowShow:nil];
}

char *ControlManagerGetHistoryString(int direction)
{
	NSString *historyString;
	static char buffer[256];
	
	historyString = [[ControlManager sharedInstance] getHistoryString:direction];
	if (historyString == nil)
		return(NULL);
	else {
		[historyString getCString:buffer maxLength:255 encoding:NSASCIIStringEncoding];
		return(buffer);
		}
}

void ControlManagerAddToHistory(char *string)
{
	NSString *historyString;
	
	historyString = [NSString stringWithCString:string encoding:NSASCIIStringEncoding];
	[[ControlManager sharedInstance] addToHistory:historyString];
	[historyString release];
}

void ControlManagerMonitorSetLabelsDirty(void)
{
	[[ControlManager sharedInstance] monitorSetLabelsDirty];
}

@implementation ControlManager
static ControlManager *sharedInstance = nil;
#define MAX_MONITOR_OUTPUT 16384
static char monitorOutput[MAX_MONITOR_OUTPUT];
static int monitorCharCount = 0;
static unsigned char old_regA, old_regX, old_regY, old_regS, old_regP;
static unsigned short old_regPC;
static BreakpointEditorDataSource *breakpointEditorDataSource;
static int monitorRunFirstTime = 1;

+ (ControlManager *)sharedInstance {
    return sharedInstance ? sharedInstance : [[self alloc] init];
}

- (id)init {
    NSDictionary *attribs;
	NSArray *cols;
	int i;

    if (sharedInstance) {
	[self dealloc];
    } else {
        [super init];
        sharedInstance = self;
        if (!errorTextField) {
				if (![NSBundle loadNibNamed:@"ControlManager" owner:self])  {
					NSLog(@"Failed to load ControlManager.nib");
					NSBeep();
					return nil;
			}
            }
	[[errorTextField window] setExcludedFromWindowsMenu:YES];
	[[errorTextField window] setMenu:nil];
	[[monitorInputField window] setExcludedFromWindowsMenu:YES];
	[[monitorInputField window] setMenu:nil];
	[[messageOutputView window] setExcludedFromWindowsMenu:NO];

	attribs = [[NSDictionary alloc] initWithObjectsAndKeys:
                   [NSFont fontWithName:@"Monaco" size:10.0], NSFontAttributeName,
                   [NSColor labelColor], NSForegroundColorAttributeName,
                   nil]; 
    [monitorOutputView setTypingAttributes:attribs];
    [messageOutputView setTypingAttributes:attribs];
    [monitorOutputView setString:@"Atari800MacX Monitor\nType '?' for help, 'CONT' to exit\n"];
    [attribs release];
	
	// Init Monitor Command History
    historyIndex = 0;
    historyLine = 0;
    historySize = 0;
    for (i = 0; i < kHistorySize; i++)
       history[i] = nil;
    }
	
	// init colors
	black = [NSColor labelColor];
	red = [NSColor redColor];
	blackDict =[NSDictionary dictionaryWithObjectsAndKeys:
				black, NSForegroundColorAttributeName,
				[NSFont labelFontOfSize:[NSFont smallSystemFontSize]],NSFontAttributeName,
				nil];
	redDict =[NSDictionary dictionaryWithObjectsAndKeys:
			    [NSFont labelFontOfSize:[NSFont smallSystemFontSize]],NSFontAttributeName,
				red, NSForegroundColorAttributeName,
				nil];
	blackNString = [[NSAttributedString alloc] initWithString:@"N" attributes:blackDict];
	redNString = [[NSAttributedString alloc] initWithString:@"N" attributes:redDict];
	blackVString = [[NSAttributedString alloc] initWithString:@"V" attributes:blackDict];
	redVString = [[NSAttributedString alloc] initWithString:@"V" attributes:redDict];
	blackBString = [[NSAttributedString alloc] initWithString:@"B" attributes:blackDict];
	redBString = [[NSAttributedString alloc] initWithString:@"B" attributes:redDict];
	blackDString = [[NSAttributedString alloc] initWithString:@"D" attributes:blackDict];
	redDString = [[NSAttributedString alloc] initWithString:@"D" attributes:redDict];
	blackIString = [[NSAttributedString alloc] initWithString:@"I" attributes:blackDict];
	redIString = [[NSAttributedString alloc] initWithString:@"I" attributes:redDict];
	blackZString = [[NSAttributedString alloc] initWithString:@"Z" attributes:blackDict];
	redZString = [[NSAttributedString alloc] initWithString:@"Z" attributes:redDict];
	blackCString = [[NSAttributedString alloc] initWithString:@"C" attributes:blackDict];
	redCString = [[NSAttributedString alloc] initWithString:@"C" attributes:redDict];
	
	// Create and hook us up to our data sources
	monitorMemoryDataSource = [[MemoryEditorDataSource alloc] init];
	[monitorMemoryDataSource setOwner:self];
	[monitorMemoryTableView setDataSource: (id <NSComboBoxDataSource>)monitorMemoryDataSource];
	cols = [monitorMemoryTableView tableColumns];
	for (i=0;i<[cols count];i++) {
		[[[cols objectAtIndex:i] dataCell] setFont:[NSFont fontWithName:@"Monaco" size:10.0]];
	}
	
	monitorDisasmDataSource = [[DisasmDataSource alloc] init];
	[monitorDisasmDataSource setOwner:self];
	[monitorDisasmTableView setDataSource:(id <NSComboBoxDataSource>)monitorDisasmDataSource];
	cols = [monitorDisasmTableView tableColumns];
	[[[cols objectAtIndex:1] dataCell] setFont:[NSFont fontWithName:@"Monaco" size:10.0]];
	[[BreakpointsController sharedInstance] setDisasmDataSource:monitorDisasmDataSource];
	
	breakpointDataSource = [[BreakpointDataSource alloc] init];
	[breakpointDataSource setOwner:self];
	[monitorBreakpointTableView setDataSource:(id <NSComboBoxDataSource>)breakpointDataSource];
	[monitorBreakpointTableView setTarget:monitorBreakpointTableView];
	[monitorBreakpointTableView setDoubleAction: @selector(editBreakpoint:)];
	[[BreakpointsController sharedInstance] setBreakpointDataSource:breakpointDataSource];
	
	breakpointEditorDataSource = [[BreakpointEditorDataSource alloc] init];
	[breakpointEditorDataSource setOwner:self];
	[monitorBreakpointEditorTableView setDataSource:(id <NSComboBoxDataSource>)breakpointEditorDataSource];
	[[BreakpointsController sharedInstance] setBreakpointEditorDataSource:breakpointEditorDataSource];
	
	monitorStackDataSource = [[StackDataSource alloc] init];
	[monitorStackDataSource setOwner:self];
	[monitorStackTableView setDataSource:(id <NSComboBoxDataSource>)monitorStackDataSource];
	cols = [monitorStackTableView tableColumns];
	for (i=0;i<[cols count];i++) {
		[[[cols objectAtIndex:i] dataCell] setFont:[NSFont fontWithName:@"Monaco" size:10.0]];
	}
	
	monitorLabelDataSource = [[LabelDataSource alloc] init];
	[monitorLabelDataSource setOwner:self];
	[monitorLabelTableView setDataSource:(id <NSComboBoxDataSource>)monitorLabelDataSource];
	[monitorLabelTableView setDelegate:monitorLabelDataSource];
	cols = [monitorLabelTableView tableColumns];
	for (i=0;i<[cols count];i++) {
		[[[cols objectAtIndex:i] dataCell] setFont:[NSFont fontWithName:@"Monaco" size:10.0]];
	}
	
	monitorPiaDataSource = [[PiaDataSource alloc] init];
	[monitorPiaDataSource setOwner:self];
	[monitorPiaTableView setDataSource:(id <NSComboBoxDataSource>)monitorPiaDataSource];
	cols = [monitorPiaTableView tableColumns];
	for (i=0;i<[cols count];i++) {
		[[[cols objectAtIndex:i] dataCell] setFont:[NSFont fontWithName:@"Monaco" size:10.0]];
	}
	
	monitorPokeyDataSource = [[PokeyDataSource alloc] init];
	[monitorPokeyDataSource setOwner:self];
	[monitorPokeyTableView setDataSource:(id <NSComboBoxDataSource>)monitorPokeyDataSource];
	cols = [monitorPokeyTableView tableColumns];
	for (i=0;i<[cols count];i++) {
		[[[cols objectAtIndex:i] dataCell] setFont:[NSFont fontWithName:@"Monaco" size:10.0]];
	}
	
	monitorAnticDataSource = [[AnticDataSource alloc] init];
	[monitorAnticDataSource setOwner:self];
	[monitorAnticTableView setDataSource:(id <NSComboBoxDataSource>)monitorAnticDataSource];
	cols = [monitorAnticTableView tableColumns];
	for (i=0;i<[cols count];i++) {
		[[[cols objectAtIndex:i] dataCell] setFont:[NSFont fontWithName:@"Monaco" size:10.0]];
	}
	
	monitorGtiaDataSource = [[GtiaDataSource alloc] init];
	[monitorGtiaDataSource setOwner:self];
	[monitorGtiaTableView setDataSource:(id <NSComboBoxDataSource>)monitorGtiaDataSource];
	cols = [monitorGtiaTableView tableColumns];
	for (i=0;i<[cols count];i++) {
		[[[cols objectAtIndex:i] dataCell] setFont:[NSFont fontWithName:@"Monaco" size:10.0]];
	}
	
	monitorWatchDataSource = [[WatchDataSource alloc] init];
	[monitorWatchDataSource setOwner:self];
	[monitorWatchTableView setDataSource:(id <NSComboBoxDataSource>)monitorWatchDataSource];
	cols = [monitorWatchTableView tableColumns];
	for (i=0;i<[cols count];i++) {
		[[[cols objectAtIndex:i] dataCell] setFont:[NSFont fontWithName:@"Monaco" size:10.0]];
	}

	foundMemoryLocations = nil;
	memorySearchLength = 0;

    return sharedInstance;
}

/*------------------------------------------------------------------------------
*  setLimitMenu - This method is used to set the menu check state for the 
*     limit emulator speed menu item.
*-----------------------------------------------------------------------------*/
- (void)setLimitMenu:(int)limit
{
    if (limit)
        [limitItem setState:NSOnState];
    else
        [limitItem setState:NSOffState];
		
	[[MediaManager sharedInstance] setLimitButton:limit];
}

/*------------------------------------------------------------------------------
*  setDisableBasicMenu - This method is used to set the menu check state for the 
*     Disable Basic menu item.
*-----------------------------------------------------------------------------*/
- (void)setDisableBasicMenu:(int)disableBasic
{
    if (disableBasic)
        [disableBasicItem setState:NSOnState];
    else
        [disableBasicItem setState:NSOffState];
		
	[[MediaManager sharedInstance] setDisableBasicButton:disableBasic];
}

/*------------------------------------------------------------------------------
 *  setCX85EnableMenu - This method is used to set the menu check state for the 
 *     Enable XC85 menu item.
 *-----------------------------------------------------------------------------*/
- (void)setCX85EnableMenu:(int)cx85Enable
{
    if (cx85Enable)
        [cx85EnableItem setState:NSOnState];
    else
        [cx85EnableItem setState:NSOffState];
}

/*------------------------------------------------------------------------------
 *  setKeyjoyEnableMenu - This method is used to set the menu check state for the 
 *     Enable Keyboard Joystcks menu item.
 *-----------------------------------------------------------------------------*/
- (void)setKeyjoyEnableMenu:(int)keyjoyEnable
{
    if (keyjoyEnable)
        [keyjoyEnableItem setState:NSOnState];
    else
        [keyjoyEnableItem setState:NSOffState];
}

/*------------------------------------------------------------------------------
 *  setMachineTypeMenu - This method is used to set the menu check state for the 
 *     Machine Type menu items.
 *-----------------------------------------------------------------------------*/
- (void)setMachineTypeMenu:(int)machineType:(int)ramSize
{
	int i, type, index, ver4type;
	type = CalcAtariType(machineType, ramSize,
						 MEMORY_axlon_enabled, MEMORY_mosaic_enabled);
	
	if (type > 13) {
		ver4type = type - 14;
		type = 0;
	} else {
		ver4type = -1;
	}
	index = [[Preferences sharedInstance] indexFromType:type :ver4type];
	
	for (i=0;i<19;i++) {
		if (i==index)
			[[machineTypeMenu itemAtIndex:i] setState:NSOnState];
		else
			[[machineTypeMenu itemAtIndex:i] setState:NSOffState];
	}
}

/*------------------------------------------------------------------------------
 *  setPBIExpansionMenu - This method is used to set the menu check state for the 
 *     PBI Expansion menu items.
 *-----------------------------------------------------------------------------*/
- (void)setPBIExpansionMenu:(int)type
{
	int i;
	
	for (i=0;i<3;i++) {
		if (i==type)
			[[pbiExpansionMenu itemAtIndex:i] setState:NSOnState];
		else
			[[pbiExpansionMenu itemAtIndex:i] setState:NSOffState];
	}
}

/*------------------------------------------------------------------------------
 *  setArrowKeysMenu - This method is used to set the menu check state for the 
 *     Arrow keys menu items.
 *-----------------------------------------------------------------------------*/
- (void)setArrowKeysMenu:(int)keys;
{
	[ctrlArrowKeysItem setState:NSOffState];
	[arrowKeysOnlyItem setState:NSOffState];
	[arrowKeysF1F4Item setState:NSOffState];
	
	switch(keys) {
		case 0:
		default:
			[ctrlArrowKeysItem setState:NSOnState];
			break;
		case 1:
			[arrowKeysOnlyItem setState:NSOnState];
			break;
		case 2:
			[arrowKeysF1F4Item setState:NSOnState];
			break;
	}
}

- (IBAction)changeMachineType:(id)sender 
{
	requestMachineTypeChange = [sender tag] + 1;
}

- (IBAction)changePBIExpansion:(id)sender 
{
	requestPBIExpansionChange = [sender tag] + 1;
}

- (IBAction)changeArrowKeys:(id)sender 
{
	useAtariCursorKeys = [sender tag];
	[self setArrowKeysMenu:useAtariCursorKeys];
}

/*------------------------------------------------------------------------------
*  browseFileInDirectory - This allows the user to chose a file to read in from
*     the specified directory.
*-----------------------------------------------------------------------------*/
- (NSString *) browseFileInDirectory:(NSString *)directory {
    NSOpenPanel *openPanel = nil;
    
    openPanel = [NSOpenPanel openPanel];
    [openPanel setCanChooseDirectories:NO];
    [openPanel setCanChooseFiles:YES];
    
    if ([openPanel runModalForDirectory:directory file:nil types:nil] == NSOKButton)
        return([[openPanel filenames] objectAtIndex:0]);
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
    
    [savePanel setRequiredFileType:type];
    
    if ([savePanel runModalForDirectory:directory file:nil] == NSOKButton)
        return([savePanel filename]);
    else
        return nil;
    }

/*------------------------------------------------------------------------------
*  coldReset - This method handles the cold reset menu selection.
*-----------------------------------------------------------------------------*/
- (IBAction)coldReset:(id)sender
{
    requestColdReset = 1;
}

/*------------------------------------------------------------------------------
*  limit - This method handles the limit emulator speed menu selection.
*-----------------------------------------------------------------------------*/
- (IBAction)limit:(id)sender
{
    requestLimitChange = 1;
}

/*------------------------------------------------------------------------------
*  disableBasic - This method handles the disable basic menu selection.
*-----------------------------------------------------------------------------*/
- (IBAction)disableBasic:(id)sender
{
    requestDisableBasicChange = 1;
}

/*------------------------------------------------------------------------------
 *  keyjoyEnable - This method handles the Enable Keyboard Joysticks menu selection.
 *-----------------------------------------------------------------------------*/
- (IBAction)keyjoyEnable:(id)sender
{
    requestKeyjoyEnableChange = 1;
}

/*------------------------------------------------------------------------------
 *  cx85Enable - This method handles the Enable CX85 menu selection.
 *-----------------------------------------------------------------------------*/
- (IBAction)cx85Enable:(id)sender
{
    requestCX85EnableChange = 1;
}

/*------------------------------------------------------------------------------
*  loadState - This method handles the load state file menu selection.
*-----------------------------------------------------------------------------*/
- (IBAction)loadState:(id)sender
{
    NSString *filename;
    
    if (FULLSCREEN) {
        UI_alt_function = UI_MENU_LOADSTATE;
        requestFullScreenUI = 1;
        return;
    }
    
    PauseAudio(1);
    filename = [self browseFileInDirectory:[NSString stringWithCString:atari_state_dir encoding:NSASCIIStringEncoding]];
    if (filename != nil) {
        [filename getCString:loadFilename maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
        requestLoadState = 1;
    }
	[[MediaManager sharedInstance] updateInfo];
    [[KeyMapper sharedInstance] releaseCmdKeys:@"l"];
    PauseAudio(0);
}

/*------------------------------------------------------------------------------
*  loadStateFile - This method handles the load state file menu selection.
*-----------------------------------------------------------------------------*/
- (void)loadStateFile:(NSString *)filename
{
    if (filename != nil) {
        [filename getCString:loadFilename maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
        requestLoadState = 1;
    }
}

/*------------------------------------------------------------------------------
*  pause - This method handles the pause emulator menu selection.
*-----------------------------------------------------------------------------*/
- (IBAction)pause:(id)sender
{
    if (1 - pauseEmulator)
        [pauseItem setState:NSOnState];
    else
        [pauseItem setState:NSOffState];
    requestPauseEmulator = 1;
}

/*------------------------------------------------------------------------------
*  saveState - This method handles the save state file menu selection.
*-----------------------------------------------------------------------------*/
- (IBAction)saveState:(id)sender
{
    NSString *filename;
    
    if (FULLSCREEN) {
        UI_alt_function = UI_MENU_SAVESTATE;
        requestFullScreenUI = 1;
        return;
    }
    
    PauseAudio(1);
    filename = [self saveFileInDirectory:[NSString stringWithCString:atari_state_dir encoding:NSASCIIStringEncoding]:@"a8s"];
    if (filename != nil) {
        [filename getCString:saveFilename maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
        requestSaveState = 1;
    }
    [[KeyMapper sharedInstance] releaseCmdKeys:@"s"];
    PauseAudio(0);
}

/*------------------------------------------------------------------------------
*  warmReset - This method handles the warm reset menu selection.
*-----------------------------------------------------------------------------*/
- (IBAction)warmReset:(id)sender
{
    requestWarmReset = 1;
}

/*------------------------------------------------------------------------------
 *  fatalError - This method displays the fatal error dialogue box.
 *-----------------------------------------------------------------------------*/
- (int)fatalError
{
    return([NSApp runModalForWindow:[errorTextField window]]);
}

/*------------------------------------------------------------------------------
 *  error - This method displays the non fatal error dialogue box.
 *-----------------------------------------------------------------------------*/
- (void)error:(NSString *)errorString
{
	[normalErrorTextField setStringValue:errorString];
    [NSApp runModalForWindow:[normalErrorTextField window]];
}

/*------------------------------------------------------------------------------
 *  error2 - This method displays the non fatal error dialogue box that has
 *           two lines of error code.
 *-----------------------------------------------------------------------------*/
- (void)error2:(char *)error1:(char *)error2;
{
	[dualErrorTextField1 setStringValue:[NSString stringWithCString:error1 encoding:NSASCIIStringEncoding]];
	[dualErrorTextField2 setStringValue:[NSString stringWithCString:error2 encoding:NSASCIIStringEncoding]];
    [NSApp runModalForWindow:[dualErrorTextField1 window]];
}

/*------------------------------------------------------------------------------
*  coldStartSelected - This method handles the warm start button from the fatal 
*     error dialogue.
*-----------------------------------------------------------------------------*/
- (IBAction)coldStartSelected:(id)sender
{
    [NSApp stopModalWithCode:2];
    [[sender window] close];
}

/*------------------------------------------------------------------------------
*  warmStartSelected - This method handles the warm start button from the fatal 
*     error dialogue.
*-----------------------------------------------------------------------------*/
- (IBAction)warmStartSelected:(id)sender
{
    [NSApp stopModalWithCode:1];
    [[sender window] close];
}

/*------------------------------------------------------------------------------
*  monitorSelected - This method handles the monitor button from the fatal 
*     error dialogue.
*-----------------------------------------------------------------------------*/
- (IBAction)monitorSelected:(id)sender
{
    [NSApp stopModalWithCode:3];
    [[sender window] close];
}

/*------------------------------------------------------------------------------
*  ejectCartridgeSelected - This method handles the eject cartridge button from 
*     the fatal error dialogue.
*-----------------------------------------------------------------------------*/
- (IBAction)ejectCartridgeSelected:(id)sender
{
    [NSApp stopModalWithCode:4];
    [[sender window] close];
}

/*------------------------------------------------------------------------------
*  ejectDiskSelected - This method handles the eject disk 1 button from 
*     the fatal error dialogue.
*-----------------------------------------------------------------------------*/
- (IBAction)ejectDiskSelected:(id)sender
{
    [NSApp stopModalWithCode:5];
    [[sender window] close];
}

/*------------------------------------------------------------------------------
 *  quitSelected - This method handles the quit button from the fatal error
 *     dialogue.
 *-----------------------------------------------------------------------------*/
- (IBAction)quitSelected:(id)sender
{
    [NSApp stopModalWithCode:0];
    [[sender window] close];
}

/*------------------------------------------------------------------------------
 *  errorOK - This method handles the OK button from the non-fatal error
 *     dialogue.
 *-----------------------------------------------------------------------------*/
- (IBAction)errorOK:(id)sender
{
    [NSApp stopModal];
    [[sender window] close];
}

/*------------------------------------------------------------------------------
*  releaseKey - This method fixes an issue when modal windows are used with
*     the Mac OSX version of the SDL library.
*     As the SDL normally captures all keystrokes, but we need to type in some 
*     Mac windows, all of the control menu windows run in modal mode.  However, 
*     when this happens, the release of function key which started the process
*     is not sent to SDL.  We have to manually cause these events to happen 
*     to keep the SDL library in a sane state, otherwise only everyother shortcut
*     keypress will work.
*-----------------------------------------------------------------------------*/
- (void) releaseKey:(int)keyCode
{
    NSEvent *event1;
    NSPoint point;
    
    event1 = [NSEvent keyEventWithType:NSKeyUp location:point modifierFlags:0
                    timestamp:0.0 windowNumber:0 context:nil characters:@" "
                    charactersIgnoringModifiers:@" " isARepeat:NO keyCode:keyCode];
    [NSApp postEvent:event1 atStart:NO];
}

/*------------------------------------------------------------------------------
*  monitorExecute - This method handles the execute button (or return key) from
*     the monitor window.
*-----------------------------------------------------------------------------*/
- (IBAction)monitorExecute:(id)sender
{
    char input[256];
    int retValue = 0;
	NSString *line;
   
    line = [monitorInputField stringValue];
	[self addToHistory:line];
    [line getCString:input maxLength:255 encoding:NSASCIIStringEncoding];
    
    monitorCharCount = 0;
    [self monitorPrint:input];
    [self monitorPrint:"\n"];
    retValue = MONITOR_monitorCmd(input); 
    if (!MONITOR_assemblerMode && retValue>-1) {
        [self monitorPrint:"> "];
		[self updateMonitorGUI];
	}

    [NSApp stopModalWithCode:retValue];
}

/*------------------------------------------------------------------------------
*  messageWindowShow - This method makes the emulator message window visable
*-----------------------------------------------------------------------------*/
- (void)messageWindowShow:(id)sender
{
	static int firstTime = 1;

	if (firstTime) {
		[[messageOutputView window] setFrameOrigin:[[Preferences sharedInstance] messagesOrigin]];
		firstTime = 0;
		}
	
    [[messageOutputView window] makeKeyAndOrderFront:self];
	[[messageOutputView window] setTitle:@"Emulator Messages"];
	
}

/*------------------------------------------------------------------------------
*  functionKeyWindowShow - This method makes the function keys window visable
*-----------------------------------------------------------------------------*/
- (IBAction)functionKeyWindowShow:(id)sender
{
	static int firstTime = 1;

	if (firstTime && !FULLSCREEN) {
		[[startButton window] setFrameOrigin:[[Preferences sharedInstance] functionKeysOrigin]];
		firstTime = 0;
		}
		
	functionKeysWindowOpen = TRUE;	
    [[startButton window] makeKeyAndOrderFront:self];
}

/*------------------------------------------------------------------------------
*  messagesOriginSave - This method saves the position of the messages
*    window
*-----------------------------------------------------------------------------*/
- (NSPoint)messagesOriginSave
{
	NSRect frame;
	
	frame = [[messageOutputView window] frame];
	return(frame.origin);
}

/*------------------------------------------------------------------------------
*  funcitonKeysOriginSave - This method saves the position of the function keys
*    window
*-----------------------------------------------------------------------------*/
- (NSPoint)functionKeysOriginSave;
{
	NSRect frame;
	
	frame = [[startButton window] frame];
	return(frame.origin);
}

/*------------------------------------------------------------------------------
 *  monitorOriginSave - This method saves the position of the monitor
 *    window
 *-----------------------------------------------------------------------------*/
- (NSPoint)monitorOriginSave
{
	NSRect frame;
	
	frame = [[monitorOutputView window] frame];
	return(frame.origin);
}

/*------------------------------------------------------------------------------
 *  monitorGUIVisableSave - This method saves if the monitor GUI is visable
 *-----------------------------------------------------------------------------*/
- (BOOL)monitorGUIVisableSave
{
	if (monitorRunFirstTime) {
		return [[Preferences sharedInstance] monitorGUIVisable];
	}
	
	if ([monitorDrawer state] == NSDrawerClosedState) 
		return NO;
	else
		return YES;
}

/*------------------------------------------------------------------------------
*  messagePrint - This method handles the printing of information to the
*     message window.  It replaces printf.
*-----------------------------------------------------------------------------*/
- (void)messagePrint:(char *)printString
{
    NSRange theEnd;
    NSString *stringObj;

    theEnd=NSMakeRange([[messageOutputView string] length],0);
    stringObj = [[NSString alloc] initWithCString:printString encoding:NSASCIIStringEncoding];
    [messageOutputView replaceCharactersInRange:theEnd withString:stringObj]; // append new string to the end
    theEnd.location += strlen(printString); // the end has moved
	[stringObj autorelease];
    [messageOutputView scrollRangeToVisible:theEnd];
}

/*------------------------------------------------------------------------------
*  monitorPrint - This method handles the printing of information from the
*     monitor routines.  It replaces printf.
*-----------------------------------------------------------------------------*/
- (void)monitorPrint:(char *)string
{
    strncpy(&monitorOutput[monitorCharCount], string, MAX_MONITOR_OUTPUT - monitorCharCount);
    monitorCharCount += strlen(string);
}

/*------------------------------------------------------------------------------
*  monitorMenuRun - This method runs the monitor from the menu, and calls the
*     normal key based monitor run.
*-----------------------------------------------------------------------------*/
- (IBAction)monitorMenuRun:(id)sender
{
    requestMonitor = 1;
}

/*------------------------------------------------------------------------------
*  monitorRun - This method runs the monitor, and contains the main loop, one
*     loop for each command entered.
*-----------------------------------------------------------------------------*/
-(int)monitorRun
{
    int retValue = 0;
    NSRange theEnd;
    NSString *stringObj;

	if (monitorRunFirstTime) {
		[[monitorOutputView window] setFrameOrigin:[[Preferences sharedInstance] monitorOrigin]];
		if ([[Preferences sharedInstance] monitorGUIVisable])
			[self monitorDrawerToggle:self];
		monitorRunFirstTime = 0;
		}

    PauseAudio(1);
            
    MONITOR_monitorEnter();
    [self monitorPrint:"> "];
	[self updateMonitorGUI];
    theEnd=NSMakeRange([[monitorOutputView string] length],0);
    stringObj = [[NSString alloc] initWithCString:monitorOutput encoding:NSASCIIStringEncoding];
    [monitorOutputView replaceCharactersInRange:theEnd withString:stringObj]; // append new string to the end
    theEnd.location += monitorCharCount; // the end has moved
    [monitorOutputView scrollRangeToVisible:theEnd];
    [[monitorInputField window] orderFront:self];
    
    while (retValue == 0) {
        retValue = [NSApp runModalForWindow:[monitorInputField window]];
        theEnd=NSMakeRange([[monitorOutputView string] length],0);
		[stringObj release];
        stringObj = [[NSString alloc] initWithCString:monitorOutput encoding:NSASCIIStringEncoding];
        [monitorOutputView replaceCharactersInRange:theEnd withString:stringObj]; // append new string to the end
        theEnd.location += monitorCharCount; // the end has moved
        [monitorOutputView scrollRangeToVisible:theEnd];
		[monitorInputField setStringValue:@""];
		[[monitorInputField window] makeFirstResponder:monitorInputField];
		}
    
    monitorCharCount = 0;
	if (retValue >= -1)
//		[[monitorInputField window] orderOut:self];
		[[monitorInputField window] close];
    [self releaseKey:QZ_F8];

    PauseAudio(0);
    
    if (retValue < 0)
        return(1);
    else
        return(0);
}

/*------------------------------------------------------------------------------
*  monitorUpArrow - Handle the user pressing the up arrow in the monitor window
*-----------------------------------------------------------------------------*/
- (void)monitorUpArrow
{
	[self historyScroll:1];
}

/*------------------------------------------------------------------------------
*  monitorDownArrow - Handle the user pressing the down arrow in the monitor 
*      window
*-----------------------------------------------------------------------------*/
- (void)monitorDownArrow
{
	[self historyScroll:-1];
}

/*------------------------------------------------------------------------------
*  addToHistory - Add the current command to the history list
*-----------------------------------------------------------------------------*/
- (void)addToHistory:(NSString *)str
{
	[str retain];
	[history[historyIndex] release];
	history[historyIndex] = str;
    historyIndex = (historyIndex + 1) % kHistorySize;
	historyLine = 0;
	
	if (historySize < kHistorySize)
		historySize++;
}

/*------------------------------------------------------------------------------
*  historyScroll - Scroll through the commands in the history in the specified
*     direction.
*-----------------------------------------------------------------------------*/
- (void)historyScroll:(int)direction
{
	NSString *historyString;
	
	historyString = [self getHistoryString:direction];
	
	if (historyString == nil)
		return;
		
	[monitorInputField setStringValue:historyString];
}

/*------------------------------------------------------------------------------
*  getHistoryString - Get the next histroy string in the specified direction
*    from the current one.
*-----------------------------------------------------------------------------*/
- (NSString *)getHistoryString:(int)direction
{
	int line;
	int index;
	
	if (historySize == 0)
		return nil;

	// Advance to the next line in the history
	line = historyLine + direction;
	if ((direction < 0 && line <= 0) || (direction > 0 && line > historySize))
		return nil;
	historyLine = line;
	
	if (historyLine > 0)
		index = (historyIndex - historyLine + historySize) % historySize;
	else
		index = historyIndex;
	
	return(history[index]);
}

/*------------------------------------------------------------------------------
*  showAboutBox - Show the about box
*-----------------------------------------------------------------------------*/
- (IBAction)showAboutBox:(id)sender
{
    [[AboutBox sharedInstance] showPanel:sender];
}

/*------------------------------------------------------------------------------
*  showDonation - Show the donation page in the web browser.
*-----------------------------------------------------------------------------*/
- (IBAction)showDonation:(id)sender;
{
	[[NSWorkspace  sharedWorkspace] openURL:
		[NSURL URLWithString:@"http://order.kagi.com/?6FBTU&lang=en"]];
}

/*------------------------------------------------------------------------------
*  startPressed - Handle the user pressing the start button in the function
*     keys window.  We want to hold it down for Duration frames.
*-----------------------------------------------------------------------------*/
- (IBAction)startPressed:(id)sender
{
	startFunctionPressed = FUNCTION_KEY_PRESS_DURATION;
}

/*------------------------------------------------------------------------------
*  breakPressed - Handle the user pressing the start button in the function
*     keys window.  We want to hold it down for Duration frames.
*-----------------------------------------------------------------------------*/
- (IBAction)breakPressed:(id)sender
{
	breakFunctionPressed = FUNCTION_KEY_PRESS_DURATION;
}

/*------------------------------------------------------------------------------
*  optionPressed - Handle the user pressing the option button in the function
*     keys window.  We want to hold it down for Duration frames.
*-----------------------------------------------------------------------------*/
- (IBAction)optionPressed:(id)sender
{
	optionFunctionPressed = FUNCTION_KEY_PRESS_DURATION;
}

/*------------------------------------------------------------------------------
*  selectPressed - Handle the user pressing the select button in the function
*     keys window.  We want to hold it down for Duration frames.
*-----------------------------------------------------------------------------*/
- (IBAction)selectPressed:(id)sender
{
	selectFunctionPressed = FUNCTION_KEY_PRESS_DURATION;
}

/*------------------------------------------------------------------------------
 *  monitorDrawerToggle - Toggle the Drawer for the GUI portion of the monitor
 *    open or closed.
 *-----------------------------------------------------------------------------*/
- (IBAction) monitorDrawerToggle:(id)sender
{
	NSWindow *window;
	NSRect frame;
	NSRange theEnd;
	
	window = [monitorInputField window];
	frame = [window frame];
	if ([monitorDrawer state] == NSDrawerClosedState) {
		frame.origin.y += kGraphicalDrawerSize;
		frame.size.height -= kGraphicalDrawerSize;
		[window setFrame:frame display:YES animate:YES];
		[window setHasShadow:NO];
		[monitorDrawer toggle:sender];
		[monitorGUIButton setState:NSOnState];
	}
	else {
		frame.origin.y -= kGraphicalDrawerSize;
		frame.size.height += kGraphicalDrawerSize;
		[monitorDrawer toggle:sender];
		[window setHasShadow:YES];
		[window setFrame:frame display:YES animate:YES];
		[monitorGUIButton setState:NSOffState];
	}
    theEnd=NSMakeRange([[monitorOutputView string] length],0);
    [monitorOutputView scrollRangeToVisible:theEnd];
}

/*------------------------------------------------------------------------------
 *  runButtonAction - Used for the monitor step buttons, this routine passes
 *   the action string to the command line processing.
 *-----------------------------------------------------------------------------*/
- (void) runButtonAction:(char *)action
{
	int retValue;
	
    retValue = MONITOR_monitorCmd(action); 
    if (!MONITOR_assemblerMode && retValue>-1) {
        [self monitorPrint:"> "];
		[self updateMonitorGUI];
	}
	
    [NSApp stopModalWithCode:retValue];
}

- (IBAction) monitorGoButton:(id)sender
{
	[self runButtonAction:"cont"];
}

- (IBAction) monitorStepButton:(id)sender
{
	[self runButtonAction:"g"];
}

- (IBAction) monitorStepOverButton:(id)sender
{
	[self runButtonAction:"o"];
}

- (IBAction) monitorStepOutButton:(id)sender
{
	[self runButtonAction:"r"];
}

- (IBAction) monitorQuitButton:(id)sender
{
	[self runButtonAction:"quit"];
}

- (IBAction) monitorCPURegChanged:(id)sender
{
	char fieldStr[10];
	[[sender stringValue] getCString:fieldStr maxLength:9 encoding:NSASCIIStringEncoding];
	
	switch([sender tag]) {
		case 0:
			CPU_regPC = strtol(fieldStr,NULL,16);
			old_regPC = CPU_regPC;
			break;
		case 1:
			CPU_regA = strtol(fieldStr,NULL,16);
			old_regA = CPU_regA;
			break;
		case 2:
			CPU_regX = strtol(fieldStr,NULL,16);
			old_regX = CPU_regX;
			break;
		case 3:
			CPU_regY = strtol(fieldStr,NULL,16);
			old_regY = CPU_regY;
			break;
		case 4:
			CPU_regS = strtol(fieldStr,NULL,16);
			old_regS = CPU_regS;
			break;
		case 5:
			CPU_regP = strtol(fieldStr,NULL,16);
			old_regP = CPU_regP;
			break;
	}
	[self updateMonitorGUIRegisters];
}

- (IBAction) monitorCPUFlagChanged:(id)sender
{
	CPU_regP ^= (1 << [sender tag]);
	old_regP = CPU_regP;
	[monitorPRegField setStringValue:[self hexStringFromByte:CPU_regP]];
	[self colorFromByte:monitorPRegField:CPU_regP:CPU_regP];
}

- (IBAction) monitorMemoryAddressChanged:(id)sender
{
	char buffer[80];
	unsigned short memVal;
	
	[[sender stringValue] getCString:buffer maxLength:80 encoding:NSASCIIStringEncoding];
	
	if (get_val_gui(buffer, &memVal) == FALSE) {
		memVal = [monitorMemoryDataSource getAddress];
		[monitorMemoryAddressField 
		 setStringValue:[self hexStringFromShort:memVal]];
		return;
	}
		
	[monitorMemoryDataSource setAddress:memVal];
	[self updateMonitorGUIMemory];
}

- (void) monitorMemoryAddress:(unsigned short)addr;
{
	[monitorMemoryDataSource setAddress:addr];
	[self updateMonitorGUIMemory];
	[monitorTabView selectFirstTabViewItem:self];
}

- (IBAction) monitorDisasmAddressChanged:(id)sender
{
	char buffer[80];
	unsigned short memVal;
	
	[[sender stringValue] getCString:buffer maxLength:80 encoding:NSASCIIStringEncoding];
	
	if (get_val_gui(buffer, &memVal) == FALSE) {
		memVal = [monitorDisasmDataSource getAddress];
		[monitorDisasmAddressField setStringValue:[NSString stringWithFormat:@"%04X", memVal]];
	}
	
	[self updateMonitorGUIDisasm:NO:memVal];
}

- (IBAction) monitorMemoryStepperChanged:(id)sender
{
	[monitorMemoryDataSource setAddress:[sender intValue]];
	[self updateMonitorGUIMemory];
}

- (IBAction) monitorMemoryFindFirst:(id)sender
{
	UWORD hexval;
	static UBYTE tab[64];
	char findString[81];
	
	[[monitorMemoryFindField stringValue] getCString:findString maxLength:80 encoding:NSASCIIStringEncoding];
	if ((foundMemoryLocations) != nil && ([foundMemoryLocations count] != 0)) 
		[foundMemoryLocations release];
	foundMemoryLocations = [NSMutableArray arrayWithCapacity:100];
	[foundMemoryLocations retain];
	memorySearchLength = 0;
	
	if (get_hex(findString, &hexval)) {
		memorySearchLength = 0;
		do {
			tab[memorySearchLength++] = (UBYTE) hexval;
			if (hexval & 0xff00)
				tab[memorySearchLength++] = (UBYTE) (hexval >> 8);
		} while (get_hex(NULL, &hexval));
	}
	if (memorySearchLength) {
		int addr;
		int addr1 = 0;
		int addr2 = 0xFFFF;
		
		for (addr = addr1; addr <= addr2; addr++) {
			int i = 0;
			while (MEMORY_dGetByte(addr + i) == tab[i]) {
				i++;
				if (i >= memorySearchLength) {
					[foundMemoryLocations addObject:[NSNumber numberWithInt:addr]];
					//printf("Found at %04x\n", addr);
					break;
				}
			}
		}

		if (foundMemoryLocations == nil)
			return;
		if ([foundMemoryLocations count] == 0) {
			NSBeep();
			return;
		}
		
		foundMemoryIndex = 0;
		
		[self showMemoryAtFoundIndex];
	}
}

- (IBAction) monitorMemoryFindNext:(id)sender
{
	if (foundMemoryLocations == nil || [foundMemoryLocations count] == 0)
		return;
	
	if (foundMemoryIndex < ([foundMemoryLocations count] - 1))
		foundMemoryIndex++;
	else
		NSBeep();
	
	[self showMemoryAtFoundIndex];
}
	
- (void) showMemoryAtFoundIndex
{
	unsigned short addr, currentAddr;
	
	addr = [[foundMemoryLocations objectAtIndex:foundMemoryIndex] intValue];
	currentAddr = [monitorMemoryDataSource getAddress];
	if (addr < currentAddr || (addr + memorySearchLength > currentAddr + 127)) {
		[monitorMemoryDataSource setAddress:addr];
		[self updateMonitorGUIMemory];
		currentAddr = [monitorMemoryDataSource getAddress];
	}
	[monitorMemoryDataSource setFoundLocation:addr Length:memorySearchLength];
    [monitorMemoryTableView reloadData];
}

- (IBAction) monitorDisasmRunToHere:(id)sender
{
	unsigned int address;
	
	address = [(DisasmDataSource *)monitorDisasmDataSource getAddress:[(DisasmTableView *)monitorDisasmTableView menuActionRow]];
	MONITOR_break_addr = address;
	MONITOR_break_active = TRUE;
	MONITOR_break_run_to_here = TRUE;
	[self runButtonAction:"cont"];
}

- (IBAction)monitorToggleTableBreakpoint:(id)sender
{
	int row;
	
	row = [monitorBreakpointTableView getActionRow];
	if (row == -1)
		return;
	[[BreakpointsController sharedInstance] toggleBreakpointEnableWithIndex:row];
	[breakpointDataSource loadBreakpoints];
}

- (IBAction) monitorAddByteWatch:(id)sender
{
	int row,col;
	unsigned short addr;
	
	row = [(MemoryEditorTableView *)monitorMemoryTableView menuActionRow];
	col = [(MemoryEditorTableView *)monitorMemoryTableView menuActionColumn];
	if (row==-1 || col==-1)
		return;
	
	addr = [monitorMemoryDataSource getAddress] + row*16 + col-1;
	[monitorWatchDataSource addByteWatch:addr];
}

- (IBAction) monitorAddWordWatch:(id)sender
{
	int row,col;
	unsigned short addr;
	
	row = [(MemoryEditorTableView *)monitorMemoryTableView menuActionRow];
	col = [(MemoryEditorTableView *)monitorMemoryTableView menuActionColumn];
	if (row==-1 || col==-1)
		return;
	
	addr = [monitorMemoryDataSource getAddress] + row*16 + col-1;
	[monitorWatchDataSource addWordWatch:addr];
}

- (IBAction) monitorDoneBreakValue:(id)sender
{
	[NSApp stopModal];
}

- (unsigned short) monitorGetBreakValue
{
	NSString *string;
	char buffer[80];
	unsigned short val;
	
	[monitorBreakValueField setStringValue:@""];
	[NSApp runModalForWindow:[monitorBreakValueField window]];
	string = [monitorBreakValueField stringValue];
	[[monitorBreakValueField window] close];
	[string getCString:buffer maxLength:80 encoding: NSASCIIStringEncoding];
	val = strtol(buffer, NULL, 16);
	
	return(val);
}

- (IBAction) monitorToggleBuiltinLabels:(id)sender
{
	if ([monitorBuiltinLabelCheckBox state] == NSOnState)
		symtable_builtin_enable = TRUE;
	else
		symtable_builtin_enable = FALSE;
	[self monitorSetLabelsDirty];
	[self updateMonitorGUILabels];
}

- (IBAction) monitorLoadLabelFile:(id)sender
{	
	NSString *filename;
    
    filename = [self browseFileInDirectory:[NSString stringWithCString:atari_exe_dir encoding:NSASCIIStringEncoding]];
    if (filename != nil) {
        [filename getCString:loadFilename maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
		load_user_labels(loadFilename);
		[self monitorSetLabelsDirty];
		[self updateMonitorGUILabels];
   }
}

- (IBAction) monitorDeleteUserLabels:(id)sender
{
	free_user_labels();
	[self monitorSetLabelsDirty];
	[self updateMonitorGUILabels];
}

- (IBAction) monitorAddUserlabel:(id)sender
{
	symtable_rec *p;
	char label[21];
	char value[5];
	int  addr;
	
	[[monitorUserLabelField stringValue] 
	    getCString:label maxLength:20 encoding:NSASCIIStringEncoding];
	[[monitorUserValueField stringValue] 
		getCString:value maxLength:25 encoding:NSASCIIStringEncoding];
	addr = strtol(value,NULL,16);
	p = find_user_label(label);
	if (p != NULL) {
		if (p->addr != addr) {
			p->addr = addr;
		}
	}
	else
		add_user_label(label, addr);	
	[self monitorSetLabelsDirty];
	[self updateMonitorGUILabels];
}

- (void) monitorSetLabelsDirty
{
	[monitorLabelDataSource setDirty];
}

/*------------------------------------------------------------------------------
 *  runBreakpointEditor - This method displays the breakpoint editor window.
 *-----------------------------------------------------------------------------*/
- (int) runBreakpointEditor:(id) sender
{
	[monitorBreakpointEditorTableView reloadData];
	return([NSApp runModalForWindow:[monitorBreakpointEditorTableView window]]);
}

/*------------------------------------------------------------------------------
 *  okBreakpointEditor - This method handles the OK button from the breakpoint 
 *     editor.
 *-----------------------------------------------------------------------------*/
- (IBAction)okBreakpointEditor:(id)sender
{
    [NSApp stopModalWithCode:1];
    [[monitorBreakpointEditorTableView window] close];
}

/*------------------------------------------------------------------------------
 *  cancelBreakpointEditor - This method handles the cancel button from the 
 *     reakpoint editor.
 *-----------------------------------------------------------------------------*/
- (IBAction)cancelBreakpointEditor:(id)sender
{
    [NSApp stopModalWithCode:0];
    [[monitorBreakpointEditorTableView window] close];
}

- (void)reloadBreakpointEditor
{
	[monitorBreakpointEditorTableView reloadData];
}

- (IBAction)addBreakpointCondition:(id)sender
{
	[breakpointEditorDataSource addCondition];
}

- (void)updateMonitorGUIRegisters
{
	static int firstTime = TRUE;
	
	if (firstTime) {
		firstTime = FALSE;
		old_regPC = CPU_regPC;
		old_regA = CPU_regA;
		old_regX = CPU_regX;
		old_regY = CPU_regY;
		old_regS = CPU_regS;
		old_regP = CPU_regP;
	}
	[monitorPCRegField setStringValue:[self hexStringFromShort:CPU_regPC]];
	[self colorFromShort:monitorPCRegField:CPU_regPC:old_regPC];
	[monitorARegField setStringValue:[self hexStringFromByte:CPU_regA]];
	[self colorFromByte:monitorARegField:CPU_regA:old_regA];
	[monitorXRegField setStringValue:[self hexStringFromByte:CPU_regX]];
	[self colorFromByte:monitorXRegField:CPU_regX:old_regX];
	[monitorYRegField setStringValue:[self hexStringFromByte:CPU_regY]];
	[self colorFromByte:monitorYRegField:CPU_regY:old_regY];
	[monitorSRegField setStringValue:[self hexStringFromByte:CPU_regS]];
	[self colorFromByte:monitorSRegField:CPU_regS:old_regS];
	[monitorPRegField setStringValue:[self hexStringFromByte:CPU_regP]];
	[self colorFromByte:monitorPRegField:CPU_regP:old_regP];
	[self setRegFlag:monitorNFlagCheckBox:old_regP:0x80];
	[self setRegFlag:monitorVFlagCheckBox:old_regP:0x40];
	[self setRegFlag:monitorBFlagCheckBox:old_regP:0x10];
	[self setRegFlag:monitorDFlagCheckBox:old_regP:0x08];
	[self setRegFlag:monitorIFlagCheckBox:old_regP:0x04];
	[self setRegFlag:monitorZFlagCheckBox:old_regP:0x02];
	[self setRegFlag:monitorCFlagCheckBox:old_regP:0x01];
	
	old_regPC = CPU_regPC;
	old_regA = CPU_regA;
	old_regX = CPU_regX;
	old_regY = CPU_regY;
	old_regS = CPU_regS;
	old_regP = CPU_regP;
}

- (void)updateMonitorGUIMemory
{
	unsigned short address;
	
	address = [monitorMemoryDataSource getAddress];
	[monitorMemoryAddressField 
	 setStringValue:[self hexStringFromShort:address]];
	[monitorMemoryStepper setIntValue:address];
	[monitorMemoryDataSource readMemory];
    [monitorMemoryTableView reloadData];
}

- (void)updateMonitorGUIBreakpoints
{
	[[BreakpointsController sharedInstance] loadBreakpoints];
}

- (void)updateMonitorBreakpointTableData
{
    [monitorBreakpointTableView reloadData];
	[monitorDisasmTableView reloadData];
}

- (void)updateMonitorGUIStack
{
	[monitorStackDataSource loadStack];
}

- (void)updateMonitorStackTableData
{
    [monitorStackTableView reloadData];
}

- (void)updateMonitorGUIWatch
{
	[monitorWatchDataSource updateWatches];
}

- (void)updateMonitorWatchTableData
{
    [monitorWatchTableView reloadData];
}

- (void)updateMonitorGUILabels
{
	[monitorBuiltinLabelCheckBox setState:(symtable_builtin_enable ? NSOnState : NSOffState)];
	[monitorUserLabelCountField setStringValue:[NSString stringWithFormat:@"%d User Labels Loaded",
												 symtable_user_size]];
	[monitorLabelDataSource loadLabels];
	[monitorLabelTableView reloadData];
}

- (void)updateMonitorGUIDisasm:(BOOL) usePc: (unsigned short) altAddress
{
	static unsigned short address = 0;
	int addrDelta;

	if (usePc) {
	addrDelta = (int) CPU_regPC - (int) address;

	if (addrDelta > 0x70 || addrDelta < 0)
		address = MONITOR_get_disasm_start(CPU_regPC - 0x40, CPU_regPC);
	} 
	else {
		address = MONITOR_get_disasm_start(altAddress - 0x40, altAddress);
	}

	[monitorDisasmDataSource disasmMemory:address:address+0x80:CPU_regPC];
	[monitorDisasmTableView reloadData];
	if (usePc) {
		[monitorDisasmTableView scrollRowToVisible:[monitorDisasmDataSource getPCRow]+1];
		[monitorDisasmTableView scrollRowToVisible:[monitorDisasmDataSource getPCRow]-1];
		[monitorDisasmAddressField setStringValue:[NSString stringWithFormat:@"%04X", CPU_regPC]];
	}
	else {
		int row;
		
		row = [monitorDisasmDataSource getRow:altAddress];
		[monitorDisasmTableView scrollRowToVisible:row+3];
		[monitorDisasmTableView scrollRowToVisible:row-3];
	}
}

- (void)updateMonitorGUIHardware
{
	[monitorAnticDataSource loadRegs];
	[monitorAnticTableView reloadData];
	[monitorGtiaDataSource loadRegs];
	[monitorGtiaTableView reloadData];
	[monitorPokeyDataSource loadRegs];
	[monitorPokeyTableView reloadData];
	[monitorPiaDataSource loadRegs];
	[monitorPiaTableView reloadData];
}

- (void)updateMonitorGUI
{
	[self updateMonitorGUIRegisters];
	[self updateMonitorGUIBreakpoints];
	[self updateMonitorGUIMemory];
	[self updateMonitorGUIDisasm:YES:0];	
	[self updateMonitorGUIStack];
	[self updateMonitorGUIWatch];
	[self updateMonitorGUILabels];
	[self updateMonitorGUIHardware];
}

- (NSString *) hexStringFromShort:(unsigned short)a
{
	return([NSString stringWithFormat:@"%04X",a]);
}

- (void) colorFromShort:(IBOutlet id)outlet:(unsigned short)a:(unsigned short)old_a
{
	if (a == old_a)
		[outlet setTextColor:black];
	else
		[outlet setTextColor:red];
}

- (void) colorFromByte:(IBOutlet id)outlet:(unsigned char)b:(unsigned char)old_b
{
	if (b == old_b)
		[outlet setTextColor:black];
	else
		[outlet setTextColor:red];
}

- (NSString *) hexStringFromByte:(unsigned char)b
{
	return([NSString stringWithFormat:@"%02X",b]);
}

- (void) setRegFlag:(IBOutlet id)outlet:(unsigned char)old_reg:(unsigned char)mask
{
	NSAttributedString *attributeTitle = nil;
	
	if (CPU_regP & mask) {
		[outlet setState:NSOnState];
	}
	else {
		[outlet setState:NSOffState];
	}
	
	if ((CPU_regP & mask) == (old_reg & mask)) {
		switch(mask) {
			case 0x80:
				attributeTitle = blackNString;
				break;
			case 0x40:
				attributeTitle = blackVString;
				break;
			case 0x10:
				attributeTitle = blackBString;
				break;
			case 0x08:
				attributeTitle = blackDString;
				break;
			case 0x04:
				attributeTitle = blackIString;
				break;
			case 0x02:
				attributeTitle = blackZString;
				break;
			case 0x01:
				attributeTitle = blackCString;
				break;
		}
	} else {
		switch(mask) {
			case 0x80:
				attributeTitle = redNString;
				break;
			case 0x40:
				attributeTitle = redVString;
				break;
			case 0x10:
				attributeTitle = redBString;
				break;
			case 0x08:
				attributeTitle = redDString;
				break;
			case 0x04:
				attributeTitle = redIString;
				break;
			case 0x02:
				attributeTitle = redZString;
				break;
			case 0x01:
				attributeTitle = redCString;
				break;
		}
	}

	[outlet setAttributedTitle:attributeTitle];
}

@end
