
/* atari_mac_sdl.c - Main C file for 
 *  Macintosh OS X SDL port of Atari800
 *  Mark Grebe <atarimacosx@gmail.com>
 *  
 *  Based on the SDL port of Atari 800 by:
 *  Jacek Poplawski <jacekp@linux.com.pl>
 *
 * Copyright (C) 2002-2003 Mark Grebe
 * Copyright (C) 2001-2003 Atari800 development team (see DOC/CREDITS)
 *
 * This file is part of the Atari800 emulator project which emulates
 * the Atari 400, 800, 800XL, 130XE, and 5200 8-bit computers.
 *
 * Atari800 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atari800 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari800; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include <SDL.h>

#include <SDL_syswm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <mach/mach.h>
#include <sys/time.h>


// Atari800 includes
#include "config.h"
#include "af80.h"
#include "akey.h"
#include "atari.h"
#include "bit3.h"
#if SUPPORTS_CHANGE_VIDEOMODE
#include "videomode.h"
#endif
#include "esc.h"
#include "input.h"
#include "mac_colours.h"
#include "platform.h"
#include "ui.h"
#include "ataritiff.h"
#include "pokeysnd.h"
#include "gtia.h"
#include "antic.h"
#include "mac_diskled.h"
#include "devices.h"
#include "screen.h"
#include "cpu.h"
#include "memory.h"
#include "pia.h"
#include "sndsave.h"
#include "statesav.h"
#include "log.h"
#include "cartridge.h"
#include "sio.h"
#include "mac_rdevice.h"
#include "scalebit.h"
#include "cassette.h"
#include "xep80.h"
#include "pbi_bb.h"
#include "pbi_mio.h"
#include "pclink.h"
#include "preferences_c.h"
#include "ultimate1mb.h"
#include "side2.h"
#include "util.h"
#include "capslock.h"
#ifdef NETSIO
#include "netsio.h"
#endif

/* Mac-specific constants and variables for compatibility */
#ifndef XEP80_SCRN_HEIGHT
#define XEP80_SCRN_HEIGHT 300
#endif

/* Mac-specific pseudo key constants */
#define AKEY_START_PSEUDO   0x100
#define AKEY_SELECT_PSEUDO  0x101  
#define AKEY_OPTION_PSEUDO  0x102
#define AKEY_DELAY_PSEUDO   0x103

/* Mac-specific XEP80 variables */
static int XEP80_first_row = 0;
static int XEP80_last_row = 0;
int COL80_autoswitch = 0;
static int XEP80_sent_count = 0;
static int XEP80_last_sent_count = 0;

/* Mac-specific jumper variable */
int Atari800_jumper_present = 0;

/* Mac-specific Alitrra ROM preference variables */
int Atari800_useAlitrra1200XLRom = 0;
int Atari800_useAlitrra5200Rom = 0;
int Atari800_useAlitrraBasicRom = 0;
int Atari800_useAlitrraOSBRom = 0;
int Atari800_useAlitrraXEGSRom = 0;
int Atari800_useAlitrraXLRom = 0;

/* Local variables that control the display and sound modes.  They need to be visable externally
   so the preferences and menu manager Objective-C files may access them. */
#define FULL_DISPLAY_COUNT 3

#define MAX(A,B)	( (A) > (B) ? (A):(B)) 

int sound_enabled = TRUE;
int sound_flags = POKEYSND_BIT16; 
int sound_bits = 16; 
double sound_volume = 1.0;
int pauseCount;
int full_display = FULL_DISPLAY_COUNT;
int must_display = FALSE;
int useBuiltinPalette = TRUE;
int adjustPalette = FALSE;
int paletteBlack = 0;
int paletteWhite = 0xf0;
int paletteIntensity = 80;
int paletteColorShift = 40;
int screen_x_offset = 0;
int screen_y_offset = 0;
int FULLSCREEN_MACOS = 0;
int SCALE_MODE;
double scaleFactor = 3;
double scaleFactorFloat = 2.0;
double scaleFactorRenderX;
double scaleFactorRenderY;
int GRAB_MOUSE = 0;
int WIDTH_MODE;  /* Width mode, and the constants that define it */
int current_w;
int current_h;

int mediaStatusWindowOpen;
int functionKeysWindowOpen;
int led_enabled_media = 1;
int led_counter_enabled_media = 1;
int PLATFORM_80col = 0;
int useAtariCursorKeys = 1;
int onlyIntegralScaling = FALSE;
int fixAspectFullscreen = FALSE;
#define USE_ATARI_CURSOR_CTRL_ARROW 0
#define USE_ATARI_CURSOR_ARROW_ONLY 1
#define USE_ATARI_CURSOR_FX         2
#define SHORT_WIDTH_MODE 0
#define DEFAULT_WIDTH_MODE 1
#define FULL_WIDTH_MODE 2
#define NORMAL_SCALE 0
#define SCANLINE_SCALE 1
#define FRAMES_TO_HOLD_KEY 1;

#define SCREEN_WIDTH_SHORT    (Screen_WIDTH - 2 * 24 - 2 * 8)
#define SCREEN_WIDTH_DEFAULT  (Screen_WIDTH - 2 * 24)
#define SCREEN_WIDTH_FULL     (Screen_WIDTH)

/* Local variables that control the speed of emulation */
int speed_limit = 1;
double emulationSpeed = 1.0;
int pauseEmulator = 0;
int currentFps;
/* Define the max frame rate when "speed limit" is off.  We can't let it run totally open
   loop, as with verison 3.x, and the updated timing loops for OSX 10.4, it may run too fast,
   and cause problems with key repeat kicking in on the atari much too fast.  5X normal spped
   seems to be about right to keep that from happening. */
#define DELTAFAST (1.0/300.0)  

/* Semaphore flags used for signalling between the Objective-C menu managers and the emulator. */
int requestWidthModeChange = 0;
int requestFullScreenChange = 0;
int request80ColChange = 0;
int request80ColModeChange = 0;
int requestGrabMouse = 0;
int requestScreenshot = 0;
int requestColdReset = 0;
int requestWarmReset = 0;
int requestToolReset = 0;
int requestSaveState = 0;
int requestLoadState = 0;
int requestLimitChange = 0;
int requestArtifChange = 0;
int requestDisableBasicChange = 0;
int requestKeyjoyEnableChange = 0;
int requestCX85EnableChange = 0;
int requestXEGSKeyboardChange = 0;
int requestMachineTypeChange = 0;
int requestPBIExpansionChange = 0;
int requestSoundEnabledChange = 0;
int requestSoundStereoChange = 0;
int requestSoundRecordingChange = 0;
int requestFpsChange = 0;
int requestPrefsChange = 0;
int requestScaleModeChange = 0;
int requestPauseEmulator = 0;
int requestMonitor = 0;
int requestQuit = 0;
int requestCaptionChange = 0;
int requestPaste = 0;
int requestCopy = 0;
int requestSelectAll = 0;

#define COPY_OFF     0
#define COPY_IDLE    1
#define COPY_STARTED 2
#define COPY_DEFINED 3
int copyStatus = COPY_IDLE;

void Reinit_Joysticks(void);
void checkForNewJoysticks(void);
/* Filenames for saving and loading the state of the emulator */
char saveFilename[FILENAME_MAX];
char loadFilename[FILENAME_MAX];

/* Filename for loading the color palette */
char paletteFilename[FILENAME_MAX];

/* Externs for misc. emulator functions */
extern int POKEYSND_stereo_enabled;
extern char sio_filename[8][FILENAME_MAX];
extern int refresh_rate;
extern double deltatime;
double real_deltatime;
extern UBYTE CPU_cim_encountered;

/* Externs from preferences_c.c which indicate if user has changed certain preferences */
extern int displaySizeChanged;
extern int showfpsChanged;
extern int scaleModeChanged;
extern int artifChanged;
extern int paletteChanged;
extern int osRomsChanged;
extern int machineTypeChanged;
extern int patchFlagsChanged;
extern int keyboardJoystickChanged;
extern int hardDiskChanged;
extern int pcLinkChanged;
extern int pcLinkOptionsChanged;
extern int af80EnabledChanged;
extern int bit3EnabledChanged;
extern int xep80EnabledChanged;
extern int xep80ColorsChanged;
extern int configurationChanged;
extern int fullscreenOptsChanged;
extern int ultimateRomChanged;
extern int side2RomChanged;
extern int side2CFChanged;

extern int fileToLoad;
extern int CARTRIDGE_type;
int disable_all_basic = 1;
extern int currPrinter;
extern int bbRequested;
extern int mioRequested;
extern int PREFS_axlon_num_banks;
extern int PREFS_mosaic_num_banks;

/* Routines used to interface with Cocoa objects */
extern void SDLMainActivate(void);
extern int  SDLMainIsActive();
extern void SDLMainLoadStartupFile(void);
extern void SDLMainLoadFile(char * file);
extern void SDLMainCloseWindow(void);
extern void SDLMainSelectAll(void);
extern void AboutBoxScroll(void);
extern void Atari800WindowCreate(NSWindow *window);
extern void Atari800WindowAspectSet(int w, int h);
extern void Atari800WindowFullscreen();
extern int  Atari800WindowIsFullscreen();
extern void Atari800OriginSet(void);
extern void Atari800WindowCenter(void);
extern void Atari800WindowDisplay(void);
extern void Atari800OriginSave(void);
extern void Atari800OriginRestore(void);
extern int  Atari800IsKeyWindow(void);
extern void Atari800MakeKeyWindow();
extern int  Atari800GetCopyData(int startx, int endx, int starty, int endy, unsigned char *data);
extern void UpdateMediaManagerInfo(void);
extern void MacCapsLockStateReset(void);
extern void MediaManagerRunDiskManagement(void);
extern void MediaManagerRunDiskEditor(void);
extern void MediaManagerRunSectorEditor(void);
extern void MediaManagerInsertCartridge(void);
extern void MediaManagerRemoveCartridge(void);
extern void MediaManagerLoadExe(void);
extern void MediaManagerInsertDisk(int diskNum);
extern void MediaManagerRemoveDisk(int diskNum);
extern void MediaManagerSectorLed(int diskNo, int sectorNo, int on);
extern void MediaManagerStatusLed(int diskNo, int on, int read);
extern void MediaManagerCassSliderUpdate(int block);
extern void MediaManagerStatusWindowShow(void);
extern void MediaManagerShowCreatePanel(void);
extern void MediaManager80ColMode(int xep80Enabled, int af80Enabled, int bit3Enabled, int xep80);
extern int  PasteManagerStartPaste(void);
extern void PasteManagerStartCopy(unsigned char *string);
extern void PasteManagerStartPasteWithString();
extern int  PasteManagerStartupPasteEnabled(void);
extern void PasteManagerUpdateEscapeCopyMenu(void);
extern void SetDisplayManagerWidthMode(int widthMode);
extern void SetDisplayManagerFps(int fpsOn);
extern void SetDisplayManagerScaleMode(int scaleMode);
extern void SetDisplayManagerArtifactMode(int scaleMode);
extern void SetDisplayManagerGrabMouse(int mouseOn);
extern void SetDisplayManager80ColMode(int xep80Enabled, int xep80Port, int af80Enabled, int bit3Enabled, int xep80);
extern int EnableDisplayManager80ColMode(int machineType, int xep80Enabled, int af80Enabled, int bit3Enabled);
extern void SetDisplayManagerXEP80Autoswitch(int autoswitchOn);
extern void SetControlManagerLimit(int limit);
extern void SetControlManagerDisableBasic(int mode, int disableBasic);
extern void SetControlManagerKeyjoyEnable(int keyjoyEnable);
extern void SetControlManagerCX85Enable(int cx85enabled);
extern void SetControlManagerXEGSKeyboard(int attached);
extern void SetControlManagerMachineType(int machineType, int ramSize);
extern void SetControlManagerPBIExpansion(int type);
extern void SetControlManagerArrowKeys(int keys);
extern int ControlManagerFatalError(void);
extern void ControlManagerSaveState(void); 
extern void ControlManagerLoadState(void);
extern void ControlManagerPauseEmulator(void);
extern void ControlManagerKeyjoyEnable(void);
extern void ControlManagerHideApp(void);
extern void ControlManagerAboutApp(void);
extern void ControlManagerMiniturize(void);
extern void ControlManagerShowHelp(void);
extern int ControlManagerMonitorRun(void);
extern void ControlManagerFunctionKeysWindowShow(void);
extern int PasteManagerGetScancode(unsigned char *code);
extern void SetSoundManagerEnable(int soundEnabled);
extern void SetSoundManagerStereo(int soundStereo);
extern void SetSoundManagerRecording(int soundRecording);
void SoundManagerStereoDo(void);
void SoundManagerRecordingDo(void);
void SoundManagerIncreaseVolume(void);
void SoundManagerDecreaseVolume(void);
extern int RtConfigLoad(char *rtconfig_filename);
extern void CalculatePrefsChanged();
extern void loadPrefsBinaries();
extern void CalcMachineTypeRam(int type, int *machineType, int *ramSize,
int *axlon, int *mosaic, int *ultimate,
int *basic, int *game, int *leds, int *jumper);
extern void BasicUIInit(void);
extern void ClearScreen();
extern void CenterPrint(int fg, int bg, char *string, int y);
extern void RunPreferences(void);
extern void UpdatePreferencesJoysticks();
extern void PreferencesIdentifyGamepadNew();
extern void PrintOutputControllerSelectPrinter(int printer);
extern void Devices_H_Init(void);
extern void PreferencesSaveDefaults(void);
extern void loadMacPrefs(int firstTime);
extern void reloadMacJoyPrefs();
extern int PreferencesTypeFromIndex(int index, int *ver4type, int *ver5type);
extern void PreferencesSaveConfiguration();
extern void PreferencesLoadConfiguration();
	
/* Externs used for initialization */
extern void init_bb();
extern void init_mio();
#ifdef SYNCHRONIZED_SOUND
extern void init_mzpokeysnd_sync(void);
#endif			 


/* Local Routines */
void Atari_DisplayScreen(UBYTE * screen);
void SoundSetup(void); 
void PauseAudio(int pause);
void CreateWindowCaption(void);
void ProcessCopySelection(int *first_row, int *last_row, int selectAll);
void HandleScreenChange(int requested_w, int requested_h);
int  GetAtariScreenWidth(void);
void CalcWindowSize(int *width, int *height);
void SetWindowAspectRatio(void);
static void SetRenderScale(void);
void SelectionNormalize(int *startX, int *startY, int *endX, int* endY);

// joystick emulation - Future enhancement will allow user to set these in 
//   Preferences.

int SDL_TRIG_0 = SDL_SCANCODE_LCTRL;
int SDL_TRIG_0_B = SDL_SCANCODE_KP_0;
int SDL_TRIG_0_R = SDL_SCANCODE_RCTRL;
int SDL_TRIG_0_B_R = SDL_SCANCODE_KP_0;
int SDL_JOY_0_LEFT = SDL_SCANCODE_KP_4;
int SDL_JOY_0_RIGHT = SDL_SCANCODE_KP_6;
int SDL_JOY_0_DOWN = SDL_SCANCODE_KP_2;
int SDL_JOY_0_UP = SDL_SCANCODE_KP_8;
int SDL_JOY_0_LEFTUP = SDL_SCANCODE_KP_7;
int SDL_JOY_0_RIGHTUP = SDL_SCANCODE_KP_9;
int SDL_JOY_0_LEFTDOWN = SDL_SCANCODE_KP_1;
int SDL_JOY_0_RIGHTDOWN = SDL_SCANCODE_KP_3;

int SDL_TRIG_1 = SDL_SCANCODE_TAB;
int SDL_TRIG_1_B = SDL_SCANCODE_LSHIFT;
int SDL_TRIG_1_R = SDL_SCANCODE_TAB;
int SDL_TRIG_1_B_R = SDL_SCANCODE_RSHIFT;
int SDL_JOY_1_LEFT = SDL_SCANCODE_A;
int SDL_JOY_1_RIGHT = SDL_SCANCODE_D;
int SDL_JOY_1_DOWN = SDL_SCANCODE_X;
int SDL_JOY_1_UP = SDL_SCANCODE_W;
int SDL_JOY_1_LEFTUP = SDL_SCANCODE_Q;
int SDL_JOY_1_RIGHTUP = SDL_SCANCODE_E;
int SDL_JOY_1_LEFTDOWN = SDL_SCANCODE_Z;
int SDL_JOY_1_RIGHTDOWN = SDL_SCANCODE_X;

int keyjoyEnable = TRUE;

// real joysticks - Pointers to SDL structures
int joystickCount;
SDL_Joystick *joystick0 = NULL;
SDL_Joystick *joystick1 = NULL;
SDL_Joystick *joystick2 = NULL;
SDL_Joystick *joystick3 = NULL;
SDL_Joystick *joysticks[4];
int joystick0_nbuttons, joystick1_nbuttons;
int joystick2_nbuttons, joystick3_nbuttons;
int joystick0_nsticks, joystick0_nhats;
int joystick1_nsticks, joystick1_nhats;
int joystick2_nsticks, joystick2_nhats;
int joystick3_nsticks, joystick3_nhats;

#define minjoy 10000            // real joystick tolerancy
#define NUM_JOYSTICKS    4
int JOYSTICK_MODE[NUM_JOYSTICKS];
int JOYSTICK_AUTOFIRE[NUM_JOYSTICKS];
#define NO_JOYSTICK      0
#define KEYPAD_JOYSTICK  1
#define USERDEF_JOYSTICK 2
#define SDL0_JOYSTICK    3
#define SDL1_JOYSTICK    4
#define SDL2_JOYSTICK    5
#define SDL3_JOYSTICK    6
#define MOUSE_EMULATION  7

#define JOY_TYPE_STICK   0
#define JOY_TYPE_HAT     1
#define JOY_TYPE_BUTTONS 2
int joystick0TypePref = JOY_TYPE_HAT;
int joystick1TypePref = JOY_TYPE_HAT;
int joystick2TypePref = JOY_TYPE_HAT;
int joystick3TypePref = JOY_TYPE_HAT;
int joystickType0, joystickType1;
int joystickType2, joystickType3;
int joystick0Num, joystick1Num;
int joystick2Num, joystick3Num;
int paddlesXAxisOnly = 0;

#define AKEY_BUTTON1  		0x100
#define AKEY_BUTTON2  		0x200
#define AKEY_BUTTON_START  	0x300
#define AKEY_BUTTON_SELECT 	0x400
#define AKEY_BUTTON_OPTION 	0x500
#define AKEY_BUTTON_RESET	0x600
#define AKEY_JOYSTICK_UP	0x700
#define AKEY_JOYSTICK_DOWN	0x701
#define AKEY_JOYSTICK_LEFT	0x702
#define AKEY_JOYSTICK_RIGHT	0x703
#define MAX_JOYSTICK_BUTTONS 24
int joystickButtonKey[4][MAX_JOYSTICK_BUTTONS];
int joystick5200ButtonKey[4][MAX_JOYSTICK_BUTTONS];

/* Function key window interface variables */
int breakFunctionPressed = 0;
int startFunctionPressed = 0;
int selectFunctionPressed = 0;
int optionFunctionPressed = 0;
int inverseFunctionPressed = 0;
int clearFunctionPressed = 0;
int helpFunctionPressed = 0;
int insertCharFunctionPressed = 0;
int insertLineFunctionPressed = 0;
int deleteCharFunctionPressed = 0;
int deleteLineFunctionPressed = 0;
int f1FunctionPressed = 0;
int f2FunctionPressed = 0;
int f3FunctionPressed = 0;
int f4FunctionPressed = 0;

/* Mac-specific input variables (others now provided by real input.c) */
int INPUT_key_break = 0;  /* Mac-specific variable not in input.c */
int pad_key_shift = 0;
int pad_key_control = 0;
int pad_key_consol = INPUT_CONSOL_NONE;
int cx85_port = 1;
int INPUT_Invert_Axis = 0;

static UBYTE STICK[4];
static UBYTE TRIG_input[4];
static int mouse_x = 0;
static int mouse_y = 0;
static int mouse_move_x = 0;
static int mouse_move_y = 0;
static int mouse_pen_show_pointer = 0;
static int mouse_last_right = 0;
static int mouse_last_down = 0;
static const UBYTE mouse_amiga_codes[16] = {
    0x00, 0x02, 0x0a, 0x08,
    0x01, 0x03, 0x0b, 0x09,
    0x05, 0x07, 0x0f, 0x0d,
    0x04, 0x06, 0x0e, 0x0c
    };
static const UBYTE mouse_st_codes[16] = {
    0x00, 0x02, 0x03, 0x01,
    0x08, 0x0a, 0x0b, 0x09,
    0x0c, 0x0e, 0x0f, 0x0d,
    0x04, 0x06, 0x07, 0x05
    };
static int max_scanline_counter;
static int scanline_counter;
#define MOUSE_SHIFT 4

/* Prefs */
extern int prefsArgc;
extern char *prefsArgv[];

// sound 
#include "mzpokeysnd.h"
#define FRAGSIZE       10      // 1<<FRAGSIZE is size of sound buffer
static int frag_samps = (1 << FRAGSIZE);
static int dsp_buffer_bytes;
static Uint8 *dsp_buffer = NULL;
static int dsprate = 44100;
#ifdef SYNCHRONIZED_SOUND
/* latency (in ms) and thus target buffer size */
static int snddelay = 20;  /* TBD MDG - Should this vary with PAL vs NTSC? */
/* allowed "spread" between too many and too few samples in the buffer (ms) */
static int sndspread = 7; /* TBD MDG - Should this vary with PAL vs NTSC? */;
/* estimated gap */
static int gap_est = 0;
/* cumulative audio difference */
static double avg_gap;
/* dsp_write_pos, dsp_read_pos and callbacktick are accessed in two different threads: */
static int dsp_write_pos;
static int dsp_read_pos;
/* tick at which callback occured */
static int callbacktick = 0;
#endif

// video
SDL_Surface *MainScreen = NULL;
SDL_Window *MainWindow = NULL;
SDL_Surface *MonitorGLScreen = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;
int screenSwitchEnabled = 1;

SDL_Color colors[256];          // palette
Uint16 Palette16[256];          // 16-bit palette
Uint32 Palette32[256];          // 32-bit palette
static int our_width, our_height; // The variables for storing screen width
char windowCaption[80];
int selectionStartX = 0;
int selectionStartY = 0;
int selectionEndX = 0;
int selectionEndY = 0;
static unsigned char ScreenCopyData[(XEP80_LINE_LEN + 1) * XEP80_HEIGHT + 1]; 

// keyboard variables 
Uint8 *kbhits;
static int last_key_break = 0;
static int last_key_code = AKEY_NONE;
static int CONTROL = 0x00;
#define CAPS_UPPER    0
#define CAPS_LOWER    1
#define CAPS_GRAPHICS 2
static int capsLockState = CAPS_UPPER;
static int capsLockPrePasteState = CAPS_UPPER;
#define PASTE_IDLE    0
#define PASTE_START   1
#define PASTE_IN_PROG 2
#define PASTE_END     3
#define PASTE_DONE	  4
#define PASTE_KEY_DELAY  8
#define FUNCTION_KEY_PRESS_DURATION 10
static int pasteState = PASTE_IDLE;

static int soundStartupDelay = 44; // No audio for first 44 SDL sound loads until Pokey
	                           // gets going.......This eliminates the startup
							   // "click"....

/* Hasktable implementation */
struct node{
    int key;
    int val;
    struct node *next;
};
struct table{
    int size;
    struct node **list;
};
struct table *createTable(int size){
    struct table *t = (struct table*)malloc(sizeof(struct table));
    t->size = size;
    t->list = (struct node**)malloc(sizeof(struct node*)*size);
    int i;
    for(i=0;i<size;i++)
        t->list[i] = NULL;
    return t;
}
void deleteTable(struct table *t){
    int i;
    struct node *n;
    struct node *next;
    for (i=0;i<t->size;i++){
        n = t->list[i];
        while(n) {
            next = n->next;
            free(n);
            n = next;
        }
    }
    free(t->list);
    free(t);
}
int hashCode(struct table *t,int key){
    if(key<0)
        return -(key%t->size);
    return key%t->size;
}
void insert(struct table *t,int key,int val){
    int pos = hashCode(t,key);
    struct node *list = t->list[pos];
    struct node *newNode = (struct node*)malloc(sizeof(struct node));
    struct node *temp = list;
    while(temp){
        if(temp->key==key){
            temp->val = val;
            return;
        }
        temp = temp->next;
    }
    newNode->key = key;
    newNode->val = val;
    newNode->next = list;
    t->list[pos] = newNode;
}
int lookup(struct table *t,int key){
    int pos = hashCode(t,key);
    struct node *list = t->list[pos];
    struct node *temp = list;
    while(temp){
        if(temp->key==key){
            return temp->val;
        }
        temp = temp->next;
    }
    return -1;
}

struct table *SDL_IsJoyKeyTable = NULL;

#ifdef SYNCHRONIZED_SOUND
/* returns a factor (1.0 by default) to adjust the speed of the emulation
 * so that if the sound buffer is too full or too empty, the emulation
 * slows down or speeds up to match the actual speed of sound output */
double PLATFORM_AdjustSpeed(void)
{
	int bytes_per_sample = (POKEYSND_stereo_enabled ? 2 : 1)*((sound_bits == 16) ? 2:1);
	
	double alpha = 2.0/(1.0+40.0);
	double gap_too_small;
	double gap_too_large;
	static int inited = FALSE;
	
	if (!inited) {
		inited = TRUE;
		avg_gap = gap_est;
	}
	else {
		avg_gap = avg_gap + alpha * (gap_est - avg_gap);
	}
	
	gap_too_small = (snddelay*dsprate*bytes_per_sample)/1000;
	gap_too_large = ((snddelay+sndspread)*dsprate*bytes_per_sample)/1000;
	if (avg_gap < gap_too_small) {
		double speed = 0.95;
//		printf("small %f %f %d %d %d %d\n",avg_gap,gap_too_small,snddelay,dsprate,bytes_per_sample,sound_bits);
		return speed;
	}
	if (avg_gap > gap_too_large) {
		double speed = 1.05;
//		printf("large %f %f %d %d %d %d\n",avg_gap,gap_too_large,snddelay,dsprate,bytes_per_sample,sound_bits);
		return speed;
	}
	return 1.0;
}
#endif /* SYNCHRONIZED_SOUND */

void SDL_Sound_Update(void)
{
#ifdef SYNCHRONIZED_SOUND
	int bytes_written = 0;
	int samples_written;
	int gap;
	int newpos;
	int bytes_per_sample;
	double bytes_per_ms;
	
	if (!sound_enabled || pauseCount) return;
	/* produce samples from the sound emulation */
	samples_written = POKEYSND_UpdateProcessBuffer();
	bytes_per_sample = (POKEYSND_stereo_enabled ? 2 : 1)*((sound_bits == 16) ? 2:1);
	bytes_per_ms = (bytes_per_sample)*(dsprate/1000.0);
	bytes_written = (sound_bits == 8 ? samples_written : samples_written*2);
	SDL_LockAudio();
	/* this is the gap as of the most recent callback */
	gap = dsp_write_pos - dsp_read_pos;
	/* an estimation of the current gap, adding time since then */
	if (callbacktick != 0) {
		gap_est = gap - (bytes_per_ms)*(SDL_GetTicks() - callbacktick);
	}
	/* if there isn't enough room... */
	while (gap + bytes_written > dsp_buffer_bytes) {
		/* then we allow the callback to run.. */
		SDL_UnlockAudio();
		/* and delay until it runs and allows space. */
		SDL_Delay(1);
		SDL_LockAudio();
		/*printf("sound buffer overflow:%d %d\n",gap, dsp_buffer_bytes);*/
		gap = dsp_write_pos - dsp_read_pos;
	}
	/* now we copy the data into the buffer and adjust the positions */
	newpos = dsp_write_pos + bytes_written;
	if (newpos/dsp_buffer_bytes == dsp_write_pos/dsp_buffer_bytes) {
		/* no wrap */
		memcpy(dsp_buffer+(dsp_write_pos%dsp_buffer_bytes), POKEYSND_process_buffer, bytes_written);
	}
	else {
		/* wraps */
		int first_part_size = dsp_buffer_bytes - (dsp_write_pos%dsp_buffer_bytes);
		memcpy(dsp_buffer+(dsp_write_pos%dsp_buffer_bytes), POKEYSND_process_buffer, first_part_size);
		memcpy(dsp_buffer, POKEYSND_process_buffer+first_part_size, bytes_written-first_part_size);
	}
	dsp_write_pos = newpos;
	if (callbacktick == 0) {
		/* Sound callback has not yet been called */
		dsp_read_pos += bytes_written;
	}
	if (dsp_write_pos < dsp_read_pos) {
		/* should not occur */
		printf("Error: dsp_write_pos < dsp_read_pos\n");
	}
	while (dsp_read_pos > dsp_buffer_bytes) {
		dsp_write_pos -= dsp_buffer_bytes;
		dsp_read_pos -= dsp_buffer_bytes;
	}
	SDL_UnlockAudio();
#else /* SYNCHRONIZED_SOUND */
	/* fake function */
#endif /* SYNCHRONIZED_SOUND */
}

/*------------------------------------------------------------------------------
*  SetPalette - Call to set the Pallete used by the emulator.  Uses the global
*   variable colors[].
*-----------------------------------------------------------------------------*/
static void SetPalette()
{
    SDL_SetPaletteColors(MainScreen->format->palette, colors, 0, 256);
}

/*------------------------------------------------------------------------------
*  CalcPalette - Call to calculate a b/w or color pallete for use by the
*   emulator.  May be replaced by ability to use user defined palletes.
*-----------------------------------------------------------------------------*/
void CalcPalette()
{
    int i, rgb;
    Uint32 c,c32;
    //static SDL_PixelFormat Format32 = {NULL,32,4,0,0,0,8,16,8,0,0,
	//									0xff0000,0x00ff00,0x0000ff,0x000000,
	//									0,255};
    SDL_PixelFormat* Format32 = SDL_AllocFormat(SDL_PIXELFORMAT_RGB888);

    for (i = 0; i < 256; i++) {
         rgb = colortable[i];
         colors[i].r = (rgb & 0x00ff0000) >> 16;
         colors[i].g = (rgb & 0x0000ff00) >> 8;
         colors[i].b = (rgb & 0x000000ff) >> 0;
        }

    for (i = 0; i < 256; i++) {
        c = SDL_MapRGBA(MainScreen->format, colors[i].r, colors[i].g,
                        colors[i].b,255);
        switch (MainScreen->format->BitsPerPixel) {
            case 16:
                Palette16[i] = (Uint16) c;
				// We always need Pallette32 for screenshots on the Mac
				c32 = SDL_MapRGBA(Format32, colors[i].r, colors[i].g,
                                  colors[i].b, 255);
                Palette32[i] = c32; // Opaque
                break;
            case 32:
                Palette32[i] = c; // Opaque
                break;
            }
        }
}

Uint32 power_of_two(Uint32 input)
    {
      Uint32 value = 1;
      while( value < input )
        value <<= 1;
      return value;
    }

/*------------------------------------------------------------------------------
*  InitializeWindow - Call to set new video mode, using SDL functions.
*-----------------------------------------------------------------------------*/
void InitializeWindow(int w, int h)
{
    Uint32 texture_w, texture_h;

    PauseAudio(1);
    Atari800OriginSave();

    // Get rid of old renderer
    if (renderer)
        SDL_DestroyRenderer(renderer);
    
    // Delete the old window, if it exists
    if (MainWindow) {
        Atari800OriginSave();
        SDL_DestroyWindow(MainWindow);
    }
    Log_print("Setting Screen: %dx%d %f",w,h,scaleFactorFloat);
    MainWindow = SDL_CreateWindow(windowCaption,
                                    SDL_WINDOWPOS_UNDEFINED,
                                    SDL_WINDOWPOS_UNDEFINED,
                                    w, h, SDL_WINDOW_RESIZABLE);
    current_w = w;
    current_h = h;
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
    renderer = SDL_CreateRenderer(MainWindow, -1, 0);
    SetRenderScale();
    
    // Save Mac Window for later use
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(MainWindow, &wmInfo);
    Atari800WindowCreate(wmInfo.info.cocoa.window);
    SetWindowAspectRatio();

    // Delete the old textures
    if(MainScreen)
        SDL_FreeSurface(MainScreen);

    texture_w = power_of_two(1024);
    texture_h = power_of_two(512);
    // Create the SDL texture
    MainScreen = SDL_CreateRGBSurface(0, texture_w, texture_h, 16,
                    0x0000F800, 0x000007E0, 0x0000001F, 0x00000000);

    // Make sure the screens are cleared to black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    Atari800OriginRestore();

    PauseAudio(0);
        
    if (MainScreen == NULL || MainWindow == NULL) {
        Log_print("Setting Video Mode: %ix%ix%i FAILED", w, h);
        Log_flushlog();
        exit(-1);
        }
}

/*------------------------------------------------------------------------------
*  InitializeVideo - Call to initialze Video and Windows
*-----------------------------------------------------------------------------*/
void InitializeVideo()
{
    int w = GetAtariScreenWidth();
    int h = Screen_HEIGHT;

    SDL_ShowCursor(SDL_ENABLE); // show mouse cursor

    InitializeWindow(scaleFactorFloat*(double)w, scaleFactorFloat*(double)h);

    SetPalette();
    
    if (GRAB_MOUSE)
        SDL_SetRelativeMouseMode(SDL_TRUE);
    else
        SDL_SetRelativeMouseMode(SDL_FALSE);
    
    /* Clear the alternate page, so the first redraw is entire screen */
    if (Screen_atari_b)
        memset(Screen_atari_b, 0, (Screen_HEIGHT * Screen_WIDTH));
}

int GetAtariScreenWidth(void)
{
    int width;
    
    if (WIDTH_MODE == SHORT_WIDTH_MODE)
        width = SCREEN_WIDTH_SHORT;
    else if (WIDTH_MODE == DEFAULT_WIDTH_MODE)
        width = SCREEN_WIDTH_DEFAULT;
    else
        width = SCREEN_WIDTH_FULL;

    return width;
}

int GetDisplayScreenWidth(void)
{
    int width;
    
    if (PLATFORM_80col) {
        if (BIT3_enabled)
            width = BIT3_SCRN_WIDTH;
        else if (AF80_enabled)
            width = AF80_SCRN_WIDTH;
        else
            width = XEP80_SCRN_WIDTH;
    } else {
        width = GetAtariScreenWidth();
    }

    return width;
}

int GetDisplayScreenHeight(void)
{
    int height;
    
    if (PLATFORM_80col) {
        if (BIT3_enabled)
            height = BIT3_SCRN_HEIGHT;
        else if (AF80_enabled)
            height = AF80_SCRN_HEIGHT;
        else
            height = XEP80_SCRN_HEIGHT;
    } else {
        height = Screen_HEIGHT;
    }

    return height;
}

void CalcWindowSize(int *width, int *height)
{
    int sWidth = GetAtariScreenWidth();
    
    *width = (double) sWidth * scaleFactorFloat;
    *height = (double) Screen_HEIGHT * scaleFactorFloat;
    
    *width *= 8;
    *width /= 8;
    *height *= 8;
    *height /= 8;
}

void SetWindowAspectRatio(void)
{
    int w, h;
    
    if (WIDTH_MODE == SHORT_WIDTH_MODE)
        {
        w = SCREEN_WIDTH_SHORT;
        }
    else if (WIDTH_MODE == DEFAULT_WIDTH_MODE)
        {
        w = SCREEN_WIDTH_DEFAULT;
        }
    else
        {
        w = SCREEN_WIDTH_FULL;
        }
    h = Screen_HEIGHT;
    w *= 8;
    w /= 8;
    h *= 8;
    h /= 8;
         
    Atari800WindowAspectSet(w, h+8);
}

static void SetRenderScale(void)
{
    if (PLATFORM_80col) {
        scaleFactorRenderX = ((double) GetAtariScreenWidth() /
                              (double) GetDisplayScreenWidth() ) *
                              scaleFactorFloat;
        scaleFactorRenderY = ((double) Screen_HEIGHT /
                              (double) GetDisplayScreenHeight() ) *
                              scaleFactorFloat;
    }
    else {
        scaleFactorRenderX = scaleFactorFloat;
        scaleFactorRenderY = scaleFactorFloat;
    }
    SDL_RenderSetScale(renderer,
                       scaleFactorRenderX,
                       scaleFactorRenderY);
}

static void Switch80Col(void)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    HandleScreenChange(current_w, current_h);
    full_display = FULL_DISPLAY_COUNT;
    Atari_DisplayScreen((UBYTE *) Screen_atari);
    CreateWindowCaption();
    SDL_SetWindowTitle(MainWindow, windowCaption);
    copyStatus = COPY_IDLE;
}

void PLATFORM_Switch80Col(void)
{
    PLATFORM_80col = 1 - PLATFORM_80col;
    Switch80Col();
}

void PLATFORM_Switch80ColMode(void)
{
    Switch80Col();
}

/*------------------------------------------------------------------------------
*  SwitchScaleMode - Called by user interface to switch between using scanlines
*    in the display, and not. 
*-----------------------------------------------------------------------------*/
void SwitchScaleMode(int scaleMode)
{
	SCALE_MODE = scaleMode;
	SetDisplayManagerScaleMode(scaleMode);
	full_display = FULL_DISPLAY_COUNT;
    Atari_DisplayScreen((UBYTE *) Screen_atari);
}

/*------------------------------------------------------------------------------
*  SwitchShowFps - Called by user interface to switch between showing and not 
*    showing Frame per second speed for the emulator in windowed mode.
*-----------------------------------------------------------------------------*/
void SwitchShowFps()
{
    Screen_show_atari_speed = 1 - Screen_show_atari_speed;
    if (!Screen_show_atari_speed)
        SDL_SetWindowTitle(MainWindow, windowCaption);
    SetDisplayManagerFps(Screen_show_atari_speed);
}

/*------------------------------------------------------------------------------
*  SwitchGrabMouse - Called by user interface to switch between grabbing the
*    mouse for Atari emulation or not.  Only works in Windowed mode, as mouse
*    is always grabbed in Full Screen mode.
*-----------------------------------------------------------------------------*/
void SwitchGrabMouse()
{
    GRAB_MOUSE = 1 - GRAB_MOUSE;
    if (GRAB_MOUSE) {
        SDL_SetRelativeMouseMode(SDL_TRUE);          }
    else {
        SDL_SetRelativeMouseMode(SDL_FALSE);         }
}

/*------------------------------------------------------------------------------
*  SwitchWidth - Called by user interface to cycle between the 3 width modes,
*    short, normal, and full.  In single mode the sizes are:
*     Short - 320x240
*     Normal -  336x240
*     Full - 384x240
*-----------------------------------------------------------------------------*/
void SwitchWidth(int width)
{
    int w, h;
    
    WIDTH_MODE = width;
    if (!FULLSCREEN_MACOS) {
        CalcWindowSize(&w, &h);
        SetWindowAspectRatio();
        SDL_SetWindowSize(MainWindow, w, h);
    }
    HandleScreenChange(current_w, current_h);
    full_display = FULL_DISPLAY_COUNT;
    Atari_DisplayScreen((UBYTE *) Screen_atari);
	SetDisplayManagerWidthMode(width);
	copyStatus = COPY_IDLE;
}

/*------------------------------------------------------------------------------
*  PauseAudio - This function pauses and unpauses the Audio output of the
*    emulator.  Used when prefs window is up, other menu windows, changing
*    from full screen to windowed, etc.  Number of calls to unpause must match
*    the number of calls to pause before unpausing will actually occur.
*-----------------------------------------------------------------------------*/
void PauseAudio(int pause)
{    
    if (pause) {
        pauseCount++;
        }
    else {
        if (pauseCount > 0) {
            pauseCount--;
            }
        }
}

static void SoundCallback(void *userdata, Uint8 *stream, int len)
{
    SDL_memset(stream, 0, len);
#ifndef SYNCHRONIZED_SOUND
	int sndn = (sound_bits == 8 ? len : len/2);
	/* in mono, sndn is the number of samples (8 or 16 bit) */
	/* in stereo, 2*sndn is the number of sample pairs */
	POKEYSND_Process(dsp_buffer, sndn);
	memcpy(stream, dsp_buffer, len);
#else
	int gap;
	int newpos;
	int underflow_amount = 0;
#define MAX_SAMPLE_SIZE 4
	static char last_bytes[MAX_SAMPLE_SIZE];
	int bytes_per_sample = (POKEYSND_stereo_enabled ? 2 : 1)*((sound_bits == 16) ? 2:1);
	gap = dsp_write_pos - dsp_read_pos;
	if (gap < len) {
		underflow_amount = len - gap;
		len = gap;
		/*return;*/
	}
	newpos = dsp_read_pos + len;
	if (newpos/dsp_buffer_bytes == dsp_read_pos/dsp_buffer_bytes) {
		/* no wrap */
		memcpy(stream, dsp_buffer + (dsp_read_pos%dsp_buffer_bytes), len);
	}
	else {
		/* wraps */
		int first_part_size = dsp_buffer_bytes - (dsp_read_pos%dsp_buffer_bytes);
		memcpy(stream,  dsp_buffer + (dsp_read_pos%dsp_buffer_bytes), first_part_size);
		memcpy(stream + first_part_size, dsp_buffer, len - first_part_size);
	}
	/* save the last sample as we may need it to fill underflow */
	if (gap >= bytes_per_sample) {
		memcpy(last_bytes, stream + len - bytes_per_sample, bytes_per_sample);
	}
	/* Just repeat the last good sample if underflow */
	if (underflow_amount > 0 ) {
		int i;
		for (i = 0; i < underflow_amount/bytes_per_sample; i++) {
			memcpy(stream + len +i*bytes_per_sample, last_bytes, bytes_per_sample);
		}
	}
	dsp_read_pos = newpos;
	callbacktick = SDL_GetTicks();
#endif /* SYNCHRONIZED_SOUND */
}

/*------------------------------------------------------------------------------
*  MacSoundReset - This function is the called on any emulator reset to skip
*      the first 10 frames of sound, so that we get rid of the anoying startup
*      click.
*-----------------------------------------------------------------------------*/
void MacSoundReset(void)
{
	soundStartupDelay = 40;
}

/*------------------------------------------------------------------------------
*  SoundSetup - This function starts the SDL audio device.
*-----------------------------------------------------------------------------*/
void SoundSetup() 
{ 
	SDL_AudioSpec desired, obtained;

	desired.freq = dsprate;
	if (sound_bits == 8)
		desired.format = AUDIO_U8;
	else if (sound_bits == 16)
		desired.format = AUDIO_S16;
	else {
		Log_print("unknown sound_bits");
		Atari800_Exit(FALSE);
		Log_flushlog();
	}
		
	desired.samples = frag_samps;
	desired.callback = SoundCallback;
	desired.userdata = NULL;
#ifdef STEREO_SOUND
	desired.channels = POKEYSND_stereo_enabled ? 2 : 1;
#else
	desired.channels = 1;
#endif /* STEREO_SOUND*/
		
	if (SDL_OpenAudio(&desired, &obtained) < 0) {
		Log_print("Problem with audio: %s", SDL_GetError());
		Log_print("Sound is disabled.");
		sound_enabled = FALSE;
		return;
	}
	//printf("obtained %d\n",obtained.freq);
		
#ifdef SYNCHRONIZED_SOUND
	{
		/* how many fragments in the dsp buffer */
#define DSP_BUFFER_FRAGS 5
		int specified_delay_samps = (dsprate*snddelay)/1000;
		int dsp_buffer_samps = frag_samps*DSP_BUFFER_FRAGS +specified_delay_samps;
		int bytes_per_sample = (POKEYSND_stereo_enabled ? 2 : 1)*((sound_bits == 16) ? 2:1);
		dsp_buffer_bytes = desired.channels*dsp_buffer_samps*(sound_bits == 8 ? 1 : 2);
		//printf("dsp_buffer_bytes = %d\n",dsp_buffer_bytes);
		dsp_read_pos = 0;
		dsp_write_pos = (specified_delay_samps+frag_samps)*bytes_per_sample;
		avg_gap = 0.0;
	}
#else
	dsp_buffer_bytes = desired.channels*frag_samps*(sound_bits == 8 ? 1 : 2);
#endif /* SYNCHRONIZED_SOUND */
	free(dsp_buffer);
	dsp_buffer = (Uint8 *)Util_malloc(dsp_buffer_bytes);
	memset(dsp_buffer, 0, dsp_buffer_bytes);
	POKEYSND_Init(POKEYSND_FREQ_17_EXACT, dsprate, desired.channels, sound_flags);
}

void Sound_Reinit(void)
{
	SDL_CloseAudio();
	SoundSetup();
}

/*------------------------------------------------------------------------------
*  SDL_Sound_Initialise - This function initializes the SDL sound.  The
*    command line arguments are not used here.
*-----------------------------------------------------------------------------*/
void SDL_Sound_Initialise(int *argc, char *argv[])
{
    int i, j;

    for (i = j = 1; i < *argc; i++) {
        if (strcmp(argv[i], "-sound") == 0)
            sound_enabled = TRUE;
        else if (strcmp(argv[i], "-nosound") == 0)
            sound_enabled = FALSE;
        else if (strcmp(argv[i], "-dsprate") == 0)
            sscanf(argv[++i], "%d", &dsprate);
        else {
            if (strcmp(argv[i], "-help") == 0) {
                Log_print("\t-sound           Enable sound\n"
                       "\t-nosound         Disable sound\n"
                       "\t-dsprate <rate>  Set DSP rate in Hz\n"
                      );
            }
            argv[j++] = argv[i];
        }
    }
    *argc = j;

    SoundSetup();
	SDL_PauseAudio(0);
}

/*------------------------------------------------------------------------------
*  SDL_Sound_Initialise - This function is modeled after a similar one in 
*    ui.c.  It is used to start/stop recording of raw sound.
*-----------------------------------------------------------------------------*/
void SDL_Sound_Recording()
{

    if (! SndSave_IsSoundFileOpen()) {
        if (SndSave_OpenSoundFile(SndSave_Find_AIFF_name())) {
            SetSoundManagerRecording(1);
            }
        }
    else {
        SndSave_CloseSoundFile();
        SetSoundManagerRecording(0);
        }
}

void MacCapsLockStateReset(void) 
{
	capsLockState = CAPS_UPPER;
    MacCapsLockSet(FALSE);
}


static int Atari_International_Char_To_Key(char ch)
{
    static int ch_to_key[] = {
        AKEY_SPACE, AKEY_EXCLAMATION, AKEY_DBLQUOTE, AKEY_HASH,
        AKEY_DOLLAR, AKEY_PERCENT, AKEY_AMPERSAND, AKEY_QUOTE,
        AKEY_PARENLEFT, AKEY_PARENRIGHT, AKEY_ASTERISK, AKEY_PLUS,
        AKEY_COMMA, AKEY_MINUS, AKEY_FULLSTOP, AKEY_SLASH,
        AKEY_0, AKEY_1, AKEY_2, AKEY_3,
        AKEY_4, AKEY_5, AKEY_6, AKEY_7,
        AKEY_8, AKEY_9, AKEY_COLON, AKEY_SEMICOLON,
        AKEY_LESS, AKEY_EQUAL, AKEY_GREATER, AKEY_QUESTION,
        AKEY_AT, AKEY_A, AKEY_B, AKEY_C,
        AKEY_D, AKEY_E, AKEY_F, AKEY_G,
        AKEY_H, AKEY_I, AKEY_J, AKEY_K,
        AKEY_L, AKEY_M, AKEY_N, AKEY_O,
        AKEY_P, AKEY_Q, AKEY_R, AKEY_S,
        AKEY_T, AKEY_U, AKEY_V, AKEY_W,
        AKEY_X, AKEY_Y, AKEY_Z, AKEY_BRACKETLEFT,
        AKEY_BACKSLASH, AKEY_BRACKETRIGHT, AKEY_CARET, AKEY_UNDERSCORE,
        AKEY_BREAK, AKEY_a, AKEY_b, AKEY_c,
        AKEY_d, AKEY_e, AKEY_f, AKEY_g,
        AKEY_h, AKEY_i, AKEY_j, AKEY_k,
        AKEY_l, AKEY_m, AKEY_n, AKEY_o,
        AKEY_p, AKEY_q, AKEY_r, AKEY_s,
        AKEY_t, AKEY_u, AKEY_v, AKEY_w,
        AKEY_x, AKEY_y, AKEY_z, AKEY_NONE,
        AKEY_BAR, AKEY_NONE, AKEY_NONE
    };
    if (ch < 32 || ch > 126)
        return AKEY_NONE;
    else
        return ch_to_key[ch-32];
}

static int Atari_International_Good_Key(int key) {
    int i;
    static int good_keys[32] = {
    SDLK_CAPSLOCK, SDLK_F1, SDLK_F2, SDLK_F3,
    SDLK_F4, SDLK_F5, SDLK_F6, SDLK_F7,
    SDLK_F8, SDLK_F9, SDLK_F10, SDLK_F11,
    SDLK_F12, SDLK_F13, SDLK_F14, SDLK_F15,
    SDLK_RETURN, SDLK_INSERT, SDLK_DELETE, SDLK_END,
    SDLK_HOME, SDLK_PAGEUP, SDLK_PAGEDOWN, SDLK_LEFT,
    SDLK_RIGHT, SDLK_UP, SDLK_DOWN, SDLK_ESCAPE,
    SDLK_TAB, SDLK_RETURN, SDLK_BACKSPACE, SDLK_DELETE
    };
    
    for (i = 0; i<32; i++)
        if (key == good_keys[i])
            return TRUE;
    return FALSE;
    }

void Atari_Consol_Key_Input() {
    UInt8 *kbhits;
    
    /* Handle the Atari Console Keys */
    kbhits = (Uint8 *) SDL_GetKeyboardState(NULL);
    if (kbhits[SDL_SCANCODE_F2] && !kbhits[SDL_SCANCODE_LGUI])
        INPUT_key_consol &= (~INPUT_CONSOL_OPTION);
    if (kbhits[SDL_SCANCODE_F3] && !kbhits[SDL_SCANCODE_LGUI])
        INPUT_key_consol &= (~INPUT_CONSOL_SELECT);
    if (kbhits[SDL_SCANCODE_F4] && !kbhits[SDL_SCANCODE_LGUI])
        INPUT_key_consol &= (~INPUT_CONSOL_START);
}

int Atari_Help_Key_Pressed() {
    UInt8 *kbhits;
    
    /* Handle the Atari Console Keys */
    kbhits = (Uint8 *) SDL_GetKeyboardState(NULL);
    return (kbhits[SDL_SCANCODE_F10] ||
            kbhits[SDL_SCANCODE_PAGEDOWN]);
}

/*------------------------------------------------------------------------------
 *  Atari_Keyboard_International - This function is called once per main loop to handle
 *    keyboard input.  It handles keys like the original SDL version,  with
 *    no access to the built in Mac handling of international keyboards.
 *-----------------------------------------------------------------------------*/
int Atari_Keyboard_International(void)
{
    static int lastkey = SDLK_UNKNOWN, key_pressed = 0;
    SDL_Event event;
    int key_option = 0;
    static int key_was_pressed = 0;
    char *text_input;
    static int text_key = AKEY_NONE;
    int text_type;
    static int paste_char;
    static int return_delay = 0;
    static unsigned char last_scancode;

    /* Check for presses in function keys window */
    INPUT_key_consol = INPUT_CONSOL_NONE;
    if (optionFunctionPressed) {
        INPUT_key_consol &= (~INPUT_CONSOL_OPTION);
        optionFunctionPressed--;
    }
    
    if (selectFunctionPressed) {
        INPUT_key_consol &= (~INPUT_CONSOL_SELECT);
        selectFunctionPressed--;
    }
    
    if (startFunctionPressed) {
        INPUT_key_consol &= (~INPUT_CONSOL_START);
        startFunctionPressed--;
    }
    
    if (breakFunctionPressed) {
        breakFunctionPressed--;
        return AKEY_BREAK;
    }
    
    if (inverseFunctionPressed) {
        inverseFunctionPressed--;
        return AKEY_ATARI;
    }
    
    if (clearFunctionPressed) {
        clearFunctionPressed--;
        return AKEY_CLEAR;
    }
    
    if (helpFunctionPressed) {
        helpFunctionPressed--;
        return AKEY_HELP;
    }
    
    if (insertCharFunctionPressed) {
        insertCharFunctionPressed--;
        return AKEY_INSERT_CHAR;
    }
    
    if (insertLineFunctionPressed) {
        insertLineFunctionPressed--;
        return AKEY_INSERT_LINE;
    }
    
    if (deleteCharFunctionPressed) {
        deleteCharFunctionPressed--;
        return AKEY_DELETE_CHAR;
    }
    
    if (deleteLineFunctionPressed) {
        deleteLineFunctionPressed--;
        return AKEY_DELETE_LINE;
    }
    
    if (f1FunctionPressed) {
        f1FunctionPressed--;
        return AKEY_F1;
    }
    
    if (f2FunctionPressed) {
        f2FunctionPressed--;
        return AKEY_F2;
    }
    
    if (f3FunctionPressed) {
        f3FunctionPressed--;
        return AKEY_F3;
    }
    
    if (f4FunctionPressed) {
        f4FunctionPressed--;
        return AKEY_F4;
    }
    
    if (pasteState != PASTE_IDLE) {
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN)
                pasteState = PASTE_END;
        }
        
        if (key_was_pressed) {
            key_was_pressed--;
            if (!key_was_pressed && (pasteState == PASTE_DONE)) {
                pasteState = PASTE_IDLE;
            }
            key_pressed = 0;
        }
        else {
            if (pasteState == PASTE_START) {
                copyStatus = COPY_IDLE;
                pasteState = PASTE_IN_PROG;
                paste_char = 0;
                if (capsLockState == CAPS_UPPER || capsLockState == CAPS_GRAPHICS) {
                    capsLockState = CAPS_LOWER;
                    key_was_pressed = PASTE_KEY_DELAY;
                    return(AKEY_CAPSTOGGLE);
                }
            }

            if (pasteState == PASTE_END) {
                pasteState = PASTE_IDLE;
                if (capsLockPrePasteState == CAPS_UPPER) {
                    pasteState = PASTE_DONE;
                    capsLockState = CAPS_UPPER;
                    key_was_pressed = PASTE_KEY_DELAY;
                    return(AKEY_CAPSLOCK);
                }
                else if (capsLockPrePasteState == CAPS_GRAPHICS) {
                    pasteState = PASTE_DONE;
                    capsLockState = CAPS_GRAPHICS;
                    CONTROL = AKEY_CTRL;
                    key_was_pressed = PASTE_KEY_DELAY;
                    return(AKEY_CAPSTOGGLE);
                }
                else {
                    pasteState = PASTE_IDLE;
                    return(AKEY_NONE);
                }
            }
            
            // To get repeat characters to be recognized properly
            // you need to have 3 frames of AKEY_NONE between the
            // keypresses.  So, at the end of the delay, for values
            // 3,2, and 1 for paste_char, AKEY_NONE will be returned.
            // Then when paste_char is zero, we get another scancode
            // from the paste manager.
            if (paste_char > 3) {
                paste_char--;
                if (last_scancode == AKEY_START_PSEUDO) {
                    INPUT_key_consol = (~INPUT_CONSOL_START);
                    return AKEY_NONE;
                } else if (last_scancode == AKEY_SELECT_PSEUDO) {
                    INPUT_key_consol = (~INPUT_CONSOL_SELECT);
                    return AKEY_NONE;
                } else if (last_scancode == AKEY_OPTION_PSEUDO) {
                    INPUT_key_consol = (~INPUT_CONSOL_OPTION);
                    return AKEY_NONE;
                } else
                    return last_scancode;
            } else if (return_delay) {
                return_delay--;
                return AKEY_NONE;
            } else if (paste_char) {
                paste_char--;
                return AKEY_NONE;
            }
            else {
                if (!PasteManagerGetScancode(&last_scancode)) {
                    pasteState = PASTE_END;
                    return AKEY_NONE;
                }
                if (last_scancode == AKEY_RETURN) {
                    // Larger delay used for return to allow Atari program
                    // to process what happens upon return.
                    paste_char = PASTE_KEY_DELAY;
                    return_delay = 9*PASTE_KEY_DELAY;
                } else if (last_scancode == AKEY_DELAY_PSEUDO) {
                    UBYTE byte1, byte2;
                    last_scancode = AKEY_NONE;
                    PasteManagerGetScancode(&byte1);
                    PasteManagerGetScancode(&byte2);
                    paste_char = (byte1 << 8) | byte2;
                } else if (last_scancode == AKEY_START_PSEUDO) {
                    INPUT_key_consol = (~INPUT_CONSOL_START);
                    paste_char = FUNCTION_KEY_PRESS_DURATION;
                    return AKEY_NONE;
                } else if (last_scancode == AKEY_SELECT_PSEUDO) {
                    INPUT_key_consol = (~INPUT_CONSOL_SELECT);
                    paste_char = FUNCTION_KEY_PRESS_DURATION;
                    return AKEY_NONE;
                } else if (last_scancode == AKEY_OPTION_PSEUDO) {
                    INPUT_key_consol = (~INPUT_CONSOL_OPTION);
                    paste_char = FUNCTION_KEY_PRESS_DURATION;
                    return AKEY_NONE;
                }
                else
                    paste_char = PASTE_KEY_DELAY;
                return last_scancode;
            }
        }
    }
    else {
        /* Poll for SDL events.  All we want here are Keydown and Keyup events,
         and the quit event. */
        checkForNewJoysticks();
        Atari_Consol_Key_Input();
        int pollEvent = 0;
        while (SDL_PollEvent(&event)) {
            pollEvent++;
            switch (event.type) {
                case SDL_DROPFILE:
                    SDLMainLoadFile(event.drop.file);
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        HandleScreenChange(event.window.data1, event.window.data2);
                    }
                case SDL_TEXTINPUT:
                    kbhits = (Uint8 *) SDL_GetKeyboardState(NULL);
                    text_input = event.text.text;
                    lastkey = 0;
                    text_type = TRUE;
                    switch(strlen(text_input)) {
                        case 0:
                        default:
                            return AKEY_NONE;
                        case 1:
                            text_key = Atari_International_Char_To_Key(text_input[0]);
                            if (kbhits[SDL_SCANCODE_CAPSLOCK] &&
                                !(kbhits[SDL_SCANCODE_LSHIFT] ||
                                  kbhits[SDL_SCANCODE_RSHIFT]))
                                text_key &= ~AKEY_SHFT;
                            if ((lookup(SDL_IsJoyKeyTable, SDL_GetKeyFromName(text_input)) == 1) && keyjoyEnable && !key_option &&
                                (pasteState == PASTE_IDLE)) {
                                text_key =  AKEY_NONE;
                            }
                            if (text_key != AKEY_NONE) {
                                 key_pressed = 1;
                            }
                            return text_key;
                    }
                    break;
                case SDL_KEYDOWN:
                    kbhits = (Uint8 *) SDL_GetKeyboardState(NULL);
                    if (!SDLMainIsActive()) {
                        return AKEY_NONE;
                    }
                    text_type = FALSE;
                    text_key = AKEY_NONE;
                    lastkey = event.key.keysym.sym;
                    if ((lookup(SDL_IsJoyKeyTable, event.key.keysym.scancode) == 1) && keyjoyEnable && !key_option &&
                        (pasteState == PASTE_IDLE)) {
                        return AKEY_NONE;
                    }
                    if (kbhits[SDL_SCANCODE_LGUI] ||
                        kbhits[SDL_SCANCODE_RCTRL] ||
                        kbhits[SDL_SCANCODE_LCTRL] ||
                        Atari_International_Good_Key(lastkey)) {
                        key_pressed = 1;
                    }
                    else if (event.key.repeat) {
                        continue;
                    }
                    else {
                        key_pressed = 0;
                        return AKEY_NONE;
                    }
                    break;
                case SDL_KEYUP:
                    kbhits = (Uint8 *) SDL_GetKeyboardState(NULL);
                    if (!SDLMainIsActive()) {
                        return AKEY_NONE;
                    }
                    lastkey = event.key.keysym.sym;
                    key_pressed = 0;
                    if (!Atari_International_Good_Key(lastkey)) {
                        return AKEY_NONE;
                    }
                    if (lastkey == SDLK_CAPSLOCK)
                        key_pressed = 1;
                    break;
                case SDL_QUIT:
                    return AKEY_EXIT;
                    break;
                default:
                    if (key_pressed)
                        break;
                    return AKEY_NONE;
            }
        }
        if (kbhits == NULL) {
            Log_print("oops, kbhits is NULL!");
            Log_flushlog();
            exit(-1);
        }
        
        /*  Determine if the shift key is pressed */
        if ((kbhits[SDL_SCANCODE_LSHIFT]) || (kbhits[SDL_SCANCODE_RSHIFT]))
            INPUT_key_shift = 1;
        else
            INPUT_key_shift = 0;
        
        /* Determine if the control key is pressed */
        if ((kbhits[SDL_SCANCODE_LCTRL]) || (kbhits[SDL_SCANCODE_RCTRL]))
            CONTROL = AKEY_CTRL;
        else
            CONTROL = 0;
        
        /* Determine if the option key is pressed */
        if ((kbhits[SDL_SCANCODE_LALT]) || (kbhits[SDL_SCANCODE_RALT]))
            key_option = 1;
        else
            key_option = 0;
        
        if (key_pressed) {
            if (!kbhits[SDL_SCANCODE_LGUI])
                copyStatus = COPY_IDLE;
            else if (lastkey != SDL_SCANCODE_LGUI && lastkey != SDLK_c)
                copyStatus = COPY_IDLE;
        }
        
        if (!pollEvent) {
            if (key_pressed) {
                if (text_type && text_key != AKEY_NONE) {
                    return text_key;
                }
            }
            else {
                return AKEY_NONE;
            }
        }
    }
    
    /* If the Option key is pressed, do the 1200 Function keys, and
     also provide alernatives to the insert/del/home/end/pgup/pgdn
     keys, since Apple in their infinite wisdom removed these keys
     from their latest keyboards */
    if (kbhits[SDL_SCANCODE_LALT] && !kbhits[SDL_SCANCODE_LGUI]) {
        if (key_pressed) {
            switch(lastkey) {
                case SDLK_F1:
                    if (INPUT_key_shift)
                        return AKEY_F1 | AKEY_SHFT;
                    else
                        return AKEY_F1;
                case SDLK_F2:
                    if (INPUT_key_shift)
                        return AKEY_F2 | AKEY_SHFT;
                    else
                        return AKEY_F2;
                case SDLK_F3:
                    if (INPUT_key_shift)
                        return AKEY_F3 | AKEY_SHFT;
                    else
                        return AKEY_F3;
                case SDLK_F4:
                    if (INPUT_key_shift)
                        return AKEY_F4 | AKEY_SHFT;
                    else
                        return AKEY_F4;
                case SDLK_F5: // INSERT
                    if (INPUT_key_shift)
                        return AKEY_INSERT_LINE;
                    else
                        return AKEY_INSERT_CHAR;
                case SDLK_F6: // DELETE
                    if (INPUT_key_shift)
                        return AKEY_DELETE_LINE;
                    else
                        return AKEY_DELETE_CHAR;
                case SDLK_F7: // HOME
                    return AKEY_CLEAR;
                case SDLK_F8: // END
                    if (INPUT_key_shift)
                        return AKEY_ATARI | AKEY_SHFT;
                    else
                        return AKEY_ATARI;
                case SDLK_F9: // PAGEUP
                    capsLockState = CAPS_UPPER;
                    return AKEY_CAPSLOCK;
                case SDLK_F10: // PAGEDOWN
                    return AKEY_HELP;
                    
            }
        }
        key_pressed = 0;
    }
    
    /*  If the command key is pressed, in fullscreen, execute the
     emulators built-in UI, otherwise, do the command key equivelents
     here.  The reason for this, is due to the SDL lib construction,
     all key events go to the SDL app, so the menus don't handle them
     directly */
    if (kbhits[SDL_SCANCODE_LGUI]) {
        if (key_pressed) {
            switch(lastkey) {
                case SDLK_f:
                case SDLK_RETURN:
                    requestFullScreenChange = 1;
                    break;
                case SDLK_x:
                    if (INPUT_key_shift && XEP80_enabled) {
                        PLATFORM_Switch80Col();
                        SetDisplayManager80ColMode(XEP80_enabled, XEP80_port, AF80_enabled, BIT3_enabled, PLATFORM_80col);
                        MediaManager80ColMode(XEP80_enabled, AF80_enabled, BIT3_enabled, PLATFORM_80col);
                    }
                    break;
                case SDLK_COMMA:
                    RunPreferences();
                    break;
                case SDLK_r:
                    MediaManagerLoadExe();
                    break;
                case SDLK_u:
                    SoundManagerStereoDo();
                    break;
                case SDLK_RIGHTBRACKET:
                    SoundManagerIncreaseVolume();
                    break;
                case SDLK_LEFTBRACKET:
                    SoundManagerDecreaseVolume();
                    break;
                case SDLK_p:
                    ControlManagerPauseEmulator();
                    break;
                case SDLK_t:
                    SoundManagerRecordingDo();
                    break;
                case SDLK_a:
                    requestSelectAll = 1; //SDLMainSelectAll();
                    break;
                case SDLK_s:
                    if (INPUT_key_shift)
                        PreferencesSaveConfiguration();
                    else
                        ControlManagerSaveState();
                     break;
                case SDLK_d:
                    MediaManagerRunDiskManagement();
                    break;
                case SDLK_e:
                    if (INPUT_key_shift)
                        MediaManagerRunSectorEditor();
                    else
                        MediaManagerRunDiskEditor();
                    break;
                case SDLK_n:
                    MediaManagerShowCreatePanel();
                    break;
                case SDLK_w:
                    SDLMainCloseWindow();
                    break;
                case SDLK_l:
                    if (INPUT_key_shift)
                        PreferencesLoadConfiguration();
                    else
                        ControlManagerLoadState();
                    break;
                case SDLK_o:
                    if (INPUT_key_shift)
                        MediaManagerRemoveCartridge();
                    else
                        MediaManagerInsertCartridge();
                    break;
                case SDLK_BACKSLASH:
                    return AKEY_PBI_BB_MENU;
                case SDLK_1:
                case SDLK_2:
                case SDLK_3:
                case SDLK_4:
                case SDLK_5:
                case SDLK_6:
                case SDLK_7:
                case SDLK_8:
                        if (CONTROL)
                            MediaManagerRemoveDisk(lastkey - SDLK_1 + 1);
                        else if (key_option & (lastkey <= SDLK_7))
                            requestWidthModeChange = (lastkey - SDLK_5 + 1);
                        else if (key_option & (lastkey == SDLK_8)) {
                            requestScaleModeChange = 1;
                        }
                        else
                            MediaManagerInsertDisk(lastkey - SDLK_1 + 1);
                    break;
                case SDLK_0:
                    if (CONTROL)
                        MediaManagerRemoveDisk(0);
                    if (key_option)
                        requestScaleModeChange = 3;
                    break;
                case SDLK_9:
                    if (key_option)
                        requestScaleModeChange = 2;
                    break;
                case SDLK_q:
                    return AKEY_EXIT;
                    break;
                case SDLK_h:
                    ControlManagerHideApp();
                    break;
                case SDLK_m:
                    ControlManagerMiniturize();
                    break;
                case SDLK_j:
                    ControlManagerKeyjoyEnable();
                    break;
                case SDLK_SLASH:
                    if (INPUT_key_shift)
                        ControlManagerShowHelp();
                    break;
                case SDLK_c:
                    requestCopy = 1;
                    break;
            }
        }
        
        key_pressed = 0;

        return AKEY_NONE;
    }
    
    if (key_pressed == 0) {
        return AKEY_NONE;
    }
    /* Handle the key translations for ctrl&shft numerics */
    if (INPUT_key_shift && CONTROL) {
        switch (lastkey) {
            case SDLK_COMMA:
                return(AKEY_COMMA | AKEY_SHFT);
            case SDLK_PERIOD:
                return(AKEY_FULLSTOP | AKEY_SHFT);
            case SDLK_SLASH:
                return(AKEY_SLASH | AKEY_SHFT);
            case SDLK_0:
                return(AKEY_0 | AKEY_SHFT);
            case SDLK_1:
                return(AKEY_1 | AKEY_SHFT);
            case SDLK_2:
                return(AKEY_2 | AKEY_SHFT);
            case SDLK_3:
                return(AKEY_3 | AKEY_SHFT);
            case SDLK_4:
                return(AKEY_4 | AKEY_SHFT);
            case SDLK_5:
                return(AKEY_5 | AKEY_SHFT);
            case SDLK_6:
                return(AKEY_6 | AKEY_SHFT);
            case SDLK_7:
                return(AKEY_7 | AKEY_SHFT);
            case SDLK_8:
                return(AKEY_8 | AKEY_SHFT);
            case SDLK_9:
                return(AKEY_9 | AKEY_SHFT);
        }
    }
    
    /* Handle the key translation for shifted characters */
    if (INPUT_key_shift)
        switch (lastkey) {
            case SDLK_a:
                return AKEY_A;
            case SDLK_b:
                return AKEY_B;
            case SDLK_c:
                return AKEY_C;
            case SDLK_d:
                return AKEY_D;
            case SDLK_e:
                return AKEY_E;
            case SDLK_f:
                return AKEY_F;
            case SDLK_g:
                return AKEY_G;
            case SDLK_h:
                return AKEY_H;
            case SDLK_i:
                return AKEY_I;
            case SDLK_j:
                return AKEY_J;
            case SDLK_k:
                return AKEY_K;
            case SDLK_l:
                return AKEY_L;
            case SDLK_m:
                return AKEY_M;
            case SDLK_n:
                return AKEY_N;
            case SDLK_o:
                return AKEY_O;
            case SDLK_p:
                return AKEY_P;
            case SDLK_q:
                return AKEY_Q;
            case SDLK_r:
                return AKEY_R;
            case SDLK_s:
                return AKEY_S;
            case SDLK_t:
                return AKEY_T;
            case SDLK_u:
                return AKEY_U;
            case SDLK_v:
                return AKEY_V;
            case SDLK_w:
                return AKEY_W;
            case SDLK_x:
                return AKEY_X;
            case SDLK_y:
                return AKEY_Y;
            case SDLK_z:
                return AKEY_Z;
            case SDLK_SEMICOLON:
                return AKEY_COLON;
            case SDLK_F5:
                return AKEY_COLDSTART;
            case SDLK_1:
                return AKEY_EXCLAMATION;
            case SDLK_2:
                return AKEY_AT;
            case SDLK_3:
                return AKEY_HASH;
            case SDLK_4:
                return AKEY_DOLLAR;
            case SDLK_5:
                return AKEY_PERCENT;
            case SDLK_6:
                return AKEY_CARET;
            case SDLK_7:
                return AKEY_AMPERSAND;
            case SDLK_8:
                return AKEY_ASTERISK;
            case SDLK_9:
                return AKEY_PARENLEFT;
            case SDLK_0:
                return AKEY_PARENRIGHT;
            case SDLK_EQUALS:
                return AKEY_PLUS;
            case SDLK_MINUS:
                return AKEY_UNDERSCORE;
            case SDLK_QUOTE:
                return AKEY_DBLQUOTE;
            case SDLK_SLASH:
                return AKEY_QUESTION;
            case SDLK_COMMA:
                return AKEY_LESS;
            case SDLK_PERIOD:
                return AKEY_GREATER;
            case SDLK_BACKSLASH:
                return AKEY_BAR;
            case SDLK_INSERT:
                return AKEY_INSERT_LINE;
            case SDLK_HOME:
                return AKEY_CLEAR;
            case SDLK_CAPSLOCK:
                key_pressed = 0;
                lastkey = SDLK_UNKNOWN;
                capsLockState = CAPS_UPPER;
                MacCapsLockSet(FALSE);
                return AKEY_CAPSLOCK;
        }
    /* Handle the key translation for non-shifted characters
     First check to make sure it isn't an emulated joystick key,
     since if it is, it should be ignored.  */
    else {
        if ((lookup(SDL_IsJoyKeyTable, lastkey) == 1) && keyjoyEnable && !key_option &&
            (pasteState == PASTE_IDLE)) {
            return AKEY_NONE;
        }
        if (Atari800_machine_type == Atari800_MACHINE_5200)
        {
            if ((INPUT_key_consol & INPUT_CONSOL_START) == 0)
                return(AKEY_5200_START);
            
            switch(lastkey) {
                case SDLK_p:
                    return(AKEY_5200_PAUSE);    /* pause */
                case SDLK_r:
                    return(AKEY_5200_RESET);    /* reset (5200 has no warmstart) */
                case SDLK_0:
                case SDLK_KP_0:
                    return(AKEY_5200_0);        /* controller numpad keys (0-9) */
                case SDLK_1:
                case SDLK_KP_1:
                    return(AKEY_5200_1);
                case SDLK_2:
                case SDLK_KP_2:
                    return(AKEY_5200_2);
                case SDLK_3:
                case SDLK_KP_3:
                    return(AKEY_5200_3);
                case SDLK_4:
                case SDLK_KP_4:
                    return(AKEY_5200_4);
                case SDLK_5:
                case SDLK_KP_5:
                    return(AKEY_5200_5);
                case SDLK_6:
                case SDLK_KP_6:
                    return(AKEY_5200_6);
                case SDLK_7:
                case SDLK_KP_7:
                    return(AKEY_5200_7);
                case SDLK_8:
                case SDLK_KP_8:
                    return(AKEY_5200_8);
                case SDLK_9:
                case SDLK_KP_9:
                    return(AKEY_5200_9);
                case SDLK_MINUS:
                case SDLK_KP_MINUS:
                    return(AKEY_5200_HASH);    /* # key on 5200 controller */
                case SDLK_ASTERISK:
                case SDLK_KP_MULTIPLY:
                    return(AKEY_5200_ASTERISK);    /* * key on 5200 controller */
                case SDLK_F9:
                case SDLK_F8:
                case SDLK_F13:
                case SDLK_F6:
                case SDLK_F1:
                case SDLK_F5:
                case SDLK_LEFT:
                case SDLK_RIGHT:
                case SDLK_UP:
                case SDLK_DOWN:
                case SDLK_ESCAPE:
                case SDLK_TAB:
                case SDLK_RETURN:
                    break;
                default:
                    return AKEY_NONE;
            }
        }
        switch (lastkey) {
            case SDLK_a:
                return AKEY_a;
            case SDLK_b:
                return AKEY_b;
            case SDLK_c:
                return AKEY_c;
            case SDLK_d:
                return AKEY_d;
            case SDLK_e:
                return AKEY_e;
            case SDLK_f:
                return AKEY_f;
            case SDLK_g:
                return AKEY_g;
            case SDLK_h:
                return AKEY_h;
            case SDLK_i:
                return AKEY_i;
            case SDLK_j:
                return AKEY_j;
            case SDLK_k:
                return AKEY_k;
            case SDLK_l:
                return AKEY_l;
            case SDLK_m:
                return AKEY_m;
            case SDLK_n:
                return AKEY_n;
            case SDLK_o:
                return AKEY_o;
            case SDLK_p:
                return AKEY_p;
            case SDLK_q:
                return AKEY_q;
            case SDLK_r:
                return AKEY_r;
            case SDLK_s:
                return AKEY_s;
            case SDLK_t:
                return AKEY_t;
            case SDLK_u:
                return AKEY_u;
            case SDLK_v:
                return AKEY_v;
            case SDLK_w:
                return AKEY_w;
            case SDLK_x:
                return AKEY_x;
            case SDLK_y:
                return AKEY_y;
            case SDLK_z:
                return AKEY_z;
            case SDLK_SEMICOLON:
                return AKEY_SEMICOLON;
            case SDLK_F5:
                return AKEY_WARMSTART;
            case SDLK_F10:
                return AKEY_HELP;
            case SDLK_0:
                return AKEY_0;
            case SDLK_1:
                return AKEY_1;
            case SDLK_2:
                return AKEY_2;
            case SDLK_3:
                return AKEY_3;
            case SDLK_4:
                return AKEY_4;
            case SDLK_5:
                return AKEY_5;
            case SDLK_6:
                return AKEY_6;
            case SDLK_7:
                return AKEY_7;
            case SDLK_8:
                return AKEY_8;
            case SDLK_9:
                return AKEY_9;
            case SDLK_COMMA:
                return AKEY_COMMA;
            case SDLK_PERIOD:
                return AKEY_FULLSTOP;
            case SDLK_EQUALS:
                return AKEY_EQUAL;
            case SDLK_MINUS:
                return AKEY_MINUS;
            case SDLK_QUOTE:
                return AKEY_QUOTE;
            case SDLK_SLASH:
                return AKEY_SLASH;
            case SDLK_BACKSLASH:
                return AKEY_BACKSLASH;
            case SDLK_LEFTBRACKET:
                return AKEY_BRACKETLEFT;
            case SDLK_RIGHTBRACKET:
                return AKEY_BRACKETRIGHT;
            case SDLK_F7:
                key_pressed = 0;
                requestLimitChange=1;
                return AKEY_NONE;
            case SDLK_F8:
                key_pressed = 0;
                requestMonitor = TRUE;
                return AKEY_NONE;
            case SDLK_F13:
                key_pressed = 0;
                return AKEY_SCREENSHOT;
            case SDLK_F6:
                SwitchGrabMouse();
                key_pressed = 0;
                return AKEY_NONE;
            case SDLK_INSERT:
                return AKEY_INSERT_CHAR;
            case SDLK_CAPSLOCK:
                key_pressed = 0;
                lastkey = SDLK_UNKNOWN;
                MacCapsLockSet(FALSE);
                if (CONTROL) {
                    capsLockState = CAPS_GRAPHICS;
                }
                else if (Atari800_machine_type == Atari800_MACHINE_XLXE) {
                    if (capsLockState == CAPS_UPPER)
                        capsLockState = CAPS_LOWER;
                    else
                        capsLockState = CAPS_UPPER;
                }
                else
                    capsLockState = CAPS_LOWER;
                return AKEY_CAPSTOGGLE;
        }
    }
    if (INPUT_cx85) switch (lastkey) {
        case SDLK_KP_1:
            return AKEY_CX85_1;
        case SDLK_KP_2:
            return AKEY_CX85_2;
        case SDLK_KP_3:
            return AKEY_CX85_3;
        case SDLK_KP_4:
            return AKEY_CX85_4;
        case SDLK_KP_5:
            return AKEY_CX85_5;
        case SDLK_KP_6:
            return AKEY_CX85_6;
        case SDLK_KP_7:
            return AKEY_CX85_7;
        case SDLK_KP_8:
            return AKEY_CX85_8;
        case SDLK_KP_9:
            return AKEY_CX85_9;
        case SDLK_KP_0:
            return AKEY_CX85_0;
        case SDLK_KP_PERIOD:
            return AKEY_CX85_PERIOD;
        case SDLK_KP_MINUS:
            return AKEY_CX85_MINUS;
        case SDLK_KP_ENTER:
            return AKEY_CX85_PLUS_ENTER;
        case SDLK_KP_DIVIDE:
            return (CONTROL ? AKEY_CX85_ESCAPE : AKEY_CX85_NO);
        case SDLK_KP_MULTIPLY:
            return AKEY_CX85_DELETE;
        case SDLK_KP_PLUS:
            return AKEY_CX85_YES;
    }
    
    /* Handle the key translation for special characters
     First check to make sure it isn't an emulated joystick key,
     since if it is, it should be ignored.  */
    if ((lookup(SDL_IsJoyKeyTable, lastkey) == 1) && keyjoyEnable && !key_option &&
        (pasteState == PASTE_IDLE)) {
        return AKEY_NONE;
    }
    switch (lastkey) {
        case SDLK_END:
            if (INPUT_key_shift)
                return AKEY_ATARI | AKEY_SHFT;
            else
                return AKEY_ATARI;
        case SDLK_PAGEDOWN:
            if (INPUT_key_shift)
                return AKEY_HELP | AKEY_SHFT;
            else
                return AKEY_HELP;
        case SDLK_PAGEUP:
            capsLockState = CAPS_UPPER;
            return AKEY_CAPSLOCK;
        case SDLK_HOME:
            if (CONTROL)
                return AKEY_LESS;
            else
                return AKEY_CLEAR;
        case SDLK_PAUSE:
        case SDLK_BACKQUOTE:
            return AKEY_BREAK;
        case SDLK_F15:
            if (INPUT_key_shift)
                return AKEY_BREAK;
        case SDLK_CAPSLOCK:
            if (INPUT_key_shift) {
                capsLockState = CAPS_UPPER;
                return AKEY_CAPSLOCK;
            }
            else {
                if (CONTROL) {
                    capsLockState = CAPS_GRAPHICS;
                }
                else if (Atari800_machine_type == Atari800_MACHINE_XLXE) {
                    if (capsLockState == CAPS_UPPER)
                        capsLockState = CAPS_LOWER;
                    else
                        capsLockState = CAPS_UPPER;
                }
                else
                    capsLockState = CAPS_LOWER;
                return AKEY_CAPSTOGGLE;
            }
        case SDLK_SPACE:
            if (INPUT_key_shift)
                return AKEY_SPACE | AKEY_SHFT;
            else
                return AKEY_SPACE;
        case SDLK_BACKSPACE:
            if (INPUT_key_shift)
                return AKEY_BACKSPACE | AKEY_SHFT;
            else
                return AKEY_BACKSPACE;
        case SDLK_RETURN:
            if (INPUT_key_shift)
                return AKEY_RETURN | AKEY_SHFT;
            else
                return AKEY_RETURN;
        case SDLK_F9:
            return AKEY_EXIT;
        case SDLK_F1:
            return AKEY_UI;
        case SDLK_LEFT:
            if (useAtariCursorKeys == USE_ATARI_CURSOR_ARROW_ONLY) {
                if (INPUT_key_shift)
                    return AKEY_PLUS | AKEY_SHFT;
                else
                    return AKEY_PLUS;
            } else if (useAtariCursorKeys == USE_ATARI_CURSOR_FX) {
                if (INPUT_key_shift)
                    return AKEY_F3 | AKEY_SHFT;
                else
                    return AKEY_F3;
            } else {
                if (INPUT_key_shift && CONTROL)
                    return AKEY_PLUS | AKEY_SHFT;
                else
                    return AKEY_LEFT;
            }
        case SDLK_RIGHT:
            if (useAtariCursorKeys == USE_ATARI_CURSOR_ARROW_ONLY) {
                if (INPUT_key_shift)
                    return AKEY_ASTERISK | AKEY_SHFT;
                else
                    return AKEY_ASTERISK;
            } else if (useAtariCursorKeys == USE_ATARI_CURSOR_FX) {
                if (INPUT_key_shift)
                    return AKEY_F4 | AKEY_SHFT;
                else
                    return AKEY_F4;
            } else {
                if (INPUT_key_shift && CONTROL)
                    return AKEY_ASTERISK | AKEY_SHFT;
                else
                    return AKEY_RIGHT;
            }
        case SDLK_UP:
            if (useAtariCursorKeys == USE_ATARI_CURSOR_ARROW_ONLY) {
                if (INPUT_key_shift)
                    return AKEY_MINUS | AKEY_SHFT;
                else
                    return AKEY_MINUS;
            } else if (useAtariCursorKeys == USE_ATARI_CURSOR_FX) {
                if (INPUT_key_shift)
                    return AKEY_F1 | AKEY_SHFT;
                else
                    return AKEY_F1;
            } else {
                if (INPUT_key_shift && CONTROL)
                    return AKEY_MINUS | AKEY_SHFT;
                else
                    return AKEY_UP;
            }
        case SDLK_DOWN:
            if (useAtariCursorKeys == USE_ATARI_CURSOR_ARROW_ONLY) {
                if (INPUT_key_shift)
                    return AKEY_EQUAL | AKEY_SHFT;
                else
                    return AKEY_EQUAL;
            } else if (useAtariCursorKeys == USE_ATARI_CURSOR_FX) {
                if (INPUT_key_shift)
                    return AKEY_F2 | AKEY_SHFT;
                else
                    return AKEY_F2;
            } else {
                if (INPUT_key_shift && CONTROL)
                    return AKEY_EQUAL | AKEY_SHFT;
                else
                    return AKEY_DOWN;
            }
        case SDLK_ESCAPE:
            if (INPUT_key_shift)
                return AKEY_ESCAPE | AKEY_SHFT;
            else
                return AKEY_ESCAPE;
        case SDLK_TAB:
            if (INPUT_key_shift)
                return AKEY_SETTAB;
            else if (CONTROL)
                return AKEY_CLRTAB;
            else
                return AKEY_TAB;
        case SDLK_DELETE:
            if (INPUT_key_shift)
                return AKEY_DELETE_LINE;
            else
                return AKEY_DELETE_CHAR;
        case SDLK_INSERT:
            if (INPUT_key_shift)
                return AKEY_INSERT_LINE;
            else
                return AKEY_INSERT_CHAR;
    }
    return AKEY_NONE;
}

/*------------------------------------------------------------------------------
*  Atari_Keyboard - This function is called once per main loop to handle 
*    keyboard input.
*-----------------------------------------------------------------------------*/
int Atari_Keyboard(void)
{
    return(Atari_Keyboard_International());
}

int PLATFORM_Keyboard(void)
{
	return(Atari_Keyboard());
}

/*------------------------------------------------------------------------------
*  Check_SDL_Joysticks - Initialize the use of up to two joysticks from the SDL
*   Library.  
*-----------------------------------------------------------------------------*/
void Check_SDL_Joysticks()
{
    if (joystick0 != NULL) {
        Log_print("Joystick 0 is found");
        joystick0_nbuttons = SDL_JoystickNumButtons(joystick0);
        joystick0_nsticks = SDL_JoystickNumAxes(joystick0)/2;
        joystick0_nhats = SDL_JoystickNumHats(joystick0);
        if (joystick0TypePref == JOY_TYPE_STICK) {
            if (joystick0_nsticks) 
                joystickType0 = JOY_TYPE_STICK;
            else if (joystick0_nhats) {
                Log_print("Joystick 0: Analog not available, using digital");
                joystickType0 = JOY_TYPE_HAT;
                }
            else {
                Log_print("Joystick 0: Analog and Digital not available, using buttons");
                joystickType0 = JOY_TYPE_BUTTONS;
                }
            }
        else if (joystick0TypePref == JOY_TYPE_HAT) {
            if (joystick0_nhats)
                joystickType0 = JOY_TYPE_HAT;
            else if (joystick0_nsticks) {
                Log_print("Joystick 0: Digital not available, using analog");
                joystickType0 = JOY_TYPE_STICK;
                }
            else {
                Log_print("Joystick 0: Analog and Digital not available, using buttons");
                joystickType0 = JOY_TYPE_BUTTONS;
                }
            }
        else
            joystickType0 = JOY_TYPE_BUTTONS;
        }
        
    if (joystick1 != NULL) {
        Log_print("Joystick 1 is found");
        joystick1_nbuttons = SDL_JoystickNumButtons(joystick1);
        joystick1_nsticks = SDL_JoystickNumAxes(joystick1)/2;
        joystick1_nhats = SDL_JoystickNumHats(joystick1);
        if (joystick1TypePref == JOY_TYPE_STICK) {
            if (joystick1_nsticks)
                joystickType1 = JOY_TYPE_STICK;
            else if (joystick1_nhats) {
                Log_print("Joystick 1: Analog not available, using digital");
                joystickType1 = JOY_TYPE_HAT;
                }
            else {
                Log_print("Joystick 1: Analog and Digital not available, using buttons");
                joystickType1 = JOY_TYPE_BUTTONS;
                }
            }
        else if (joystick1TypePref == JOY_TYPE_HAT) {
            if (joystick1_nhats)
                joystickType1 = JOY_TYPE_HAT;
            else if (joystick1_nsticks) {
                Log_print("Joystick 1: Digital not available, using analog");
                joystickType1 = JOY_TYPE_STICK;
                }
            else {
                Log_print("Joystick 1: Analog and Digital not available, using buttons");
                joystickType1 = JOY_TYPE_BUTTONS;
                }
            }
        else
            joystickType1 = JOY_TYPE_BUTTONS;
        }

    if (joystick2 != NULL) {
        Log_print("Joystick 2 is found");
        joystick2_nbuttons = SDL_JoystickNumButtons(joystick2);
        joystick2_nsticks = SDL_JoystickNumAxes(joystick2)/2;
        joystick2_nhats = SDL_JoystickNumHats(joystick2);
        if (joystick2TypePref == JOY_TYPE_STICK) {
            if (joystick2_nsticks)
                joystickType2 = JOY_TYPE_STICK;
            else if (joystick2_nhats) {
                Log_print("Joystick 2: Analog not available, using digital");
                joystickType2 = JOY_TYPE_HAT;
                }
            else {
                Log_print("Joystick 2: Analog and Digital not available, using buttons");
                joystickType2 = JOY_TYPE_BUTTONS;
                }
            }
        else if (joystick2TypePref == JOY_TYPE_HAT) {
            if (joystick2_nhats)
                joystickType2 = JOY_TYPE_HAT;
            else if (joystick2_nsticks) {
                Log_print("Joystick 2: Digital not available, using analog");
                joystickType2 = JOY_TYPE_STICK;
                }
            else {
                Log_print("Joystick 2: Analog and Digital not available, using buttons");
                joystickType2 = JOY_TYPE_BUTTONS;
                }
            }
        else
            joystickType2 = JOY_TYPE_BUTTONS;
        }

    if (joystick3 != NULL) {
        Log_print("Joystick 3 is found");
        joystick3_nbuttons = SDL_JoystickNumButtons(joystick3);
        joystick3_nsticks = SDL_JoystickNumAxes(joystick3)/2;
        joystick3_nhats = SDL_JoystickNumHats(joystick3);
        if (joystick1TypePref == JOY_TYPE_STICK) {
            if (joystick3_nsticks)
                joystickType3 = JOY_TYPE_STICK;
            else if (joystick3_nhats) {
                Log_print("Joystick 3: Analog not available, using digital");
                joystickType3 = JOY_TYPE_HAT;
                }
            else {
                Log_print("Joystick 3: Analog and Digital not available, using buttons");
                joystickType3 = JOY_TYPE_BUTTONS;
                }
            }
        else if (joystick3TypePref == JOY_TYPE_HAT) {
            if (joystick3_nhats)
                joystickType3 = JOY_TYPE_HAT;
            else if (joystick3_nsticks) {
                Log_print("Joystick 3: Digital not available, using analog");
                joystickType3 = JOY_TYPE_STICK;
                }
            else {
                Log_print("Joystick 3: Analog and Digital not available, using buttons");
                joystickType3 = JOY_TYPE_BUTTONS;
                }
            }
        else
            joystickType3 = JOY_TYPE_BUTTONS;
        }
}

/*------------------------------------------------------------------------------
*  Open_SDL_Joysticks - Initialize the use of up to two joysticks from the SDL
*   Library.  
*-----------------------------------------------------------------------------*/
void Open_SDL_Joysticks()
{
    if (joystick0)
        SDL_JoystickClose(joystick0);
    joystick0 = SDL_JoystickOpen(0);
    joysticks[0] = joystick0;
    if (joystick1)
        SDL_JoystickClose(joystick1);
    joystick1 = SDL_JoystickOpen(1);
    joysticks[1] = joystick1;
    if (joystick2)
        SDL_JoystickClose(joystick2);
    joystick2 = SDL_JoystickOpen(2);
    joysticks[2] = joystick2;
    if (joystick3)
        SDL_JoystickClose(joystick3);
    joystick3 = SDL_JoystickOpen(3);
    joysticks[3] = joystick3;
    Check_SDL_Joysticks();
}

/*------------------------------------------------------------------------------
*  Remove_Bad_SDL_Joysticks - If the user has requested joysticks in the
*   preferences that aren't plugged in, make sure that it is switch to no 
*   joystick for that port.
*-----------------------------------------------------------------------------*/
void Remove_Bad_SDL_Joysticks()
{
    int i;

    if (joystick0 == NULL) {
        for (i=0;i<NUM_JOYSTICKS;i++) {
            if (JOYSTICK_MODE[i] == SDL0_JOYSTICK)
                JOYSTICK_MODE[i] = NO_JOYSTICK;
            }
        Log_print("joystick 0 not found");
        }

    if (joystick1 == NULL) {
        for (i=0;i<NUM_JOYSTICKS;i++) {
            if (JOYSTICK_MODE[i] == SDL1_JOYSTICK)
                JOYSTICK_MODE[i] = NO_JOYSTICK;
            }
        Log_print("joystick 1 not found");
        }

    if (joystick2 == NULL) {
        for (i=0;i<NUM_JOYSTICKS;i++) {
            if (JOYSTICK_MODE[i] == SDL2_JOYSTICK)
                JOYSTICK_MODE[i] = NO_JOYSTICK;
            }
        Log_print("joystick 2 not found");
        }

    if (joystick3 == NULL) {
        for (i=0;i<NUM_JOYSTICKS;i++) {
            if (JOYSTICK_MODE[i] == SDL3_JOYSTICK)
                JOYSTICK_MODE[i] = NO_JOYSTICK;
            }
        Log_print("joystick 3 not found");
        }
}

/*------------------------------------------------------------------------------
*  Init_SDL_Joykeys - Initializes the array that is used to determine if a 
*    keypress is used by the emulated joysticks.
*-----------------------------------------------------------------------------*/
void Init_SDL_Joykeys()
{
    int i;
    int keypadJoystick = FALSE;
    int userDefJoystick = FALSE;

    if (SDL_IsJoyKeyTable)
        deleteTable(SDL_IsJoyKeyTable);
    SDL_IsJoyKeyTable = createTable(256);
    
    /*  Check to see if any of the joystick ports use the Keypad as joystick */
    for (i=0;i<NUM_JOYSTICKS;i++)
        if (JOYSTICK_MODE[i] == KEYPAD_JOYSTICK)
            keypadJoystick = TRUE;
    /*  Check to see if any of the joystick ports use the User defined keys
          as joystick */
    for (i=0;i<NUM_JOYSTICKS;i++)
        if (JOYSTICK_MODE[i] == USERDEF_JOYSTICK)
            userDefJoystick = TRUE;
     
    /* If used as a joystick, set the keypad keys as emulated */               
    if (keypadJoystick) {
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_TRIG_0), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_TRIG_0_B), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_TRIG_0_R), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_TRIG_0_B_R), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_JOY_0_LEFT), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_JOY_0_RIGHT), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_JOY_0_DOWN), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_JOY_0_UP), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_JOY_0_LEFTUP), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_JOY_0_RIGHTUP), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_JOY_0_LEFTDOWN), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_JOY_0_RIGHTDOWN), 1);
    }

    /* If used as a joystick, set the user defined keys as emulated */               
    if (userDefJoystick) {
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_TRIG_1), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_TRIG_1_B), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_TRIG_1_R), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_TRIG_1_B_R), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_JOY_1_LEFT), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_JOY_1_RIGHT), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_JOY_1_DOWN), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_JOY_1_UP), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_JOY_1_LEFTUP), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_JOY_1_RIGHTUP), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_JOY_1_LEFTDOWN), 1);
        insert(SDL_IsJoyKeyTable, SDL_GetKeyFromScancode(SDL_JOY_1_RIGHTDOWN), 1);
    }
}

/*------------------------------------------------------------------------------
*  Init_Joysticks - Initializes the joysticks for the emulator.  The command
*    line args are currently unused.
*-----------------------------------------------------------------------------*/
void Init_Joysticks(int *argc, char *argv[])
{
    Open_SDL_Joysticks();
    Remove_Bad_SDL_Joysticks();
    Init_SDL_Joykeys();        
}

/*------------------------------------------------------------------------------
*  checkForNewJoysticks - Check if new joysticks have been adeed/removed
*-----------------------------------------------------------------------------*/
void checkForNewJoysticks(void) {
    SDL_JoystickUpdate();
    if (joystickCount != SDL_NumJoysticks()) {
        joystickCount = SDL_NumJoysticks();
        Reinit_Joysticks();
        UpdatePreferencesJoysticks();
        PreferencesIdentifyGamepadNew();
    }
}

/*------------------------------------------------------------------------------
*  Reinit_Joysticks - Initializes the joysticks for the emulator.  The command
*    line args are currently unused.
*-----------------------------------------------------------------------------*/
void Reinit_Joysticks(void)
{
	printf("Reiniting joystics....\n");
    reloadMacJoyPrefs();
	Init_Joysticks(NULL,NULL);
}

/*------------------------------------------------------------------------------
*  Atari_Initialise - Initializes the SDL video and sound functions.  The command
*    line args are currently unused.
*-----------------------------------------------------------------------------*/
int PLATFORM_Initialise(int *argc, char *argv[])
{
    int i, j;
    int no_joystick;
    int help_only = FALSE;

    no_joystick = 0;
    our_width = Screen_WIDTH;
    our_height = Screen_HEIGHT;

    for (i = j = 1; i < *argc; i++) {
                if (strcmp(argv[i], "-nojoystick") == 0) {
            no_joystick = 1;
            i++;
        }
       else {
            if (strcmp(argv[i], "-help") == 0) {
                help_only = TRUE;
                Log_print("\t-nojoystick      Disable joystick");
            }
            argv[j++] = argv[i];
        }
    }
    *argc = j;

    i = SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO;
    if (SDL_Init(i) != 0) {
        Log_print("SDL_Init FAILED");
        Log_print((char *) SDL_GetError());
        Log_flushlog();
        exit(-1);
    }
    joystickCount = SDL_NumJoysticks();
    atexit(SDL_Quit);

    SDL_Sound_Initialise(argc, argv);

    if (help_only)
        return TRUE;     /* return before changing the gfx mode */
    
    InitializeVideo();
    Atari800OriginSet();
    CalcPalette();
    SetPalette();
    
    if (no_joystick == 0)
        Init_Joysticks(argc, argv);
    
    return TRUE;
}

/*------------------------------------------------------------------------------
*  PLATFORM_Exit - Exits the emulator.  Someday, it will allow you to run the 
*     built in monitor.  Right now, if monitor is called for, displays fatal
*     error dialog box.
*-----------------------------------------------------------------------------*/
int PLATFORM_Exit(int run_monitor)
{
    int restart;
    int action;

    if (run_monitor) {
        if (requestMonitor || !CPU_cim_encountered) {
            /* run the monitor....*/ 
            restart = ControlManagerMonitorRun();
            requestMonitor = FALSE;
            CPU_cim_encountered = 0;
            }
        else {
            /* Run the crash dialogue */
            action = ControlManagerFatalError();
            if (action == 1) {
                Atari800_Warmstart();
                restart = TRUE;
                }
            else if (action == 2) {
                Atari800_Coldstart();
                restart = TRUE;
                }
            else if (action == 3) {
                restart = ControlManagerMonitorRun();
                }
            else if (action == 4) {
                CARTRIDGE_Remove();
                UpdateMediaManagerInfo();
                Atari800_Coldstart();
                restart = TRUE;
                }
            else if (action == 5) {
                SIO_Dismount(1);
                UpdateMediaManagerInfo();
                Atari800_Coldstart();
                restart = TRUE;
                }
            else if (action == 6) {
                RunPreferences();
                restart = TRUE;
                }
            else {
                restart = FALSE;
                }
            }
        }
    else
        restart = FALSE;

    if (restart) {
        /* set up graphics and all the stuff */
        return 1;
    }
    else {
		PreferencesSaveDefaults();
        MacCapsLockSet(FALSE);
        return(0);
		}
}

/*------------------------------------------------------------------------------
*  DisplayWithoutScaling16bpp - Displays the emulated atari screen, in single
*    size mode, with 16 bits per pixel.
*-----------------------------------------------------------------------------*/
void DisplayWithoutScaling16bpp(Uint8 * screen, int jumped, int width,
                                 int first_row, int last_row)
{
    register Uint8 *fromPtr;
    register Uint16 *toPtr;
    register int i,j;
    register Uint32 *start32;
    register int pitch4;
	int screen_width = (PLATFORM_80col ? XEP80_SCRN_WIDTH : Screen_WIDTH);

    pitch4 = MainScreen->pitch / 4;
    start32 = (Uint32 *) MainScreen->pixels + (first_row * pitch4);

    screen = screen + jumped + (first_row * screen_width);
    i = last_row - first_row + 1;
    while (i > 0) {
        j = width;
        toPtr = (Uint16 *) start32;
        fromPtr = screen;
        while (j > 0) {
            *toPtr++ = Palette16[*fromPtr++];
            j--;
            }
        screen += screen_width;
        start32 += pitch4;
        i--;
        }
}

void DisplayAF80WithoutScaling16bpp(int first_row, int last_row, int blink)
{
    Uint32 const black = (Uint32)Palette16[0];
    Uint32 const black2 = black << 16;
    register Uint32 *start32;
    unsigned int column;
    UBYTE pixels;
    register int pitch4;

    pitch4 = MainScreen->pitch / 4;
    start32 = (Uint32 *) MainScreen->pixels + (first_row * pitch4);
    register Uint32 quad;
    for (; first_row < last_row; first_row++) {
        for (column = 0; column < 80; column++) {
            int i;
            int colour;
            pixels = AF80_GetPixels(first_row, column, &colour, blink);
            for (i = 0; i < 4; i++) {
                if (pixels & 0x01)
                    quad = AF80_palette[colour];
                else
                    quad = black;
                if (pixels & 0x02)
                    quad |= AF80_palette[colour] << 16;
                else
                    quad |= black2;
                *start32++ = quad;
                pixels >>= 2;
            }
        }
        start32 += pitch4 - (4*80);
    }
}

void DisplayBit3WithoutScaling16bpp(int first_line, int last_line, int blink)
{
    register Uint32 *start32;
    Uint32 const black = (Uint32)Palette16[0];
    Uint32 const black2 = black << 16;

    register int pitch4;

    pitch4 = MainScreen->pitch / 4;
    start32 = (Uint32 *) MainScreen->pixels + (first_line * pitch4);
    unsigned int column;
    UBYTE pixels;
    register Uint32 quad;
    for (; first_line < last_line; first_line++) {
        for (column = 0; column < 80; column++) {
            int i;
            int colour;
            pixels = BIT3_GetPixels(first_line, column, &colour, blink);
            for (i = 0; i < 4; i++) {
                if (pixels & 0x01)
                    quad = BIT3_palette[colour];
                else
                    quad = black;
                if (pixels & 0x02)
                    quad |= BIT3_palette[colour] << 16;
                else
                    quad |= black2;
                *start32++ = quad;
                pixels >>= 2;
            }
        }
        start32 += pitch4 - (4*80);
    }
}

/*------------------------------------------------------------------------------
*  Display_Line_Equal - Determines if two display lines are equal.  Used as a 
*     test to determine which parts of the screen must be redrawn.
*-----------------------------------------------------------------------------*/
int Display_Line_Equal(ULONG *line1, ULONG *line2)
{
    int i;
    int equal = TRUE;
    
    i = Screen_WIDTH/4;
    while (i>0) {
        if (*line1++ != *line2++) {
            equal = FALSE;
            break;
            }
        i--;
        }
    
    return(equal);      
}

void PLATFORM_DisplayScreen(void)
{
	Atari_DisplayScreen((UBYTE *) Screen_atari);
}	

/*------------------------------------------------------------------------------
*  Atari_DisplayScreen - Displays the Atari screen, redrawing only lines in the
*    middle of the screen that have changed.
*-----------------------------------------------------------------------------*/
void Atari_DisplayScreen(UBYTE * screen)
{
    int width, jumped;
    int first_row = 0;
    int last_row = Screen_HEIGHT - 1;
    ULONG *line_start1, *line_start2;
    static int xep80Frame = 0;
    static int af80Frame = 0;
    static int bit3Frame = 0;
    SDL_Rect rect;
	
    switch (WIDTH_MODE) {
        case SHORT_WIDTH_MODE:
            width = SCREEN_WIDTH_SHORT;
            jumped = 24 + 8;
            break;
        case DEFAULT_WIDTH_MODE:
            width = SCREEN_WIDTH_DEFAULT;
            jumped = 24;
            break;
        case FULL_WIDTH_MODE:
            width = SCREEN_WIDTH_FULL;
            jumped = 0;
            break;
        default:
            Log_print("unsupported WIDTH_MODE");
            Log_flushlog();
            exit(-1);
            break;
        }
	
    if (PLATFORM_80col && XEP80_enabled) {
		width = XEP80_SCRN_WIDTH;
		jumped = 0;
		if (xep80Frame < 30) {
            screen = (UBYTE *) XEP80_screen_1;
            }
		else {
            screen = (UBYTE *) XEP80_screen_2;
            }
        xep80Frame++;
        if (xep80Frame >= 60) {
            xep80Frame = 0;
			}
		if (xep80Frame == 1 || xep80Frame == 31 || full_display) {
			XEP80_first_row = 0;
			XEP80_last_row = XEP80_SCRN_HEIGHT - 1;
		}
		if (XEP80_last_row == 0) {
			first_row = 0;
			last_row = 0;
		}
		else {
			first_row = XEP80_first_row;
			last_row = XEP80_last_row;
		}
		XEP80_last_row = 0;
		XEP80_first_row = XEP80_SCRN_HEIGHT - 1;
		}
    else if (PLATFORM_80col && AF80_enabled) {
        width = AF80_SCRN_WIDTH;
        first_row = 0;
        last_row = AF80_SCRN_HEIGHT - 1;;
        af80Frame++;
        if (af80Frame >= 60) {
            af80Frame = 0;
            }
        }
    else if (PLATFORM_80col && BIT3_enabled) {
        width = BIT3_SCRN_WIDTH;
        first_row = 0;
        last_row = BIT3_SCRN_HEIGHT - 1;;
        bit3Frame++;
        if (bit3Frame >= 60) {
            bit3Frame = 0;
            }
        }
	else if (!full_display) {
		line_start1 = Screen_atari + (Screen_WIDTH/4 * (Screen_HEIGHT-1));
		line_start2 = Screen_atari_b + (Screen_WIDTH/4 * (Screen_HEIGHT-1));
		last_row = Screen_HEIGHT-1;
		while((0 < last_row) && Display_Line_Equal(line_start1, line_start2)) {
			last_row--;
			line_start1 -= Screen_WIDTH/4;
			line_start2 -= Screen_WIDTH/4;
			}
		/* Find the first row of the screen that changed since last redraw */
		line_start1 = Screen_atari;
		line_start2 = Screen_atari_b;
		while((first_row < last_row) && Display_Line_Equal(line_start1, line_start2)) {
			first_row++;
			line_start1 += Screen_WIDTH/4;
			line_start2 += Screen_WIDTH/4;
			}
		}
	else
		full_display--;
		
    if (PLATFORM_80col && AF80_enabled) {
        DisplayAF80WithoutScaling16bpp(first_row, last_row, af80Frame >= 30);
    } else if (PLATFORM_80col && BIT3_enabled) {
            DisplayBit3WithoutScaling16bpp(first_row, last_row, bit3Frame / 30);
    } else {
        DisplayWithoutScaling16bpp(screen, jumped, width, first_row, last_row);
    }
    
    // If not in mouse emulation or Fullscreen, check for copy selection
	if (INPUT_mouse_mode == INPUT_MOUSE_OFF)
		ProcessCopySelection(&first_row, &last_row, requestSelectAll);
	requestSelectAll = 0;
	
    Uint32 scaledWidth, scaledHeight;
    int screen_height, screen_width;
    if (PLATFORM_80col) {
        if (XEP80_enabled) {
            screen_height = XEP80_SCRN_HEIGHT;
            screen_width = XEP80_SCRN_WIDTH;
        } else if (AF80_enabled) {
            screen_height = AF80_SCRN_HEIGHT;
            screen_width = AF80_SCRN_WIDTH;
        } else {
            screen_height = BIT3_SCRN_HEIGHT;
            screen_width = BIT3_SCRN_WIDTH;
        }
    } else {
        screen_height = Screen_HEIGHT;
        screen_width = Screen_WIDTH;
    }
    
    scaledWidth = width;
    scaledHeight = screen_height;
    
    texture = SDL_CreateTextureFromSurface(renderer, MainScreen);

    //Copying the texture on to the window using renderer and rectangle
    rect.x = screen_x_offset;
    rect.y = screen_y_offset;
    rect.w = MainScreen->w;
    rect.h = MainScreen->h;
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, &rect);

    // Add the scanlines if we are in that mode
    if (SCALE_MODE == SCANLINE_SCALE)
        {
        int rows = 0;
        SDL_Rect scanlineRect;
        float oldScaleX, oldScaleY;
        
        SDL_RenderGetScale(renderer, &oldScaleX, &oldScaleY);
        SDL_RenderSetScale(renderer, scaleFactorFloat, 1);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        for (rows = 0; rows < MainScreen->h; rows ++)
            {
            scanlineRect.x = 0;
            scanlineRect.w = (screen_width*3)/2;
            scanlineRect.y = rows*scaleFactorFloat+scaleFactorFloat;
            scanlineRect.h = 1;
            SDL_RenderFillRect(renderer, &scanlineRect);
            }
        SDL_RenderSetScale(renderer, oldScaleX, oldScaleY);
        }

    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(texture);
}

// two empty functions, needed by input.c and platform.h

int Atari_PORT(int num)
{
    return 0;
}

int Atari_TRIG(int num)
{
    return 0;
}

/* INPUT_Exit removed - now provided by real input.c */

/*------------------------------------------------------------------------------
*  convert_SDL_to_Pot - Convert SDL axis values into paddle values
*-----------------------------------------------------------------------------*/
UBYTE convert_SDL_to_Pot(SWORD axis)
{
    UBYTE port;
    SWORD naxis;
    
    naxis = -axis;
    
    if (naxis < 0) {
        naxis = -(naxis - 256);
        naxis = naxis >> 8;
        if (naxis > 128)
            naxis = 128;
        port = 128 - naxis;
        }
    else
        {
        naxis = naxis >> 8;
        if (naxis > 99)
            naxis = 99;
        port = 128 + naxis;
        }
    return(port);
}

/*------------------------------------------------------------------------------
*  convert_SDL_to_Pot5200 - Convert SDL axis values into analog 5200 joystick 
*    values
*-----------------------------------------------------------------------------*/
UBYTE convert_SDL_to_Pot5200(SWORD axis)
{
    SWORD port;
    
    if (axis < 0) {
        axis = -(axis + 256);
        axis = axis >> 8;
        if (axis > (INPUT_joy_5200_center - INPUT_joy_5200_min))
            axis = INPUT_joy_5200_center - INPUT_joy_5200_min;
        port = INPUT_joy_5200_center - axis;
        }
    else
        {
        axis = axis >> 8;
        if (axis > (INPUT_joy_5200_max - INPUT_joy_5200_center))
            axis = INPUT_joy_5200_max - INPUT_joy_5200_center;
        port = INPUT_joy_5200_center + axis;
        }
        
    return(port);
}

/*------------------------------------------------------------------------------
*  get_SDL_joystick_state - Return the states of one of the Attached HID 
*     joysticks.
*-----------------------------------------------------------------------------*/
int get_SDL_joystick_state(SDL_Joystick *joystick, int stickNum, int *x_pot, int *y_pot)
{
    int x;
    int y;

    x = SDL_JoystickGetAxis(joystick, stickNum*2);
    y = SDL_JoystickGetAxis(joystick, stickNum*2+1);

    if (Atari800_machine_type == Atari800_MACHINE_5200) {
        *x_pot = convert_SDL_to_Pot5200(x);
        *y_pot = convert_SDL_to_Pot5200(y);
        }
    else {
        *x_pot = convert_SDL_to_Pot(x);
        *y_pot = convert_SDL_to_Pot(y);
        }

    if (x > minjoy) {
        if (y < -minjoy)
            return INPUT_STICK_UR;
        else if (y > minjoy)
            return INPUT_STICK_LR;
        else
            return INPUT_STICK_RIGHT;
    } else if (x < -minjoy) {
        if (y < -minjoy)
            return INPUT_STICK_UL;
        else if (y > minjoy)
            return INPUT_STICK_LL;
        else
            return INPUT_STICK_LEFT;
    } else {
        if (y < -minjoy)
            return INPUT_STICK_FORWARD;
        else if (y > minjoy)
            return INPUT_STICK_BACK;
        else
            return INPUT_STICK_CENTRE;
    }
}

/*------------------------------------------------------------------------------
*  get_SDL_hat_state - Return the states of one of the Attached HID 
*     joystick hats.
*-----------------------------------------------------------------------------*/
int get_SDL_hat_state(SDL_Joystick *joystick, int hatNum)
{
    int stick, astick = 0;

    stick = SDL_JoystickGetHat(joystick, hatNum);

    switch (stick) {
        case SDL_HAT_CENTERED:
            astick = INPUT_STICK_CENTRE;
            break;
        case SDL_HAT_UP:
            astick = INPUT_STICK_FORWARD;
            break;
        case SDL_HAT_RIGHT:
            astick = INPUT_STICK_RIGHT;
            break;
        case SDL_HAT_DOWN:
            astick = INPUT_STICK_BACK;
            break;
        case SDL_HAT_LEFT:
            astick = INPUT_STICK_LEFT;
            break;
        case SDL_HAT_RIGHTUP:
            astick = INPUT_STICK_UR;
            break;
        case SDL_HAT_RIGHTDOWN:
            astick = INPUT_STICK_LR;
            break;
        case SDL_HAT_LEFTUP:
            astick = INPUT_STICK_UL;
            break;
        case SDL_HAT_LEFTDOWN:
            astick = INPUT_STICK_LL;
            break;
        }

    return(astick);
}

int SDL_JoystickIndex(SDL_Joystick *joystick)
{
    int i;
    
    for (i=0;i<4;i++) {
        if (joysticks[i] == joystick)
            return i;
    }
    return 0;
}

/*------------------------------------------------------------------------------
*  get_SDL_joy_buttons_state - Return the states of one of the Attached HID 
*     joystick based on four buttons assigned to joystick directions.
*-----------------------------------------------------------------------------*/
int get_SDL_joy_buttons_state(SDL_Joystick *joystick, int numberOfButtons)
{
    int stick = 0;
    int up = 0;
    int down = 0;
    int left = 0;
    int right = 0;
    int i;
    int joyindex;
    
    joyindex = SDL_JoystickIndex(joystick);

    if (Atari800_machine_type != Atari800_MACHINE_5200) {
        for (i = 0; i < numberOfButtons; i++) {
            if (SDL_JoystickGetButton(joystick, i)) {
                switch(joystickButtonKey[joyindex][i]) {
                    case AKEY_JOYSTICK_UP:
                        up = 1;
                        break;
                    case AKEY_JOYSTICK_DOWN:
                        down = 1;
                        break;
                    case AKEY_JOYSTICK_LEFT:
                        left = 1;
                        break;
                    case AKEY_JOYSTICK_RIGHT:
                        right = 1;
                        break;
                        }
                    }
                }
            }
    else {
        for (i = 0; i < numberOfButtons; i++) {
            if (SDL_JoystickGetButton(joystick, i)) {
                switch(joystick5200ButtonKey[joyindex][i]) {
                    case AKEY_JOYSTICK_UP:
                        up = 1;
                        break;
                    case AKEY_JOYSTICK_DOWN:
                        down = 1;
                        break;
                    case AKEY_JOYSTICK_LEFT:
                        left = 1;
                        break;
                    case AKEY_JOYSTICK_RIGHT:
                        right = 1;
                        break;
                        }
                    }
                }
            }
            
    if (right) {
        if (up)
            return INPUT_STICK_UR;
        else if (down)
            return INPUT_STICK_LR;
        else
            return INPUT_STICK_RIGHT;
    } else if (left) {
        if (up)
            return INPUT_STICK_UL;
        else if (down)
            return INPUT_STICK_LL;
        else
            return INPUT_STICK_LEFT;
    } else {
        if (up)
            return INPUT_STICK_FORWARD;
        else if (down)
            return INPUT_STICK_BACK;
        else
            return INPUT_STICK_CENTRE;
    }

    return(stick);
}


/*------------------------------------------------------------------------------
*  SDL_Atari_PORT - Read the status of the four joystick ports.
*-----------------------------------------------------------------------------*/
void SDL_Atari_PORT(Uint8 * s0, Uint8 * s1, Uint8 *s2, Uint8 *s3)
{
    int stick[NUM_JOYSTICKS];
    static int last_stick[NUM_JOYSTICKS] =  {INPUT_STICK_CENTRE, INPUT_STICK_CENTRE, INPUT_STICK_CENTRE, INPUT_STICK_CENTRE};
    int i;
	int joy_x_pot[NUM_JOYSTICKS], joy_y_pot[NUM_JOYSTICKS];
	int joy0_multi_num = 0;
	int joy1_multi_num = 0;
	int joy2_multi_num = 0;
	int joy3_multi_num = 0;

    if ((joystick0 != NULL) || (joystick1 != NULL) || (joystick2 != NULL) || (joystick3 != NULL)) // can only joystick1!=NULL ?
        {
        SDL_JoystickUpdate();
        }

    /* Get the basic joystick info, from either the keyboard or HID joysticks */
    for (i=0;i<NUM_JOYSTICKS;i++) {
        stick[i] = INPUT_STICK_CENTRE;
            
        switch(JOYSTICK_MODE[i]) {
            case NO_JOYSTICK:
                break;
            case KEYPAD_JOYSTICK:
				if (!keyjoyEnable)
					{
					stick[i] = INPUT_STICK_CENTRE;
					break;
					}
                if (kbhits[SDL_JOY_0_LEFT])
                    stick[i] = INPUT_STICK_LEFT;
                if (kbhits[SDL_JOY_0_RIGHT])
                    stick[i] = INPUT_STICK_RIGHT;
                if (kbhits[SDL_JOY_0_UP])
                    stick[i] = INPUT_STICK_FORWARD;
                if (kbhits[SDL_JOY_0_DOWN])
                    stick[i] = INPUT_STICK_BACK;
                if ((kbhits[SDL_JOY_0_LEFTUP])
                    || ((kbhits[SDL_JOY_0_LEFT]) && (kbhits[SDL_JOY_0_UP])))
                    stick[i] = INPUT_STICK_UL;
                if ((kbhits[SDL_JOY_0_LEFTDOWN])
                    || ((kbhits[SDL_JOY_0_LEFT]) && (kbhits[SDL_JOY_0_DOWN])))
                    stick[i] = INPUT_STICK_LL;
                if ((kbhits[SDL_JOY_0_RIGHTUP])
                    || ((kbhits[SDL_JOY_0_RIGHT]) && (kbhits[SDL_JOY_0_UP])))
                    stick[i] = INPUT_STICK_UR;
                if ((kbhits[SDL_JOY_0_RIGHTDOWN])
                    || ((kbhits[SDL_JOY_0_RIGHT]) && (kbhits[SDL_JOY_0_DOWN])))
                    stick[i] = INPUT_STICK_LR;
                break;
            case USERDEF_JOYSTICK:
				if (!keyjoyEnable)
					{
					stick[i] = INPUT_STICK_CENTRE;
					break;
					}
                if (kbhits[SDL_JOY_1_LEFT])
                    stick[i] = INPUT_STICK_LEFT;
                if (kbhits[SDL_JOY_1_RIGHT])
                    stick[i] = INPUT_STICK_RIGHT;
                if (kbhits[SDL_JOY_1_UP])
                    stick[i] = INPUT_STICK_FORWARD;
                if (kbhits[SDL_JOY_1_DOWN])
                    stick[i] = INPUT_STICK_BACK;
                if ((kbhits[SDL_JOY_1_LEFTUP])
                    || ((kbhits[SDL_JOY_1_LEFT]) && (kbhits[SDL_JOY_1_UP])))
                    stick[i] = INPUT_STICK_UL;
                if ((kbhits[SDL_JOY_1_LEFTDOWN])
                    || ((kbhits[SDL_JOY_1_LEFT]) && (kbhits[SDL_JOY_1_DOWN])))
                    stick[i] = INPUT_STICK_LL;
                if ((kbhits[SDL_JOY_1_RIGHTUP])
                    || ((kbhits[SDL_JOY_1_RIGHT]) && (kbhits[SDL_JOY_1_UP])))
                    stick[i] = INPUT_STICK_UR;
                if ((kbhits[SDL_JOY_1_RIGHTDOWN])
                    || ((kbhits[SDL_JOY_1_RIGHT]) && (kbhits[SDL_JOY_1_DOWN])))
                    stick[i] = INPUT_STICK_LR;
                break;
            case SDL0_JOYSTICK:
                if (joystickType0 == JOY_TYPE_STICK) {
					if (joystick0Num == 9999) {
						stick[i] = get_SDL_joystick_state(joystick0, joy0_multi_num, 
														  &joy_x_pot[i], &joy_y_pot[i]);
						joy0_multi_num = (joy0_multi_num + 1) % joystick0_nsticks;
					} else {
						stick[i] = get_SDL_joystick_state(joystick0, joystick0Num, 
														  &joy_x_pot[i], &joy_y_pot[i]);
						}
				}
                else if (joystickType0 == JOY_TYPE_HAT) {
					if (joystick0Num == 9999) {
						stick[i] = get_SDL_hat_state(joystick0, joy0_multi_num);
						joy0_multi_num = (joy0_multi_num + 1) % joystick0_nsticks;
					} else {
                    stick[i] = get_SDL_hat_state(joystick0, joystick0Num);
					}
				}
                else
                    stick[i] = get_SDL_joy_buttons_state(joystick0, joystick0_nbuttons);
                break;
            case SDL1_JOYSTICK:
                if (joystickType1 == JOY_TYPE_STICK) {
					if (joystick1Num == 9999) {
						stick[i] = get_SDL_joystick_state(joystick1, joystick1Num, 
														  &joy_x_pot[i], &joy_y_pot[i]);
						joy1_multi_num = (joy1_multi_num + 1) % joystick1_nhats;
					} else {
						stick[i] = get_SDL_joystick_state(joystick1, joy1_multi_num, 
														  &joy_x_pot[i], &joy_y_pot[i]);
						}
				}
                else if (joystickType1 == JOY_TYPE_HAT) {
					if (joystick1Num == 9999) {
						stick[i] = get_SDL_hat_state(joystick1, joy1_multi_num);
						joy1_multi_num = (joy1_multi_num + 1) % joystick1_nhats;
					} else {
                    stick[i] = get_SDL_hat_state(joystick1, joystick1Num);
					}
				}
                else
                    stick[i] = get_SDL_joy_buttons_state(joystick1, joystick1_nbuttons);
                break;
            case SDL2_JOYSTICK:
				if (joystickType2 == JOY_TYPE_STICK) {
					if (joystick2Num == 9999) {
						stick[i] = get_SDL_joystick_state(joystick2, joy2_multi_num,
														  &joy_x_pot[i], &joy_y_pot[i]);
						joy2_multi_num = (joy2_multi_num + 1) % joystick2_nsticks;
					} else {
						stick[i] = get_SDL_joystick_state(joystick2, joystick2Num,
														  &joy_x_pot[i], &joy_y_pot[i]);
						}
				}
                else if (joystickType2 == JOY_TYPE_HAT) {
					if (joystick2Num == 9999) {
						stick[i] = get_SDL_hat_state(joystick2, joy2_multi_num);
						joy2_multi_num = (joy2_multi_num + 1) % joystick2_nhats;
					} else {
                    stick[i] = get_SDL_hat_state(joystick2, joystick2Num);
					}
				}	
                else
                    stick[i] = get_SDL_joy_buttons_state(joystick2, joystick2_nbuttons);
                break;
            case SDL3_JOYSTICK:
				if (joystickType3 == JOY_TYPE_STICK) {
					if (joystick3Num == 9999) {
						stick[i] = get_SDL_joystick_state(joystick3, joy0_multi_num,
														  &joy_x_pot[i], &joy_y_pot[i]);
						joy3_multi_num = (joy3_multi_num + 1) % joystick3_nhats;
					} else {
						stick[i] = get_SDL_joystick_state(joystick3, joystick3Num,
														  &joy_x_pot[i], &joy_y_pot[i]);
					}
				}
                else if (joystickType3 == JOY_TYPE_HAT) {
					if (joystick3Num == 9999) {
						stick[i] = get_SDL_hat_state(joystick3, joy3_multi_num);
						joy3_multi_num = (joy3_multi_num + 1) % joystick3_nsticks;
					} else {
                    stick[i] = get_SDL_hat_state(joystick3, joystick3Num);
					}
				}
                else
                    stick[i] = get_SDL_joy_buttons_state(joystick3, joystick3_nbuttons);
                break;
            }
        
        /* Make sure that left/right and up/down aren't pressed at the same time
            from keyboard */
        if (INPUT_joy_block_opposite_directions) {
            if ((stick[i] & 0x0c) == 0) {   /* right and left simultaneously */
                if (last_stick[i] & 0x04)   /* if wasn't left before, move left */
                    stick[i] |= 0x08;
                else            /* else move right */
                stick[i] |= 0x04;
                }
            else {
                last_stick[i] &= 0x03;
                last_stick[i] |= stick[i] & 0x0c;
                }
            if ((stick[i] & 0x03) == 0) {   /* up and down simultaneously */
                if (last_stick[i] & 0x01)   /* if wasn't up before, move up */
                    stick[i] |= 0x02;
                else                        /* else move down */
                    stick[i] |= 0x01;
                }
            else {
                last_stick[i] &= 0x0c;
                last_stick[i] |= stick[i] & 0x03;
                }
            }
        else
            last_stick[i] = stick[i];
        }

    /* handle  SDL joysticks as paddles in non-5200 */
    if (Atari800_machine_type != Atari800_MACHINE_5200) {
        for (i=0;i<NUM_JOYSTICKS;i++) {
            if ((JOYSTICK_MODE[i] == SDL0_JOYSTICK) && 
                (joystickType0 == JOY_TYPE_STICK)){
                POKEY_POT_input[2*i] = joy_x_pot[i];
                POKEY_POT_input[2*i+1] = joy_y_pot[i];
                if (paddlesXAxisOnly)
                    POKEY_POT_input[2*i+1] = joy_x_pot[i];
                }
            else if ((JOYSTICK_MODE[i] == SDL1_JOYSTICK) && 
                     (joystickType1 == JOY_TYPE_STICK)) {
                POKEY_POT_input[2*i] = joy_x_pot[i];
                POKEY_POT_input[2*i+1] = joy_y_pot[i];
                if (paddlesXAxisOnly)
                    POKEY_POT_input[2*i+1] = joy_x_pot[i];
                }
            else if ((JOYSTICK_MODE[i] == SDL2_JOYSTICK) && 
                     (joystickType2 == JOY_TYPE_STICK)) {
                POKEY_POT_input[2*i] = joy_x_pot[i];
                POKEY_POT_input[2*i+1] = joy_y_pot[i];
                if (paddlesXAxisOnly)
                    POKEY_POT_input[2*i+1] = joy_x_pot[i];
                }
            else if ((JOYSTICK_MODE[i] == SDL3_JOYSTICK) && 
                     (joystickType3 == JOY_TYPE_STICK)) {
                POKEY_POT_input[2*i] = joy_x_pot[i];
                POKEY_POT_input[2*i+1] = joy_y_pot[i];
                if (paddlesXAxisOnly)
                    POKEY_POT_input[2*i+1] = joy_x_pot[i];
                }
            else {
                POKEY_POT_input[2*i] = 228;
                POKEY_POT_input[2*i+1] = 228;
                }
            }
        }
        
    /* handle analog joysticks in Atari 5200 */
    else {
        for (i = 0; i < 4; i++) {
            if ((JOYSTICK_MODE[i] == SDL0_JOYSTICK) && 
                (joystickType0 == JOY_TYPE_STICK)) {
                POKEY_POT_input[2*i] = joy_x_pot[i];
                POKEY_POT_input[2*i+1] = joy_y_pot[i];
                }
            else if ((JOYSTICK_MODE[i] == SDL1_JOYSTICK) && 
                     (joystickType0 == JOY_TYPE_STICK)) {
                POKEY_POT_input[2*i] = joy_x_pot[i];
                POKEY_POT_input[2*i+1] = joy_y_pot[i];
                }
            else if ((JOYSTICK_MODE[i] == SDL2_JOYSTICK) && 
                     (joystickType0 == JOY_TYPE_STICK)) {
                POKEY_POT_input[2*i] = joy_x_pot[i];
                POKEY_POT_input[2*i+1] = joy_y_pot[i];
                }
            else if ((JOYSTICK_MODE[i] == SDL3_JOYSTICK) && 
                     (joystickType0 == JOY_TYPE_STICK)) {
                POKEY_POT_input[2*i] = joy_x_pot[i];
                POKEY_POT_input[2*i+1] = joy_y_pot[i];
                }
            else {
                if ((stick[i] & (INPUT_STICK_CENTRE ^ INPUT_STICK_LEFT)) == 0) 
                    POKEY_POT_input[2 * i] = INPUT_joy_5200_min;
                else if ((stick[i] & (INPUT_STICK_CENTRE ^ INPUT_STICK_RIGHT)) == 0) 
                    POKEY_POT_input[2 * i] = INPUT_joy_5200_max;
                else 
                    POKEY_POT_input[2 * i] = INPUT_joy_5200_center;
                if ((stick[i] & (INPUT_STICK_CENTRE ^ INPUT_STICK_FORWARD)) == 0) 
                    POKEY_POT_input[2 * i + 1] = INPUT_joy_5200_min;
                else if ((stick[i] & (INPUT_STICK_CENTRE ^ INPUT_STICK_BACK)) == 0) 
                    POKEY_POT_input[2 * i + 1] = INPUT_joy_5200_max;
                else 
                    POKEY_POT_input[2 * i + 1] = INPUT_joy_5200_center;
                }
            }
        }

    *s0 = stick[0];
    *s1 = stick[1];
    *s2 = stick[2];
    *s3 = stick[3];
}

/*------------------------------------------------------------------------------
*  get_SDL_button_state - Return the states of one of the Attached HID 
*     joysticks buttons.
*-----------------------------------------------------------------------------*/
int get_SDL_button_state(SDL_Joystick *joystick, int numberOfButtons, int *INPUT_key_code, 
                         int *INPUT_key_shift, int *key_control, int *key_consol, int multistick_num)
{
    int i;
    int trig = 1;
    int joyindex;
	int button_multistick_num = 0;
    
    joyindex = SDL_JoystickIndex(joystick);
    
    *INPUT_key_code = AKEY_NONE;
    
    if (Atari800_machine_type != Atari800_MACHINE_5200) {
        for (i = 0; i < numberOfButtons; i++) {
            if (SDL_JoystickGetButton(joystick, i)) {
                switch(joystickButtonKey[joyindex][i]) {
                    case AKEY_BUTTON1:
						if (button_multistick_num == multistick_num) {
                        trig = 0;
						}
						button_multistick_num++;
                        break;
                    case AKEY_SHFT:
                        *INPUT_key_shift = 1;
                        break;
                    case AKEY_CTRL:
                        *key_control = AKEY_CTRL;
                        break;
                    case AKEY_BUTTON_OPTION:
                        *key_consol &= (~INPUT_CONSOL_OPTION);
                        break;
                    case AKEY_BUTTON_SELECT:
                        *key_consol &= (~INPUT_CONSOL_SELECT);
                        break;
                    case AKEY_BUTTON_START:
                        *key_consol &= (~INPUT_CONSOL_START);
                        break;
                    case AKEY_JOYSTICK_UP:
                    case AKEY_JOYSTICK_DOWN:
                    case AKEY_JOYSTICK_LEFT:
                    case AKEY_JOYSTICK_RIGHT:
                        break;
                    default:
                        *INPUT_key_code = joystickButtonKey[joyindex][i];
                    }
                }
            }
        }
    else {
        for (i = 0; i < numberOfButtons; i++) {
            if (SDL_JoystickGetButton(joystick, i)) {
                switch(joystick5200ButtonKey[joyindex][i]) {
                    case AKEY_BUTTON1:
						if (button_multistick_num == multistick_num) {
                        trig = 0;
						}
						button_multistick_num++;
                        break;
                    case AKEY_JOYSTICK_UP:
                    case AKEY_JOYSTICK_DOWN:
                    case AKEY_JOYSTICK_LEFT:
                    case AKEY_JOYSTICK_RIGHT:
                        break;
                    default:
                        *INPUT_key_code = joystick5200ButtonKey[joyindex][i];
                    }
                }
            }
        }

    return(trig);
}

/*------------------------------------------------------------------------------
*  SDL_Atari_TRIG - Read the status of the four joystick buttons.
*-----------------------------------------------------------------------------*/
int SDL_Atari_TRIG(Uint8 * t0, Uint8 * t1, Uint8 *t2, Uint8 *t3, 
                   int *INPUT_key_shift, int *key_control, int *key_consol)
{
    int trig[NUM_JOYSTICKS],i;
    int key_code0 = AKEY_NONE;
    int key_code1 = AKEY_NONE;
    int key_code2 = AKEY_NONE;
    int key_code3 = AKEY_NONE;
	int joy0_multi_num = 0;
	int joy1_multi_num = 0;
	int joy2_multi_num = 0;
	int joy3_multi_num = 0;
    
    *INPUT_key_shift = 0;
    *key_control = 0;
    *key_consol = INPUT_CONSOL_NONE;
        
    for (i=0;i<NUM_JOYSTICKS;i++) {
        switch(JOYSTICK_MODE[i]) {
            case NO_JOYSTICK:
                trig[i] = 1;
                break;
            case KEYPAD_JOYSTICK:
				if (!keyjoyEnable)
					{
					trig[i] = 1;
					break;
					}
                if ((kbhits[SDL_TRIG_0]) || (kbhits[SDL_TRIG_0_B]) ||
				    (kbhits[SDL_TRIG_0_R]) || (kbhits[SDL_TRIG_0_B_R]))
                    trig[i] = 0;
                else
                    trig[i] = 1;
                break;
            case USERDEF_JOYSTICK:
				if (!keyjoyEnable)
					{
					trig[i] = 1;
					break;
					}
                if ((kbhits[SDL_TRIG_1]) || (kbhits[SDL_TRIG_1_B]) ||
					(kbhits[SDL_TRIG_1_R]) || (kbhits[SDL_TRIG_1_B_R]))
                    trig[i] = 0;
                else
                    trig[i] = 1;
                break;
            case SDL0_JOYSTICK: 
				if (joystick0Num == 9999) {
                trig[i] = get_SDL_button_state(joystick0, joystick0_nbuttons, 
                                               &key_code0, INPUT_key_shift, key_control,
												   key_consol,joy0_multi_num);
					joy0_multi_num = (joy0_multi_num + 1) % (MAX(joystick0_nsticks,joystick0_nhats));
				} else {
					trig[i] = get_SDL_button_state(joystick0, joystick0_nbuttons, 
												   &key_code0, INPUT_key_shift, key_control,
												   key_consol,0);
				}
                break;
            case SDL1_JOYSTICK:
				if (joystick0Num == 9999) {
					trig[i] = get_SDL_button_state(joystick1, joystick1_nbuttons, 
                                               &key_code1, INPUT_key_shift, key_control,
												   key_consol,joy1_multi_num);
					joy1_multi_num = (joy1_multi_num + 1) % (MAX(joystick1_nsticks,joystick1_nhats));
				} else {
					trig[i] = get_SDL_button_state(joystick1, joystick1_nbuttons, 
												   &key_code1, INPUT_key_shift, key_control,
												   key_consol,0);
				}
                break;
            case SDL2_JOYSTICK:
				if (joystick0Num == 9999) {
					trig[i] = get_SDL_button_state(joystick2, joystick2_nbuttons, 
                                               &key_code2, INPUT_key_shift, key_control,
												   key_consol,joy2_multi_num);
					joy2_multi_num = (joy2_multi_num + 1) % (MAX(joystick2_nsticks,joystick3_nhats));
				} else {
					trig[i] = get_SDL_button_state(joystick2, joystick2_nbuttons, 
												   &key_code2, INPUT_key_shift, key_control,
												   key_consol,0);
				}
                break;
            case SDL3_JOYSTICK:
				if (joystick3Num == 9999) {
					trig[i] = get_SDL_button_state(joystick3, joystick3_nbuttons, 
                                               &key_code3, INPUT_key_shift, key_control,
												   key_consol,joy3_multi_num);
					/* TBD MDG need to fix this to be greater of nsticks and hats */ 
					joy3_multi_num = (joy3_multi_num + 1) % (MAX(joystick3_nsticks,joystick3_nhats));
				} else {
					trig[i] = get_SDL_button_state(joystick3, joystick3_nbuttons, 
												   &key_code3, INPUT_key_shift, key_control,
												   key_consol,0);
				}
                break;
            }
        if ((JOYSTICK_AUTOFIRE[i] == INPUT_AUTOFIRE_FIRE && !trig[i]) || 
            (JOYSTICK_AUTOFIRE[i] == INPUT_AUTOFIRE_CONT))
            trig[i] = (Atari800_nframes & 2) ? 1 : 0;
        }
           
    *t0 = trig[0];
    *t1 = trig[1];
    *t2 = trig[2];
    *t3 = trig[3];

    if (key_code0 == AKEY_NONE) {
        if (key_code1 == AKEY_NONE) {
            if (key_code2 == AKEY_NONE) 
                return(key_code3);
            else
                return(key_code2);
            }
        else
            return(key_code1);
        }
    else
        return(key_code0);
}

/* mouse_step is used in Amiga, ST, trak-ball and joystick modes.
   It moves mouse_x and mouse_y in the direction given by
   mouse_move_x and mouse_move_y.
   Bresenham's algorithm is used:
   if (abs(deltaX) >= abs(deltaY)) {
     if (deltaX == 0)
       return;
     MoveHorizontally();
     e -= abs(gtaY);
     if (e < 0) {
       e += abs(deltaX);
       MoveVertically();
     }
   }
   else {
     MoveVertically();
     e -= abs(deltaX);
     if (e < 0) {
       e += abs(deltaY);
       MoveHorizontally();
     }
   }
   Returned is stick code for the movement direction.
*/
static UBYTE mouse_step(void)
{
	static int e = 0;
	UBYTE r = INPUT_STICK_CENTRE;
	int dx = mouse_move_x >= 0 ? mouse_move_x : -mouse_move_x;
	int dy = mouse_move_y >= 0 ? mouse_move_y : -mouse_move_y;
	if (dx >= dy) {
		if (mouse_move_x == 0)
			return r;
		if (mouse_move_x < 0) {
			r &= INPUT_STICK_LEFT;
			mouse_last_right = 0;
			mouse_x--;
			mouse_move_x += 1 << MOUSE_SHIFT;
			if (mouse_move_x > 0)
				mouse_move_x = 0;
		}
		else {
			r &= INPUT_STICK_RIGHT;
			mouse_last_right = 1;
			mouse_x++;
			mouse_move_x -= 1 << MOUSE_SHIFT;
			if (mouse_move_x < 0)
				mouse_move_x = 0;
		}
		e -= dy;
		if (e < 0) {
			e += dx;
			if (mouse_move_y < 0) {
				r &= INPUT_STICK_FORWARD;
				mouse_last_down = INPUT_Invert_Axis;
				mouse_y--;
				mouse_move_y += 1 << MOUSE_SHIFT;
				if (mouse_move_y > 0)
					mouse_move_y = 0;
			}
			else {
				r &= INPUT_STICK_BACK;
				mouse_last_down = 1 - INPUT_Invert_Axis;
				mouse_y++;
				mouse_move_y -= 1 << MOUSE_SHIFT;
				if (mouse_move_y < 0)
					mouse_move_y = 0;
			}
		}
	}
	else {
		if (mouse_move_y < 0) {
			r &= INPUT_STICK_FORWARD;
			mouse_last_down = 1 - INPUT_Invert_Axis;
			mouse_y--;
			mouse_move_y += 1 << MOUSE_SHIFT;
			if (mouse_move_y > 0)
				mouse_move_y = 0;
		}
		else {
			r &= INPUT_STICK_BACK;
			mouse_last_down = INPUT_Invert_Axis;
			mouse_y++;
			mouse_move_y -= 1 << MOUSE_SHIFT;
			if (mouse_move_y < 0)
				mouse_move_y = 0;
		}
		e -= dx;
		if (e < 0) {
			e += dy;
			if (mouse_move_x < 0) {
				r &= INPUT_STICK_LEFT;
				mouse_last_right = 0;
				mouse_x--;
				mouse_move_x += 1 << MOUSE_SHIFT;
				if (mouse_move_x > 0)
					mouse_move_x = 0;
			}
			else {
				r &= INPUT_STICK_RIGHT;
				mouse_last_right = 1;
				mouse_x++;
				mouse_move_x -= 1 << MOUSE_SHIFT;
				if (mouse_move_x < 0)
					mouse_move_x = 0;
			}
		}
	}
	return r;
}

/*------------------------------------------------------------------------------
*  SDL_Atari_Mouse - Read the status the device emulated by the mouse, if there
*     is one.  Emulated devices include paddles, light pens, etc...  Most of this
*     was brought over from input.c.  I hated to duplicate it, but the author
*     of the SDL port indicated there were problems with Atari_Frame and 
*     INPUT_Frame.
*-----------------------------------------------------------------------------*/
void SDL_Atari_Mouse(Uint8 *s, Uint8 *t)
{
    int STICK = INPUT_STICK_CENTRE;
    int TRIG = 1;
    int i;
    static int last_mouse_buttons = 0;
    int sdl_mouse_buttons;

    INPUT_mouse_buttons = 0;
    sdl_mouse_buttons = SDL_GetRelativeMouseState(&INPUT_mouse_delta_x, &INPUT_mouse_delta_y);

    if (sdl_mouse_buttons & SDL_BUTTON(SDL_BUTTON_RIGHT))
        INPUT_mouse_buttons |= 2;
    if (sdl_mouse_buttons & SDL_BUTTON(SDL_BUTTON_LEFT))
        INPUT_mouse_buttons |= 1;

    /* handle mouse */
    switch (INPUT_mouse_mode) {
    case INPUT_MOUSE_OFF:
        break;
    case INPUT_MOUSE_PAD:
    case INPUT_MOUSE_TOUCH:
    case INPUT_MOUSE_KOALA:
        if (INPUT_mouse_mode != INPUT_MOUSE_PAD || Atari800_machine_type == Atari800_MACHINE_5200)
            mouse_x += INPUT_mouse_delta_x * INPUT_mouse_speed;
        else
            mouse_x -= INPUT_mouse_delta_x * INPUT_mouse_speed;
        if (mouse_x < INPUT_mouse_pot_min << MOUSE_SHIFT)
            mouse_x = INPUT_mouse_pot_min << MOUSE_SHIFT;
        else if (mouse_x > INPUT_mouse_pot_max << MOUSE_SHIFT)
            mouse_x = INPUT_mouse_pot_max << MOUSE_SHIFT;
        if (INPUT_mouse_mode == INPUT_MOUSE_KOALA || Atari800_machine_type == Atari800_MACHINE_5200)
            mouse_y += INPUT_mouse_delta_y * INPUT_mouse_speed;
        else
            mouse_y -= INPUT_mouse_delta_y * INPUT_mouse_speed;
        if (mouse_y < INPUT_mouse_pot_min << MOUSE_SHIFT)
            mouse_y = INPUT_mouse_pot_min << MOUSE_SHIFT;
        else if (mouse_y > INPUT_mouse_pot_max << MOUSE_SHIFT)
            mouse_y = INPUT_mouse_pot_max << MOUSE_SHIFT;
        POKEY_POT_input[INPUT_mouse_port << 1] = mouse_x >> MOUSE_SHIFT;
        POKEY_POT_input[(INPUT_mouse_port << 1) + 1] = mouse_y >> MOUSE_SHIFT;
        if (paddlesXAxisOnly && (INPUT_mouse_mode == INPUT_MOUSE_PAD))
            POKEY_POT_input[(INPUT_mouse_port << 1) + 1] = mouse_x >> MOUSE_SHIFT;

        if (Atari800_machine_type == Atari800_MACHINE_5200) {
            if (INPUT_mouse_buttons & 1)
                TRIG = 0;
            if (INPUT_mouse_buttons & 2)
                POKEY_SKSTAT &= ~8;
			if (INPUT_mouse_buttons & 4)
				POKEY_POT_input[(INPUT_mouse_port << 1) + 1] = 1;
		    }
        else
            STICK &= ~(INPUT_mouse_buttons << 2);
        break;
    case INPUT_MOUSE_PEN:
    case INPUT_MOUSE_GUN:
        mouse_x += INPUT_mouse_delta_x * INPUT_mouse_speed;
        if (mouse_x < 0)
            mouse_x = 0;
        else if (mouse_x > 167 << MOUSE_SHIFT)
            mouse_x = 167 << MOUSE_SHIFT;
        mouse_y += INPUT_mouse_delta_y * INPUT_mouse_speed;
        if (mouse_y < 0)
            mouse_y = 0;
        else if (mouse_y > 119 << MOUSE_SHIFT)
            mouse_y = 119 << MOUSE_SHIFT;
        ANTIC_PENH_input = 44 + INPUT_mouse_pen_ofs_h + (mouse_x >> MOUSE_SHIFT);
        ANTIC_PENV_input = 4 + INPUT_mouse_pen_ofs_v + (mouse_y >> MOUSE_SHIFT);
        if ((INPUT_mouse_buttons & 1) == (INPUT_mouse_mode == INPUT_MOUSE_PEN))
            STICK &= ~1;
        if ((INPUT_mouse_buttons & 2) && !(last_mouse_buttons & 2))
            mouse_pen_show_pointer = !mouse_pen_show_pointer;
        break;
    case INPUT_MOUSE_AMIGA:
    case INPUT_MOUSE_ST:
    case INPUT_MOUSE_TRAK:
        mouse_move_x += (INPUT_mouse_delta_x * INPUT_mouse_speed) >> 1;
        mouse_move_y += (INPUT_mouse_delta_y * INPUT_mouse_speed) >> 1;

        /* i = max(abs(mouse_move_x), abs(mouse_move_y)); */
        i = mouse_move_x >= 0 ? mouse_move_x : -mouse_move_x;
        if (mouse_move_y > i)
            i = mouse_move_y;
        else if (-mouse_move_y > i)
            i = -mouse_move_y;

        {
            if (i > 0) {
                i += (1 << MOUSE_SHIFT) - 1;
                i >>= MOUSE_SHIFT;
                if (i > 50)
                    max_scanline_counter = scanline_counter = 5;
                else
                    max_scanline_counter = scanline_counter = Atari800_tv_mode / i;
                mouse_step();
            }
            if (INPUT_mouse_mode == INPUT_MOUSE_TRAK) {
                /* bit 3 toggles - vertical movement, bit 2 = 0 - up */
                /* bit 1 toggles - horizontal movement, bit 0 = 0 - left */
				STICK = ((mouse_y & 1) << 3) | (mouse_last_down << 2)
						| ((mouse_x & 1) << 1) | mouse_last_right;
            }
            else {
                STICK = (INPUT_mouse_mode == INPUT_MOUSE_AMIGA ? mouse_amiga_codes : mouse_st_codes)
                            [(mouse_y & 3) * 4 + (mouse_x & 3)];
            }
        }

        if (INPUT_mouse_buttons & 1)
            TRIG = 0;
        if (INPUT_mouse_buttons & 2)
            POKEY_POT_input[INPUT_mouse_port << 1] = 1;
        break;
    case INPUT_MOUSE_JOY:
        mouse_move_x += INPUT_mouse_delta_x * INPUT_mouse_speed;
        mouse_move_y += INPUT_mouse_delta_y * INPUT_mouse_speed;
        if (mouse_move_x < -INPUT_mouse_joy_inertia << MOUSE_SHIFT ||
            mouse_move_x > INPUT_mouse_joy_inertia << MOUSE_SHIFT ||
            mouse_move_y < -INPUT_mouse_joy_inertia << MOUSE_SHIFT ||
            mouse_move_y > INPUT_mouse_joy_inertia << MOUSE_SHIFT) {
               mouse_move_x >>= 1;
               mouse_move_y >>= 1;
            }
        STICK &= mouse_step();
        if (Atari800_machine_type == Atari800_MACHINE_5200) {
            if ((STICK & (INPUT_STICK_CENTRE ^ INPUT_STICK_LEFT)) == 0)
                POKEY_POT_input[(INPUT_mouse_port << 1)] = INPUT_joy_5200_min;
            else if ((STICK & (INPUT_STICK_CENTRE ^ INPUT_STICK_RIGHT)) == 0)
                POKEY_POT_input[(INPUT_mouse_port << 1)] = INPUT_joy_5200_max;
            else
                POKEY_POT_input[(INPUT_mouse_port << 1)] = INPUT_joy_5200_center;
            if ((STICK & (INPUT_STICK_CENTRE ^ INPUT_STICK_FORWARD)) == 0)
                POKEY_POT_input[(INPUT_mouse_port << 1) + 1] = INPUT_joy_5200_min;
            else if ((STICK & (INPUT_STICK_CENTRE ^ INPUT_STICK_BACK)) == 0)
                POKEY_POT_input[(INPUT_mouse_port << 1) + 1] = INPUT_joy_5200_max;
            else
                POKEY_POT_input[(INPUT_mouse_port << 1) + 1] = INPUT_joy_5200_center;
            }
        if (INPUT_mouse_buttons & 1)
            TRIG = 0;
        break;
    default:
        break;
    }
    last_mouse_buttons = INPUT_mouse_buttons;
        
    *s = STICK;
    *t = TRIG;
        
}

/* INPUT functions now provided by real input.c - removed duplicates */

/* RTIME_Initialise now provided by real rtime.c - removed duplicate */

/* Mac-specific XEP80 copy function stub */
int XEP80GetCopyData(int startx, int endx, int starty, int endy, unsigned char *data)
{
	/* Stub implementation - return false indicating no data copied */
	return FALSE;
}

/* Mac-specific screen drawing function stubs */
void Screen_DrawHDDiskLED(void)
{
	/* Stub implementation - no hard disk LED drawing */
}

void Screen_DrawCapslock(int state)
{
	/* Stub implementation - no caps lock display */
}

/* Mac-specific SIO function stub */
int SIO_IsVapi(int diskno)
{
	/* Stub implementation - return false indicating no VAPI support */
	return FALSE;
}

/* INPUT_joy_multijoy now provided by real input.c */
static int joy_multijoy_no = 0;	/* number of selected joy */

/* INPUT_SelectMultiJoy now provided by real input.c - removed duplicate */

/* INPUT_Scanline now provided by real input.c - removed duplicate */

/*------------------------------------------------------------------------------
*  SDL_CenterMousePointer - Center the mouse pointer on the emulated 
*     screen.
*-----------------------------------------------------------------------------*/
void SDL_CenterMousePointer(void)
{
    switch (INPUT_mouse_mode) {
    case INPUT_MOUSE_PAD:
    case INPUT_MOUSE_TOUCH:
    case INPUT_MOUSE_KOALA:
        mouse_x = 114 << MOUSE_SHIFT;
        mouse_y = 114 << MOUSE_SHIFT;
        break;
    case INPUT_MOUSE_PEN:
    case INPUT_MOUSE_GUN:
        mouse_x = 84 << MOUSE_SHIFT;
        mouse_y = 60 << MOUSE_SHIFT;
        break;
    case INPUT_MOUSE_AMIGA:
    case INPUT_MOUSE_ST:
    case INPUT_MOUSE_TRAK:
    case INPUT_MOUSE_JOY:
        mouse_move_x = 0;
        mouse_move_y = 0;
        break;
    }
}

#define PLOT(dx, dy)    do {\
                            ptr[(dx) + Screen_WIDTH * (dy)] ^= 0x0f0f;\
                            ptr[(dx) + Screen_WIDTH * (dy) + Screen_WIDTH / 2] ^= 0x0f0f;\
                        } while (0)

/*------------------------------------------------------------------------------
*  SDL_DrawMousePointer - Draws the mouse pointer in light pen/gun modes.
*-----------------------------------------------------------------------------*/
void SDL_DrawMousePointer(void)
{
    if ((INPUT_mouse_mode == INPUT_MOUSE_PEN || INPUT_mouse_mode == INPUT_MOUSE_GUN) && mouse_pen_show_pointer) {
        int x = mouse_x >> MOUSE_SHIFT;
        int y = mouse_y >> MOUSE_SHIFT;
        if (x >= 0 && x <= 167 && y >= 0 && y <= 119) {
            UWORD *ptr = & ((UWORD *) Screen_atari)[12 + x + Screen_WIDTH * y];
                        if (x >= 1) {
                            PLOT(-1, 0);
                            if (x>= 2)
                                PLOT(-2, 0);
                        }
                        if (x <= 166) {
                            PLOT(1, 0);
                            if (x<=165)
                                PLOT(2, 0);
                            }
            if (y >= 1) {
                PLOT(0, -1);
                if (y >= 2)
                    PLOT(0, -2);
            }
            if (y <= 118) {
                PLOT(0, 1);
                if (y <= 117)
                    PLOT(0, 2);
            }
        }
    }
}

#define SPEEDLED_FONT_WIDTH		5
#define SPEEDLED_FONT_HEIGHT		7
#define SPEEDLED_COLOR		0xAC

/*------------------------------------------------------------------------------
*  CountFPS - Displays the Frames per second in windowed or fullscreen mode.
*-----------------------------------------------------------------------------*/
void CountFPS()
{
    static int ticks1 = 0, ticks2, shortframes, fps;
	char title[80];
    char count[20];
        
    if (Screen_show_atari_speed) {    
        if (ticks1 == 0)
            ticks1 = SDL_GetTicks();
        ticks2 = SDL_GetTicks();
        shortframes++;
        if (ticks2 - ticks1 > 1000) {
            ticks1 = ticks2;
			fps = shortframes;
            strcpy(title,windowCaption);
            sprintf(count," - %3d fps",shortframes);
            strcat(title,count);
            currentFps = shortframes;
            SDL_SetWindowTitle(MainWindow, title);
            shortframes = 0;
			}
        }
}

void HandleScreenChange(int requested_w, int requested_h)
{
    //int wasFullscreen = FULLSCREEN_MACOS;
    FULLSCREEN_MACOS = Atari800WindowIsFullscreen();
    if (FULLSCREEN_MACOS) {
        Log_print("Setting FullSreeen: %dx%d ",requested_w, requested_h);
        /* Destroying and recreating the renderer here is neccesary to
           prevent unexplained artifacts on the side of the screen.
           The screen clearing at the end of this function doesn't fix
           it, and I don't know why.  I think it's a Metal or libSDL
           issue */
        SDL_DestroyRenderer(renderer);
        renderer = SDL_CreateRenderer(MainWindow, -1, 0);
        if (!fixAspectFullscreen) {
            scaleFactorRenderX = (double) requested_w /
                                 (double) GetDisplayScreenWidth();
            scaleFactorRenderY = (double) requested_h /
                                 (double) GetDisplayScreenHeight();
            SDL_RenderSetScale(renderer, scaleFactorRenderX,
                               scaleFactorRenderY);
            screen_x_offset = 0;
            screen_y_offset = 0;
            }
        else if (onlyIntegralScaling) {
            double thisXScaleFactor;
            double thisYScaleFactor;
            
            thisYScaleFactor = trunc((double) requested_h /
                                     (double) GetDisplayScreenHeight());
            if (thisYScaleFactor < 1.0)
                thisYScaleFactor = 1.0;
            thisXScaleFactor = ((double) GetAtariScreenWidth() /
                    (double) GetDisplayScreenWidth()) * thisYScaleFactor;
            scaleFactorRenderX = thisXScaleFactor;
            scaleFactorRenderY = thisYScaleFactor;
            SDL_RenderSetScale(renderer, thisXScaleFactor, thisYScaleFactor);
            screen_x_offset = ((double)((requested_w -
                                       ((int)(GetDisplayScreenWidth()* thisXScaleFactor)))/2))/thisXScaleFactor;;
            screen_y_offset = ((double)((requested_h -
                                         ((int)(GetDisplayScreenHeight()* thisYScaleFactor)))/2))/thisYScaleFactor;
        }
        else {
            double thisXScaleFactor;
            double thisYScaleFactor;
            
            thisYScaleFactor = (double) requested_h /
                               (double) GetDisplayScreenHeight();
            thisXScaleFactor = ((double) GetAtariScreenWidth() /
                    (double) GetDisplayScreenWidth()) * thisYScaleFactor;
            scaleFactorRenderX = thisXScaleFactor;
            scaleFactorRenderY = thisYScaleFactor;
            SDL_RenderSetScale(renderer, thisXScaleFactor, thisYScaleFactor);
            screen_x_offset = ((double)((requested_w -
                                       ((int)(GetDisplayScreenWidth()* thisXScaleFactor)))/2))/thisXScaleFactor;;
            screen_y_offset = 0;
        }
        current_w = requested_w;
        current_h = requested_h;
    }
    else {
        int new_w, new_h;
        scaleFactorFloat = ((double) requested_h /
                            (double) Screen_HEIGHT);
        Log_print("Reqeusted Sreeen: %dx%d %f ",requested_w, requested_h, scaleFactorFloat);
        if (onlyIntegralScaling) {
            scaleFactorFloat = trunc(scaleFactorFloat+0.4);
            if (scaleFactorFloat < 1.0)
                scaleFactorFloat = 1.0;
        }
        new_w = GetAtariScreenWidth() * scaleFactorFloat;
        new_h = Screen_HEIGHT * scaleFactorFloat;
        Log_print("Setting Screen: %dx%d %f",new_w,new_h,scaleFactorFloat);
        SetRenderScale();
        SDL_SetWindowSize(MainWindow, new_w, new_h);
        current_w = new_w;
        current_h = new_h;
        screen_x_offset = 0;
        screen_y_offset = 0;
        }
    // Make sure the full display is shown and clear
    memset(Screen_atari1, 0, (Screen_HEIGHT * Screen_WIDTH));
    memset(Screen_atari2, 0, (Screen_HEIGHT * Screen_WIDTH));
    full_display = FULL_DISPLAY_COUNT;
    Atari_DisplayScreen((UBYTE *) Screen_atari);
    SDL_FillRect(MainScreen, NULL, SDL_MapRGB(MainScreen->format, 0, 0, 0));
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

/*------------------------------------------------------------------------------
*  ProcessMacMenus - Handle requested changes do to menu operations in the
*      Objective-C Cocoa code.
*-----------------------------------------------------------------------------*/
void ProcessMacMenus()
{
    if (requestFullScreenChange) {
         copyStatus = COPY_IDLE;
         Atari800WindowFullscreen();
         requestFullScreenChange = 0;
         }
    if (request80ColChange) {
		 if (XEP80_enabled || AF80_enabled || BIT3_enabled)
			PLATFORM_Switch80Col();
         request80ColChange = 0;
		 SetDisplayManager80ColMode(XEP80_enabled, XEP80_port, AF80_enabled, BIT3_enabled, PLATFORM_80col);
 		 MediaManager80ColMode(XEP80_enabled, AF80_enabled, BIT3_enabled, PLATFORM_80col);
         }
    if (requestWidthModeChange) {
        SwitchWidth(requestWidthModeChange-1);
        requestWidthModeChange = 0;
		UpdateMediaManagerInfo();
        }                
    if (requestScreenshot) {
        Save_TIFF_file(Find_TIFF_name());
        requestScreenshot = 0;
        }                
    if (requestGrabMouse) {
        SwitchGrabMouse();
        requestGrabMouse = 0;
        }
    if (requestFpsChange) {
        SwitchShowFps();
        requestFpsChange = 0;
        }
    if (requestScaleModeChange) {
        SwitchScaleMode(requestScaleModeChange-1);
        requestScaleModeChange = 0;
		UpdateMediaManagerInfo();
        }
    if (requestSoundEnabledChange) {
         sound_enabled = 1 - sound_enabled;
         requestSoundEnabledChange = 0;
         }
    if (requestSoundStereoChange) {
         POKEYSND_stereo_enabled = 1 - POKEYSND_stereo_enabled;
         requestSoundStereoChange = 0;
         }
    if (requestSoundRecordingChange) {
         SDL_Sound_Recording();
         requestSoundRecordingChange = 0;
         }
    if (requestArtifChange) {
         ANTIC_UpdateArtifacting();
		 UpdateMediaManagerInfo();
		 SetDisplayManagerArtifactMode(ANTIC_artif_mode);
	     requestArtifChange = 0;
	     }
    if (requestLimitChange) {
         if (speed_limit == 1) { 
            deltatime = DELTAFAST;
#ifdef SYNCHRONIZED_SOUND			 
			init_mzpokeysnd_sync();
#endif			 
			screenSwitchEnabled = FALSE;
			}
         else {
            deltatime = real_deltatime;
#ifdef SYNCHRONIZED_SOUND			 
			 init_mzpokeysnd_sync();
#endif			 
			 screenSwitchEnabled = TRUE;
			}
         speed_limit = 1 - speed_limit;
         requestLimitChange = 0;
         SetControlManagerLimit(speed_limit);
         }
    if (requestDisableBasicChange) {
         Atari800_disable_basic = 1 - Atari800_disable_basic;
         requestDisableBasicChange = 0;
         SetControlManagerDisableBasic(Atari800_machine_type,  Atari800_disable_basic);
		 Atari800_Coldstart();
         }
    if (requestKeyjoyEnableChange) {
         keyjoyEnable = !keyjoyEnable;
         requestKeyjoyEnableChange = 0;
         SetControlManagerKeyjoyEnable(keyjoyEnable);
         }
    if (requestCX85EnableChange) {
        INPUT_cx85 = !INPUT_cx85;
        requestCX85EnableChange = 0;
        SetControlManagerCX85Enable(INPUT_cx85);
        }
    if (requestXEGSKeyboardChange) {
        Atari800_keyboard_detached = !Atari800_keyboard_detached;
        requestXEGSKeyboardChange = 0;
        SetControlManagerXEGSKeyboard(!Atari800_keyboard_detached);
        Atari800_UpdateKeyboardDetached();
        }
	if (requestMachineTypeChange) {
		int compositeType, type, ver4type, ver5type;
        int axlon_enabled, mosaic_enabled;

        type = PreferencesTypeFromIndex((requestMachineTypeChange-1),&ver4type,&ver5type);
        if (ver5type == -1) {
            if (ver4type == -1)
                compositeType = type;
            else
                compositeType = 14+ver4type;
        } else {
            compositeType = 19+ver5type;
        }
		CalcMachineTypeRam(compositeType, &Atari800_machine_type,
						   &MEMORY_ram_size, &axlon_enabled,
                           &mosaic_enabled, &ULTIMATE_enabled,
                           &Atari800_builtin_basic, &Atari800_builtin_game,
                           &Atari800_keyboard_leds, &Atari800_jumper_present);
        CARTRIDGE_Remove();
        if (!axlon_enabled)
            MEMORY_axlon_num_banks = 0;
        else
            MEMORY_axlon_num_banks = PREFS_axlon_num_banks;

        if (!mosaic_enabled)
            MEMORY_mosaic_num_banks = 0;
        else
            MEMORY_mosaic_num_banks = PREFS_mosaic_num_banks;

        if (!EnableDisplayManager80ColMode(Atari800_machine_type, XEP80_enabled, AF80_enabled, BIT3_enabled))
            {
            XEP80_enabled = AF80_enabled = BIT3_enabled = FALSE;
            PLATFORM_80col = FALSE;
            PLATFORM_Switch80ColMode();
            }

        if (Atari800_machine_type == Atari800_MACHINE_5200)
            if (CARTRIDGE_main.type != CARTRIDGE_NONE)
                CARTRIDGE_Remove();
        memset(Screen_atari, 0, (Screen_HEIGHT * Screen_WIDTH));
        Atari800_InitialiseMachine();
		Atari800_Coldstart();
        CreateWindowCaption();
        SDL_SetWindowTitle(MainWindow, windowCaption);
        Atari_DisplayScreen((UBYTE *) Screen_atari);
		requestMachineTypeChange = 0;
		SetControlManagerMachineType(Atari800_machine_type, MEMORY_ram_size);
        SetControlManagerDisableBasic(Atari800_machine_type,  Atari800_disable_basic);
		UpdateMediaManagerInfo();
		}
	if (requestPBIExpansionChange) {
		if (requestPBIExpansionChange == 1) {
			PBI_BB_enabled = FALSE;
			PBI_MIO_enabled = FALSE;
			if (bbRequested || mioRequested) {
				bbRequested = FALSE;
				mioRequested = FALSE;
				Atari800_Coldstart();
			}
		} else if (requestPBIExpansionChange == 2) {
			PBI_MIO_enabled = FALSE;
			mioRequested = FALSE;
			if (!bbRequested) {
				init_bb();
				Atari800_Coldstart();
			}
			bbRequested = TRUE;
		} else {
			PBI_BB_enabled = FALSE;
			bbRequested = FALSE;
			if (!mioRequested) {
				init_mio();
				Atari800_Coldstart();
			}
			mioRequested = TRUE;
		}
		SetControlManagerPBIExpansion(requestPBIExpansionChange-1);
		requestPBIExpansionChange = 0;
		}	
    if (requestPauseEmulator) {
         pauseEmulator = 1-pauseEmulator;
         if (pauseEmulator) 
            PauseAudio(1);
         else
            PauseAudio(0);
         requestPauseEmulator = 0;
         }
    if (request80ColModeChange)
        {
        PLATFORM_Switch80ColMode();
        if (request80ColModeChange == 2)
            AF80_RemoveRightCartridge();
        Atari800_Coldstart();
        request80ColModeChange = 0;
        }
    if (requestColdReset) {
        Atari800_Coldstart();
        requestColdReset = 0;
        }
    if (requestWarmReset) {
        if (Atari800_disable_basic && disable_all_basic) {
            /* Disable basic on a warmstart, even though the real atarixl
               didn't work this way */
            GTIA_consol_override = 2;
            }
        Atari800_Warmstart();
        requestWarmReset = 0;
        }
    if (requestToolReset) {
        UInt8 *kbhits;
        kbhits = (Uint8 *) SDL_GetKeyboardState(NULL);
        if ((kbhits[SDL_SCANCODE_LSHIFT]) || (kbhits[SDL_SCANCODE_RSHIFT])) {
            Atari800_Coldstart();
            }
        else {
            if (Atari800_disable_basic && disable_all_basic) {
                /* Disable basic on a warmstart, even though the real atarixl
                   didn't work this way */
                GTIA_consol_override = 2;
                }
            Atari800_Warmstart();
            }
        requestToolReset = 0;
        }
    if (requestSaveState) {
        StateSav_SaveAtariState(saveFilename, "wb", TRUE);
        requestSaveState = 0;
        }
    if (requestLoadState) {
        StateSav_ReadAtariState(loadFilename, "rb");
		UpdateMediaManagerInfo();
        requestLoadState = 0;
        }
    if (requestCaptionChange) {
        CreateWindowCaption();
        SDL_SetWindowTitle(MainWindow, windowCaption);
		SetControlManagerMachineType(Atari800_machine_type, MEMORY_ram_size);
        if (!EnableDisplayManager80ColMode(Atari800_machine_type, XEP80_enabled, AF80_enabled, BIT3_enabled))
            {
            XEP80_enabled = AF80_enabled = BIT3_enabled = FALSE;
            PLATFORM_80col = FALSE;
            PLATFORM_Switch80ColMode();
            }
        requestCaptionChange = 0;
        }
	if (requestPaste) {
		if (PasteManagerStartPaste())
			pasteState = PASTE_START;
		capsLockPrePasteState = capsLockState;
		requestPaste = 0;
		}
	if (requestCopy) {
		copyStatus = COPY_IDLE;
        SelectionNormalize(&selectionStartX, &selectionStartY,
                      &selectionEndX, &selectionEndY);
		if (PLATFORM_80col) {
            if (XEP80_enabled) {
                if (XEP80GetCopyData(selectionStartX,selectionEndX,
								 selectionStartY,selectionEndY,
								 ScreenCopyData) != 0)
                    PasteManagerStartCopy(ScreenCopyData);
            } else if (AF80_enabled) {
                if (AF80GetCopyData(selectionStartX,selectionEndX,
                                 selectionStartY,selectionEndY,
                                 ScreenCopyData) != 0)
                    PasteManagerStartCopy(ScreenCopyData);
            } else if (BIT3_enabled) {
                if (Bit3GetCopyData(selectionStartX,selectionEndX,
                                 selectionStartY,selectionEndY,
                                 ScreenCopyData) != 0)
                    PasteManagerStartCopy(ScreenCopyData);
            }
		} else {
			int width = 0;
			int startx, endx; 
			
			switch (WIDTH_MODE) {
				case SHORT_WIDTH_MODE:
					width = 0;
					break;
				case DEFAULT_WIDTH_MODE:
					width = 8;
					break;
				case FULL_WIDTH_MODE:
					width = 32;
					break;
			}	
			if (selectionStartX < width)
				startx = 0;
			else if (selectionStartX > 319 + width)
				startx = 319;
			else
				startx = selectionStartX -width;
			if (selectionEndX < width)
				endx = 0;
			else if (selectionEndX > 319 + width)
				endx = 319;
			else
				endx = selectionEndX -width;
			Atari800GetCopyData(startx,endx,
								selectionStartY,selectionEndY,
								ScreenCopyData);
			PasteManagerStartCopy(ScreenCopyData);
		}
		requestCopy = 0;
		}
    if (requestMonitor)
        {
        if (!Atari800_Exit(TRUE))
            requestQuit = TRUE;
        }
}

/*------------------------------------------------------------------------------
*  ProcessMacPrefsChange - Handle requested changes do to preference window operations
*      in the Objective-C Cocoa code.
*-----------------------------------------------------------------------------*/
void ProcessMacPrefsChange()
{
    int retVal;
    
    if (requestPrefsChange) {
        CalculatePrefsChanged();
        loadMacPrefs(FALSE);
        if (displaySizeChanged)
            {
            SwitchWidth(WIDTH_MODE);
            }
        if (showfpsChanged)
            {
			if (!Screen_show_atari_speed)
                SDL_SetWindowTitle(MainWindow, windowCaption);
			SetDisplayManagerFps(Screen_show_atari_speed);
            }
		if (scaleModeChanged)
			{
            SwitchScaleMode(SCALE_MODE);
			}
        if (artifChanged)
            {
            ANTIC_UpdateArtifacting();
			SetDisplayManagerArtifactMode(ANTIC_artif_mode);
            }
        if (paletteChanged)
            {
            if (useBuiltinPalette) {
                Colours_Generate(paletteBlack, paletteWhite, paletteIntensity, paletteColorShift);
                Colours_Format(paletteBlack, paletteWhite, paletteIntensity);
                }
            else {
                retVal = read_palette(paletteFilename, adjustPalette);
                if (!retVal)
                    Colours_Generate(paletteBlack, paletteWhite, paletteIntensity, paletteColorShift);
                if (!retVal || adjustPalette)
                    Colours_Format(paletteBlack, paletteWhite, paletteIntensity);
                }
            CalcPalette();
            SetPalette();
            /* Clear the alternate page, so the first redraw is entire screen */
            if (Screen_atari)
                memset(Screen_atari, 0, (Screen_HEIGHT * Screen_WIDTH));
            }
        if (machineTypeChanged || osRomsChanged)
            {
            memset(Screen_atari_b, 0, (Screen_HEIGHT * Screen_WIDTH));
            Atari800_InitialiseMachine();
            CreateWindowCaption();
            SDL_SetWindowTitle(MainWindow, windowCaption);
            if (Atari800_machine_type == Atari800_MACHINE_5200)
                Atari_DisplayScreen((UBYTE *) Screen_atari_b);
            }
        if (patchFlagsChanged)
            {
            ESC_UpdatePatches();
			Devices_UpdatePatches();
            Atari800_Coldstart();
            }
        if (keyboardJoystickChanged)
            Init_SDL_Joykeys();
        if (hardDiskChanged)
            Devices_H_Init();
        if (pcLinkChanged) {
            Atari800_Coldstart();
        }
        if (pcLinkOptionsChanged) {
            Link_Device_Init_Devices();
        }
        if (xep80EnabledChanged) {
            if (!XEP80_enabled && PLATFORM_80col)
                PLATFORM_Switch80Col();
            }
        if (af80EnabledChanged) {
            if (!XEP80_enabled && PLATFORM_80col)
                PLATFORM_Switch80Col();
            }
        if (bit3EnabledChanged) {
            if (!BIT3_enabled && PLATFORM_80col)
                PLATFORM_Switch80Col();
            }
		if (xep80ColorsChanged && XEP80_enabled) {
			XEP80_ChangeColors();
			}
		if (configurationChanged) {
			configurationChanged = 0;
			if (clearCurrentMedia) {
				int i;
				for (i = 1; i <= SIO_MAX_DRIVES; i++)
					SIO_DisableDrive(i);
				CASSETTE_Remove();
				CARTRIDGE_Remove();
				}
			loadPrefsBinaries();
			UpdateMediaManagerInfo();
            Atari800_Coldstart();
		}
        if (fullscreenOptsChanged && FULLSCREEN_MACOS) {
            HandleScreenChange(current_w, current_h);
        }
        if (ultimateRomChanged) {
            int loaded;
            loaded = ULTIMATE_Change_Rom(ultimate_rom_filename, FALSE);
            if (loaded) {
                memset(Screen_atari, 0, (Screen_HEIGHT * Screen_WIDTH));
                Atari_DisplayScreen((UBYTE *) Screen_atari);
                Atari800_Coldstart();
            }
        }
        if (side2RomChanged) {
            int loaded;
            loaded = SIDE2_Change_Rom(side2_rom_filename, FALSE);
            if (loaded) {
                memset(Screen_atari, 0, (Screen_HEIGHT * Screen_WIDTH));
                Atari_DisplayScreen((UBYTE *) Screen_atari);
                Atari800_Coldstart();
            }
        }
        if (side2CFChanged) {
            SIDE2_Add_Block_Device(side2_nvram_filename);
        }
    if (Atari800_tv_mode == Atari800_TV_PAL)
            deltatime = (1.0 / 50.0) / emulationSpeed;
    else
            deltatime = (1.0 / 60.0) / emulationSpeed;
#ifdef SYNCHRONIZED_SOUND			 
	init_mzpokeysnd_sync();
#endif			 
        
    SetSoundManagerEnable(sound_enabled);
    SetSoundManagerStereo(POKEYSND_stereo_enabled);
    SetSoundManagerRecording(SndSave_IsSoundFileOpen());
    SetDisplayManagerWidthMode(WIDTH_MODE);
    SetDisplayManagerFps(Screen_show_atari_speed);
    SetDisplayManagerScaleMode(SCALE_MODE);
	SetDisplayManagerArtifactMode(ANTIC_artif_mode);
	SetDisplayManager80ColMode(XEP80_enabled, XEP80_port, AF80_enabled, BIT3_enabled, PLATFORM_80col);
    SetDisplayManagerXEP80Autoswitch(COL80_autoswitch);
	MediaManager80ColMode(XEP80_enabled, AF80_enabled, BIT3_enabled, PLATFORM_80col);
    real_deltatime = deltatime;
	SetControlManagerDisableBasic(Atari800_machine_type, Atari800_disable_basic);
	SetControlManagerKeyjoyEnable(keyjoyEnable);
    SetControlManagerCX85Enable(INPUT_cx85);
    SetControlManagerXEGSKeyboard(!Atari800_keyboard_detached);
    SetControlManagerLimit(speed_limit);
	SetControlManagerMachineType(Atari800_machine_type, MEMORY_ram_size);
    if (!EnableDisplayManager80ColMode(Atari800_machine_type, XEP80_enabled, AF80_enabled, BIT3_enabled))
        {
        XEP80_enabled = AF80_enabled = BIT3_enabled = FALSE;
        PLATFORM_80col = FALSE;
        PLATFORM_Switch80ColMode();
        }
	if (bbRequested)
		SetControlManagerPBIExpansion(1);
	else if (mioRequested)
		SetControlManagerPBIExpansion(2);
	else
		SetControlManagerPBIExpansion(0);
	SetControlManagerArrowKeys(useAtariCursorKeys);
    UpdateMediaManagerInfo();
    Check_SDL_Joysticks();
    Remove_Bad_SDL_Joysticks();
    Init_SDL_Joykeys();
    if (speed_limit == 0) {
        deltatime = DELTAFAST;
#ifdef SYNCHRONIZED_SOUND			 
		init_mzpokeysnd_sync();
#endif			 
		screenSwitchEnabled = FALSE;
		}
	else {
        deltatime = real_deltatime;
#ifdef SYNCHRONIZED_SOUND			 
		init_mzpokeysnd_sync();
#endif			 
		screenSwitchEnabled = TRUE;
		}

    Atari800_UpdateKeyboardDetached();
    if (Atari800_jumper_present)
        Atari800_UpdateJumper();
    requestPrefsChange = 0;
	}
                    
}

/*------------------------------------------------------------------------------
* CreateWindowCaption - Change the window title in windowed mode based on type
*     of emulated machine.
*-----------------------------------------------------------------------------*/
void CreateWindowCaption()
{
    char *machineType;
	char xep80String[10];

    if (PLATFORM_80col && XEP80_enabled) {
        strcpy(xep80String," - XEP80");
        }
    else if (PLATFORM_80col && AF80_enabled) {
        strcpy(xep80String," - AF80");
        }
    else if (PLATFORM_80col && BIT3_enabled) {
        strcpy(xep80String," - Bit3");
        }
    else {
		xep80String[0] = 0;
		}
    
    switch(Atari800_machine_type) {
        case Atari800_MACHINE_800:
            machineType = "400/800";
            break;
        case Atari800_MACHINE_XLXE:
            if (Atari800_builtin_game)
                machineType = "XEGS";
            else if (Atari800_keyboard_leds)
                machineType = "1200XL";
            else
                machineType = "XL/XE";
            break;
        case Atari800_MACHINE_5200:
            machineType = "5200";
            break;
        default:
            machineType = "";
            break;
        }
    
    if (ULTIMATE_enabled) {
        if (Atari800_builtin_game) {
            if (SIDE2_enabled)
                sprintf(windowCaption, "XEGS Ultimate 1MB + SIDE 2 %dK", MEMORY_ram_size);
            else
                sprintf(windowCaption, "XEGS Ultimate 1MB %dK", MEMORY_ram_size);
        } else if (Atari800_keyboard_leds) {
            if (SIDE2_enabled)
                sprintf(windowCaption, "1200XL Ultimate 1MB + SIDE 2 %dK", MEMORY_ram_size);
            else
                sprintf(windowCaption, "1200XL Ultimate 1MB %dK", MEMORY_ram_size);
        } else {
            if (SIDE2_enabled)
                sprintf(windowCaption, "XL Ultimate 1MB + SIDE 2 %dK", MEMORY_ram_size);
            else
                sprintf(windowCaption, "XL Ultimate 1MB %dK", MEMORY_ram_size);
        }
    }
	else if (MEMORY_axlon_num_banks > 0)
        sprintf(windowCaption, "%s Axlon %dK%s", machineType,
				((MEMORY_axlon_num_banks) * 16) + 32,xep80String);
	else if (MEMORY_mosaic_num_banks > 0)
        sprintf(windowCaption, "%s Mosaic %dK%s", machineType,
				((MEMORY_mosaic_num_banks) * 4) + 48,xep80String);
    else if (MEMORY_ram_size == MEMORY_RAM_320_RAMBO)
        sprintf(windowCaption, "%s 320K RAMBO%s", machineType,xep80String);
    else if (MEMORY_ram_size == MEMORY_RAM_320_COMPY_SHOP)
        sprintf(windowCaption, "%s 320K COMPY SHOP%s", machineType,xep80String);
    else
        sprintf(windowCaption, "%s %dK%s", machineType, MEMORY_ram_size,xep80String); 
}

void MAC_LED_Frame(void)
{
	static int last_led_status = 0;
	static int last_led_sector = 9999;
	int diskNo, read;

	if (led_status && led_counter_enabled_media && led_enabled_media) {
		if (led_status < 10) 
			diskNo = led_status-1;
		else 
			diskNo = led_status-10;
		if (led_sector != last_led_sector)
			MediaManagerSectorLed(diskNo, led_sector, 1);
		last_led_sector = led_sector;
		}
	
	if (led_status != last_led_status && led_enabled_media)
		{
		if (led_status != 0) {
			/* Turn on status LED */
			if (led_status < 10) {
				diskNo = led_status-1;
				read = 1;
				}
			else {
				diskNo = led_status-10;
				read = 0;
				}
			MediaManagerStatusLed(diskNo, 1, read);
			}
		if (last_led_status != 0)
			{
			/* Turn off status LED */
			if (last_led_status < 10)
				diskNo = last_led_status-1;
			else
				diskNo = last_led_status-10;
			MediaManagerStatusLed(diskNo, 0, 0);
			MediaManagerSectorLed(diskNo, 0, 0);
			last_led_sector = 9999;
			}
		last_led_status = led_status;
		}
}

void Casette_Frame(void)
{
	static int last_block = 0;
    int current_block = CASSETTE_GetPosition();
	if (current_block != last_block) {
		MediaManagerCassSliderUpdate(current_block);
		last_block = current_block;
		}
}

static double Atari800Time(void)
{
  struct timeval tp;

  gettimeofday(&tp, NULL);
  return tp.tv_sec + 1e-6 * tp.tv_usec;
}

static void SDL_Atari_CX85(void)
{
	/* CX85 numeric keypad */
	if (INPUT_key_code <= AKEY_CX85_1 && INPUT_key_code >= AKEY_CX85_YES) {
		int val = 0;
		switch (INPUT_key_code) {
			case AKEY_CX85_1:
				val = 0x19;
				break;
			case AKEY_CX85_2:
				val = 0x1a;
				break;
			case AKEY_CX85_3:
				val = 0x1b;
				break;
			case AKEY_CX85_4:
				val = 0x11;
				break;
			case AKEY_CX85_5:
				val = 0x12;
				break;
			case AKEY_CX85_6:
				val = 0x13;
				break;
			case AKEY_CX85_7:
				val = 0x15;
				break;
			case AKEY_CX85_8:
				val = 0x16;
				break;
			case AKEY_CX85_9:
				val = 0x17;
				break;
			case AKEY_CX85_0:
				val = 0x1c;
				break;
			case AKEY_CX85_PERIOD:
				val = 0x1d;
				break;
			case AKEY_CX85_MINUS:
				val = 0x1f;
				break;
			case AKEY_CX85_PLUS_ENTER:
				val = 0x1e;
				break;
			case AKEY_CX85_ESCAPE:
				val = 0x0c;
				break;
			case AKEY_CX85_NO:
				val = 0x14;
				break;
			case AKEY_CX85_DELETE:
				val = 0x10;
				break;
			case AKEY_CX85_YES:
				val = 0x18;
				break;
		}
		if (val > 0) {
			STICK[cx85_port] = (val&0x0f);
			POKEY_POT_input[cx85_port*2+1] = ((val&0x10) ? 0 : 228);
			TRIG_input[cx85_port] = 0;
		}
	}
}

void SelectionNormalize(int *startX, int *startY, int *endX, int* endY)
{
    if (*endX < *startX) {
        int swap_x;
        
        swap_x = *endX;
        *endX = *startX;
        *startX = swap_x;
    }
    if (*endY < *startY) {
        int swap_y;
        
        swap_y = *endY;
        *endY = *startY;
        *startY = swap_y;
    }

    if (FULLSCREEN_MACOS) {
        *startX -= screen_x_offset;
        *startY -= screen_y_offset;
        *endX -= screen_x_offset;
        *endY -= screen_y_offset;
    }
}

void DrawSelectionRectangle(int orig_x, int orig_y, int copy_x, int copy_y)
{
    int i,y;
    double scaleX, scaleY;

	if (copy_x < orig_x) {
		int swap_x;
		
		swap_x = copy_x;
		copy_x = orig_x;
		orig_x = swap_x;
	}
	if (copy_y < orig_y) {
		int swap_y;
		
		swap_y = copy_y;
		copy_y = orig_y;
		orig_y = swap_y;
	}
	
    scaleX = scaleFactorRenderX;
    scaleY = scaleFactorRenderY;
	
	if (SCALE_MODE==NORMAL_SCALE || SCALE_MODE == SCANLINE_SCALE) {
		register int pitch2;
		register Uint16 *start16;
		
		pitch2 = MainScreen->pitch / 2;

		orig_x = (double) orig_x/scaleX;
		orig_y = (double) orig_y/scaleY;
		copy_x = (double) copy_x/scaleX;
		copy_y = (double) copy_y/scaleY;
		
        if (FULLSCREEN_MACOS) {
            orig_x -= screen_x_offset;
            orig_y -= screen_y_offset;
            copy_x -= screen_x_offset;
            copy_y -= screen_y_offset;
        }
        
		start16 = (Uint16 *) MainScreen->pixels +
		(orig_y * pitch2) + orig_x;
		for (i=0;i<(copy_x-orig_x+1);i++,start16++)
			*start16 ^= 0xFFFF;
		if (copy_y > orig_y) {
			start16 = (Uint16 *) MainScreen->pixels + 
			(copy_y * pitch2) + orig_x;
			for (i=0;i<(copy_x-orig_x+1);i++,start16++)
				*start16 ^= 0xFFFF;
		}
		for (y=orig_y+1;y < copy_y;y++) {
			start16 = (Uint16 *) MainScreen->pixels + 
				(y * pitch2) + orig_x;
			*start16 ^= 0xFFFF;
		}
		if (copy_x > orig_x) {
			for (y=orig_y+1;y < copy_y;y++) {
				start16 = (Uint16 *) MainScreen->pixels + 
					(y * pitch2) + copy_x;
				*start16 ^= 0xFFFF;
			}
		}
	}
}

void ProcessCopySelection(int *first_row, int *last_row, int selectAll)
{
	static int orig_x = 0;
	static int orig_y = 0;
	int copy_x, copy_y;
	static Uint8 mouse = 0;
    double scaleX = 1.0;
    double scaleY = 1.0;

    scaleX = scaleFactorRenderX;
    scaleY = scaleFactorRenderY;

	if (selectAll) {
		orig_x = 0;
		orig_y = 0;
        int win_x, win_y;
        SDL_GetWindowSize(MainWindow, &win_x, &win_y);
        copy_x = win_x - 1;
        copy_y = win_y - 1;
	} else {
		if (Atari800IsKeyWindow()) {
			mouse = SDL_GetMouseState(&copy_x, &copy_y);
			if ((copyStatus == COPY_IDLE) &&
				((mouse & SDL_BUTTON(1)) == 0)) {
				return;		
				}
			}
		else {
			mouse = 0;
			copyStatus = COPY_IDLE;
			return;		
		}
	}
	*first_row = 0;
	*last_row = Screen_HEIGHT - 1;
	full_display = FULL_DISPLAY_COUNT;
	
	switch(copyStatus) {
		case COPY_IDLE:
			if (selectAll) {
				copyStatus = COPY_DEFINED;
				orig_x = 0;
				orig_y = 0;
				selectionStartX = (double)orig_x/scaleX;
				selectionStartY = (double)orig_y/scaleY;
				selectionEndX = (double)copy_x/scaleX;
				selectionEndY = (double)copy_y/scaleY;
				DrawSelectionRectangle(orig_x, orig_y, copy_x, copy_y);
			}
			else if (mouse & SDL_BUTTON(1) ) {
				copyStatus = COPY_STARTED;
				orig_x = copy_x;
				orig_y = copy_y;
				DrawSelectionRectangle(orig_x, orig_y, copy_x, copy_y);
			}
			break;
		case COPY_STARTED:
			DrawSelectionRectangle(orig_x, orig_y, copy_x, copy_y);
			if ((mouse & SDL_BUTTON(1)) == 0) {
				if (orig_x == copy_x && orig_y == copy_y) {
					copyStatus = COPY_IDLE;
				} else {
					copyStatus = COPY_DEFINED;
					selectionStartX = (double)orig_x/scaleX;
					selectionStartY = (double)orig_y/scaleY;
					selectionEndX = (double)copy_x/scaleX;
					selectionEndY = (double)copy_y/scaleY;
				}
			}
			break;
		case COPY_DEFINED:
			if (selectAll){
				selectionStartX = (double)orig_x/scaleX;
				selectionStartY = (double)orig_y/scaleY;
				selectionEndX = (double)copy_x/scaleX;
				selectionEndY = (double)copy_y/scaleY;

				DrawSelectionRectangle(orig_x, orig_y, copy_x, copy_y);
			} else if (mouse & SDL_BUTTON(1)) {
				copyStatus = COPY_STARTED;
				orig_x = copy_x;
				orig_y = copy_y;
				DrawSelectionRectangle(orig_x, orig_y, copy_x, copy_y);
			} else {
				DrawSelectionRectangle(orig_x, orig_y, 
									   selectionEndX*scaleX,
                                       selectionEndY*scaleY);
			}
			break;
	}
}

int IsCopyDefined(void)
{
	if (copyStatus == COPY_DEFINED)
		return TRUE;
	else
		return FALSE;
}

/*------------------------------------------------------------------------------
* main - main function of emulator with main execution loop.
*-----------------------------------------------------------------------------*/
int SDL_main(int argc, char **argv)
{
    Log_print("TEST: Starting SDL_main - FujiNet debug test");
    int keycode;
    int done = 0;
    int i;
    int retVal;
	double last_time = 0.0;
    /* Flag for delayed FujiNet restart */
    static int fujinet_restart_pending = 0;
    static int fujinet_restart_delay = 0;

    POKEYSND_stereo_enabled = FALSE; /* Turn this off here....otherwise games only come
                             from one channel...you only want this for demos mainly
                             anyway */
    SDL_version v;
	SDL_GetVersion(&v);
	Log_print("SDL Version: %u.%u.%u", v.major, v.minor, v.patch);
	SDLMainActivate();
	
    if (!fileToLoad) {
	  argc = prefsArgc;
      argv = prefsArgv;
	  }

    if (!Atari800_Initialise(&argc, argv))
        return 3;

    /* Load Mac preferences to properly set machine type and other settings for first time startup */
    loadMacPrefs(TRUE);

    if (!EnableDisplayManager80ColMode(Atari800_machine_type, XEP80_enabled, AF80_enabled, BIT3_enabled))
        {
        XEP80_enabled = AF80_enabled = BIT3_enabled = FALSE;
        }

    CreateWindowCaption();
    SDL_SetWindowTitle(MainWindow, windowCaption);
    PasteManagerUpdateEscapeCopyMenu();
    
    if (useBuiltinPalette) {
        Colours_Generate(paletteBlack, paletteWhite, paletteIntensity, paletteColorShift);
        Colours_Format(paletteBlack, paletteWhite, paletteIntensity);
        }
    else {
        retVal = read_palette(paletteFilename, adjustPalette);
        if (!retVal)
            Colours_Generate(paletteBlack, paletteWhite, paletteIntensity, paletteColorShift);
        if (!retVal || adjustPalette)
            Colours_Format(paletteBlack, paletteWhite, paletteIntensity);
        }
    CalcPalette();
    SetPalette();

    SetSoundManagerEnable(sound_enabled);
    SetSoundManagerStereo(POKEYSND_stereo_enabled);
    SetSoundManagerRecording(SndSave_IsSoundFileOpen());
    SetDisplayManagerWidthMode(WIDTH_MODE);
    SetDisplayManagerFps(Screen_show_atari_speed);
    SetDisplayManagerScaleMode(SCALE_MODE);
	SetDisplayManagerArtifactMode(ANTIC_artif_mode);
	SetDisplayManager80ColMode(XEP80_enabled, XEP80_port, AF80_enabled, BIT3_enabled, PLATFORM_80col);
    SetDisplayManagerXEP80Autoswitch(COL80_autoswitch);
	MediaManager80ColMode(XEP80_enabled, AF80_enabled, BIT3_enabled, PLATFORM_80col);
    real_deltatime = deltatime;
    SetControlManagerLimit(speed_limit);
	SetControlManagerDisableBasic(Atari800_machine_type, Atari800_disable_basic);
	SetControlManagerKeyjoyEnable(keyjoyEnable);
	SetControlManagerCX85Enable(INPUT_cx85);
    SetControlManagerXEGSKeyboard(!Atari800_keyboard_detached);
	SetControlManagerMachineType(Atari800_machine_type, MEMORY_ram_size);
	if (bbRequested)
		SetControlManagerPBIExpansion(1);
	else if (mioRequested)
		SetControlManagerPBIExpansion(2);
	else
		SetControlManagerPBIExpansion(0);
		
#ifdef NETSIO
    /* Initialize NetSIO for FujiNet connectivity if enabled in preferences */
    /* This happens after machine type and SIO patches are properly set by loadMacPrefs() */
    {
        ATARI800MACX_PREF *prefs = getPrefStorage();
        if (prefs->fujiNetEnabled) {
            int port = prefs->fujiNetPort;
            if (port <= 0 || port > 65535) port = 9997; // Default port if invalid
            
            printf("DEBUG: About to call netsio_init on port %d\n", port);
            if (netsio_init(port) == 0) {
                printf("DEBUG: netsio_init succeeded, will trigger restart after main loop starts\n");
                /* Set flag to trigger restart after main loop starts */
                fujinet_restart_pending = 1;
                fujinet_restart_delay = 60; /* Wait 60 frames (~1 second) */
            } else {
                printf("DEBUG: netsio_init FAILED\n");
            }
        }
    }
#endif
	SetControlManagerArrowKeys(useAtariCursorKeys);
	PrintOutputControllerSelectPrinter(currPrinter);
    UpdateMediaManagerInfo(); 
    if (speed_limit == 0) {
        deltatime = DELTAFAST;
#ifdef SYNCHRONIZED_SOUND			 
		init_mzpokeysnd_sync();
#endif			 
	}

    /* Clear the alternate page, so the first redraw is entire screen */
    memset(Screen_atari_b, 0, (Screen_HEIGHT * Screen_WIDTH));
	
	/* Create the OpenGL Monitor Screen */
	MonitorGLScreen = SDL_CreateRGBSurface(SDL_SWSURFACE, 640, 480, 16,
								0x0000F800, 0x000007E0, 0x0000001F, 0x00000000);

    SDL_PauseAudio(0);
    SDL_CenterMousePointer();
    SDL_JoystickEventState(SDL_IGNORE);

    if (fileToLoad) 
        SDLMainLoadStartupFile();
	if (mediaStatusWindowOpen)
		MediaManagerStatusWindowShow();
	if (functionKeysWindowOpen)
		ControlManagerFunctionKeysWindowShow();
	Atari800MakeKeyWindow();

    /* First time thru, get the keystate since we will be doing joysticks first */
    int numKbstate;
    kbhits = (Uint8 *) SDL_GetKeyboardState(&numKbstate);
	
	chdir("../");
    
    /* Start bootup paste, if any */
    if (PasteManagerStartupPasteEnabled()) {
        PasteManagerStartPasteWithString();
        pasteState = PASTE_START;
    }

    while (!done) {
        /* Handle delayed FujiNet restart */
        if (fujinet_restart_pending && fujinet_restart_delay > 0) {
            fujinet_restart_delay--;
            if (fujinet_restart_delay == 0) {
                printf("DEBUG: Triggering delayed FujiNet machine initialization\n");
                Atari800_InitialiseMachine();
                fujinet_restart_pending = 0;
                printf("DEBUG: FujiNet delayed machine initialization completed\n");
            }
        }
        
        /* Handle joysticks and paddles */
        SDL_Atari_PORT(&STICK[0], &STICK[1], &STICK[2], &STICK[3]);
        keycode = SDL_Atari_TRIG(&TRIG_input[0], &TRIG_input[1], &TRIG_input[2], &TRIG_input[3],
                                 &pad_key_shift, &pad_key_control, &pad_key_consol);
        for (i=0;i<4;i++) {
            if (JOYSTICK_MODE[i] == MOUSE_EMULATION)
                break;
            }
        if ((i<4) && (INPUT_mouse_mode != INPUT_MOUSE_OFF)) { 
			INPUT_mouse_port = i;
            SDL_Atari_Mouse(&STICK[i],&TRIG_input[i]);
			}

        INPUT_key_consol = INPUT_CONSOL_NONE;
        switch(keycode) {
            case AKEY_NONE:
                keycode = Atari_Keyboard();
                break;
            case AKEY_BUTTON2:
                INPUT_key_shift = 1;
                keycode = AKEY_NONE;
                break;
            case AKEY_5200_RESET:
                if (Atari800_machine_type == Atari800_MACHINE_5200)
                    keycode = AKEY_WARMSTART;
                break;
            case AKEY_BUTTON_RESET:
                if (Atari800_machine_type != Atari800_MACHINE_5200)
                    keycode = AKEY_WARMSTART;
                break;
            default:
                Atari_Keyboard(); /* Get the keyboard state (for shift and control), but 
                                      throw away the key, since the keypad key takes precedence */
                if (INPUT_key_shift)
                    keycode |= AKEY_SHFT;
            }
		
        // Merge the gamepad buttons and keyboard mod keys
		if (pad_key_shift && !INPUT_key_shift)
		   keycode |= AKEY_SHFT;
        INPUT_key_shift |= pad_key_shift;
		CONTROL |= pad_key_control;
		INPUT_key_consol &= pad_key_consol;
		
        switch (keycode) {
        case AKEY_EXIT:
            done = 1;
            break;
        case AKEY_COLDSTART:
            Atari800_Coldstart();
            break;
        case AKEY_WARMSTART:
			if (Atari800_disable_basic && disable_all_basic) {
				/* Disable basic on a warmstart, even though the real atarixl
					didn't work this way */
                GTIA_consol_override = 2;
				}
            Atari800_Warmstart();
            break;
        case AKEY_SCREENSHOT:
            Save_TIFF_file(Find_TIFF_name());
            break;
        case AKEY_BREAK:
            INPUT_key_break = 1;
            if (!last_key_break) {
                if (POKEY_IRQEN & 0x80) {
                    POKEY_IRQST &= ~0x80;
                    CPU_GenerateIRQ();
                }
                break;
		case AKEY_PBI_BB_MENU:
			PBI_BB_Menu();
			break;
		default:
            INPUT_key_break = 0;
            INPUT_key_code = keycode;
            break;
            }
        }
		
		SDL_Atari_CX85();
        
        if (Atari800_machine_type == Atari800_MACHINE_5200) {
            if (INPUT_key_shift & !last_key_break) {
		if (POKEY_IRQEN & 0x80) {
		    POKEY_IRQST &= ~0x80;
                    CPU_GenerateIRQ();
                    }
                }
             last_key_break = INPUT_key_shift;
           }
        else
            last_key_break = INPUT_key_break;

        INPUT_key_code = INPUT_key_code | CONTROL;

        POKEY_SKSTAT |= 0xc;
        if (INPUT_key_shift)
            POKEY_SKSTAT &= ~8;
        if (INPUT_key_code == AKEY_NONE)
            last_key_code = AKEY_NONE;
		/* Following keys cannot be read with both shift and control pressed:
		 J K L ; + * Z X C V B F1 F2 F3 F4 HELP	 */
		/* which are 0x00-0x07 and 0x10-0x17 */
		/* This is caused by the keyboard itself, these keys generate 'ghost keys'
		 * when pressed with shift and control */
		if (Atari800_machine_type != Atari800_MACHINE_5200 && (INPUT_key_code&~0x17) == AKEY_SHFTCTRL) {
			INPUT_key_code = AKEY_NONE;
		}
        if (INPUT_key_code != AKEY_NONE) {
			/* The 5200 has only 4 of the 6 keyboard scan lines connected */
			/* Pressing one 5200 key is like pressing 4 Atari 800 keys. */
			/* The LSB (bit 0) and bit 5 are the two missing lines. */
			/* When debounce is enabled, multiple keys pressed generate
			 * no results. */
			/* When debounce is disabled, multiple keys pressed generate
			 * results only when in numerical sequence. */
			/* Thus the LSB being one of the missing lines is important
			 * because that causes events to be generated. */
			/* Two events are generated every 64 scan lines
			 * but this code only does one every frame. */
			/* Bit 5 is different for each keypress because it is one
			 * of the missing lines. */
			if (Atari800_machine_type == Atari800_MACHINE_5200) {
				static int bit5_5200 = 0;
				if (bit5_5200) {
					INPUT_key_code &= ~0x20;
				}
				bit5_5200 = !bit5_5200;
				/* 5200 2nd fire button generates CTRL as well */
				if (INPUT_key_shift) {
					INPUT_key_code |= AKEY_SHFTCTRL;
				}
			}
            POKEY_SKSTAT &= ~4;
            if ((INPUT_key_code ^ last_key_code) & ~AKEY_SHFTCTRL) {
            /* ignore if only shift or control has changed its state */
                last_key_code = INPUT_key_code;
                POKEY_KBCODE = (UBYTE) INPUT_key_code;
                if (POKEY_IRQEN & 0x40) {
                    if (POKEY_IRQST & 0x40) {
                        POKEY_IRQST &= ~0x40;
                        CPU_GenerateIRQ();
                    }
                    else {
                        /* keyboard over-run */
                        POKEY_SKSTAT &= ~0x40;
                        /* assert(CPU_IRQ != 0); */
                    }
                }
            }
        }

	if (INPUT_joy_multijoy && Atari800_machine_type != Atari800_MACHINE_5200) {
		PIA_PORT_input[0] = 0xf0 | STICK[joy_multijoy_no];
		PIA_PORT_input[1] = 0xff;
		GTIA_TRIG[0] = TRIG_input[joy_multijoy_no];
        GTIA_TRIG[1] = 1;
	}
	else {
		GTIA_TRIG[0] = TRIG_input[0];
		GTIA_TRIG[1] = TRIG_input[1];
		PIA_PORT_input[0] = (STICK[1] << 4) | STICK[0];
		PIA_PORT_input[1] = (STICK[3] << 4) | STICK[2];
	}
    if (Atari800_machine_type != Atari800_MACHINE_XLXE) {
        GTIA_TRIG[2] = TRIG_input[2];
        GTIA_TRIG[3] = TRIG_input[3];
    }

        /* switch between screens to enable delta output */
		if (screenSwitchEnabled) {
			if (Screen_atari==Screen_atari1) {
				Screen_atari = Screen_atari2;
				Screen_atari_b = Screen_atari1;
			}
			else {
				Screen_atari = Screen_atari1;
				Screen_atari_b = Screen_atari2;
			}
			if (speed_limit == 0 || (speed_limit == 1 && deltatime <= 1.0/Atari800_FPS_PAL))
				screenSwitchEnabled = FALSE;
		}

        ProcessMacMenus();
        
        if (XEP80_enabled && COL80_autoswitch) {
            if (XEP80_sent_count > XEP80_last_sent_count + 1) {
                if (!PLATFORM_80col)
                    PLATFORM_Switch80Col();
            }
            XEP80_last_sent_count = XEP80_sent_count;
        }

        /* If emulator is in Ultimate mode without a valid ROM */
        if (ULTIMATE_enabled && !ULTIMATE_have_rom) {
            /* Clear the screen if we are in 5200 mode, with no cartridge */
            BasicUIInit();
            memset(Screen_atari_b, 0, (Screen_HEIGHT * Screen_WIDTH));
            ClearScreen();
            CenterPrint(0x9e, 0x94, "Atari Ultimate1mb Emulator", 11);
            CenterPrint(0x9e, 0x94, "Error Loading Ultimate1mb ROM File", 12);
            Atari_DisplayScreen((UBYTE *) Screen_atari);
        }
        /* If emulator is in SIDE2 mode without a valid ROM */
        else if (SIDE2_enabled && !SIDE2_have_rom) {
            /* Clear the screen if we are in 5200 mode, with no cartridge */
            BasicUIInit();
            memset(Screen_atari_b, 0, (Screen_HEIGHT * Screen_WIDTH));
            ClearScreen();
            CenterPrint(0x9e, 0x94, "Atari SIDE2 Emulator", 11);
            CenterPrint(0x9e, 0x94, "Error Loading SIDE2 ROM File", 12);
            Atari_DisplayScreen((UBYTE *) Screen_atari);
        }
        /* If emulator isn't paused, and 5200 has a cartridge */
        else if (!pauseEmulator && !((Atari800_machine_type == Atari800_MACHINE_5200) && (CARTRIDGE_main.type == CARTRIDGE_NONE))) {
			PBI_BB_Frame(); /* just to make the menu key go up automatically */
            Devices_Frame();
            GTIA_Frame();
            ANTIC_Frame(TRUE);
			if (mediaStatusWindowOpen)
				MAC_LED_Frame();
			if (mediaStatusWindowOpen)
				Casette_Frame();
            SDL_DrawMousePointer();
            POKEY_Frame();
			SDL_Sound_Update();
            Atari800_nframes++;
            Atari800_Sync();
            CountFPS();
			if (speed_limit == 0 || (speed_limit == 1 && deltatime <= 1.0/Atari800_FPS_PAL)) {
				if (Atari800Time() >= last_time + 1.0/60.0) {
                    Screen_DrawDiskLED();
                    Screen_DrawHDDiskLED();
                    if (FULLSCREEN_MACOS)
                        Screen_DrawAtariSpeed(currentFps);
                    Screen_Draw1200LED();
                    Screen_DrawCapslock(MEMORY_dGetByte(0x2BE));
					Atari_DisplayScreen((UBYTE *) Screen_atari);
					last_time = Atari800Time();
					screenSwitchEnabled = TRUE;
					}
				}
			else {
                Screen_DrawDiskLED();
                Screen_DrawHDDiskLED();
                Screen_Draw1200LED();
                Screen_DrawCapslock(MEMORY_dGetByte(0x2BE));
				Atari_DisplayScreen((UBYTE *) Screen_atari);
				}
            }
        else if ((Atari800_machine_type == Atari800_MACHINE_5200) && (CARTRIDGE_main.type == CARTRIDGE_NONE)){
            /* Clear the screen if we are in 5200 mode, with no cartridge */
            BasicUIInit();
            memset(Screen_atari_b, 0, (Screen_HEIGHT * Screen_WIDTH));
            ClearScreen();
            CenterPrint(0x9e, 0x94, "Atari 5200 Emulator", 11);
            CenterPrint(0x9e, 0x94, "Please Insert Cartridge", 12);
            Atari_DisplayScreen((UBYTE *) Screen_atari);
            }
        else {
            usleep(100000);
			Atari_DisplayScreen((UBYTE *) Screen_atari);
			}

        ProcessMacPrefsChange();

        if (requestQuit)
            done = TRUE;
			
		AboutBoxScroll();
		
        }
    Atari800_Exit(FALSE);
    Log_flushlog();
    return 0;
}

#if SUPPORTS_CHANGE_VIDEOMODE
/* Stub implementations for videomode support functions.
   These provide basic compatibility but don't implement full video mode switching.
   TODO: Implement proper video mode management for macOS. */

int PLATFORM_SupportsVideomode(VIDEOMODE_MODE_t mode, int stretch, int rotate90)
{
    /* For now, only support basic modes without stretching/rotation */
    return (mode == VIDEOMODE_MODE_NORMAL && !stretch && !rotate90);
}

void PLATFORM_SetVideoMode(VIDEOMODE_resolution_t const *res, int windowed, VIDEOMODE_MODE_t mode, int rotate90)
{
    /* Stub: Video mode changes not yet implemented for macOS.
       The existing SDL implementation handles basic windowing. */
    Log_print("PLATFORM_SetVideoMode: Video mode switching not yet implemented for macOS");
}

VIDEOMODE_resolution_t *PLATFORM_DesktopResolution(void)
{
    /* Return a static default resolution for compatibility */
    static VIDEOMODE_resolution_t desktop_res = { 1024, 768 };
    return &desktop_res;
}

int PLATFORM_WindowMaximised(void)
{
    /* Stub: Window state detection not implemented */
    return 0;
}

VIDEOMODE_resolution_t *PLATFORM_AvailableResolutions(unsigned int *size)
{
    /* Return a basic set of resolutions for compatibility */
    static VIDEOMODE_resolution_t resolutions[] = {
        { 640, 480 },
        { 800, 600 },
        { 1024, 768 },
        { 1280, 1024 }
    };
    
    if (size)
        *size = sizeof(resolutions) / sizeof(resolutions[0]);
    
    return resolutions;
}

#endif /* SUPPORTS_CHANGE_VIDEOMODE */

/* PLATFORM functions required by new Atari800 core */
int PLATFORM_SoundSetup(Sound_setup_t *setup)
{
    /* Mac uses existing SDL audio setup in Sound_Initialise() */
    return sound_enabled;
}

void PLATFORM_SoundExit(void)
{
    /* Mac handles sound cleanup in Sound_Exit() */
    SDL_CloseAudio();
}

void PLATFORM_SoundPause(void)
{
    SDL_PauseAudio(1);
}

void PLATFORM_SoundContinue(void)
{
    SDL_PauseAudio(0);
}

unsigned int PLATFORM_SoundAvailable(void)
{
    /* Return reasonable buffer size for Mac audio */
    return sound_enabled ? 4096 : 0;
}

void PLATFORM_SoundWrite(UBYTE const *buffer, unsigned int size)
{
    /* Mac handles sound writing through existing callback system */
    /* This is a compatibility stub */
}

int PLATFORM_PORT(int num)
{
    /* Mac-specific controller port implementation */
    return 0xFF; /* No input */
}

int PLATFORM_TRIG(int num)
{
    /* Mac-specific trigger implementation */
    return 1; /* Not pressed */
}

void PLATFORM_GetPixelFormat(PLATFORM_pixel_format_t *format)
{
    /* Mac-specific pixel format */
    if (format) {
        format->bpp = 32;
        format->rmask = 0x00FF0000;
        format->gmask = 0x0000FF00;
        format->bmask = 0x000000FF;
    }
}

void PLATFORM_MapRGB(void *dest, int const *palette, int size)
{
    /* Mac delegate to standard palette mapping */
    int i;
    Uint32 *colors = (Uint32 *)dest;
    
    for (i = 0; i < size; i += 3) {
        /* Convert RGB to 32-bit ARGB format */
        colors[i/3] = (0xFF << 24) | (palette[i] << 16) | (palette[i+1] << 8) | palette[i+2];
    }
}

void PLATFORM_PaletteUpdate(void)
{
    /* Mac handles palette updates through existing system */
}

/* Mac-specific wrapper functions for legacy device support */
void Devices_H_Init(void)
{
    /* Device H initialization - handled internally by new core */
}

int Devices_enable_d_patch = 1; /* Mac device patch enable flag */

/* Mac-specific flash memory functions for legacy support */
UBYTE MEMORY_FlashGetByte(UWORD addr)
{
    /* Flash memory read - delegated to standard memory system */
    return MEMORY_GetByte(addr);
}

void MEMORY_FlashPutByte(UWORD addr, UBYTE byte)
{
    /* Flash memory write - delegated to standard memory system */
    MEMORY_PutByte(addr, byte);
}

/* Additional legacy flash memory support */
void MEMORY_SetFlashRoutines(UBYTE (*get)(UWORD), void (*put)(UWORD, UBYTE))
{
    /* Flash routine setup - handled internally by new core */
}

/* MONITOR system stubs for Mac GUI compatibility */
int MONITOR_assemblerMode = 0;
int MONITOR_break_active = 0;
int MONITOR_break_run_to_here = 0;

UWORD MONITOR_get_disasm_start(void)
{
    return 0x0600; /* Default disassembly start address */
}

void MONITOR_monitorCmd(char *cmd)
{
    /* Monitor command execution - handled by Mac GUI */
}

void MONITOR_monitorEnter(void)
{
    /* Monitor entry - handled by Mac GUI */
}

/* POKEY sound system variables */
int POKEYSND_serio_sound_enabled = 1;

/* SIDE2 cartridge system stubs - these are provided by the real side2.c */
/* Variables and simple stubs that don't conflict with headers */
int SIDE2_enabled = 0;
int SIDE2_have_rom = 1;
int SIDE2_SDX_Mode_Switch = 0;

/* SIDE2 filename variables - these need to be defined */
char side2_compact_flash_filename[FILENAME_MAX] = "";
char side2_nvram_filename[FILENAME_MAX] = "";
char side2_rom_filename[FILENAME_MAX] = "";

/* SIDE2 function implementations with correct signatures */
int SIDE2_Add_Block_Device(char *filename) { return FALSE; }
void SIDE2_Bank_Reset_Button_Change(void) {}
int SIDE2_Block_Device = 0; /* This is a variable, not a function */
int SIDE2_Change_Rom(char *filename, int new) { return FALSE; }
int SIDE2_Flash_Type = 0; /* This is a variable, not a function */
void SIDE2_Remove_Block_Device(void) {}
void SIDE2_SDX_Switch_Change(int state) {}
int SIDE2_Save_Rom(char *filename) { return FALSE; }

/* ULTIMATE 1MB cartridge system stubs */
int ULTIMATE_enabled = 0;
int ULTIMATE_have_rom = 0;

/* ULTIMATE filename variables - these need to be defined */
char ultimate_nvram_filename[FILENAME_MAX] = "";
char ultimate_rom_filename[FILENAME_MAX] = "";

/* ULTIMATE function implementations with correct signatures */
int ULTIMATE_Change_Rom(char *filename, int new) { return FALSE; }
UBYTE ULTIMATE_D1GetByte(UWORD addr, int no_side_effects) { return 0xFF; }
void ULTIMATE_D1PutByte(UWORD addr, UBYTE byte) {}
int ULTIMATE_D1ffPutByte(UBYTE byte) { return FALSE; }
UBYTE ULTIMATE_D6D7GetByte(UWORD addr, int no_side_effects) { return 0xFF; }
void ULTIMATE_D6D7PutByte(UWORD addr, UBYTE byte) {}
int ULTIMATE_Flash_Type = 0; /* This is a variable, not a function */
int ULTIMATE_Save_Rom(char *filename) { return FALSE; }

/* Screen display functions */
int Screen_show_capslock = 0;
int Screen_show_hd_sector_counter = 0;

/* UI system stubs - UI_BASIC_driver now provided by ui_basic.c */

/* System ROM functions */
int SYSROM_FindType(void) { return 0; }

/* Monitor symbol table support */
int symtable_builtin_enable = 1;
int symtable_user_size = 0;
void *symtable_user = NULL;

/* Monitor utility functions */
void add_user_label(char *name, UWORD addr) {}
void free_user_labels(void) {}
char *find_user_label(UWORD addr) { return NULL; }
void load_user_labels(char *filename) {}

int break_table_on = 0;
int machine_switch_type = 0;
double deltatime = 0.0;

void init_mzpokeysnd_sync(void) {}

void show_instruction_string(char *buffer, UWORD addr)
{
    sprintf(buffer, "NOP");
}

/* Symbol table structures for monitor compatibility */
typedef struct {
    char *name;
    UWORD addr;
} symtable_rec;

/* Built-in symbol tables for monitor/debugger */
const symtable_rec symtable_builtin[] = {
    {"RTCLOK", 0x0012},
    {"COLPM0", 0xD012},
    {"COLPM1", 0xD013},
    {"COLPM2", 0xD014},
    {"COLPM3", 0xD015},
    {NULL, 0}
};

const symtable_rec symtable_builtin_5200[] = {
    {"POKEY", 0xE800},
    {"GTIA", 0xC000},
    {NULL, 0}
};

/* Monitor GUI helper functions */
int get_hex_gui(char *string, UWORD *result)
{
    static char *next_token = NULL;
    char *current_string;
    int value = 0;
    int i;
    char c;
    
    if (string != NULL) {
        current_string = string;
        next_token = string;
    } else {
        if (next_token == NULL) return FALSE;
        current_string = next_token;
    }
    
    /* Skip whitespace */
    while (*current_string == ' ' || *current_string == '\t') {
        current_string++;
    }
    
    if (*current_string == '\0') return FALSE;
    
    /* Parse hex digits */
    for (i = 0; i < 4 && current_string[i]; i++) {
        c = current_string[i];
        if (c >= '0' && c <= '9') {
            value = (value << 4) + (c - '0');
        } else if (c >= 'A' && c <= 'F') {
            value = (value << 4) + (c - 'A' + 10);
        } else if (c >= 'a' && c <= 'f') {
            value = (value << 4) + (c - 'a' + 10);
        } else {
            break;
        }
    }
    
    if (i == 0) return FALSE; /* No digits found */
    
    *result = value;
    next_token = current_string + i;
    
    /* Skip whitespace for next token */
    while (*next_token == ' ' || *next_token == '\t') {
        next_token++;
    }
    
    return TRUE;
}

int get_val_gui(char *string, UWORD *result)
{
    if (string[0] == '$') {
        return get_hex_gui(string + 1, result);
    } else {
        *result = atoi(string);
        return (*result != 0 || string[0] == '0') ? TRUE : FALSE;
    }
}

/* Additional missing symbols for Mac compatibility */

/* Cartridge BountyBob support stubs */
UBYTE CARTRIDGE_BountyBob1[256];
UBYTE CARTRIDGE_BountyBob2[256];

/* Device system stubs - variable already defined earlier in file */
/* Devices_H_Init function is already defined at line 5883 */

/* Additional MONITOR system functions - removed duplicate symbols */

/* Additional ULTIMATE cartridge functions */
UBYTE ULTIMATE_D3GetByte(UWORD addr, int no_side_effects)
{
    return 0xFF; /* Return default value for unimplemented hardware */
}

void ULTIMATE_D3PutByte(UWORD addr, UBYTE byte)
{
    /* ULTIMATE D3 write - no action needed for stub */
}
