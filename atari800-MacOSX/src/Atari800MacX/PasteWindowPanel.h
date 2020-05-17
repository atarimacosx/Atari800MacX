/* PasteWindowPanel.h - PasteWindowPanel joint 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */


/*------------------------------------------------------------------------------
 *  validateUserInterfaceItem - Verifies there is text in the clipboard to paste
 *-----------------------------------------------------------------------------*/
- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)anItem
{
    SEL theAction = [anItem action];
	NSPasteboard *pb = [NSPasteboard generalPasteboard];
	NSArray *pasteTypes = [NSArray arrayWithObjects: NSStringPboardType, nil];
	NSString *bestType = [pb availableTypeFromArray:pasteTypes];
	
    if (theAction == @selector(paste:))
    {
        if (bestType != nil)
        {
            return YES;
        }
        return NO;
    } 
    else if (theAction == @selector(copy:))
    {
		if (IsCopyDefined())
			return YES;
		else
			return NO;
    } 
    else if (theAction == @selector(selectAll:))
    {
		return YES;
    } 
	else
		return YES;
}

/*------------------------------------------------------------------------------
 *  paste - Starts Paste from Mac to Atari
 *-----------------------------------------------------------------------------*/
- (void) paste:(id) sender
{
	requestPaste = 1;
}

/*------------------------------------------------------------------------------
 *  selectAll - Starts selectAll from Mac to Atari
 *-----------------------------------------------------------------------------*/
- (void) selectAll:(id) sender
{
	requestSelectAll = 1;
}

/*------------------------------------------------------------------------------
 *  copy - Starts copy from Mac to Atari
 *-----------------------------------------------------------------------------*/
- (void) copy:(id) sender
{
	requestCopy = 1;
}

