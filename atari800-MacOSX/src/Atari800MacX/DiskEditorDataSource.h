/* DiskEditorDataSource.h - DiskEditorDataSource 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */


#import <Cocoa/Cocoa.h>


@interface DiskEditorDataSource : NSObject {
	AtrDiskInfo *diskinfo;
	id owner;
	ADosFileEntry fileList[64];
	UWORD fileCount;
	int dosType;
	int readWrite;
	int subdirs;
	int writeProtect;
	ULONG freeBytes;
	int mounted;
	NSEvent *mouseEvent;
	NSString *dropPath;
	NSArray *dragRows;
	NSString *imageType;
	NSString *myId;
}

- (void) setOwner:(id)theOwner;
- (id) tableView:(NSTableView *) aTableView 
      objectValueForTableColumn:(NSTableColumn *)aTableColumn
	  row:(int) rowIndex;
- (int) numberOfRowsInTableView:(NSTableView *)aTableView;
- (void) reset;
- (NSString *) mountDisk:(NSString *)filename;
- (NSString *) getDiskInfo:(int *)readWriteFlag:
						   (int *) subdirsFlag:
						   (int *) writeProtectFlag;
- (int) importFile:(NSString *)filename:(int) lfConvert:(int) tabConvert;
- (int) exportFile:(int) index:(NSString *)dirName:(int) lfConvert:(int) tabConvert;
- (int) changeDirectory:(int) index;
- (int) changeDirectoryUp:(int) count;
- (int) deleteFile:(int) index;
- (int) unlockFile:(int) index;
- (int) lockFile:(int) index;
- (int) renameFile:(int) index:(NSString *) newName;
- (int) writeProtect:(int) protect;
- (int) createDirectory:(NSString *) name;
- (int) isLocked:(int) index;
- (int) isDirectory:(int) index;
- (void) getFilename:(int) index:(char *)filename;
- (void) displayError:(int)errorCode;
- (NSString *)translateCodeToMsg:(int)errorCode;
- (BOOL)isMounted;
- (BOOL)tableView:(NSTableView *)tableView writeRows:(NSArray *)rows 
			toPasteboard:(NSPasteboard *)pboard;
- (NSDragOperation)tableView:(NSTableView *)tableView 
					validateDrop:(id <NSDraggingInfo>)info 
					proposedRow:(int)row 
					proposedDropOperation:(NSTableViewDropOperation)operation;
- (BOOL)tableView:(NSTableView *)tableView 
		acceptDrop:(id <NSDraggingInfo>)info 
		row:(int)row 
		dropOperation:(NSTableViewDropOperation)operation;	
- (void)saveEvent:(NSEvent *)theEvent;
- (void)allSelected;

@end
