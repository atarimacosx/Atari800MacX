//
//  Atari800WindowController.m
//  Atari800MacX
//
//  Created by markg on 8/11/20.
//

#import "Atari800WindowController.h"
#import "ControlManager.h"

#define ColdButtonIdentifier @"ColdButton"
#define WarmButtonIdentifier @"WarmButton"
#define SelectButtonIdentifier @"SelectButton"
#define OptionButtonIdentifier @"OptionButton"
#define StartButtonIdentifier @"StartButton"
#define ClearButtonIdentifier @"ClearButton"
#define InverseButtonIdentifier @"InverseButton"
#define PopoverIdentifier @"Popover"
#define InsertCharButtonIdentifier @"InsertCharButton"
#define InsertLineButtonIdentifier @"InsertLineButton"
#define DeleteCharButtonIdentifier @"DeleteCharButton"
#define DeleteLineButtonIdentifier @"DeleteLineButton"
#define F1ButtonIdentifier @"F1Button"
#define F2ButtonIdentifier @"F2Button"
#define F3ButtonIdentifier @"F3Button"
#define F4ButtonIdentifier @"F4Button"

@interface Atari800WindowController ()

@end

@implementation Atari800WindowController

- (id)init:(NSWindow *) nswindow
    {
    self = [super initWithWindow:nswindow];
    return self;
    }

- (NSTouchBar *)makeTouchBar
    {
    NSTouchBar *bar = [[NSTouchBar alloc] init];
    bar.delegate = self;
        
    // Set the default ordering of items.
    bar.defaultItemIdentifiers =
        @[ColdButtonIdentifier,WarmButtonIdentifier,OptionButtonIdentifier,SelectButtonIdentifier,StartButtonIdentifier,ClearButtonIdentifier, InverseButtonIdentifier,PopoverIdentifier,NSTouchBarItemIdentifierOtherItemsProxy];
    
    return bar;
    }

// This gets called while the NSTouchBar is being constructed, for each NSTouchBarItem to be created.
- (nullable NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
{
    if ([identifier isEqualToString:ColdButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"Power", @"") target:[ControlManager sharedInstance] action:@selector(coldReset:)];
        [theLabel sizeToFit];
        theLabel.target = [ControlManager sharedInstance];
        theLabel.action = @selector(coldReset:);
        
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:ColdButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityHigh;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:WarmButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"Reset", @"") target:[ControlManager sharedInstance] action:@selector(warmReset:)];

        theLabel.bezelColor = [NSColor colorWithSRGBRed:0.639 green:0.420 blue:0.2 alpha:1.0];
        NSMutableParagraphStyle *style = [[NSMutableParagraphStyle alloc] init];
        [style setAlignment:NSTextAlignmentCenter];
        NSDictionary *attrsDictionary  = [NSDictionary dictionaryWithObjectsAndKeys:
                    NSColor.blackColor, NSForegroundColorAttributeName,
                    theLabel.font, NSFontAttributeName,
                    style, NSParagraphStyleAttributeName, nil];
        NSAttributedString *attrString = [[NSAttributedString alloc]initWithString:theLabel.title attributes:attrsDictionary];
        [theLabel setAttributedTitle:attrString];

        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:WarmButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityHigh;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:OptionButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"Option", @"") target:[ControlManager sharedInstance] action:@selector(optionPressed:)];

        theLabel.bezelColor = [NSColor colorWithSRGBRed:0.761 green:0.525 blue:0.247 alpha:1.0];
        NSMutableParagraphStyle *style = [[NSMutableParagraphStyle alloc] init];
        [style setAlignment:NSTextAlignmentCenter];
        NSDictionary *attrsDictionary  = [NSDictionary dictionaryWithObjectsAndKeys:
                    NSColor.blackColor, NSForegroundColorAttributeName,
                    theLabel.font, NSFontAttributeName,
                    style, NSParagraphStyleAttributeName, nil];
        NSAttributedString *attrString = [[NSAttributedString alloc]initWithString:theLabel.title attributes:attrsDictionary];
        [theLabel setAttributedTitle:attrString];
        [theLabel sizeToFit];

        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:OptionButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityHigh;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:SelectButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"Select", @"") target:[ControlManager sharedInstance] action:@selector(selectPressed:)];

        theLabel.bezelColor = [NSColor colorWithSRGBRed:0.890 green:0.635 blue:0.267 alpha:1.0];
        NSMutableParagraphStyle *style = [[NSMutableParagraphStyle alloc] init];
        [style setAlignment:NSTextAlignmentCenter];
        NSDictionary *attrsDictionary  = [NSDictionary dictionaryWithObjectsAndKeys:
                    NSColor.blackColor, NSForegroundColorAttributeName,
                    theLabel.font, NSFontAttributeName,
                    style, NSParagraphStyleAttributeName, nil];
        NSAttributedString *attrString = [[NSAttributedString alloc]initWithString:theLabel.title attributes:attrsDictionary];
        [theLabel setAttributedTitle:attrString];
        [theLabel sizeToFit];

        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:SelectButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityHigh;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:StartButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"Start", @"") target:[ControlManager sharedInstance] action:@selector(startPressed:)];

        theLabel.bezelColor = [NSColor colorWithSRGBRed:0.906 green:0.682 blue:0.263 alpha:1.0];
        NSMutableParagraphStyle *style = [[NSMutableParagraphStyle alloc] init];
        [style setAlignment:NSTextAlignmentCenter];
        NSDictionary *attrsDictionary  = [NSDictionary dictionaryWithObjectsAndKeys:
                    NSColor.blackColor, NSForegroundColorAttributeName,
                    theLabel.font, NSFontAttributeName,
                    style, NSParagraphStyleAttributeName, nil];
        NSAttributedString *attrString = [[NSAttributedString alloc]initWithString:theLabel.title attributes:attrsDictionary];
        [theLabel setAttributedTitle:attrString];
        [theLabel sizeToFit];

        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:StartButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityHigh;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:ClearButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"Clear", @"") target:[ControlManager sharedInstance] action:@selector(clearPressed:)];
        [theLabel sizeToFit];

        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:ClearButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityHigh;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:InverseButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"Inverse", @"") target:[ControlManager sharedInstance] action:@selector(inversePressed:)];
        [theLabel sizeToFit];
        
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:InverseButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityHigh;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:PopoverIdentifier])
    {
        NSPopoverTouchBarItem *thePopover = [[NSPopoverTouchBarItem alloc] initWithIdentifier:PopoverIdentifier];
        thePopover.collapsedRepresentationLabel = NSLocalizedString(@"Ins/Del", @"");
        
        thePopover.popoverTouchBar.delegate = self;
        thePopover.popoverTouchBar.defaultItemIdentifiers =
        @[InsertCharButtonIdentifier, InsertLineButtonIdentifier,DeleteCharButtonIdentifier,DeleteLineButtonIdentifier, F1ButtonIdentifier, F2ButtonIdentifier, F3ButtonIdentifier, F4ButtonIdentifier];
 
        return thePopover;
    }
    else if ([identifier isEqualToString:InsertCharButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"Ins Char", @"") target:[ControlManager sharedInstance] action:@selector(insertCharPressed:)];
        
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:InsertCharButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityNormal;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:InsertLineButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"Ins Line", @"") target:[ControlManager sharedInstance] action:@selector(insertLinePressed:)];
        
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:InsertLineButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityNormal;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:DeleteCharButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"Del Char", @"") target:[ControlManager sharedInstance] action:@selector(deleteCharPressed:)];
        
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:DeleteCharButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityNormal;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:DeleteLineButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"Del Line", @"") target:[ControlManager sharedInstance] action:@selector(deleteLinePressed:)];
        
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:DeleteLineButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityNormal;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:F1ButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"F1", @"") target:[ControlManager sharedInstance] action:@selector(f1Pressed:)];
        
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:F1ButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityNormal;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:F2ButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"F2", @"") target:[ControlManager sharedInstance] action:@selector(f2Pressed:)];
        
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:F2ButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityNormal;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:F3ButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"F3", @"") target:[ControlManager sharedInstance] action:@selector(f3Pressed:)];
        
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:F3ButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityNormal;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:F4ButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"F4", @"") target:[ControlManager sharedInstance] action:@selector(f4Pressed:)];
        
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:F4ButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityNormal;
        
        return customItemForLabel;
    }

    return nil;
}

@end
