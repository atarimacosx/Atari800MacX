/* SectorEditorWindow.h - SectorEditorWindow window 
   manager class header for the
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
*/
#import <Cocoa/Cocoa.h>
#import "SectorEditorDataSource.h"

@interface SectorEditorWindow : NSWindowController
{
	SectorEditorDataSource *sectorDataSource;
	AtrDiskInfo *diskinfo;
	int dosType;
	int readWrite;
	int writeProtect;
	int currentSector;
	int maxSector;
	bool dirty;

    IBOutlet id diskImageWriteProtectButton;
    IBOutlet id sectorSaveButton;
    IBOutlet id sectorRevertButton;
    IBOutlet id sectorTableView;
    IBOutlet id headerTableColumn;
    IBOutlet id c00TableColumn;
    IBOutlet id c01TableColumn;
    IBOutlet id c02TableColumn;
    IBOutlet id c03TableColumn;
    IBOutlet id c04TableColumn;
    IBOutlet id c05TableColumn;
    IBOutlet id c06TableColumn;
    IBOutlet id c07TableColumn;
    IBOutlet id c08TableColumn;
    IBOutlet id c09TableColumn;
    IBOutlet id c10TableColumn;
    IBOutlet id c11TableColumn;
    IBOutlet id c12TableColumn;
    IBOutlet id c13TableColumn;
    IBOutlet id c14TableColumn;
    IBOutlet id c15TableColumn;
    IBOutlet id asciiTableColumn;
	IBOutlet id sectorField;
	IBOutlet id sectorStepper;
	IBOutlet id errorButton;
    IBOutlet id errorField;
	IBOutlet id sectorImageErrorSheet;
	IBOutlet id sectorNumberField;
	IBOutlet id confirmOKButton;
}
+ (SectorEditorWindow *)sharedInstance;
- (id)init;
- (void)showSectorPanel;
- (int) mountDisk:(NSString *)filename;
- (void) setControls;
- (void)displaySector;
- (void)setCellsEditable:(bool) editable;
- (AtrDiskInfo *)getDiskInfo;
- (void)reloadSectorData;
- (IBAction)revertSector:(id)sender;
- (IBAction)saveSector:(id)sender;
- (IBAction)diskImageWriteProtect:(id)sender;
- (IBAction)changeSector:(id)sender;
- (IBAction)displaySectorWindow:(id)sender;
- (IBAction)sectorNumberOK:(id)sender;
- (IBAction)sectorOK:(id)sender;
- (void)setDirty;
- (void)clearDirty;
- (bool)confirmDiscard;
- (IBAction)confirmOK:(id)sender;
- (IBAction)confirmCancel:(id)sender;
@end
