/* DiskEditorDataSource.m - 
 DiskEditorDataSource class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 */

#import "atari.h"
#import "atrUtil.h"
#import "atrErr.h"
#import "atrMount.h"
#import "DiskEditorWindow.h"
#import "DiskEditorDataSource.h"
#import <unistd.h>

static int idCounter = 0;

@implementation DiskEditorDataSource
-(id) init
{
	mounted = 0;
	fileCount = 0;
	return self;
}

-(void) setOwner:(id)theOwner
{
	owner = theOwner;
}

-(id) tableView:(NSTableView *) aTableView 
      objectValueForTableColumn:(NSTableColumn *)aTableColumn
	  row:(int) rowIndex
	{
    char XEName[20];
    char *XENamePtr;
	char *XEPrintPtr;
	char dirEntry[64];
	int  j;
    static char month[13][3] = {"  ","JA","FB","MR","AP","MA","JN",
                                "JL","AG","SE","OC","NO","DE"};

    if (dosType == DOS_SPARTA2) {
             if ((fileList[rowIndex].flags & 0x30) == 0x30)
                 sprintf(dirEntry,"* %11s  <DIR>\n",
                        fileList[rowIndex].aname);
             else if (fileList[rowIndex].flags & 0x20)
                 sprintf(dirEntry,"* %11s %6ld %02d/%02d/%02d %02d:%02d\n",
                        fileList[rowIndex].aname,(long int) fileList[rowIndex].bytes,
                        fileList[rowIndex].day, fileList[rowIndex].month, 
                        fileList[rowIndex].year,
                        fileList[rowIndex].hour, fileList[rowIndex].minute);
             else if (fileList[rowIndex].flags & 0x10)
                 sprintf(dirEntry,"  %11s  <DIR>\n",
                        fileList[rowIndex].aname);
             else
                 sprintf(dirEntry,"  %11s %6ld %02d/%02d/%02d %02d:%02d\n",
                        fileList[rowIndex].aname,(long int) fileList[rowIndex].bytes,
                        fileList[rowIndex].day, fileList[rowIndex].month, 
                        fileList[rowIndex].year,
                        fileList[rowIndex].hour, fileList[rowIndex].minute);
        }
    else if (dosType == DOS_ATARIXE) {
			 XEPrintPtr = dirEntry;
             if (fileList[rowIndex].flags & 0x20)
                 XEPrintPtr += sprintf(XEPrintPtr,"* ");
             else
                 XEPrintPtr += sprintf(XEPrintPtr,"  ");
                        
             XENamePtr = XEName;
             for (j=0;j<8;j++) {
                 if (fileList[rowIndex].aname[j] != ' ')
                     *XENamePtr++ = fileList[rowIndex].aname[j];
                  }
             *XENamePtr++ = '.';
             for (j=8;j<11;j++) {
                  if (fileList[rowIndex].aname[j] != ' ')
                      *XENamePtr++ = fileList[rowIndex].aname[j];
                 }
             if (fileList[rowIndex].flags & 0x10)
                 *XENamePtr++ = '>';
             *XENamePtr = 0;
             XEPrintPtr += sprintf(XEPrintPtr,"%-15s  %02d%2s%02d %02d%2s%02d",
                                   XEName,
                                   fileList[rowIndex].createdDay, 
                                   month[fileList[rowIndex].createdMonth], 
                                   fileList[rowIndex].createdYear,
                                   fileList[rowIndex].day, 
								   month[fileList[rowIndex].month], 
                                   fileList[rowIndex].year);
              if (fileList[rowIndex].flags & 0x10)
                  XEPrintPtr += sprintf(XEPrintPtr,"       >\n");
              else
                  XEPrintPtr += sprintf(XEPrintPtr,"  %6ld\n",(long int) fileList[rowIndex].bytes);
              }
     else {
              if ((fileList[rowIndex].flags & 0x30) == 0x30)
                  sprintf(dirEntry,"*:%11s %04d\n",fileList[rowIndex].aname,
                         fileList[rowIndex].sectors);
              else if (fileList[rowIndex].flags & 0x20)
                  sprintf(dirEntry,"* %11s %04d\n",fileList[rowIndex].aname,
                         fileList[rowIndex].sectors);
              else if (fileList[rowIndex].flags & 0x10)
                  sprintf(dirEntry," :%11s %04d\n",fileList[rowIndex].aname,
                         fileList[rowIndex].sectors);
              else
                  sprintf(dirEntry,"  %11s %04d\n",fileList[rowIndex].aname,
                         fileList[rowIndex].sectors);
         }
	
	return([NSString stringWithCString:dirEntry encoding:NSASCIIStringEncoding]);
	}
	
-(int) numberOfRowsInTableView:(NSTableView *)aTableView
	{
	return(fileCount);
	}

-(void) reset
	{
	if (mounted)
		AtrUnmount(diskinfo);
	}

- (NSString *) mountDisk:(NSString *)filename
	{
	int status = 0;
	char cfilename[FILENAME_MAX];
	
	[filename getCString:cfilename maxLength:FILENAME_MAX encoding:NSUTF8StringEncoding];
	if ((status = AtrMount(cfilename ,&dosType, &readWrite, &writeProtect,&diskinfo)))
        {
		AtrUnmount(diskinfo);
		mounted = 0;
		fileCount = 0;
        }
	else
		{
		mounted = 1;
		switch(dosType) {
			case DOS_UNKNOWN:
			default:
				AtrUnmount(diskinfo);
				mounted = 0;
				fileCount = 0;
				status = ADOS_UNKNOWN_FORMAT;
				break;
			case DOS_ATARI1:
				imageType = @"Atari DOS 1.0 Image";
				subdirs = 0;
                break;
			case DOS_ATARI:
				imageType = @"Atari DOS 2.x Image";
				subdirs = 0;
				break;
			case DOS_TOP:
				imageType = @"TopDos Image";
				subdirs = 0;
                break;
            case DOS_BIBO:
				imageType = @"BiboDos Image";
				subdirs = 0;
                break;
			case DOS_ATARI3:
				imageType = @"Atari DOS 3.0 Image";
				subdirs = 0;
				break;
			case DOS_ATARI4:
				imageType = @"Atari DOS 4.0 Image";
				subdirs = 0;
				break;
			case DOS_ATARIXE:
				imageType = @"Atari DOS XE Image";
				subdirs = 1;
				break;
			case DOS_MYDOS:
				imageType = @"MYDOS Image        ";
				subdirs = 1;
				break;
			case DOS_SPARTA2:
				imageType = @"SpartaDOS 2 Compatible Image  ";
				subdirs = 1;
				break;
			}
			
		}
	
	if (status) {
		return([self translateCodeToMsg:status]);
		}
	else
		return(nil);
	}
	
- (NSString *) getDiskInfo:(int *)readWriteFlag:
						   (int *) subdirsFlag:
						   (int *) writeProtectFlag
{
	int status;
	
	*readWriteFlag = readWrite;
	*subdirsFlag = subdirs;
	*writeProtectFlag = writeProtect;
	
	status = AtrGetDir(diskinfo, &fileCount, fileList, &freeBytes);

	if (status == 0) {
		if (!(readWrite))
			imageType = [imageType stringByAppendingString:@" Read-Only"];
		[owner diskImageDisplayFreeBytes:freeBytes];
		}

	[owner reloadDirectoryData];
		
	if (status) {
		[self displayError:status];
		return(nil);
		}
	else
		return(imageType);
}
	
	
- (int) importFile:(NSString *)filename:(int) lfConvert:(int) tabConvert
{
	char cstring[FILENAME_MAX];
	int status;
	
	[filename getCString:cstring maxLength:FILENAME_MAX encoding:NSUTF8StringEncoding];
	
	status = AtrImportFile(diskinfo, cstring, lfConvert, tabConvert);
	
	if (status == 0) {
		status = AtrGetDir(diskinfo, &fileCount, fileList, &freeBytes);
		[owner diskImageDisplayFreeBytes:freeBytes];
		}

	[owner reloadDirectoryData];

	if (status)
		[self displayError:status];
	
	return(status);
}

- (int) exportFile:(int) index:(NSString *)dirName:(int) lfConvert:(int) tabConvert
{
	char fileName[13];
	char cname[FILENAME_MAX];
	int status;
	
	if (![self isDirectory:index]) {
		[self getFilename:index:fileName];
		[dirName getCString:cname maxLength:FILENAME_MAX encoding:NSUTF8StringEncoding];
		strcat(cname,"/");
		strcat(cname, fileName);
	
		status = AtrExportFile(diskinfo, fileName,cname,lfConvert,tabConvert);
	
		if (status)
			[self displayError:status];
	
		return(status);
		}
	else
		return(0);
}

- (int) changeDirectory:(int) index
{
	char dirName[13];
	int status;
	
	[self getFilename:index:dirName];
	
	status = AtrChangeDir(diskinfo, CD_NAME, dirName);
	
	if (status == 0) {
		[owner 
			diskImageMenuAdd:[NSString stringWithCString:dirName encoding:NSASCIIStringEncoding]];
        status = AtrGetDir(diskinfo, &fileCount, fileList, &freeBytes);
		}

	[owner reloadDirectoryData];

	if (status)
		[self displayError:status];
	
	return(status);
}

- (int) changeDirectoryUp:(int) count
{
	int i, status = 0 ;

    if (count == 0)
		return(FALSE);
		
	if (count == -1) {
		status = AtrChangeDir(diskinfo, CD_ROOT, NULL);
		
		if (status == 0) {
			[owner diskImageMenuClear];
			status = AtrGetDir(diskinfo, &fileCount, fileList, &freeBytes);
			}
		}
	else {
		for (i=0;i<count;i++) {
			status = AtrChangeDir(diskinfo, CD_UP, NULL);
			if (status)
				break;
			}
		if (status == 0) {
			[owner diskImageMenuDelete:count];
			status = AtrGetDir(diskinfo, &fileCount, fileList, &freeBytes);
			}
		}

	[owner reloadDirectoryData];

	if (status)
		[self displayError:status];
	
	return(status);
}

- (int) deleteFile:(int) index
{
	char dirName[13];
	int status;
	
	if (![self isLocked:index]) {
		[self getFilename:index:dirName];
	
		if ([self isDirectory:index])
			status = AtrDeleteDir(diskinfo, dirName);
		else
			status = AtrDeleteFile(diskinfo, dirName);
	
		if (status == 0) {
			status = AtrGetDir(diskinfo, &fileCount, fileList, &freeBytes);
			[owner diskImageDisplayFreeBytes:freeBytes];
			}

		[owner reloadDirectoryData];

		if (status)
			[self displayError:status];
	
		return(status);
		}
	else
		return(0);
}

- (int) lockFile:(int) index
{
	char dirName[13];
	int status;
	
	if (![self isDirectory:index]) {
		[self getFilename:index:dirName];
	
		status = AtrLockFile(diskinfo, dirName,1);
	
		if (status == 0) {
			status = AtrGetDir(diskinfo, &fileCount, fileList, &freeBytes);
			[owner diskImageDisplayFreeBytes:freeBytes];
			}

		[owner reloadDirectoryData];

		if (status)
			[self displayError:status];
	
		return(status);
		}
	else
		return(0);
}

- (int) unlockFile:(int) index
{
	char dirName[13];
	int status;
	
	if (![self isDirectory:index]) {
		[self getFilename:index:dirName];
	
		status = AtrLockFile(diskinfo, dirName,0);
	
		if (status == 0) {
			status = AtrGetDir(diskinfo, &fileCount, fileList, &freeBytes);
			[owner diskImageDisplayFreeBytes:freeBytes];
			}

		[owner reloadDirectoryData];

		if (status)
			[self displayError:status];
	
		return(status);
		}
	else
		return(0);
}

- (int) renameFile:(int) index:(NSString *) newName
{
	int status;
	char cName[FILENAME_MAX];
	char dirName[13];
	
	[newName getCString:cName maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
	
	if ([self isDirectory:index])
		if (strlen(cName) > 8)
			cName[8] = 0;
	
	[self getFilename:index:dirName];
		
	status = AtrRenameFile(diskinfo, dirName, cName);
	
	if (status == 0) {
		status = AtrGetDir(diskinfo, &fileCount, fileList, &freeBytes);
		[owner diskImageDisplayFreeBytes:freeBytes];
		}

	[owner reloadDirectoryData];

	if (status)
		[self displayError:status];
	
	return(status);
}

- (int) writeProtect:(int) protect
{
	int status;
	
	status = AtrSetWriteProtect(diskinfo, protect);
	
	if (status)
		[self displayError:status];
	
	return(status);
}

- (int) createDirectory:(NSString *) name
{
	int status;
	char cName[FILENAME_MAX];
	
	[name getCString:cName maxLength:FILENAME_MAX encoding:NSASCIIStringEncoding];
	if (strlen(cName) > 8)
		cName[8] = 0;
		
	status = AtrMakeDir(diskinfo, cName);
	
	if (status == 0) {
		status = AtrGetDir(diskinfo, &fileCount, fileList, &freeBytes);
		[owner diskImageDisplayFreeBytes:freeBytes];
		}

	[owner reloadDirectoryData];


	if (status)
		[self displayError:status];
	
	return(status);
}

- (int) isLocked:(int) index
{
	return((fileList[index].flags & DIRE_LOCKED) == DIRE_LOCKED);
}

- (int) isDirectory:(int) index
{
	return((fileList[index].flags & DIRE_SUBDIR) == DIRE_SUBDIR);
}

- (void) getFilename:(int) index:(char *)filename
{
	int i;
	
	for (i=0;i<8;i++) {
		if (fileList[index].aname[i] == ' ')
			break;
		*filename++ = fileList[index].aname[i];
		}
		
	if (fileList[index].aname[8] != ' ') {
		*filename++ = '.';
		
		for (i=8;i<11;i++) {
			if (fileList[index].aname[i] == ' ')
				break;
			*filename++ = fileList[index].aname[i];
			}
		}
	*filename = 0;
}

/*------------------------------------------------------------------------------
*  displayError - This method displays an error dialog box, based on
*     the passed in error code.
*-----------------------------------------------------------------------------*/
- (void)displayError:(int)errorCode {
	NSString *errorMsg;
	
	errorMsg = [self translateCodeToMsg:errorCode];
	[owner displayErrorWindow:errorMsg];
	}

/*------------------------------------------------------------------------------
*  displayError - This method displays an error dialog box, based on
*     the passed in error code.
*-----------------------------------------------------------------------------*/
- (NSString *)translateCodeToMsg:(int)errorCode {
	NSString *errorMsg;
	
	switch(errorCode) {
        case ADOS_UNKNOWN_FORMAT:
        	errorMsg = @"Unsupported DOS Format!";
        	break;
        case ADOS_FILE_NOT_FOUND:
            errorMsg = @"File not found on image!";
            break;
        case ADOS_NOT_A_ATR_IMAGE:
            errorMsg = @"Not a valid image file!";
            break;
        case ADOS_FILE_LOCKED:
        	errorMsg = @"File on image is locked!";
        	break;
        case ADOS_NOT_A_DIRECTORY:
            errorMsg = @"Not a directory!";
            break;
        case ADOS_DIR_READ_ERR:
        	errorMsg = @"Error Reading Directory on image!";
        	break;
        case ADOS_DIR_WRITE_ERR:
        	errorMsg = @"Error Writing Directory on image!";
        	break;
        case ADOS_DIR_NOT_EMPTY:
            errorMsg = @"Can't Delete Directory which isn't empty!";
            break;
        case ADOS_FILE_IS_A_DIR:
            errorMsg = @"Cannot perform operation on a directory!";
            break;
        case ADOS_DUPLICATE_NAME:
            errorMsg = @"That file/directory name already exists!";
            break;
        case ADOS_VTOC_READ_ERR:
        	errorMsg = @"Error Reading Table of Contents on image!";
        	break;
        case ADOS_VTOC_WRITE_ERR:
        	errorMsg = @"Error Writing Table of Contents on image!";
        	break;
        case ADOS_FILE_CORRUPTED:
        	errorMsg = @"File on image is corrputed!";
        	break;
        case ADOS_DELETE_FILE_ERR:
        	errorMsg = @"Delete failed, file locked?";
        	break;
        case ADOS_DISK_FULL:
        	errorMsg = @"Image file does not have enough space!";
        	break;
        case ADOS_DIR_FULL:
        	errorMsg = @"Directory on image is full!";
        	break;
        case ADOS_FILE_WRITE_ERR:
        	errorMsg = @"Error writing file on image!";
        	break;
        case ADOS_FILE_READ_ERR:
        	errorMsg = @"Error reading file on image!";
        	break;
        case ADOS_MEM_ERR:
        	errorMsg = @"Memory allocation error!";
        	break;
        case ADOS_HOST_FILE_NOT_FOUND:
        	errorMsg = @"File not found on host!";
        	break;
        case ADOS_HOST_CREATE_ERR:
        	errorMsg = @"Unable to create file on Mac!";
        	break;
        case ADOS_HOST_READ_ERR:
        	errorMsg = @"Error reading file on Mac!";
        	break;
        case ADOS_HOST_WRITE_ERR:
        	errorMsg = @"Error writing file on Mac!";
        	break;
        case ADOS_FUNC_NOT_SUPPORTED:
            errorMsg = @"Function not support on image DOS type!";
            break;
        default:
            errorMsg = @"Unknown Error code!";
        	break;
		}
	return errorMsg;
}

- (BOOL)isMounted
{
	if (mounted)
		return(YES);
	else
		return(NO);
}

- (NSDragOperation)tableView:(NSTableView *)tableView 
					validateDrop:(id <NSDraggingInfo>)info 
					proposedRow:(int)row 
					proposedDropOperation:(NSTableViewDropOperation)operation
{
    NSPasteboard *pboard;
    NSDragOperation sourceDragMask;
	NSArray *types;

    sourceDragMask = [info draggingSourceOperationMask];
    pboard = [info draggingPasteboard];
	
	if ([owner getWriteProtect] || !readWrite)
		return NSDragOperationNone;
	
	if ([[info draggingSource] isEqual:self])
		return NSDragOperationNone;
		
    /* Check for filenames type drag */
    if ( [[pboard types] containsObject:NSFilenamesPboardType] ) {
		if (sourceDragMask & NSDragOperationCopy)
			return NSDragOperationCopy; 
		if (sourceDragMask & NSDragOperationMove)
			return NSDragOperationMove; 
		}
#ifndef OSX_10_1_COMPAT	
    else if ( [[pboard types] containsObject:NSFilesPromisePboardType] ) {
		types = [pboard propertyListForType:NSFilesPromisePboardType];
		if (([types count] == 2) &&
		    [[types objectAtIndex:1] isEqual:myId])
			return NSDragOperationNone;
		if (sourceDragMask & NSDragOperationCopy)
			return NSDragOperationCopy; 
		if (sourceDragMask & NSDragOperationMove)
			return NSDragOperationMove; 
		}
#endif		
    return NSDragOperationNone;
}

- (BOOL)tableView:(NSTableView *)tableView 
		acceptDrop:(id <NSDraggingInfo>)info 
		row:(int)row 
		dropOperation:(NSTableViewDropOperation)operation	
{
    NSPasteboard *pboard;
    NSDragOperation sourceDragMask;
	static NSURL *fileURL=nil;
	char cname[FILENAME_MAX];
	int i;

	if (fileURL == nil)
		fileURL = [[NSURL alloc] initFileURLWithPath:@"/private/tmp"];

    sourceDragMask = [info draggingSourceOperationMask];
    pboard = [info draggingPasteboard];

    /* Check for filenames type drag */
    if ( [[pboard types] containsObject:NSFilenamesPboardType] ) {
		NSArray *files = [pboard propertyListForType:NSFilenamesPboardType];

		/* Import the files into the emulator */
		[owner diskImageImportDrag:files];
    }
#ifndef OSX_10_1_COMPAT	
    /* Check for promises type drag */
    else if ( [[pboard types] containsObject:NSFilesPromisePboardType] ) {
		NSArray *files = [info namesOfPromisedFilesDroppedAtDestination:fileURL];
		/* Import the files into the emulator */
		[owner diskImageImportDragLocal:files];
		
		/* Delete the files, since they are no longer needed */
		for (i=0;i<[files count];i++) {
			[[files objectAtIndex:i] getCString:cname maxLength:FILENAME_MAX encoding:NSUTF8StringEncoding ];
			unlink(cname);
			}
		
    }
#endif	
    return YES;
}						

- (BOOL)tableView:(NSTableView *)tableView writeRows:(NSArray *)rows 
			toPasteboard:(NSPasteboard *)pboard
{
    NSPoint dragPosition;
    NSRect imageLocation;
	int i,containsDirectory = 0;
	char myIdStr[10];
	
	for (i=0;i<[rows count];i++) {
		if ([self isDirectory:[[rows objectAtIndex:i] intValue]])
			containsDirectory = 1;
		}
		
	if (!containsDirectory) {
		dragRows = rows;
	
		dragPosition = [tableView convertPoint:[mouseEvent locationInWindow]
							fromView:nil];
		dragPosition.x -= 16;
		dragPosition.y -= 16;
		imageLocation.origin = dragPosition;
		imageLocation.size = NSMakeSize(32,32);
		sprintf(myIdStr,"id%04d",idCounter++);
		myId = [NSString stringWithCString:myIdStr encoding:NSASCIIStringEncoding];
		if (!(floor(NSAppKitVersionNumber) <= 663)) { 
			[tableView dragPromisedFilesOfTypes:[NSArray arrayWithObjects:@"txt",myId,nil]
					fromRect:imageLocation
					source:self
					slideBack:YES
					event:mouseEvent]; 
			}
		}
	return(NO);
}

- (NSArray *)namesOfPromisedFilesDroppedAtDestination:(NSURL *)dropDestination
{
	int i;
	NSMutableArray *filenames;
	char fileName[13];
	
	if (![[dropDestination path] isEqual:@"/private/tmp"]) {
		dropPath = [dropDestination path];
		return nil;
		}
	else {
		filenames = [[[NSMutableArray alloc] initWithCapacity:[dragRows count]] autorelease];

		dropPath = nil;
		for (i=0;i<[dragRows count];i++) { 
			[self exportFile:[[dragRows objectAtIndex:i] intValue]:
             @"/private/tmp":0:0];
			[self getFilename:[[dragRows objectAtIndex:i] intValue]:fileName];
			[filenames addObject:
					[@"/tmp/" stringByAppendingString:[NSString stringWithCString:fileName encoding:NSASCIIStringEncoding]]];
			}
		return(filenames);
		}
}

- (void)draggedImage:(NSImage *)anImage endedAt:(NSPoint)aPoint operation:(NSDragOperation)operation
{
	int i;
	
	if (dropPath != nil) {
		if (operation != NSDragOperationNone) {
			for (i=0;i<[dragRows count];i++) 
				[self exportFile:[[dragRows objectAtIndex:i] intValue]:
								dropPath:
                 [owner getLfConvert]:[owner getTabConvert]];
			}
		}
		
}

- (void)saveEvent:(NSEvent *)theEvent
{
	mouseEvent = theEvent;
}

-(NSDragOperation)draggingSourceOperationMaskForLocal:(BOOL)isLocal
{
	return(NSDragOperationCopy);
}

- (void)allSelected
{
	[owner allSelected:fileCount];
}

@end
