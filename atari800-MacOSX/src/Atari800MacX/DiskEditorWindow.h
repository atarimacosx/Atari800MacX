/* DiskEditorWindow.h - DiskEditorWindow window 
   manager class header for the
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
*/
#import <Cocoa/Cocoa.h>
#import "DiskEditorDataSource.h"

@interface DiskEditorWindow : NSWindowController
{
	DiskEditorDataSource *directoryDataSource;
	int diskImageReadWrite;
	int diskImageSubdirs;
	int diskImageWriteProtect;
	int lfConvert;
	int imageEditorOpen;
	NSString *imageFilename;

    IBOutlet id directoryTableColumn;
    IBOutlet id directoryTableView;
    IBOutlet id diskImageCloseButton;
    IBOutlet id diskImageDeleteButton;
    IBOutlet id diskImageDirPulldown;
    IBOutlet id diskImageExportButton;
    IBOutlet id diskImageFreeSectorsField;
    IBOutlet id diskImageImportButton;
    IBOutlet id diskImageLfTranslateButton;
    IBOutlet id diskImageLockButton;
    IBOutlet id diskImageMkdirButton;
    IBOutlet id diskImageNameField;
    IBOutlet id diskImageRenameButton;
    IBOutlet id diskImageUnlockButton;
    IBOutlet id diskImageWriteProtectButton;
	IBOutlet id errorButton;
    IBOutlet id errorField;
	IBOutlet id diskImageNewNameSheet;
	IBOutlet id diskImageErrorSheet;
}
+(NSMutableArray *)getEditorArray;
+(DiskEditorWindow *)isAlreadyOpen:(NSString *)filename;
- (id)initWithWindowNibName:
	(NSString *)windowNibName:
	(DiskEditorDataSource *)dataSource:
	(NSString *)filename;
- (NSString *)filename;
- (NSArray *) browseFiles;
- (NSString *) browseDirectory;
- (void)diskImageEnableButtons;
- (IBAction)diskImageSelect:(id)sender;
- (IBAction)diskImageDoubleClick:(id)sender;
- (void)diskImageOpenDone;
- (IBAction)diskImageImport:(id)sender;
- (void)diskImageImportDrag:(NSArray *)filenames;
- (void)diskImageImportDragLocal:(NSArray *)filenames;
- (IBAction)diskImageExport:(id)sender;
- (IBAction)diskImageDelete:(id)sender;
- (IBAction)diskImageRename:(id)sender;
- (IBAction)diskImageMkdir:(id)sender;
- (IBAction)diskImageLock:(id)sender;
- (IBAction)diskImageUnlock:(id)sender;
- (IBAction)diskImageWriteProtect:(id)sender;
- (IBAction)diskImageDirChange:(id)sender;
- (IBAction)diskImageNameOK:(id)sender;
- (IBAction)diskImageNameCancel:(id)sender;
- (IBAction)diskImageTranslateChange:(id)sender;
- (BOOL)isDiskImageMounted;
- (void)diskImageMenuAdd:(NSString *) itemName;
- (void)diskImageMenuClear;
- (void)diskImageMenuDelete:(int) count;
- (void)diskImageDisplayFreeBytes:(int) free;
- (void)reloadDirectoryData;
- (int)getLfConvert;
- (int)getWriteProtect;
- (void)displayErrorWindow:(NSString *)errorMsg;
- (IBAction)errorOK:(id)sender;
- (void) allSelected:(int)count;
@end
