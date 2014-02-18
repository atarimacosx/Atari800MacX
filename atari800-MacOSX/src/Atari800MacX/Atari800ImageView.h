/* Atari800ImageView.h - ImageView class to
   support Drag and Drop to the disk drive image.
   For the Macintosh OS X SDL port 
   of Atari800
   Mark Grebe <atarimac@cox.net>
   
*/
#import <Cocoa/Cocoa.h>

/* Subclass of NSIMageView to allow for drag and drop and other specific functions  */
@interface Atari800ImageView : NSImageView 
- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender;
- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender;
- (void)mouseDown:(NSEvent *)theEvent;
- (unsigned int)draggingSourceOperationMaskForLocal:(BOOL)isLocal;
- (void)draggedImage:(NSImage *)anImage endedAt:(NSPoint)aPoint operation:(NSDragOperation)operation;

@end

