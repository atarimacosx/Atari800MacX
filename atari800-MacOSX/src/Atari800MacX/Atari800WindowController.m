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
        @[ColdButtonIdentifier,WarmButtonIdentifier,OptionButtonIdentifier,SelectButtonIdentifier,StartButtonIdentifier,ClearButtonIdentifier, InverseButtonIdentifier,PopoverIdentifier,NSTouchBarItemIdentifierOtherItemsProxy];// InsertCharButtonIdentifier, InsertLineButtonIdentifier,DeleteCharButtonIdentifier,DeleteLineButtonIdentifier,NSTouchBarItemIdentifierOtherItemsProxy];//,WarmButtonIdentifier, NSTouchBarItemIdentifierOtherItemsProxy];
    
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
        theLabel.action = @selector(startPressed:);
        
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:ColdButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        // You want this label to always be visible no matter how many items are in the NSTouchBar instance.
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
        
        
        // You want this label to always be visible no matter how many items are in the NSTouchBar instance.
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
        
        // You want this label to always be visible no matter how many items are in the NSTouchBar instance.
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
        
        // You want this label to always be visible no matter how many items are in the NSTouchBar instance.
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
        
        // You want this label to always be visible no matter how many items are in the NSTouchBar instance.
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityHigh;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:ClearButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"Clear", @"") target:self action:nil];
        [theLabel sizeToFit];

        theLabel.target = [ControlManager sharedInstance];
        theLabel.action = @selector(startPressed:);
        
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:ClearButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        // You want this label to always be visible no matter how many items are in the NSTouchBar instance.
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityHigh;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:InverseButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"Inverse", @"") target:self action:nil];
        [theLabel sizeToFit];

        theLabel.target = [ControlManager sharedInstance];
        theLabel.action = @selector(inversePressed:);
        
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:InverseButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        // You want this label to always be visible no matter how many items are in the NSTouchBar instance.
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityHigh;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:PopoverIdentifier])
    {
        NSPopoverTouchBarItem *thePopover = [[NSPopoverTouchBarItem alloc] initWithIdentifier:PopoverIdentifier];

        [thePopover setCustomizationLabel:NSLocalizedString(@"Ins/Del", @"")];
        //thePopover.popoverTouchBar =
        return thePopover;
    }
#if 0
    else if ([identifier isEqualToString:InsertCharButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"InsertC", @"") target:self action:nil];
        
        theLabel.target = [ControlManager sharedInstance];
        theLabel.action = @selector(startPressed:);
        
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:InsertCharButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        // You want this label to always be visible no matter how many items are in the NSTouchBar instance.
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityNormal;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:InsertLineButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"InsertL", @"") target:self action:nil];
        
        theLabel.target = [ControlManager sharedInstance];
        theLabel.action = @selector(startPressed:);
        
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:InsertLineButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        // You want this label to always be visible no matter how many items are in the NSTouchBar instance.
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityNormal;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:DeleteCharButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"DeleteC", @"") target:self action:nil];
        
        theLabel.target = [ControlManager sharedInstance];
        theLabel.action = @selector(startPressed:);
        
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:DeleteCharButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        // You want this label to always be visible no matter how many items are in the NSTouchBar instance.
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityNormal;
        
        return customItemForLabel;
    }
    else if ([identifier isEqualToString:DeleteLineButtonIdentifier])
    {
        NSButton *theLabel = [NSButton buttonWithTitle:NSLocalizedString(@"DeleteL", @"") target:self action:nil];
        
        theLabel.target = [ControlManager sharedInstance];
        theLabel.action = @selector(startPressed:);
        
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:DeleteLineButtonIdentifier];
        customItemForLabel.view = theLabel;
        
        // You want this label to always be visible no matter how many items are in the NSTouchBar instance.
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityNormal;
        
        return customItemForLabel;
    }
#endif
    return nil;
}

@end
