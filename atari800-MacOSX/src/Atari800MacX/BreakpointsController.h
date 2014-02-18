/* BreakpointsController.h - BreakpointsController 
 header  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 
 */


#import <Cocoa/Cocoa.h>
#import "Breakpoint.h"
#import "DisasmDataSource.h"
#import "BreakpointDataSource.h"
#import "BreakpointEditorDataSource.h"
#import "monitor.h"

@interface BreakpointsController : NSObject {
	BOOL dirty;
	NSMutableArray *breakpoints;
	DisasmDataSource *disDataSource;
	BreakpointDataSource *breakpointDataSource;
	BreakpointEditorDataSource *breakpointEditorDataSource;
}

+ (BreakpointsController *)sharedInstance;
-(void) setDisasmDataSource:(DisasmDataSource *)dataSource;
-(void) setBreakpointDataSource:(BreakpointDataSource *)dataSource;
-(void) setBreakpointEditorDataSource:(BreakpointEditorDataSource *)dataSource;
-(NSMutableArray *) getBreakpoints;
-(void) loadBreakpoints;
-(void) adjustBreakpointEnables;
-(void) setDirty;
-(void) addBreakpointWithPC:(unsigned short) pc;
-(void) addBreakpointWithType:(int) type Mem:(unsigned short) memAddr Size:(int) size;
-(void) addBreakpointWithMem:(unsigned short) memAddr Size:(int) size Value: (unsigned short) memValue;
-(void) addBreakpointWithRow:(int) row;
-(void) replaceBreakpoint:(Breakpoint *) old with:(Breakpoint *) new;
-(void) toggleBreakpointEnable:(Breakpoint *) theBreakpoint;
-(void) toggleBreakpointEnableWithPC:(unsigned short) pc;
-(void) toggleBreakpointEnableWithRow:(int) row;
-(void) toggleBreakpointEnableWithIndex:(int) index;
-(void) editBreakpoint:(Breakpoint *) theBreakpoint;
-(void) editBreakpointWithPC:(unsigned short) pc;
-(void) editBreakpointWithMem:(unsigned short) memAddr;
-(void) editBreakpointWithRow:(int) row;
-(void) editBreakpointWithIndex:(int) index;
-(void) deleteBreakpoint:(Breakpoint *) theBreakpoint;
-(void) deleteBreakpointWithMem:(unsigned short) memAddr;
-(void) deleteBreakpointWithPC:(unsigned short) pc;
-(void) deleteBreakpointWithRow:(int) row;
-(void) deleteBreakpointWithIndex:(int) index;
-(void) deleteAllBreakpoints;
-(Breakpoint *) getBreakpointWithPC:(unsigned short) pc;
-(Breakpoint *) getBreakpointWithMem:(unsigned short) addr;
-(Breakpoint *) getBreakpointWithPCRow:(unsigned short) row;
-(int) getBreakpointNumForConditionNum:(int) num;
-(int) getBreakpointCount;

@end

