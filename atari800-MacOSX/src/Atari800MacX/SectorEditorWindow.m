/* SectorEditorWindow.m - 
 SectorEditorWindow class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */
#import "atari.h"
#import "atrUtil.h"
#import "atrErr.h"
#import "atrMount.h"
#import "Preferences.h"
#import "DisplayManager.h"
#import "SectorEditorWindow.h"

extern void PauseAudio(int pause);

@implementation SectorEditorWindow

static SectorEditorWindow *sharedInstance = nil;

+ (SectorEditorWindow *)sharedInstance {
    return sharedInstance ? sharedInstance : [[self alloc] init];
}
- (id)init {
	NSTextView *fieldEditor;
    NSArray *top;
	
    if (sharedInstance) {
	[self dealloc];
    } else {
        [super init];
		[Preferences sharedInstance];
        sharedInstance = self;
        /* load the nib and all the windows */
        if (!sectorTableView) {
				if (![[NSBundle mainBundle] loadNibNamed:@"SectorEditorWindow" owner:self topLevelObjects:&top])  {
					NSLog(@"Failed to load SectorEditorWindow.nib");
					NSBeep();
					return nil;
			}
            [top retain];
            }
	[[sectorTableView window] setExcludedFromWindowsMenu:YES];
	[[sectorTableView window] setMenu:nil];
	
	// Create and hook us up to our data source
	sectorDataSource = [[SectorEditorDataSource alloc] init];
	[sectorDataSource setOwner:self];
	[sectorTableView setDataSource:(id <NSComboBoxDataSource>)sectorDataSource];
	// Set the font sizes
	[[headerTableColumn dataCell] setFont:[NSFont fontWithName:@"Helvetica-Bold" size:14]];
	[[asciiTableColumn headerCell] setFont:[NSFont fontWithName:@"Helvetica-Bold" size:14]];
	[[asciiTableColumn dataCell] setFont:[NSFont fontWithName:@"Courier" size:14]];
	[[c00TableColumn headerCell] setFont:[NSFont fontWithName:@"Helvetica-Bold" size:14]];
	[[c00TableColumn dataCell] setFont:[NSFont fontWithName:@"Courier" size:14]];
	[[c01TableColumn headerCell] setFont:[NSFont fontWithName:@"Helvetica-Bold" size:14]];
	[[c01TableColumn dataCell] setFont:[NSFont fontWithName:@"Courier" size:14]];
	[[c02TableColumn headerCell] setFont:[NSFont fontWithName:@"Helvetica-Bold" size:14]];
	[[c02TableColumn dataCell] setFont:[NSFont fontWithName:@"Courier" size:14]];
	[[c03TableColumn headerCell] setFont:[NSFont fontWithName:@"Helvetica-Bold" size:14]];
	[[c03TableColumn dataCell] setFont:[NSFont fontWithName:@"Courier" size:14]];
	[[c04TableColumn headerCell] setFont:[NSFont fontWithName:@"Helvetica-Bold" size:14]];
	[[c04TableColumn dataCell] setFont:[NSFont fontWithName:@"Courier" size:14]];
	[[c05TableColumn headerCell] setFont:[NSFont fontWithName:@"Helvetica-Bold" size:14]];
	[[c05TableColumn dataCell] setFont:[NSFont fontWithName:@"Courier" size:14]];
	[[c06TableColumn headerCell] setFont:[NSFont fontWithName:@"Helvetica-Bold" size:14]];
	[[c06TableColumn dataCell] setFont:[NSFont fontWithName:@"Courier" size:14]];
	[[c07TableColumn headerCell] setFont:[NSFont fontWithName:@"Helvetica-Bold" size:14]];
	[[c07TableColumn dataCell] setFont:[NSFont fontWithName:@"Courier" size:14]];
	[[c08TableColumn headerCell] setFont:[NSFont fontWithName:@"Helvetica-Bold" size:14]];
	[[c08TableColumn dataCell] setFont:[NSFont fontWithName:@"Courier" size:14]];
	[[c09TableColumn headerCell] setFont:[NSFont fontWithName:@"Helvetica-Bold" size:14]];
	[[c09TableColumn dataCell] setFont:[NSFont fontWithName:@"Courier" size:14]];
	[[c10TableColumn headerCell] setFont:[NSFont fontWithName:@"Helvetica-Bold" size:14]];
	[[c10TableColumn dataCell] setFont:[NSFont fontWithName:@"Courier" size:14]];
	[[c11TableColumn headerCell] setFont:[NSFont fontWithName:@"Helvetica-Bold" size:14]];
	[[c11TableColumn dataCell] setFont:[NSFont fontWithName:@"Courier" size:14]];
	[[c12TableColumn headerCell] setFont:[NSFont fontWithName:@"Helvetica-Bold" size:14]];
	[[c12TableColumn dataCell] setFont:[NSFont fontWithName:@"Courier" size:14]];
	[[c13TableColumn headerCell] setFont:[NSFont fontWithName:@"Helvetica-Bold" size:14]];
	[[c13TableColumn dataCell] setFont:[NSFont fontWithName:@"Courier" size:14]];
	[[c14TableColumn headerCell] setFont:[NSFont fontWithName:@"Helvetica-Bold" size:14]];
	[[c14TableColumn dataCell] setFont:[NSFont fontWithName:@"Courier" size:14]];
	[[c15TableColumn headerCell] setFont:[NSFont fontWithName:@"Helvetica-Bold" size:14]];
	[[c15TableColumn dataCell] setFont:[NSFont fontWithName:@"Courier" size:14]];
	fieldEditor = (NSTextView *) [[self window] fieldEditor:YES forObject:nil];
	[fieldEditor setAllowsUndo:NO];
	[[self window] setDelegate:(id <NSWindowDelegate>)self];
		
	}
	
    return sharedInstance;
}

- (void)dealloc {
	[super dealloc];
}

/*------------------------------------------------------------------------------
*  showSectorPanel - This method displays the sector editor window.
*-----------------------------------------------------------------------------*/
- (void)showSectorPanel
{
    [[DisplayManager sharedInstance] enableMacCopyPaste];
    [NSApp runModalForWindow:[sectorTableView window]];
    [[DisplayManager sharedInstance] enableAtariCopyPaste];
}

/*------------------------------------------------------------------------------
*  mountDisk - This method mounts a ATR file for use by the sector editor.
*-----------------------------------------------------------------------------*/
- (int)mountDisk:(NSString *)filename
{
	int status = 0;
	char cfilename[FILENAME_MAX];
	NSString *title;

	// Call the Atr library with the filename that is passed in to mount the disk
	[filename getCString:cfilename maxLength:FILENAME_MAX encoding:NSUTF8StringEncoding];
    status = AtrMount(cfilename ,&dosType, &readWrite, &writeProtect,&diskinfo);
    if (( status == ADOS_NOT_A_ATR_IMAGE ) ||
        ( status == ADOS_DISK_READ_ERR )   ||
        ( status == ADOS_MEM_ERR ))
        {
		// If it fails, unmount the disk
		AtrUnmount(diskinfo);
		return(-1);
        }

	// Set up the current and maximum sectors for the image.
	currentSector = 1;
	maxSector = AtrSectorCount(diskinfo);

	// Program the stepper in the window based on the current and max sizes.
	[sectorStepper setIntValue:currentSector];
	[sectorStepper setMinValue:1];
	[sectorStepper setMaxValue:maxSector];
	
	// The sector has no changes to start, and set the button states appropriately.
	dirty = NO;
	[self setControls];
	
	// Set the window title of the editor
	title = [@"ATR Sector Editor - " stringByAppendingString:[filename lastPathComponent]];
	if (!readWrite)
		title = [title stringByAppendingString:@" - Read Only"];
	[[self window] setTitle:title];
	
	// Display the current sector number in the window
	[self displaySector];
	
	// Load the current sector data from disk, and display it in the window
	[sectorDataSource readSector:currentSector];
	[self reloadSectorData];
	return(0);
}

/*------------------------------------------------------------------------------
*  setControls - Enable/disable the buttons and controls in the window
*-----------------------------------------------------------------------------*/
- (void) setControls
{
	if (writeProtect)
		[diskImageWriteProtectButton setState:NSOnState];
	else
		[diskImageWriteProtectButton setState:NSOffState];

	if (readWrite)
		[diskImageWriteProtectButton setEnabled:YES];
	else
		[diskImageWriteProtectButton setEnabled:NO];

	if (readWrite && !writeProtect) 
		[self setCellsEditable:YES];
	else
		[self setCellsEditable:NO];

	if (dirty) {
		[sectorSaveButton setEnabled:YES];
		[sectorRevertButton setEnabled:YES];
		}
	else {
		[sectorSaveButton setEnabled:NO];
		[sectorRevertButton setEnabled:NO];
		}
}

/*------------------------------------------------------------------------------
*  displaySector - Display the current and max sectors in the window
*-----------------------------------------------------------------------------*/
- (void)displaySector
{
	[sectorField setStringValue:[NSString stringWithFormat:@"Sector %05d of %05d",currentSector,maxSector]];
}

/*------------------------------------------------------------------------------
*  setCellsEditable - Allow/disallow the editing of data cells in the table
*-----------------------------------------------------------------------------*/
- (void)setCellsEditable:(bool) editable
{
	[[asciiTableColumn dataCell] setEditable:editable];
	[[c00TableColumn dataCell] setEditable:editable];
	[[c01TableColumn dataCell] setEditable:editable];
	[[c02TableColumn dataCell] setEditable:editable];
	[[c03TableColumn dataCell] setEditable:editable];
	[[c04TableColumn dataCell] setEditable:editable];
	[[c05TableColumn dataCell] setEditable:editable];
	[[c06TableColumn dataCell] setEditable:editable];
	[[c07TableColumn dataCell] setEditable:editable];
	[[c08TableColumn dataCell] setEditable:editable];
	[[c09TableColumn dataCell] setEditable:editable];
	[[c10TableColumn dataCell] setEditable:editable];
	[[c11TableColumn dataCell] setEditable:editable];
	[[c12TableColumn dataCell] setEditable:editable];
	[[c13TableColumn dataCell] setEditable:editable];
	[[c14TableColumn dataCell] setEditable:editable];
	[[c15TableColumn dataCell] setEditable:editable];
}

/*------------------------------------------------------------------------------
*  getDiskInfo - Return the info about the mounted atr image
*-----------------------------------------------------------------------------*/
- (AtrDiskInfo *)getDiskInfo
{
	return(diskinfo);
}

/*------------------------------------------------------------------------------
*  reloadSectorData - This method causes the directory data to be updated.
*-----------------------------------------------------------------------------*/
- (void)reloadSectorData
{
    [sectorTableView reloadData];
}

/*------------------------------------------------------------------------------
*  revertSector - Reread the sector from disk and revert any changes which have
*                   been made.
*-----------------------------------------------------------------------------*/
- (IBAction)revertSector:(id)sender
{
	[[self window] endEditingFor:nil];
	if (dirty) {
		[sectorDataSource readSector:currentSector];
		[self reloadSectorData];
	}
}

/*------------------------------------------------------------------------------
*  saveSector - Save the changed sector to the disk.
*-----------------------------------------------------------------------------*/
- (IBAction)saveSector:(id)sender
{
	[[self window] endEditingFor:nil];
	if (dirty) {
		[sectorDataSource writeSector:currentSector];
	}
}

/*------------------------------------------------------------------------------
*  diskImageWriteProtect - Handle user changing the header write protect status
*-----------------------------------------------------------------------------*/
- (IBAction)diskImageWriteProtect:(id)sender
{
	if ([diskImageWriteProtectButton state] == NSOnState) { 
		[[self window] endEditingFor:nil];
		if (dirty) {
			if (![self confirmDiscard])
				return;
			[self revertSector:nil];
			}
		writeProtect = 1;
		}
	else {
		writeProtect = 0;
		}
	AtrSetWriteProtect(diskinfo, writeProtect);

	[self setControls];
}

/*------------------------------------------------------------------------------
*  changeSector - Handle the user pressing the sector stepper up or down
*-----------------------------------------------------------------------------*/
- (IBAction)changeSector:(id)sender
{
    [[self window] endEditingFor:nil];
	if (dirty) {
		if (![self confirmDiscard])
			return;
		}
	currentSector = [sender intValue];
	[self displaySector];
	[sectorDataSource readSector:currentSector];
	[self reloadSectorData];
}

/*------------------------------------------------------------------------------
*  displaySectorWindow - This method displays an input box to get a new sector
*  number.
*-----------------------------------------------------------------------------*/
- (IBAction)displaySectorWindow:(id)sender
{
	[[sectorNumberField formatter] setMaximum:[NSDecimalNumber decimalNumberWithMantissa:maxSector exponent:0 isNegative:NO]];
    [[self window] endEditingFor:nil];
    [self.window beginSheet:[sectorNumberField window]
      completionHandler:^(NSModalResponse returnCode){}];
    [NSApp runModalForWindow: [sectorNumberField window]];
    [NSApp endSheet: [sectorNumberField window]];
}

/*------------------------------------------------------------------------------
*  sectorNumberOK - This method handles the OK button press from the sector
*      number query sheet.
*-----------------------------------------------------------------------------*/
- (IBAction)sectorNumberOK:(id)sender
{
	currentSector = [sectorNumberField intValue];
    [[sectorNumberField window] orderOut: self];
	[self displaySector];
	[sectorDataSource readSector:currentSector];
	[self reloadSectorData];
    [sectorStepper setIntValue:currentSector];
    [NSApp stopModal];
}

/*------------------------------------------------------------------------------
*  sectorOK - This method handles the Done button from the sector editor window.
*-----------------------------------------------------------------------------*/
- (IBAction)sectorOK:(id)sender 
{
    [[self window] endEditingFor:nil];
	if (dirty) {
		if (![self confirmDiscard])
			return;
		}
	AtrUnmount(diskinfo);
    [NSApp stopModalWithCode:0];
    [[sender window] close];
}

/*------------------------------------------------------------------------------
*  setDirty - Mark the current sector being edited as dirty.
*-----------------------------------------------------------------------------*/
- (void)setDirty
{
	if (readWrite && !writeProtect) {
		[sectorSaveButton setEnabled:YES];
		[sectorRevertButton setEnabled:YES];
		}
	dirty = YES;
}

/*------------------------------------------------------------------------------
*  clearDirty - Mark the current sector being edited as clean from changes.
*-----------------------------------------------------------------------------*/
- (void)clearDirty
{
	if (readWrite && !writeProtect) {
		[sectorSaveButton setEnabled:NO];
		[sectorRevertButton setEnabled:NO];
		}
	dirty = NO;
}

/*------------------------------------------------------------------------------
*  confirmDiscard - Display a dialog to confirm the user wants to discard 
*      changes to the sector.
*-----------------------------------------------------------------------------*/
- (bool)confirmDiscard
{
	__block int confirm;
	
    [self.window beginSheet:[confirmOKButton window]
                       completionHandler:^(NSModalResponse returnCode){
        confirm = returnCode;
    }];
    [NSApp runModalForWindow: [confirmOKButton window]];
    [NSApp endSheet: [confirmOKButton window]];
 	if (confirm)
		return YES;
	else
		return NO;
}

/*------------------------------------------------------------------------------
*  confirmOK - Handle the user pressing OK in the confirm sheet.
*-----------------------------------------------------------------------------*/
- (IBAction)confirmOK:(id)sender
{
    [[confirmOKButton window] orderOut: self];
    [NSApp stopModalWithCode:1];
	[[self window] makeKeyAndOrderFront:self];
}

/*------------------------------------------------------------------------------
*  confirmCancel - Handle the user pressing Cancel in the confirm sheet.
*-----------------------------------------------------------------------------*/
- (IBAction)confirmCancel:(id)sender
{
    [[confirmOKButton window] orderOut: self];
    [NSApp stopModalWithCode:0];
	[[self window] makeKeyAndOrderFront:self];
}

@end
