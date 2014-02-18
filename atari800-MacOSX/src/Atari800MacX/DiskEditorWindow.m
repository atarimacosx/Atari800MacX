/* DiskEditorWindow.m - DiskEditorWindow window 
   manager class and support functions for the
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimac@kc.rr.com>
*/
#import "atari.h"
#import "AtrUtil.h"
#import "AtrMount.h"
#import "DiskEditorWindow.h"

extern void PauseAudio(int pause);
extern char atari_disk_dirs[][FILENAME_MAX];

static NSMutableArray *editorArray = nil;

@implementation DiskEditorWindow

/*------------------------------------------------------------------------------
*  getEditorArray - Get the pointer to the array that stores the list of editor
*     windows.  If it doesn't exist yet, create it.
*-----------------------------------------------------------------------------*/
+(NSMutableArray *)getEditorArray
{
	if (editorArray == nil) {
		editorArray = [[NSMutableArray alloc] initWithCapacity:5];
		[editorArray retain];
		}
	return(editorArray);
}

/*------------------------------------------------------------------------------
*  isAlreadyOpen - Determine if an editor is already open for a particular 
*     filename.
*-----------------------------------------------------------------------------*/
+(DiskEditorWindow *)isAlreadyOpen:(NSString *)filename
{
	NSMutableArray *array;
	int isopen = FALSE;
	int i;
	DiskEditorWindow *editor;
	
	array = [DiskEditorWindow getEditorArray];
	for (i=0;i<[array count];i++)
		{
		editor = [array objectAtIndex:i]; 
		if ([[editor filename] isEqual:filename]) {
			isopen = TRUE;
			break;
			}
		}
	if (isopen)
		return([array objectAtIndex:i]);
	else
		return(nil);
}

/*------------------------------------------------------------------------------
*  initWithWindowNibName - Initialize the window, using the NIB file, and add
*   itself to the list of windows.
*-----------------------------------------------------------------------------*/
- (id)initWithWindowNibName:
	(NSString *)windowNibName:
	(DiskEditorDataSource *)dataSource:
	(NSString *)filename
	{
	[super initWithWindowNibName:windowNibName];
	directoryDataSource = dataSource;
	diskImageReadWrite = 0;
	diskImageSubdirs = 0;
	diskImageWriteProtect = 0;
	lfConvert = 0;
	imageEditorOpen = 0;
	imageFilename = filename;
	[imageFilename retain];
	[[DiskEditorWindow getEditorArray] addObject:self];
    return self;
	}

-(NSString *)filename
{
	return(imageFilename);
}
	
/*------------------------------------------------------------------------------
*  windowDidLoad - Initialize the window components after it has loaded.
*-----------------------------------------------------------------------------*/
- (void)windowDidLoad
	{
	static int firstTime = 1;
	
	if (firstTime) {
	   [[self window] center];
	   firstTime = 0;
	   }
	   
	[directoryDataSource setOwner:self];
	[directoryTableView setDataSource:(id <NSComboBoxDataSource>)directoryDataSource];
	[directoryTableView setDoubleAction:@selector(diskImageDoubleClick:)];
#ifndef OSX_10_1_COMPAT	
	[directoryTableView registerForDraggedTypes:[NSArray arrayWithObjects:
            NSFilenamesPboardType, NSFilesPromisePboardType, nil]]; // Register for Drag and Drop
#else
	[directoryTableView registerForDraggedTypes:[NSArray arrayWithObjects:
            NSFilenamesPboardType, nil]]; // Register for Drag and Drop
#endif			
	[[directoryTableColumn headerCell] setFont:[NSFont fontWithName:@"Courier" size:12]];
	[[directoryTableColumn dataCell] setFont:[NSFont fontWithName:@"Monaco" size:12]];
	[[self window] setDelegate:(id <NSWindowDelegate>)self];
	[self diskImageOpenDone];
	
	[self diskImageEnableButtons];
	}
	
/*------------------------------------------------------------------------------
*  browseFiles - This allows the user to chose a file(s) to read in.
*-----------------------------------------------------------------------------*/
- (NSArray *) browseFiles {
    NSOpenPanel *openPanel = nil;
    
    openPanel = [NSOpenPanel openPanel];
    [openPanel setCanChooseDirectories:NO];
    [openPanel setCanChooseFiles:YES];
	[openPanel setAllowsMultipleSelection:YES];
	
    if ([openPanel runModalForDirectory:nil file:nil types:nil] == NSOKButton)
        return([openPanel filenames]);
    else
        return nil;
	}	

/*------------------------------------------------------------------------------
*  browseDirectory - This allows the user to chose a directory to write in.
*-----------------------------------------------------------------------------*/
- (NSString *) browseDirectory {
    NSOpenPanel *openPanel = nil;
    
    openPanel = [NSOpenPanel openPanel];
    [openPanel setCanChooseDirectories:YES];
    [openPanel setCanChooseFiles:NO];
	[openPanel setAllowsMultipleSelection:NO];

    if ([openPanel runModalForDirectory:nil file:nil types:nil] == NSOKButton)
        return([[openPanel filenames] objectAtIndex:0]);
    else
        return nil;
    
	}
		
/*------------------------------------------------------------------------------
*  diskImageSelect - Called when a table row is selected.
*-----------------------------------------------------------------------------*/
- (IBAction)diskImageSelect:(id)sender
{
	[self diskImageEnableButtons];
}

/*------------------------------------------------------------------------------
*  diskImageEnableButtons - Enable/Disable the editor buttons based on the type
*     of image, and what is selected.
*-----------------------------------------------------------------------------*/
- (void)diskImageEnableButtons
{
	int numSelected = [directoryTableView numberOfSelectedRows];

	if (numSelected == 0) {
		[diskImageExportButton setEnabled:NO];
		[diskImageDeleteButton setEnabled:NO];
		[diskImageRenameButton setEnabled:NO];
		[diskImageLockButton setEnabled:NO];
		[diskImageUnlockButton setEnabled:NO];
		}
	else if (numSelected == 1) {
		[diskImageExportButton setEnabled:YES];
		if (diskImageReadWrite && !diskImageWriteProtect) {
			[diskImageDeleteButton setEnabled:YES];
			[diskImageRenameButton setEnabled:YES];
			[diskImageLockButton setEnabled:YES];
			[diskImageUnlockButton setEnabled:YES];
			}
		else {
			[diskImageDeleteButton setEnabled:NO];
			[diskImageRenameButton setEnabled:NO];
			[diskImageLockButton setEnabled:NO];
			[diskImageUnlockButton setEnabled:NO];
			}
		}
	else {
		[diskImageExportButton setEnabled:YES];
		if (diskImageReadWrite && !diskImageWriteProtect) {
			[diskImageDeleteButton setEnabled:YES];
			[diskImageRenameButton setEnabled:NO];
			[diskImageUnlockButton setEnabled:YES];
			[diskImageLockButton setEnabled:YES];
			}
		else {
			[diskImageDeleteButton setEnabled:NO];
			[diskImageRenameButton setEnabled:NO];
			[diskImageLockButton setEnabled:NO];
			[diskImageUnlockButton setEnabled:NO];
			}
		}
		
	if (diskImageReadWrite && !diskImageWriteProtect) {
		[diskImageImportButton setEnabled:YES];
		if (diskImageSubdirs)
			[diskImageMkdirButton setEnabled:YES];
		else
			[diskImageMkdirButton setEnabled:NO];
		}
	else {
		[diskImageImportButton setEnabled:NO];
		[diskImageMkdirButton setEnabled:NO];
		}
		
	if (diskImageWriteProtect)
		[diskImageWriteProtectButton setState:NSOnState];
	else
		[diskImageWriteProtectButton setState:NSOffState];

	if (diskImageReadWrite)
		[diskImageWriteProtectButton setEnabled:YES];
	else
		[diskImageWriteProtectButton setEnabled:NO];		

	if (lfConvert)
		[diskImageLfTranslateButton setState:NSOnState];
	else
		[diskImageLfTranslateButton setState:NSOffState];

}

/*------------------------------------------------------------------------------
*  diskImageDoubleClick - Called when a user double clicks on a table row.
*     Either exports the file, or opens the directory clicked on.
*-----------------------------------------------------------------------------*/
- (IBAction)diskImageDoubleClick:(id)sender
{
	int row = [sender selectedRow];
	
	if ([directoryDataSource isDirectory:row])
		[directoryDataSource changeDirectory:row];
	else
		[self diskImageExport:sender];

	[self diskImageEnableButtons];
}

/*------------------------------------------------------------------------------
*  diskImageOpenDone - Called when a image has been opened.  This calls the disk
*     image library, to load the disk, and get the initial directory.
*-----------------------------------------------------------------------------*/
- (void)diskImageOpenDone
{
	NSString *imageType;
	
	//printf("In Disk Image Open Done 1\n");
	[directoryTableView deselectAll:self];
	//printf("In Disk Image Open Done 2\n");
	imageType = [directoryDataSource getDiskInfo:
							&diskImageReadWrite:
							&diskImageSubdirs:
							&diskImageWriteProtect]; 
	//printf("In Disk Image Open Done 3\n");
	[[directoryTableColumn headerCell] setStringValue:imageType];
	//printf("In Disk Image Open Done 4\n");
	[[self window] 
		setTitle:[@"ATR Disk Image Editor - " stringByAppendingString:[imageFilename lastPathComponent]]];
	//printf("In Disk Image Open Done 5\n");
	[self diskImageEnableButtons];
	//printf("In Disk Image Open Done 6\n");
}

/*------------------------------------------------------------------------------
*  diskImageImport - Called when the import button is pressed.
*-----------------------------------------------------------------------------*/
- (IBAction)diskImageImport:(id)sender
{
	NSArray *filenames;
	int i;

    filenames = [self browseFiles];
	if (filenames != nil) {
		for (i=0;i < [filenames count]; i++)
			[directoryDataSource importFile:[filenames objectAtIndex:i]:lfConvert];
		[self diskImageEnableButtons];
		}
}

/*------------------------------------------------------------------------------
*  diskImageImportDrag - Called when files are dragged to the editor from the 
*     Finder.
*-----------------------------------------------------------------------------*/
- (void)diskImageImportDrag:(NSArray *)filenames
{
	int i;
	
	for (i=0;i < [filenames count]; i++)
		[directoryDataSource importFile:[filenames objectAtIndex:i]:lfConvert];
	[self diskImageEnableButtons];
}

/*------------------------------------------------------------------------------
*  diskImageImportDragLocal - Called when files are dragged to the editor from  
*     another editor instance.
*-----------------------------------------------------------------------------*/
- (void)diskImageImportDragLocal:(NSArray *)filenames
{
	int i;
	
	for (i=0;i < [filenames count]; i++)
		[directoryDataSource importFile:[filenames objectAtIndex:i]:0];
	[self diskImageEnableButtons];
}

/*------------------------------------------------------------------------------
*  diskImageExport - Called when the export button is pressed.
*-----------------------------------------------------------------------------*/
- (IBAction)diskImageExport:(id)sender
{
	NSString *filename;
	NSEnumerator *enumerator;
	NSArray *indicies;
	int i;
	
	filename = [self browseDirectory];
	if (filename != nil) {
		enumerator = [directoryTableView selectedRowEnumerator]; 
	
		indicies = [enumerator allObjects];
	
		//printf("lfconvert = %d\n",lfConvert);
		for (i=0;i<[indicies count];i++)
			[directoryDataSource exportFile:[[indicies objectAtIndex:i] intValue]:filename:lfConvert];
		}
}

/*------------------------------------------------------------------------------
*  diskImageDelete - Called when the delete button is pressed.
*-----------------------------------------------------------------------------*/
- (IBAction)diskImageDelete:(id)sender
{
	NSEnumerator *enumerator;
	NSArray *indicies;
	int i;
	
	enumerator = [directoryTableView selectedRowEnumerator]; 
	
	indicies = [enumerator allObjects];
	
	for (i=0;i<[indicies count];i++)
		[directoryDataSource deleteFile:[[indicies objectAtIndex:i] intValue]];
	
	[self diskImageEnableButtons];
}

/*------------------------------------------------------------------------------
*  diskImageRename - Called when the rename button is pressed.
*-----------------------------------------------------------------------------*/
- (IBAction)diskImageRename:(id)sender
{
	int result;
	
	[diskImageNameField setStringValue:@""];
    PauseAudio(1);
	[NSApp beginSheet: diskImageNewNameSheet
            modalForWindow: [self window]
            modalDelegate: nil
            didEndSelector: nil
            contextInfo: nil];
    result = [NSApp runModalForWindow: diskImageNewNameSheet];
    // Sheet is up here.
    [NSApp endSheet: diskImageNewNameSheet];
    [diskImageNewNameSheet orderOut: self];

	if (result) {
		[directoryDataSource renameFile:[directoryTableView selectedRow]:
									[diskImageNameField stringValue]];
		}
}

/*------------------------------------------------------------------------------
*  diskImageLock - Called when the lock button is pressed.
*-----------------------------------------------------------------------------*/
- (IBAction)diskImageLock:(id)sender
{
	NSEnumerator *enumerator;
	NSArray *indicies;
	int i;
	
	enumerator = [directoryTableView selectedRowEnumerator]; 
	
	indicies = [enumerator allObjects];
	
	for (i=0;i<[indicies count];i++)
		[directoryDataSource lockFile:[[indicies objectAtIndex:i] intValue]];
}

/*------------------------------------------------------------------------------
*  diskImageUnlock - Called when the unlock button is pressed.
*-----------------------------------------------------------------------------*/
- (IBAction)diskImageUnlock:(id)sender
{
	NSEnumerator *enumerator;
	NSArray *indicies;
	int i;
	
	enumerator = [directoryTableView selectedRowEnumerator]; 
	
	indicies = [enumerator allObjects];
	
	for (i=0;i<[indicies count];i++)
		[directoryDataSource unlockFile:[[indicies objectAtIndex:i] intValue]];
}

/*------------------------------------------------------------------------------
*  diskImageMkdir - Called when the Make Directory button is pressed.
*-----------------------------------------------------------------------------*/
- (IBAction)diskImageMkdir:(id)sender
{
	int result;
	
	[diskImageNameField setStringValue:@""];
    PauseAudio(1);
	[NSApp beginSheet: diskImageNewNameSheet
            modalForWindow: [self window]
            modalDelegate: nil
            didEndSelector: nil
            contextInfo: nil];
    result = [NSApp runModalForWindow: diskImageNewNameSheet];
    // Sheet is up here.
    [NSApp endSheet: diskImageNewNameSheet];
    [diskImageNewNameSheet orderOut: self];

	if (result) {
		[directoryDataSource createDirectory:[diskImageNameField stringValue]];
		}
	[self diskImageEnableButtons];
}

/*------------------------------------------------------------------------------
*  diskImageWriteProtect - Called when the Write Protect button is pressed.
*-----------------------------------------------------------------------------*/
- (IBAction)diskImageWriteProtect:(id)sender
{
	if ([diskImageWriteProtectButton state] == NSOnState) { 
		[directoryDataSource writeProtect:1];
		diskImageWriteProtect = 1;
		}
	else {
		[directoryDataSource writeProtect:0];
		diskImageWriteProtect = 0;
		}
	[self diskImageEnableButtons];
}

/*------------------------------------------------------------------------------
*  diskImageDirChange - Called when the directory pulldown is chosen.
*-----------------------------------------------------------------------------*/
- (IBAction)diskImageDirChange:(id)sender
{
	int index = [diskImageDirPulldown indexOfSelectedItem];
	int count = [diskImageDirPulldown numberOfItems] - 1 - index;
	
	if (index == 0)
		[directoryDataSource changeDirectoryUp:-1];
	else
		[directoryDataSource changeDirectoryUp:count];

	[self diskImageEnableButtons];
}

/*------------------------------------------------------------------------------
*  windowShouldClose - Cleanup when the window is closed.
*-----------------------------------------------------------------------------*/
- (BOOL)windowShouldClose:(id)sender
{
	[[DiskEditorWindow getEditorArray] removeObject:self];
	[directoryDataSource reset];
	[imageFilename autorelease];
	[self autorelease];
    return YES;
}

/*------------------------------------------------------------------------------
*  diskImageMenuAdd - Add a menu item for a lower directory we've clicked on.
*-----------------------------------------------------------------------------*/
- (void) diskImageMenuAdd:(NSString *) itemName
{
	[diskImageDirPulldown addItemWithTitle:itemName];
	[diskImageDirPulldown selectItemWithTitle:itemName];
}

/*------------------------------------------------------------------------------
*  diskImageMenuClear - Clear the menu for the root directory.
*-----------------------------------------------------------------------------*/
- (void) diskImageMenuClear
{
	[self diskImageMenuDelete:([diskImageDirPulldown numberOfItems] - 1)];
}

/*------------------------------------------------------------------------------
*  diskImageMenuDelete - Delete directory menu entries of lower directories we
*    have moved above.
*-----------------------------------------------------------------------------*/
- (void) diskImageMenuDelete:(int) count;
{
	int i, index = [diskImageDirPulldown numberOfItems]-1;
	
	for (i=0;i<count;i++,index--) 
		[diskImageDirPulldown removeItemAtIndex:index];
}

/*------------------------------------------------------------------------------
*  diskImageDisplayFreeBytes - Display the number of free bytes on the disk 
*    image.
*-----------------------------------------------------------------------------*/
- (void) diskImageDisplayFreeBytes:(int) free
{
	char sizeStr[80];
	NSString *size;
	
	sprintf(sizeStr,"%.1f Kilobytes",(((float) free)/1024));
	size = [NSString stringWithCString:sizeStr encoding:NSASCIIStringEncoding];
	[diskImageFreeSectorsField setStringValue:size];
}

/*------------------------------------------------------------------------------
*  diskImageNameOK - Called when user clicks OK on rename sheet.
*-----------------------------------------------------------------------------*/
- (IBAction)diskImageNameOK:(id)sender
{
    [NSApp stopModalWithCode:1];
    PauseAudio(0);
	[[self window] makeKeyAndOrderFront:self];
}

/*------------------------------------------------------------------------------
*  diskImageNameCancel - Called when user clicks cancel on rename sheet.
*-----------------------------------------------------------------------------*/
- (IBAction)diskImageNameCancel:(id)sender
{
    [NSApp stopModalWithCode:0];
    PauseAudio(0);
	[[self window] makeKeyAndOrderFront:self];
}

/*------------------------------------------------------------------------------
*  diskImageTranslateChange - Called when user clicks cancel on rename sheet.
*-----------------------------------------------------------------------------*/
- (IBAction)diskImageTranslateChange:(id)sender
{
	if ([diskImageLfTranslateButton state] == NSOnState)
		lfConvert = 1;
	else
		lfConvert = 0;
}

/*------------------------------------------------------------------------------
*  Internal class variable access functions.
*-----------------------------------------------------------------------------*/

- (BOOL)isDiskImageMounted
{
	return [directoryDataSource isMounted];
}

- (int)getLfConvert
{
	return lfConvert;
}

-(int)getWriteProtect
{
	return diskImageWriteProtect;
}

/*------------------------------------------------------------------------------
*  reloadDirectoryData - This method causes the directory data to be updated.
*-----------------------------------------------------------------------------*/
- (void)reloadDirectoryData
{
    [directoryTableView reloadData];
}

/*------------------------------------------------------------------------------
*  displayErrorWindow - This method displays an error dialog box with the passed
*      in error message.
*-----------------------------------------------------------------------------*/
- (void)displayErrorWindow:(NSString *)errorMsg {
    [errorField setStringValue:errorMsg];
    PauseAudio(1);
	[NSApp beginSheet: diskImageErrorSheet
            modalForWindow: [self window]
            modalDelegate: nil
            didEndSelector: nil
            contextInfo: nil];
    [NSApp runModalForWindow: diskImageErrorSheet];
    // Sheet is up here.
    [NSApp endSheet: diskImageErrorSheet];
    [diskImageErrorSheet orderOut: self];
}

/*------------------------------------------------------------------------------
*  errorOK - This method handles the OK button press from the error sheet.
*-----------------------------------------------------------------------------*/
- (IBAction)errorOK:(id)sender;
{
    [NSApp stopModal];
	PauseAudio(0);
}

/*------------------------------------------------------------------------------
*  allSelected - Called to update buttons when user has selected Select All from
*     the menu.
*-----------------------------------------------------------------------------*/
- (void) allSelected:(int)count
{
		[diskImageExportButton setEnabled:YES];
		if (diskImageReadWrite && !diskImageWriteProtect) {
			[diskImageDeleteButton setEnabled:YES];
			if (count != 1)
				[diskImageRenameButton setEnabled:NO];
			else
				[diskImageRenameButton setEnabled:YES];
			[diskImageUnlockButton setEnabled:YES];
			[diskImageLockButton setEnabled:YES];
			}
		else {
			[diskImageDeleteButton setEnabled:NO];
			[diskImageRenameButton setEnabled:NO];
			[diskImageLockButton setEnabled:NO];
			[diskImageUnlockButton setEnabled:NO];
			}
}


@end
