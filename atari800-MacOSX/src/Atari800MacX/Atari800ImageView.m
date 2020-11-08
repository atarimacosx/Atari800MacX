/* Atari800ImageView.m - ImageView class to
   support Drag and Drop to the disk drive image.
   For the Macintosh OS X SDL port 
   of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   
*/
#import "Atari800ImageView.h"
#import "Preferences.h"
#import "MediaManager.h"
#import "cartridge.h"
#import "sio.h"

extern char SIO_filename[SIO_MAX_DRIVES][FILENAME_MAX];
extern int showUpperDrives;
extern NSImage *disketteImage;


/* Subclass of NSIMageView to allow for drag and drop and other specific functions  */

@implementation Atari800ImageView

static char fileToCopy[FILENAME_MAX];

/*------------------------------------------------------------------------------
*  init - Registers for a drag and drop to this window. 
*-----------------------------------------------------------------------------*/
-(id) init
{
	id me;
	
	me = [super init];
	
	[ self registerForDraggedTypes:[NSArray arrayWithObjects:
            NSFilenamesPboardType, nil]]; // Register for Drag and Drop
			
	return(me);
}

/*------------------------------------------------------------------------------
*  mouseDown - Start a drag from one of the drives. 
*-----------------------------------------------------------------------------*/
- (void)mouseDown:(NSEvent *)theEvent
{
   int tag;
   NSImage *dragImage;
   NSPoint dragPosition;
   
   tag = [self tag];

   if (tag < 4) {
		if (showUpperDrives)
			tag += 4;

		if (SIO_drive_status[tag] == SIO_READ_ONLY || SIO_drive_status[tag] == SIO_READ_WRITE) {
            NSURL *fileURL = [NSURL fileURLWithPath: [NSString stringWithCString:SIO_filename[tag] encoding:NSUTF8StringEncoding]];
            NSDraggingItem* dragItem = [[NSDraggingItem alloc] initWithPasteboardWriter:fileURL];

            dragImage = disketteImage;
            dragPosition = [self convertPoint:[theEvent locationInWindow]
                            fromView:nil];
            dragPosition.x -= 32;

            NSRect dragFrame;
            dragFrame.origin = dragPosition;
            dragFrame.size = dragImage.size;

            [dragItem setDraggingFrame:dragFrame contents:dragImage];
            
            NSArray *draggingItems = [NSArray arrayWithObject:dragItem];

            [self beginDraggingSessionWithItems:draggingItems event:theEvent source:self];
			}
		}
}

- (NSDragOperation)draggingSession:(NSDraggingSession *)session
sourceOperationMaskForDraggingContext:(NSDraggingContext)context
    {
    if (context == NSDraggingContextWithinApplication)
        return NSDragOperationCopy | NSDragOperationMove;
    else
        return NSDragOperationNone;
    }

/*------------------------------------------------------------------------------
*  draggingDone - Runs to cleanup source image when
*  image has been dropped on destination.
*-----------------------------------------------------------------------------*/
- (void)draggingDone:(int) driveNo operation:(NSDragOperation)operation
{
	if (showUpperDrives)
		driveNo += 4;

	if (operation & NSDragOperationCopy)
		{
		/* If there is no disk in the destination drive, it's the same as a move */
		if (strcmp("Off",fileToCopy) == 0 || strcmp("Empty",fileToCopy) == 0)
			[[MediaManager sharedInstance] diskRemoveKey:(driveNo + 1)];
		else
			[[MediaManager sharedInstance] 
				diskNoInsertFile:[NSString stringWithCString:fileToCopy encoding:NSUTF8StringEncoding]:driveNo];
		}
	else if (operation & NSDragOperationMove)
		{
		[[MediaManager sharedInstance] diskRemoveKey:(driveNo + 1)];
		}
}

/*------------------------------------------------------------------------------
*  draggingEntered - Checks for a valid drag and drop to this disk drive. 
*-----------------------------------------------------------------------------*/
- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender {
    NSPasteboard *pboard;
    NSDragOperation sourceDragMask;
    int filecount;
    NSString *suffix;

    sourceDragMask = [sender draggingSourceOperationMask];
    pboard = [sender draggingPasteboard];
    
    /* Check for filenames type drag */
    if ( [[pboard types] containsObject:NSFilenamesPboardType] ) {
        /* Check for copy being valid */
        if (sourceDragMask & NSDragOperationCopy ||
		    sourceDragMask & NSDragOperationMove) {
            /* Check here for valid file types */
            NSArray *files = [pboard propertyListForType:NSFilenamesPboardType];
            
            filecount = [files count];
			
			if (filecount != 1)
				return NSDragOperationNone;
			
            suffix = [[files objectAtIndex:0] pathExtension];
            
			if ([self tag] < 4)
				if (!([suffix isEqualToString:@"atr"] ||
					  [suffix isEqualToString:@"ATR"] ||
					  [suffix isEqualToString:@"atx"] ||
					  [suffix isEqualToString:@"ATX"] ||
					  [suffix isEqualToString:@"pro"] ||
					  [suffix isEqualToString:@"PRO"] ||
					[suffix isEqualToString:@"xfd"] ||
					[suffix isEqualToString:@"XFD"] ||
					[suffix isEqualToString:@"dcm"] ||
					[suffix isEqualToString:@"DCM"]) ||
					[[files objectAtIndex:0] 
						isEqualToString:[NSString stringWithCString:SIO_filename[[self tag]] encoding:NSUTF8StringEncoding]])
					return NSDragOperationNone;
			if ([self tag] == 8)
				if (!([suffix isEqualToString:@"cas"] ||
					[suffix isEqualToString:@"CAS"]))
					return NSDragOperationNone;
			if ([self tag] == 9)
				if (!([suffix isEqualToString:@"car"] ||
					[suffix isEqualToString:@"CAR"] ||
                    [suffix isEqualToString:@"rom"] ||
                    [suffix isEqualToString:@"ROM"] ||
                    [suffix isEqualToString:@"bin"] ||
                    [suffix isEqualToString:@"BIN"] ||
                    [suffix isEqualToString:@"img"] ||
                    [suffix isEqualToString:@"IMG"] ||
                    [suffix isEqualToString:@"vhd"] ||
					[suffix isEqualToString:@"VHD"] ) &&
                     (CARTRIDGE_main.type == CARTRIDGE_SIDE2))
					return NSDragOperationNone;
            }
		if (sourceDragMask & NSDragOperationMove)
			return NSDragOperationMove; 
		if (sourceDragMask & NSDragOperationCopy)
			return NSDragOperationCopy; 
    }
    return NSDragOperationNone;
}

/*------------------------------------------------------------------------------
*  performDragOperation - Executes the actual drap and drop of a filename onto
*     a disk drive. 
*-----------------------------------------------------------------------------*/
- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
    NSPasteboard *pboard;
    NSDragOperation sourceDragMask;
	int driveNo;
    id source;
    NSString *suffix;

    sourceDragMask = [sender draggingSourceOperationMask];
    pboard = [sender draggingPasteboard];
    source = [sender draggingSource];

    /* Check for filenames type drag */
    if ( [[pboard types] containsObject:NSFilenamesPboardType] ) {
       NSArray *files = [pboard propertyListForType:NSFilenamesPboardType];
       suffix = [[files objectAtIndex:0] pathExtension];

       /* Load the first file into the emulator */
	   if ([self tag] < 4) {
			if (showUpperDrives)
				driveNo = [self tag] + 4;
			else
				driveNo = [self tag];
			if (sourceDragMask & NSDragOperationCopy) {
				strcpy(fileToCopy,SIO_filename[driveNo]);
				}
			[[MediaManager sharedInstance] diskNoInsertFile:[files objectAtIndex:0]:driveNo];
            if (source)
                [self draggingDone:[source tag] operation:sourceDragMask];
			} 
	   else if ([self tag] == 8)
			[[MediaManager sharedInstance] cassInsertFile:[files objectAtIndex:0]]; 
       else if ([self tag] == 9) {
           if ((CARTRIDGE_main.type == CARTRIDGE_SIDE2) &&
               ([suffix isEqualToString:@"img"] ||
                [suffix isEqualToString:@"IMG"] ||
                [suffix isEqualToString:@"vhd"] ||
                [suffix isEqualToString:@"VHD"])) {
               [[MediaManager sharedInstance] side2AttachCFFile:[files objectAtIndex:0]];
           } else {
           [[MediaManager sharedInstance] cartInsertFile:[files objectAtIndex:0]];
            }
       }
    }
    return YES;
}

@end
