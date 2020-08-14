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
#define TopLevelGroup @"TopLevel"
#define PopoverGroup @"PopGroup"
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
        @[TopLevelGroup,PopoverIdentifier,NSTouchBarItemIdentifierOtherItemsProxy];
    
    return bar;
    }

// This gets called while the NSTouchBar is being constructed, for each NSTouchBarItem to be created.
- (nullable NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
{
    NSDictionary *attrsDictionary;
    NSAttributedString *attrString;
    NSMutableParagraphStyle *style;
    
    if ([identifier isEqualToString:TopLevelGroup])
    {
        NSButton *coldButton = [NSButton buttonWithTitle:NSLocalizedString(@"Power", @"") target:[ControlManager sharedInstance] action:@selector(coldReset:)];
        
       NSButton *warmButton = [NSButton buttonWithTitle:NSLocalizedString(@"Reset", @"") target:[ControlManager sharedInstance] action:@selector(warmReset:)];

        warmButton.bezelColor = [NSColor colorWithSRGBRed:0.639 green:0.420 blue:0.2 alpha:1.0];
        style = [[NSMutableParagraphStyle alloc] init];
        [style setAlignment:NSTextAlignmentCenter];
        attrsDictionary  = [NSDictionary dictionaryWithObjectsAndKeys:
                    NSColor.blackColor, NSForegroundColorAttributeName,
                    warmButton.font, NSFontAttributeName,
                    style, NSParagraphStyleAttributeName, nil];
        attrString = [[NSAttributedString alloc]initWithString:warmButton.title attributes:attrsDictionary];
        [warmButton setAttributedTitle:attrString];

        NSButton *optionButton = [NSButton buttonWithTitle:NSLocalizedString(@"Option", @"") target:[ControlManager sharedInstance] action:@selector(optionPressed:)];

        optionButton.bezelColor = [NSColor colorWithSRGBRed:0.761 green:0.525 blue:0.247 alpha:1.0];
        style = [[NSMutableParagraphStyle alloc] init];
        [style setAlignment:NSTextAlignmentCenter];
        attrsDictionary  = [NSDictionary dictionaryWithObjectsAndKeys:
                    NSColor.blackColor, NSForegroundColorAttributeName,
                    optionButton.font, NSFontAttributeName,
                    style, NSParagraphStyleAttributeName, nil];
        attrString = [[NSAttributedString alloc]initWithString:optionButton.title attributes:attrsDictionary];
        [optionButton setAttributedTitle:attrString];

        NSButton *selectButton = [NSButton buttonWithTitle:NSLocalizedString(@"Select", @"") target:[ControlManager sharedInstance] action:@selector(selectPressed:)];

        selectButton.bezelColor = [NSColor colorWithSRGBRed:0.890 green:0.635 blue:0.267 alpha:1.0];
        style = [[NSMutableParagraphStyle alloc] init];
        [style setAlignment:NSTextAlignmentCenter];
        attrsDictionary  = [NSDictionary dictionaryWithObjectsAndKeys:
                    NSColor.blackColor, NSForegroundColorAttributeName,
                    selectButton.font, NSFontAttributeName,
                    style, NSParagraphStyleAttributeName, nil];
        attrString = [[NSAttributedString alloc]initWithString:selectButton.title attributes:attrsDictionary];
        [selectButton setAttributedTitle:attrString];
 
        NSButton *startButton = [NSButton buttonWithTitle:NSLocalizedString(@"Start", @"") target:[ControlManager sharedInstance] action:@selector(startPressed:)];

        startButton.bezelColor = [NSColor colorWithSRGBRed:0.906 green:0.682 blue:0.263 alpha:1.0];
        style = [[NSMutableParagraphStyle alloc] init];
        [style setAlignment:NSTextAlignmentCenter];
        attrsDictionary  = [NSDictionary dictionaryWithObjectsAndKeys:
                    NSColor.blackColor, NSForegroundColorAttributeName,
                    startButton.font, NSFontAttributeName,
                    style, NSParagraphStyleAttributeName, nil];
        attrString = [[NSAttributedString alloc]initWithString:startButton.title attributes:attrsDictionary];
        [startButton setAttributedTitle:attrString];

        NSButton *clearButton = [NSButton buttonWithTitle:NSLocalizedString(@"Clear", @"") target:[ControlManager sharedInstance] action:@selector(clearPressed:)];

        NSButton *inverseButton = [NSButton buttonWithTitle:NSLocalizedString(@"Inverse", @"") target:[ControlManager sharedInstance] action:@selector(inversePressed:)];
        
        NSStackView *stackView = [NSStackView stackViewWithViews:@[coldButton, warmButton, optionButton, selectButton, startButton, clearButton, inverseButton]];
        stackView.spacing = 1;
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:TopLevelGroup];
        customItemForLabel.view = stackView;
        
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
