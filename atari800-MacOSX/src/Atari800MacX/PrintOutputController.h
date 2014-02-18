/* PrintOutputController */

#import <Cocoa/Cocoa.h>
#import "PrinterSimulator.h"

@interface PrintOutputController : NSWindowController
{
    IBOutlet NSButton *mContinueButton;
    IBOutlet NSButton *mDeleteButton;
    IBOutlet NSButton *mPenChangeButton;
    IBOutlet NSScrollView *mMainScrollView;
    IBOutlet NSButton *mSaveAsButton;
    IBOutlet NSButton *mResetConfirmButton;
	bool preview;
	int numPages;
	float printOffset;
	PrinterSimulator *currentPrinter;
}
+ (PrintOutputController *)sharedInstance;
- (id)init;
- (int)calcNumPages;
- (void)calcPrinterOffset;
- (void)updatePages;
- (void)printChar:(char) character;
- (void)showPrinterOutput:(id)sender;
- (IBAction)onContinue:(id)sender;
- (IBAction)onDelete:(id)sender;
- (IBAction)onSelectPrinter:(id)sender;
- (void)selectPrinter:(int)printer;
- (IBAction)onResetPrinter:(id)sender;
- (int)resetConfirm; 
- (IBAction)resetConfirmYes:(id)sender; 
- (IBAction)resetConfirmNo:(id)sender;
- (IBAction)onSaveAs:(id)sender;
- (IBAction)onSkipLine:(id)sender;
- (IBAction)onRevSkipLine:(id)sender;
- (IBAction)onSkipPage:(id)sender;
- (IBAction)onChangePen:(id)sender;
-(void)setPenButtonColor;
- (void)addToPrintArray:(NSObject *)object;
- (NSArray *)getPrintArray;
-(float)getPrintOffset;
- (bool)isPreview;
- (void)setPrinter:(NSObject *)printer;
- (void)enablePenChange:(BOOL)enable;
@end
