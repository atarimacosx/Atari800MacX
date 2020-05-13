/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>
    
       Macintosh OS X SDL port of Atari800
       Mark Grebe <atarimacosx@gmail.com>
   

    Feel free to customize this file to suit your needs
*/

#import "SDL.h"
#import "SDLMain.h"
#import "Atari800Window.h"
#import "Preferences.h"
#import "ControlManager.h"
#import "MediaManager.h"
#import <sys/param.h> /* for MAXPATHLEN */
#import <unistd.h>

/* Use this flag to determine whether we use SDLMain.nib or not */
#define		SDL_USE_NIB_FILE	0

/* Use this flag to determine whether we use CPS (docking) or not */
#define		SDL_USE_CPS		1
#ifdef SDL_USE_CPS
/* Portions of CPS.h */
typedef struct CPSProcessSerNum
{
	UInt32		lo;
	UInt32		hi;
} CPSProcessSerNum;

extern OSErr	CPSGetCurrentProcess( CPSProcessSerNum *psn);
extern OSErr 	CPSEnableForegroundOperation( CPSProcessSerNum *psn, UInt32 _arg2, UInt32 _arg3, UInt32 _arg4, UInt32 _arg5);
extern OSErr	CPSSetFrontProcess( CPSProcessSerNum *psn);

#endif /* SDL_USE_CPS */
extern int PLATFORM_Exit(int run_monitor);

static int    gArgc;
static char  **gArgv;
static BOOL   started=NO;
int fileToLoad = FALSE;
static char startupFile[FILENAME_MAX];
static char startupDir[FILENAME_MAX];

extern void PauseAudio(int pause);
extern int dontMuteAudio;
extern int requestPrefsChange;
extern int configurationChanged;


void SDLMainLoadStartupFile() {
	// If it isn't a full path name, then add the working directory we saved...
	if (startupFile[0] != '/') {
		strcat(startupDir, "/");
		strcat(startupDir, startupFile);
		strcpy(startupFile, startupDir);
		printf("Changed\n");
		}
    [SDLMain loadFile:[NSString stringWithCString:startupFile]];
}

void SDLMainCloseWindow() {
	[[NSApp keyWindow] performClose:NSApp];
}

void SDLMainSelectAll() {
	[[[NSApp keyWindow] firstResponder] selectAll:NSApp];
}

void SDLMainCopy() {
	[[[NSApp keyWindow] firstResponder] copy:NSApp];
}

void SDLMainPaste() {
	[[[NSApp keyWindow] firstResponder] paste:NSApp];
}

void SDLMainActivate() {
	[NSApp activateIgnoringOtherApps:YES];
}

int SDLMainIsActive() {
	return([NSApp isActive]);
}


/* A helper category for NSString */
@interface NSString (ReplaceSubString)
- (NSString *)stringByReplacingRange:(NSRange)aRange with:(NSString *)aString;
@end

@interface SDLApplication : NSApplication
@end

@implementation SDLApplication
/* Invoked from the Quit menu item */
- (void)terminate:(id)sender
{
    /* Post a SDL_QUIT event */
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}
@end


/* The main class of the application, the application's delegate */
@implementation SDLMain

/* Fix menu to contain the real app name instead of "SDL App" */
- (void)fixMenu:(NSMenu *)aMenu withAppName:(NSString *)appName
{
    NSRange aRange;
    NSEnumerator *enumerator;
    NSMenuItem *menuItem;

    aRange = [[aMenu title] rangeOfString:@"SDL App"];
    if (aRange.length != 0)
        [aMenu setTitle: [[aMenu title] stringByReplacingRange:aRange with:appName]];

    enumerator = [[aMenu itemArray] objectEnumerator];
    while ((menuItem = [enumerator nextObject]))
    {
        aRange = [[menuItem title] rangeOfString:@"SDL App"];
        if (aRange.length != 0)
            [menuItem setTitle: [[menuItem title] stringByReplacingRange:aRange with:appName]];
        if ([menuItem hasSubmenu])
            [self fixMenu:[menuItem submenu] withAppName:appName];
    }
    [ aMenu sizeToFit ];
}

/* Called when the internal event loop has just started running */
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
    int status;

    started = YES;
    
    /* Set the main menu to contain the real app name instead of "SDL App" */
    [self fixMenu:[NSApp mainMenu] withAppName:[[NSProcessInfo processInfo] processName]];

    /* Hand off to main application code */
    status = SDL_main (gArgc, gArgv);
    
    /* We're done, thank you for playing */
    exit(status);
}

- (void) applicationWillTerminate: (NSNotification *) note
{
    PLATFORM_Exit(FALSE);
 }
/*------------------------------------------------------------------------------
*  application openFile - Open a file dragged to the application.
*-----------------------------------------------------------------------------*/
- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
    if (started)
        [SDLMain loadFile:filename];
    else {
        fileToLoad = TRUE;
        [filename getCString:startupFile];
    }

    return(FALSE);
}

/*------------------------------------------------------------------------------
*  loadFile - Load a file into the emulator at startup.
*-----------------------------------------------------------------------------*/
+(void)loadFile:(NSString *)filename
{
    NSString *suffix;
    
    suffix = [filename pathExtension];
    
    if ([suffix isEqualToString:@"a8s"] || [suffix isEqualToString:@"A8S"]) 
        [[ControlManager sharedInstance] loadStateFile:filename];
    else if ([suffix isEqualToString:@"a8c"] || [suffix isEqualToString:@"A8C"]) { 
        [[Preferences sharedInstance] loadConfigFile:filename];
        [[Preferences sharedInstance] transferValuesToEmulator];
        [[Preferences sharedInstance] transferValuesToAtari825];
        [[Preferences sharedInstance] transferValuesToAtari1020];
        [[Preferences sharedInstance] transferValuesToEpson];
		if (started) {
			requestPrefsChange = 1;
			configurationChanged = 1;
			}
		}
    else if ([suffix isEqualToString:@"car"] || [suffix isEqualToString:@"CAR"] ||
             [suffix isEqualToString:@"rom"] || [suffix isEqualToString:@"ROM"] ||
             [suffix isEqualToString:@"bin"] || [suffix isEqualToString:@"BIN"])
        [[MediaManager sharedInstance] cartInsertFile:filename];
    else if ([suffix isEqualToString:@"atr"] || [suffix isEqualToString:@"ATR"] ||
             [suffix isEqualToString:@"xfd"] || [suffix isEqualToString:@"XFD"] ||
             [suffix isEqualToString:@"pro"] || [suffix isEqualToString:@"PRO"] ||
             [suffix isEqualToString:@"atx"] || [suffix isEqualToString:@"ATX"] ||
             [suffix isEqualToString:@"dcm"] || [suffix isEqualToString:@"DCM"])
        [[MediaManager sharedInstance] diskInsertFile:filename];
    else if ([suffix isEqualToString:@"cas"] || [suffix isEqualToString:@"CAS"])
        [[MediaManager sharedInstance] cassInsertFile:filename];
    else if ([suffix isEqualToString:@"xex"] || [suffix isEqualToString:@"XEX"])
        [[MediaManager sharedInstance] loadExeFileFile:filename];
    else if ([suffix isEqualToString:@"set"] || [suffix isEqualToString:@"SET"])
        [[MediaManager sharedInstance] diskSetLoadFile:filename];
}

/*------------------------------------------------------------------------------
*  applicationDidBecomeActive - Called when we are no longer hidden.
*-----------------------------------------------------------------------------*/
- (void)applicationDidBecomeActive:(NSNotification *)aNotification
{
	if (!dontMuteAudio)
		PauseAudio(0);
}


/*------------------------------------------------------------------------------
*  applicationDidResignActive - Called when we are hidden.
*-----------------------------------------------------------------------------*/
- (void)applicationDidResignActive:(NSNotification *)aNotificatio
{
	if (!dontMuteAudio)
		PauseAudio(1);
}

@end


@implementation NSString (ReplaceSubString)

- (NSString *)stringByReplacingRange:(NSRange)aRange with:(NSString *)aString
{
    unsigned int bufferSize;
    unsigned int selfLen = [self length];
    unsigned int aStringLen = [aString length];
    unichar *buffer;
    NSRange localRange;
    NSString *result;

    bufferSize = selfLen + aStringLen - aRange.length;
    buffer = NSAllocateMemoryPages(bufferSize*sizeof(unichar));
    
    /* Get first part into buffer */
    localRange.location = 0;
    localRange.length = aRange.location;
    [self getCharacters:buffer range:localRange];
    
    /* Get middle part into buffer */
    localRange.location = 0;
    localRange.length = aStringLen;
    [aString getCharacters:(buffer+aRange.location) range:localRange];
     
    /* Get last part into buffer */
    localRange.location = aRange.location + aRange.length;
    localRange.length = selfLen - localRange.location;
    [self getCharacters:(buffer+aRange.location+aStringLen) range:localRange];
    
    /* Build output string */
    result = [NSString stringWithCharacters:buffer length:bufferSize];
    
    NSDeallocateMemoryPages(buffer, bufferSize);
    
    return result;
}

@end



#ifdef main
#  undef main
#endif

/* Set the working directory to the .app's parent directory */
void setupWorkingDirectory()
{
    char parentdir[MAXPATHLEN];
    char *c;
	
	getcwd(startupDir, MAXPATHLEN);

    strncpy ( parentdir, gArgv[0], sizeof(parentdir) );
    c = (char*) parentdir;

    while (*c != '\0')     /* go to end */
        c++;
    
    while (*c != '/')      /* back up to parent */
        c--;
    
    *c++ = '\0';             /* cut off last part (binary name) */
  
    assert ( chdir (parentdir) == 0 );   /* chdir to the binary app's parent */
    assert ( chdir ("../../") == 0 ); /* chdir to the .app's parent */
}



/* Main entry point to executable - should *not* be SDL_main! */
int main (int argc, char **argv)
{

    /* Copy the arguments into a global variable */
    int i;
    
    /* This is passed if we are launched by double-clicking */
    if ( argc >= 2 && strncmp (argv[1], "-psn", 4) == 0 ) {
        gArgc = 1;
    } else {
        gArgc = argc;
    }
    gArgv = (char**) malloc (sizeof(*gArgv) * (gArgc+1));
    assert (gArgv != NULL);
    for (i = 0; i < gArgc; i++)
        gArgv[i] = argv[i];
    gArgv[i] = NULL;
	
    /* Set the working directory to the .app's parent directory */
    setupWorkingDirectory();
	
    /* Set the working directory for preferences, so defaults for 
       directories are set correctly */
    [Preferences setWorkingDirectory:gArgv[0]];

    //[SDLApplication poseAsClass:[NSApplication class]];
    NSApplicationMain (argc, (const char **) argv);
    return 0;
}
