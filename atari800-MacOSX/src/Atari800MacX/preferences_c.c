/* preferences_c.c - Preferences 'c' 
 module  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimacosx@gmail.com>
 
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "af80.h"
#include "akey.h"
#include "atari.h"
#include "bit3.h"
#include "cfg.h"
#include "prompts.h"
#include "input.h"
#include "memory.h"
#include "preferences_c.h"
#include "SDL.h"
#include "sio.h"
#include "cassette.h"
#include "cartridge.h"
#include "xep80.h"
#include "xep80_fonts.h"
#include "esc.h"
#include "pokeysnd.h"
#include "rdevice.h"
#include "pbi_bb.h"
#include "pbi_mio.h"
#include "ultimate1mb.h"
#include "side2.h"
#include "binload.h"

#define MAX_DIRECTORIES 8

extern void ReturnPreferences(struct ATARI800MACX_PREFSAVE *prefssave);
extern void SaveMedia(char disk_filename[][FILENAME_MAX], 
			   char cassette_filename[FILENAME_MAX],
			   char cart_filename[FILENAME_MAX],
			   char cart2_filename[FILENAME_MAX]);
extern void PrintOutputControllerSelectPrinter(int printer);

extern char Devices_h_exe_path[FILENAME_MAX];
extern int useBuiltinPalette;
extern int adjustPalette;
extern int paletteBlack;
extern int paletteWhite;
extern int paletteIntensity;
extern int paletteColorShift;
extern int PLATFORM_80col;
extern int SCALE_MODE;
extern int onlyIntegralScaling;
extern int fixAspectFullscreen;
extern int scaleFactor;
extern double scaleFactorFloat;
extern int WIDTH_MODE;
extern int Screen_show_atari_speed;
extern int Screen_show_disk_led;
extern int Screen_show_hd_sector_counter;
extern int Screen_show_sector_counter;
extern int Screen_show_1200_leds;
extern int Screen_show_capslock;
extern int led_enabled_media;
extern int led_counter_enabled_media;
extern int JOYSTICK_MODE[4];
extern int JOYSTICK_AUTOFIRE[4];
extern int INPUT_mouse_mode;
extern int INPUT_mouse_speed;
extern int INPUT_mouse_pot_min;
extern int INPUT_mouse_pot_max;
extern int INPUT_mouse_pen_ofs_h;
extern int INPUT_mouse_pen_ofs_v;
extern int INPUT_mouse_joy_inertia;
extern int ANTIC_artif_mode;
extern int ANTIC_artif_new;
extern int sound_enabled;
extern double sound_volume;
extern int sound_flags;
extern int sound_bits;
extern int speed_limit;
extern int disable_all_basic;
extern int CASSETTE_hold_start_on_reboot;
extern int CASSETTE_hold_start;
extern int requestPrefsChange;
extern char paletteFilename[FILENAME_MAX];
extern int portnum;
extern int ignore_header_writeprotect;
extern int joystickButtonKey[4][24]; 
extern int joystick5200ButtonKey[4][24];
extern int joystick0TypePref, joystick1TypePref;
extern int joystick2TypePref, joystick3TypePref;
extern int joystick0Num, joystick1Num;
extern int joystick2Num, joystick3Num;
extern int paddlesXAxisOnly;
extern int keyjoyEnable;
extern int SDL_TRIG_0;
extern int SDL_TRIG_0_B;
extern int SDL_TRIG_0_R;
extern int SDL_TRIG_0_B_R;
extern int SDL_JOY_0_LEFT;
extern int SDL_JOY_0_RIGHT;
extern int SDL_JOY_0_DOWN;
extern int SDL_JOY_0_UP;
extern int SDL_JOY_0_LEFTUP;
extern int SDL_JOY_0_RIGHTUP;
extern int SDL_JOY_0_LEFTDOWN;
extern int SDL_JOY_0_RIGHTDOWN;
extern int SDL_TRIG_1;
extern int SDL_TRIG_1_B;
extern int SDL_TRIG_1_R;
extern int SDL_TRIG_1_B_R;
extern int SDL_JOY_1_LEFT;
extern int SDL_JOY_1_RIGHT;
extern int SDL_JOY_1_DOWN;
extern int SDL_JOY_1_UP;
extern int SDL_JOY_1_LEFTUP;
extern int SDL_JOY_1_RIGHTUP;
extern int SDL_JOY_1_LEFTDOWN;
extern int SDL_JOY_1_RIGHTDOWN;
extern int mediaStatusWindowOpen;
extern int functionKeysWindowOpen;
extern int machine_switch_type;
extern double emulationSpeed;
extern int cx85_port;
extern int useAtariCursorKeys;
extern int Atari800_useAlitrraXEGSRom;
extern int Atari800_useAlitrra1200XLRom;
extern int Atari800_useAlitrraOSBRom;
extern int Atari800_useAlitrraXLRom;
extern int Atari800_useAlitrra5200Rom;
extern int Atari800_useAlitrraBasicRom;

extern int Devices_enable_h_patch;
extern int Devices_enable_d_patch;
extern int Devices_enable_p_patch;
extern int Devices_enable_r_patch;
extern int POKEYSND_serio_sound_enabled;
extern int POKEYSND_console_sound_enabled;
extern char Devices_atari_h_dir[4][FILENAME_MAX];
extern int Devices_h_read_only;
extern char Devices_print_command[256];

extern char bb_rom_filename[FILENAME_MAX];
extern char mio_rom_filename[FILENAME_MAX];
extern char bb_scsi_disk_filename[FILENAME_MAX];
extern char mio_scsi_disk_filename[FILENAME_MAX];
extern char af80_rom_filename[FILENAME_MAX];
extern char af80_charset_filename[FILENAME_MAX];
extern char bit3_rom_filename[FILENAME_MAX];
extern char bit3_charset_filename[FILENAME_MAX];

extern void init_af80();
extern void init_bb();
extern void init_bit3();
extern void init_mio();

/* Variables brought over from RT_CONFIG.C */
char atari_disk_dirs[MAX_DIRECTORIES][FILENAME_MAX];
char atari_diskset_dir[FILENAME_MAX];
char atari_rom_dir[FILENAME_MAX];
char atari_image_dir[FILENAME_MAX];
char atari_print_dir[FILENAME_MAX];
char atari_exe_dir[FILENAME_MAX];
char atari_cass_dir[FILENAME_MAX];
char atari_state_dir[FILENAME_MAX];
char atari_config_dir[FILENAME_MAX];
int  refresh_rate;

int PREFS_axlon_num_banks;
int PREFS_mosaic_num_banks;

/* Variables used for boot devices */
int prefsArgc = 0;
char *prefsArgv[15];
/* Varibles used for printer */
int currPrinter;
/* Storage for current and former preferences */
static ATARI800MACX_PREF prefs, lastPrefs;
static ATARI800MACX_PREFSAVE prefssave;
/* Variables used to determine what has changed after a preferences update */
int displaySizeChanged;
int scaleModeChanged;
int showfpsChanged;
int artifChanged;
int paletteChanged;
int machineTypeChanged;
int patchFlagsChanged;
int keyboardJoystickChanged;
int hardDiskChanged;
int osRomsChanged;
int af80EnabledChanged;
int bit3EnabledChanged;
int xep80EnabledChanged;
int xep80ColorsChanged;
int configurationChanged;
int mioChanged;
int bbChanged;
int fullscreenOptsChanged;
int bbRequested = FALSE;
int mioRequested = FALSE;
int dontMuteAudio = 0;
int diskDriveSound = 1;

int saveCurrentMedia = 1;
int clearCurrentMedia = 0;

int CalcAtariType(int machineType, int ramSize, int axlon, int mosaic, int ultimate, int basic, int game, int leds, int jumper);

/* Arrays to translate menu indicies for gamepad keys to key values */
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

int joystickIndexToKey[] = 
    {AKEY_BUTTON1, AKEY_HELP, AKEY_BUTTON_RESET, AKEY_BUTTON_SELECT, 
     AKEY_BUTTON_OPTION, AKEY_BUTTON_START, AKEY_a, AKEY_b, AKEY_c,
     AKEY_d, AKEY_e, AKEY_f, AKEY_g, AKEY_h, AKEY_i, AKEY_j, AKEY_k,
     AKEY_l, AKEY_m, AKEY_n, AKEY_o, AKEY_p, AKEY_q, AKEY_r, AKEY_s, 
     AKEY_t, AKEY_u, AKEY_v, AKEY_w, AKEY_x, AKEY_y, AKEY_z, AKEY_SPACE,
     AKEY_ESCAPE, AKEY_0, AKEY_1, AKEY_2, AKEY_3, AKEY_4, AKEY_5, AKEY_6, AKEY_7,
     AKEY_8, AKEY_9, AKEY_LESS, AKEY_GREATER, AKEY_BACKSPACE, AKEY_BREAK,
     AKEY_TAB, AKEY_MINUS, AKEY_EQUAL, AKEY_RETURN, AKEY_CTRL, AKEY_COLON,
     AKEY_PLUS, AKEY_ASTERISK, AKEY_CAPSTOGGLE, AKEY_SHFT, AKEY_COMMA,
     AKEY_FULLSTOP, AKEY_SLASH, AKEY_ATARI, AKEY_JOYSTICK_UP, AKEY_JOYSTICK_DOWN,
     AKEY_JOYSTICK_LEFT, AKEY_JOYSTICK_RIGHT,AKEY_NONE};
int joystick5200IndexToKey[] = 
    {AKEY_BUTTON1, AKEY_BUTTON2, AKEY_5200_START, AKEY_5200_PAUSE,
     AKEY_5200_RESET, AKEY_5200_0, AKEY_5200_1, AKEY_5200_2, AKEY_5200_3,
     AKEY_5200_4, AKEY_5200_5, AKEY_5200_6, AKEY_5200_7, AKEY_5200_8,
     AKEY_5200_9, AKEY_5200_HASH, AKEY_5200_ASTERISK, AKEY_JOYSTICK_UP, 
     AKEY_JOYSTICK_DOWN, AKEY_JOYSTICK_LEFT, AKEY_JOYSTICK_RIGHT,AKEY_NONE};

/* Arrays to translate menu indicies for keyboard joystick keys to key values */
int keyboardStickIndexToKey[] =
    {SDL_SCANCODE_A, SDL_SCANCODE_B, SDL_SCANCODE_C, SDL_SCANCODE_D, SDL_SCANCODE_E, SDL_SCANCODE_F, SDL_SCANCODE_G, SDL_SCANCODE_H, SDL_SCANCODE_I, SDL_SCANCODE_J,
     SDL_SCANCODE_K, SDL_SCANCODE_L, SDL_SCANCODE_M, SDL_SCANCODE_N, SDL_SCANCODE_O, SDL_SCANCODE_P, SDL_SCANCODE_Q, SDL_SCANCODE_R, SDL_SCANCODE_S, SDL_SCANCODE_T,
     SDL_SCANCODE_U, SDL_SCANCODE_V, SDL_SCANCODE_W, SDL_SCANCODE_X, SDL_SCANCODE_Y, SDL_SCANCODE_Z, SDL_SCANCODE_GRAVE, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3,
     SDL_SCANCODE_4, SDL_SCANCODE_5, SDL_SCANCODE_6, SDL_SCANCODE_7, SDL_SCANCODE_8, SDL_SCANCODE_9, SDL_SCANCODE_0, SDL_SCANCODE_MINUS, SDL_SCANCODE_EQUALS,
     SDL_SCANCODE_BACKSPACE, SDL_SCANCODE_TAB, SDL_SCANCODE_LEFTBRACKET, SDL_SCANCODE_RIGHTBRACKET, SDL_SCANCODE_BACKSLASH,
     SDL_SCANCODE_RETURN, SDL_SCANCODE_PERIOD, SDL_SCANCODE_COMMA, SDL_SCANCODE_SLASH, SDL_SCANCODE_LSHIFT, SDL_SCANCODE_LCTRL, SDL_SCANCODE_LALT,
     SDL_SCANCODE_SPACE, SDL_SCANCODE_INSERT, SDL_SCANCODE_HOME, SDL_SCANCODE_PAGEUP, SDL_SCANCODE_DELETE, SDL_SCANCODE_END, SDL_SCANCODE_PAGEDOWN,
     SDL_SCANCODE_LEFT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_RIGHT, SDL_SCANCODE_KP_0, SDL_SCANCODE_KP_1, SDL_SCANCODE_KP_2,
     SDL_SCANCODE_KP_3, SDL_SCANCODE_KP_4, SDL_SCANCODE_KP_5, SDL_SCANCODE_KP_6, SDL_SCANCODE_KP_7, SDL_SCANCODE_KP_8, SDL_SCANCODE_KP_9, SDL_SCANCODE_KP_PERIOD,
     SDL_SCANCODE_KP_ENTER, SDL_SCANCODE_KP_PLUS, SDL_SCANCODE_KP_MINUS, SDL_SCANCODE_KP_MULTIPLY, SDL_SCANCODE_KP_DIVIDE,
	 SDL_SCANCODE_SEMICOLON, SDL_SCANCODE_APOSTROPHE
     };

ATARI800MACX_PREF *getPrefStorage(void) {
    int static firstTime = TRUE;
    
    if (firstTime) {
        firstTime = FALSE;
        return(&prefs);
        }
    else {
        lastPrefs = prefs;
        return(&prefs);
        }
    }

void saveMediaPrefs() {
    if (CARTRIDGE_main.type == CARTRIDGE_NONE) {
        if (CARTRIDGE_piggyback.type == CARTRIDGE_NONE)
			SaveMedia(SIO_filename, CASSETTE_filename, "", "");
		else
            SaveMedia(SIO_filename, CASSETTE_filename, "", CARTRIDGE_piggyback.filename);
	}
	else {
		if (CARTRIDGE_piggyback.type == CARTRIDGE_NONE)
			SaveMedia(SIO_filename, CASSETTE_filename, CARTRIDGE_main.filename, "");
		else
			SaveMedia(SIO_filename, CASSETTE_filename, CARTRIDGE_main.filename, CARTRIDGE_piggyback.filename);
	}
}

void savePrefs() {
    prefssave.scaleFactor = scaleFactor;
    prefssave.scaleFactorFloat = scaleFactorFloat;
    prefssave.widthMode = WIDTH_MODE;
	prefssave.scaleMode = SCALE_MODE;
    prefssave.showFPS = Screen_show_atari_speed;
	prefssave.ledStatus = Screen_show_disk_led;
	prefssave.ledSector = Screen_show_sector_counter;
    prefssave.speedLimit = speed_limit; 
	prefssave.xep80 = PLATFORM_80col;
    prefssave.af80_enabled = AF80_enabled;
    prefssave.bit3_enabled = BIT3_enabled;
    prefssave.xep80_enabled = XEP80_enabled;
	prefssave.xep80_port = XEP80_port;
    prefssave.COL80_autoswitch = COL80_autoswitch;
    prefssave.enableSound = sound_enabled;
	prefssave.soundVolume = sound_volume;
    prefssave.enableStereo = POKEYSND_stereo_enabled; 
	prefssave.mediaStatusDisplayed = mediaStatusWindowOpen;
	prefssave.functionKeysDisplayed = functionKeysWindowOpen;
	prefssave.disableBasic = Atari800_disable_basic;
	prefssave.bootFromCassette = CASSETTE_hold_start;
	prefssave.enableSioPatch = ESC_enable_sio_patch;
	prefssave.enableHPatch = Devices_enable_h_patch;
	prefssave.enableDPatch = Devices_enable_d_patch;
	prefssave.hardDrivesReadOnly = Devices_h_read_only;
	prefssave.enablePPatch = Devices_enable_p_patch;
	prefssave.enableRPatch = Devices_enable_r_patch;
		

    prefssave.atariType = CalcAtariType(Atari800_machine_type, MEMORY_ram_size,
										MEMORY_axlon_num_banks > 0, MEMORY_mosaic_num_banks > 0,
                                        ULTIMATE_enabled,
                                        Atari800_builtin_basic,
                                        Atari800_builtin_game,
                                        Atari800_keyboard_leds,
                                        Atari800_jumper_present);

	prefssave.currPrinter = currPrinter;
	prefssave.artifactingMode = ANTIC_artif_mode;
	prefssave.keyjoyEnable = keyjoyEnable;
    prefssave.cx85Enable = INPUT_cx85;
	prefssave.blackBoxEnabled = bbRequested;
	prefssave.mioEnabled = mioRequested;
	prefssave.useAtariCursorKeys = useAtariCursorKeys;
    prefssave.a1200xlJumper = Atari800_jumper;
    prefssave.xegsKeyboard = !Atari800_keyboard_detached;
    prefssave.side2SDXMode = SIDE2_SDX_Mode_Switch;
    strcpy(prefssave.side2FlashFileName, side2_rom_filename);
    //strcpy(prefssave.side2NVRAMFileName, side2_nvram_filename);
    strcpy(prefssave.side2CFFileName, side2_compact_flash_filename);

	ReturnPreferences(&prefssave);
	if (saveCurrentMedia)
        {
        if (CARTRIDGE_main.type == CARTRIDGE_NONE) {
            if (CARTRIDGE_piggyback.type == CARTRIDGE_NONE)
				SaveMedia(SIO_filename, CASSETTE_filename, "", "");
			else
                SaveMedia(SIO_filename, CASSETTE_filename, "", CARTRIDGE_piggyback.filename);
		}
		else {
			if (CARTRIDGE_piggyback.type == CARTRIDGE_NONE) 
				SaveMedia(SIO_filename, CASSETTE_filename, CARTRIDGE_main.filename, "");
			else
				SaveMedia(SIO_filename, CASSETTE_filename, CARTRIDGE_main.filename, CARTRIDGE_piggyback.filename);
		}
    }
	if (saveCurrentMedia)
		saveMediaPrefs(); 
}
	

void commitPrefs() {
    int i;
    
    prefsArgv[prefsArgc++] = "Atari800MacX";
    if (prefs.cartFileEnabled) {
        prefsArgv[prefsArgc++] = "-cart";
        prefsArgv[prefsArgc++] = prefs.cartFile;
	}
    if (prefs.cart2FileEnabled) {
        prefsArgv[prefsArgc++] = "-cart2";
        prefsArgv[prefsArgc++] = prefs.cart2File;
	}
    if (prefs.exeFileEnabled) {
        prefsArgv[prefsArgc++] = "-run";
        prefsArgv[prefsArgc++] = prefs.exeFile;
	}
    if (prefs.cassFileEnabled) {
        if (prefs.bootFromCassette)
            prefsArgv[prefsArgc++] = "-boottape";
        else
            prefsArgv[prefsArgc++] = "-tape";
        prefsArgv[prefsArgc++] = prefs.cassFile;
	}
    for (i=0;i<8;i++) {
        if (prefs.dFileEnabled[i]) {
            prefsArgv[prefsArgc++] = prefs.dFile[i];
		}
	}
}

void loadPrefsBinaries() {
	int i;
	
    if (prefs.cartFileEnabled) {
        if (strcmp(prefs.cartFile, "SIDE2") == 0)
            CARTRIDGE_Insert_SIDE2();
        else if (strcmp(prefs.cartFile, "BASIC") == 0)
            CARTRIDGE_Insert_BASIC();
        else if (strcmp(prefs.cartFile, "ULTIMATE-SDX") != 0)
            CARTRIDGE_Insert(prefs.cartFile);
	}
    if (prefs.cart2FileEnabled) {
        if (strcmp(prefs.cart2File, "SIDE2") == 0)
            CARTRIDGE_Insert_SIDE2();
        else
            CARTRIDGE_Insert_Second(prefs.cart2File);
	}
    if (prefs.exeFileEnabled) {
        BINLOAD_Loader(prefs.exeFile);
	}
    if (prefs.cassFileEnabled) {
        CASSETTE_Insert(prefs.cassFile);
	}
    for (i=0;i<8;i++) {
        if (prefs.dFileEnabled[i]) {
            SIO_Mount(i+1,prefs.dFile[i],FALSE);
		}
	}
}

/**************************************************************
*  Prior to version 4.0, the mapping of Atari Type to 
*  Machine Type and RAM is as follows:
*    0: Atari800_MACHINE_OSA, 16
*    1: Atari800_MACHINE_OSA, 48
*    2: Atari800_MACHINE_OSA, 52
*    3: Atari800_MACHINE_OSB, 16
*    4: Atari800_MACHINE_OSB, 48
*    5: Atari800_MACHINE_OSB, 52
*    6: Atari800_MACHINE_XLXE, 16
*    7: Atari800_MACHINE_XLXE, 64
*    8: Atari800_MACHINE_XLXE, 128
*    9: Atari800_MACHINE_XLXE, 320_RAMBO
*   10: Atari800_MACHINE_XLXE, 320_COMPY_SHOP
*   11: Atari800_MACHINE_XLXE, 576
*   12: Atari800_MACHINE_XLXE, 1088
*   13: Atari800_MACHINE_5200, 16
* Version 4.0+ adds the following types:
*   14: Atari800_MACHINE_OSA, Axlon Ram Expansion
*   15: Atari800_MACHINE_OSA, Mosaic Ram Expansion
*   16: Atari800_MACHINE_OSB, Axlon Ram Expansion
*   17: Atari800_MACHINE_OSB, Mosaic Ram Expansion
*   18: Atari800_MACHINE_XLXE, 192
* Version 5.4+ adds the following types:
*   19: Atari800_MACHINE_XLXE, 1200XL 64
*   20: Atari800_MACHINE_XLXE, XEGS, 64
*   21: Atari800_MACHINE_XLXE XL Ultimate 1MB
*   22: Atari800_MACHINE_XLXE XEGS Ultimate 1MB
**************************************************************/

int CalcAtariType(int machineType, int ramSize, int axlon, int mosaic, int ultimate, int basic, int game, int leds, int jumper)
{
	if (machineType == Atari800_MACHINE_800) {
		if (axlon) {
			if (ramSize == 48)
				return 16;
			else
				return 0;
		} else if (mosaic) {
			if (ramSize == 48)
				return 17;
			else
				return 0;
		} else {
			if (ramSize == 16)
				return 3;
			else if (ramSize == 48)
				return 4;
			else if (ramSize == 52)
				return 5;
			else
				return 0;
		}
	}
	else if (machineType == Atari800_MACHINE_XLXE) {
        if (ultimate) {
            if (game)
                return 22;
            else
                return 21;
        }
        if (game)
            return 20;
        if (leds)
            return 19;
		if (ramSize == 16)
			return 6;
		else if (ramSize == 64)
			return 7;
		else if (ramSize == 128)
			return 8;
		else if (ramSize == 192)
			return 18;
		else if (ramSize == MEMORY_RAM_320_RAMBO)
			return 9;
		else if (ramSize == MEMORY_RAM_320_COMPY_SHOP)
			return 10;
		else if (ramSize == 576)
			return 11;
		else if (ramSize == 1088)
			return 12;
		else
			return 0;
	}
	else if (machineType == Atari800_MACHINE_5200) {
		if (ramSize == 16)
			return(13);
		else
			return(0);
	}
	else
		return 0;
}

void CalcMachineTypeRam(int type, int *machineType, int *ramSize,
						int *axlon, int *mosaic, int *ultimate, int *basic, int *game, int *leds, int *jumper)
{
    switch(type) {
        case 0:
            *machineType = Atari800_MACHINE_800;
            *ramSize = 16;
			*axlon = FALSE;
			*mosaic = FALSE;
            *ultimate = FALSE;
            *basic = FALSE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 1:
            *machineType = Atari800_MACHINE_800;
            *ramSize = 48;
			*axlon = FALSE;
			*mosaic = FALSE;
            *ultimate = FALSE;
            *basic = FALSE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 2:
            *machineType = Atari800_MACHINE_800;
            *ramSize = 52;
			*axlon = FALSE;
			*mosaic = FALSE;
            *ultimate = FALSE;
            *basic = FALSE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 3:
            *machineType = Atari800_MACHINE_800;
            *ramSize = 16;
			*axlon = FALSE;
			*mosaic = FALSE;
            *ultimate = FALSE;
            *basic = FALSE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 4:
            *machineType = Atari800_MACHINE_800;
            *ramSize = 48;
			*axlon = FALSE;
			*mosaic = FALSE;
            *ultimate = FALSE;
            *basic = FALSE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 5:
            *machineType = Atari800_MACHINE_800;
            *ramSize = 52;
			*axlon = FALSE;
			*mosaic = FALSE;
            *ultimate = FALSE;
            *basic = FALSE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 6:
            *machineType = Atari800_MACHINE_XLXE;
            *ramSize = 16;
			*axlon = FALSE;
			*mosaic = FALSE;
            *ultimate = FALSE;
            *basic = TRUE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 7:
            *machineType = Atari800_MACHINE_XLXE;
            *ramSize = 64;
			*axlon = FALSE;
			*mosaic = FALSE;
            *ultimate = FALSE;
            *basic = TRUE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 8:
            *machineType = Atari800_MACHINE_XLXE;
            *ramSize = 128;
			*axlon = FALSE;
			*mosaic = FALSE;
            *ultimate = FALSE;
            *basic = TRUE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 9:
            *machineType = Atari800_MACHINE_XLXE;
            *ramSize = MEMORY_RAM_320_RAMBO;
			*axlon = FALSE;
			*mosaic = FALSE;
            *ultimate = FALSE;
            *basic = TRUE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 10:
            *machineType = Atari800_MACHINE_XLXE;
            *ramSize = MEMORY_RAM_320_COMPY_SHOP;
			*axlon = FALSE;
			*mosaic = FALSE;
            *ultimate = FALSE;
            *basic = TRUE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 11:
            *machineType = Atari800_MACHINE_XLXE;
            *ramSize = 576;
			*axlon = FALSE;
			*mosaic = FALSE;
            *ultimate = FALSE;
            *basic = TRUE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 12:
            *machineType = Atari800_MACHINE_XLXE;
            *ramSize = 1088;
			*axlon = FALSE;
			*mosaic = FALSE;
            *ultimate = FALSE;
            *basic = TRUE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 13:
            *machineType = Atari800_MACHINE_5200;
            *ramSize = 16;
			*axlon = FALSE;
			*mosaic = FALSE;
            *ultimate = FALSE;
            *basic = FALSE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 14:
            *machineType = Atari800_MACHINE_800;
            *ramSize = 48;
			*axlon = TRUE;
			*mosaic = FALSE;
            *ultimate = FALSE;
            *basic = FALSE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 15:
            *machineType = Atari800_MACHINE_800;
            *ramSize = 48;
			*axlon = FALSE;
			*mosaic = TRUE;
            *ultimate = FALSE;
            *basic = FALSE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 16:
            *machineType = Atari800_MACHINE_800;
            *ramSize = 48;
			*axlon = TRUE;
			*mosaic = FALSE;
            *ultimate = FALSE;
            *basic = FALSE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 17:
            *machineType = Atari800_MACHINE_800;
            *ramSize = 48;
			*axlon = FALSE;
			*mosaic = TRUE;
            *ultimate = FALSE;
            *basic = FALSE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 18:
            *machineType = Atari800_MACHINE_XLXE;
            *ramSize = 192;
            *axlon = FALSE;
            *mosaic = FALSE;
            *ultimate = FALSE;
            *basic = TRUE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 19: // 1200XL
            *machineType = Atari800_MACHINE_XLXE;
            *ramSize = 64;
            *axlon = FALSE;
            *mosaic = FALSE;
            *ultimate = FALSE;
            *basic = FALSE;
            *game = FALSE;
            *leds = TRUE;
            *jumper = TRUE;
            break;
        case 20: // XEGS
            *machineType = Atari800_MACHINE_XLXE;
            *ramSize = 64;
            *axlon = FALSE;
            *mosaic = FALSE;
            *ultimate = FALSE;
            *basic = TRUE;
            *game = TRUE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 21: // Ultimate XL
            *machineType = Atari800_MACHINE_XLXE;
            *ramSize = 1088;
            *axlon = FALSE;
            *mosaic = FALSE;
            *ultimate = TRUE;
            *basic = TRUE;
            *game = FALSE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
        case 22: // Ultimate XEGS
            *machineType = Atari800_MACHINE_XLXE;
            *ramSize = 1088;
            *axlon = FALSE;
            *mosaic = FALSE;
            *ultimate = TRUE;
            *basic = TRUE;
            *game = TRUE;
            *leds = FALSE;
            *jumper = FALSE;
            break;
	}

}

void CalculatePrefsChanged()
{
    int new_machine_type = 0;
    int new_ram_size = 0;
	int new_axlon = 0;
	int new_mosaic = 0;
    int new_ultimate = 0;
    int new_basic = 0;
    int new_game = 0;
    int new_leds = 0;
    int new_jumper = 0;
 
    if (WIDTH_MODE != prefs.widthMode)
        displaySizeChanged = TRUE;
    else
        displaySizeChanged = FALSE;
		
    if (Screen_show_atari_speed != prefs.showFPS)
        showfpsChanged = TRUE;
    else
        showfpsChanged = FALSE;
        
    if (strcmp(prefs.paletteFile, paletteFilename) == 0 &&
        (useBuiltinPalette == prefs.useBuiltinPalette) &&
        (adjustPalette == prefs.adjustPalette) &&
        (paletteBlack == prefs.blackLevel) &&
        (paletteWhite == prefs.whiteLevel) &&
        (paletteIntensity == prefs.intensity) &&
        (paletteColorShift == prefs.colorShift)) 
        paletteChanged = FALSE;
    else
        paletteChanged = TRUE;

    if (ANTIC_artif_mode != prefs.artifactingMode ||
	    ANTIC_artif_new != prefs.artifactNew)
        artifChanged = TRUE;
    else
        artifChanged = FALSE;
		
	if (SCALE_MODE != prefs.scaleMode)
		scaleModeChanged = TRUE;
	else
		scaleModeChanged = FALSE;
    strcpy(bb_rom_filename, prefs.blackBoxRomFile);
	strcpy(mio_rom_filename, prefs.mioRomFile);
	strcpy(bb_scsi_disk_filename, prefs.blackBoxScsiDiskFile);
	strcpy(mio_scsi_disk_filename, prefs.mioScsiDiskFile);
    strcpy(ultimate_rom_filename, prefs.ultimate1MBFlashFileName);
    strcpy(ultimate_nvram_filename, prefs.ultimate1MBNVRAMFileName);
    strcpy(side2_rom_filename, prefs.side2FlashFileName);
    strcpy(side2_nvram_filename, prefs.side2NVRAMFileName);
    strcpy(side2_compact_flash_filename, prefs.side2CFFileName);
    strcpy(af80_rom_filename, prefs.af80RomFile);
    strcpy(af80_charset_filename, prefs.af80CharsetFile);
    strcpy(bit3_rom_filename, prefs.bit3RomFile);
    strcpy(bit3_charset_filename, prefs.bit3CharsetFile);

    if (ULTIMATE_enabled) {
        CalcMachineTypeRam(prefs.atariType, &new_machine_type,
                           &new_ram_size, &new_axlon,
                           &new_mosaic, &new_ultimate,
                           &new_basic, &new_game,
                           &new_leds, &new_jumper);
        if (ULTIMATE_enabled != new_ultimate)
            machineTypeChanged = TRUE;
        else
            machineTypeChanged = FALSE;
        
    } else {
        CalcMachineTypeRam(prefs.atariType, &new_machine_type,
                           &new_ram_size, &new_axlon,
                           &new_mosaic, &new_ultimate,
                           &new_basic, &new_game,
                           &new_leds, &new_jumper);
        if ((Atari800_machine_type != new_machine_type) ||
            (MEMORY_ram_size != new_ram_size) ||
            ((MEMORY_axlon_num_banks > 0) != new_axlon) ||
            ((MEMORY_mosaic_num_banks > 0) != new_mosaic) ||
            ((MEMORY_axlon_num_banks - 1) != prefs.axlonBankMask) ||
            ((MEMORY_mosaic_num_banks - 1) != prefs.mosaicMaxBank) ||
            (Atari800_builtin_basic != new_basic) ||
            (Atari800_builtin_game != new_game) ||
            (Atari800_keyboard_leds != new_leds) ||
            (Atari800_jumper_present != new_jumper) ||
            (bbRequested != prefs.blackBoxEnabled) ||
            (mioRequested != prefs.mioEnabled) ||
            (ULTIMATE_enabled != new_ultimate))
            machineTypeChanged = TRUE;
        else
            machineTypeChanged = FALSE;
    }
    
    if ((Atari800_disable_basic != prefs.disableBasic) ||
        (disable_all_basic != prefs.disableAllBasic) ||
        (ESC_enable_sio_patch != prefs.enableSioPatch) ||
        (Devices_enable_h_patch != prefs.enableHPatch) ||
        (Devices_enable_d_patch != prefs.enableDPatch) ||
        (Devices_enable_p_patch != prefs.enablePPatch) ||
        (Devices_enable_r_patch != prefs.enableRPatch) ||
        (CASSETTE_hold_start_on_reboot != prefs.bootFromCassette))
        patchFlagsChanged = TRUE;
    else
        patchFlagsChanged = FALSE;
        
    if ((SDL_TRIG_1 != keyboardStickIndexToKey[prefs.leftJoyFire]) ||
        (SDL_TRIG_1_B != keyboardStickIndexToKey[prefs.leftJoyAltFire]) ||
        (SDL_JOY_1_LEFT != keyboardStickIndexToKey[prefs.leftJoyLeft]) ||
        (SDL_JOY_1_RIGHT != keyboardStickIndexToKey[prefs.leftJoyRight]) ||
        (SDL_JOY_1_DOWN != keyboardStickIndexToKey[prefs.leftJoyDown]) ||
        (SDL_JOY_1_UP != keyboardStickIndexToKey[prefs.leftJoyUp]) ||
        (SDL_JOY_1_LEFTUP != keyboardStickIndexToKey[prefs.leftJoyUpLeft]) ||
        (SDL_JOY_1_RIGHTUP != keyboardStickIndexToKey[prefs.leftJoyUpRight]) ||
        (SDL_JOY_1_LEFTDOWN != keyboardStickIndexToKey[prefs.leftJoyDownLeft]) ||
        (SDL_JOY_1_RIGHTDOWN != keyboardStickIndexToKey[prefs.leftJoyDownRight]) ||
        (SDL_TRIG_0 != keyboardStickIndexToKey[prefs.padJoyFire]) ||
        (SDL_TRIG_0_B != keyboardStickIndexToKey[prefs.padJoyAltFire]) ||
        (SDL_JOY_0_LEFT != keyboardStickIndexToKey[prefs.padJoyLeft]) ||
        (SDL_JOY_0_RIGHT != keyboardStickIndexToKey[prefs.padJoyRight]) ||
        (SDL_JOY_0_DOWN != keyboardStickIndexToKey[prefs.padJoyDown]) ||
        (SDL_JOY_0_UP != keyboardStickIndexToKey[prefs.padJoyUp]) ||
        (SDL_JOY_0_LEFTUP != keyboardStickIndexToKey[prefs.padJoyUpLeft]) ||
        (SDL_JOY_0_RIGHTUP != keyboardStickIndexToKey[prefs.padJoyUpRight]) ||
        (SDL_JOY_0_LEFTDOWN != keyboardStickIndexToKey[prefs.padJoyDownLeft]) ||
        (SDL_JOY_0_RIGHTDOWN != keyboardStickIndexToKey[prefs.padJoyDownRight]))
        keyboardJoystickChanged = TRUE;
    else
        keyboardJoystickChanged = FALSE;
        
    if ((strcmp(Devices_atari_h_dir[0], prefs.hardDiskDir[0]) != 0) ||
        (strcmp(Devices_atari_h_dir[1], prefs.hardDiskDir[1]) != 0) ||
        (strcmp(Devices_atari_h_dir[2], prefs.hardDiskDir[2]) != 0) ||
        (strcmp(Devices_atari_h_dir[3], prefs.hardDiskDir[3]) != 0) ||
        (strcmp(Devices_h_exe_path, prefs.hPath) != 0))
        hardDiskChanged = TRUE;
    else
        hardDiskChanged = FALSE;

    if ((strcmp(CFG_osb_filename, prefs.osBRomFile) !=0) ||
        (strcmp(CFG_xegs_filename, prefs.xegsRomFile) !=0) ||
        (strcmp(CFG_xegsGame_filename, prefs.xegsGameRomFile) !=0) ||
        (strcmp(CFG_1200xl_filename, prefs.a1200XLRomFile) !=0) ||
		(strcmp(CFG_xlxe_filename, prefs.xlRomFile) !=0) ||
		(strcmp(CFG_basic_filename, prefs.basicRomFile) !=0) ||
		(strcmp(CFG_5200_filename, prefs.a5200RomFile) !=0) ||
        (strcmp(af80_rom_filename, prefs.af80RomFile) != 0) ||
        (strcmp(af80_charset_filename, prefs.af80CharsetFile) != 0) ||
        (strcmp(bit3_rom_filename, prefs.bit3RomFile) != 0) ||
        (strcmp(bit3_charset_filename, prefs.bit3CharsetFile) != 0) ||
        (strcmp(bb_rom_filename, prefs.blackBoxRomFile) != 0) ||
        (strcmp(bb_rom_filename, prefs.blackBoxRomFile) != 0) ||
		(strcmp(mio_rom_filename, prefs.mioRomFile) != 0) ||
		(strcmp(bb_scsi_disk_filename, prefs.blackBoxScsiDiskFile) != 0) ||
		(strcmp(mio_scsi_disk_filename, prefs.mioScsiDiskFile) != 0) ||
        (Atari800_useAlitrraXEGSRom != prefs.useAltirraXEGSRom) ||
        (Atari800_useAlitrra1200XLRom != prefs.useAltirra1200XLRom) ||
        (Atari800_useAlitrraOSBRom != prefs.useAltirraOSBRom) ||
        (Atari800_useAlitrraXLRom != prefs.useAltirraXLRom) ||
        (Atari800_useAlitrraBasicRom != prefs.useAltirraBasicRom) ||
        (Atari800_useAlitrra5200Rom != prefs.useAltirra5200Rom))
		osRomsChanged = TRUE;
	else
		osRomsChanged = FALSE;

#if 0 /* enableHifiSound is deprecated from 4.2.2 on */    
    if (POKEYSND_enable_new_pokey != prefs.enableHifiSound)
        hifiSoundChanged = TRUE;
    else
        hifiSoundChanged = FALSE;
#endif		

	if (currPrinter != prefs.currPrinter)
		PrintOutputControllerSelectPrinter(prefs.currPrinter);
	
    if (XEP80_enabled != prefs.xep80_enabled)
        xep80EnabledChanged = TRUE;
    else
        xep80EnabledChanged = FALSE;
        
    if (BIT3_enabled != prefs.bit3_enabled)
        bit3EnabledChanged = TRUE;
    else
        bit3EnabledChanged = FALSE;
        
    if (AF80_enabled != prefs.af80_enabled)
        af80EnabledChanged = TRUE;
    else
        af80EnabledChanged = FALSE;
        
	if ((XEP80_FONTS_oncolor != prefs.xep80_oncolor) || (XEP80_FONTS_offcolor != prefs.xep80_offcolor))
		xep80ColorsChanged = TRUE;
	else
		xep80ColorsChanged = FALSE;
    
    if ((onlyIntegralScaling != prefs.onlyIntegralScaling) ||
        (fixAspectFullscreen != prefs.fixAspectFullscreen))
        fullscreenOptsChanged = TRUE;
    else
        fullscreenOptsChanged = FALSE;
}

int loadMacPrefs(int firstTime)
{
    int i,j;
    printf("-----------------------------------\n");
    int axlon_enabled;
    int mosaic_enabled;

    Atari800_collisions_in_skipped_frames = prefs.spriteCollisions;
    scaleFactor = prefs.scaleFactor;
    scaleFactorFloat = prefs.scaleFactorFloat;
	SCALE_MODE = prefs.scaleMode; 
    WIDTH_MODE = prefs.widthMode; 
    Screen_show_atari_speed = prefs.showFPS;
    onlyIntegralScaling = prefs.onlyIntegralScaling;
    fixAspectFullscreen = prefs.fixAspectFullscreen;
	Screen_show_disk_led = prefs.ledStatus;
    Screen_show_sector_counter = prefs.ledSector;
    Screen_show_1200_leds = prefs.ledFKeys;
    Screen_show_capslock = prefs.ledCapsLock;
    Screen_show_hd_sector_counter = prefs.ledHDSector;
	led_enabled_media = prefs.ledStatusMedia;
	led_counter_enabled_media = prefs.ledSectorMedia;

    if (prefs.tvMode == 0) {
        Atari800_tv_mode = Atari800_TV_NTSC;
        }
    else {
        Atari800_tv_mode = Atari800_TV_PAL;
        }
	emulationSpeed = prefs.emulationSpeed;
    refresh_rate = prefs.refreshRatio;
    ANTIC_artif_mode = prefs.artifactingMode;
	ANTIC_artif_new = prefs.artifactNew;
    useBuiltinPalette = prefs.useBuiltinPalette;
    adjustPalette = prefs.adjustPalette;
    paletteBlack = prefs.blackLevel;
    paletteWhite = prefs.whiteLevel;
    paletteIntensity = prefs.intensity;
    paletteColorShift = prefs.colorShift; 
    strcpy(paletteFilename, prefs.paletteFile);
    CalcMachineTypeRam(prefs.atariType, &Atari800_machine_type,
                       &MEMORY_ram_size, &axlon_enabled,
                       &mosaic_enabled, &ULTIMATE_enabled,
                       &Atari800_builtin_basic, &Atari800_builtin_game,
                       &Atari800_keyboard_leds, &Atari800_jumper_present);
    if (axlon_enabled) {
        MEMORY_axlon_num_banks = prefs.axlonBankMask + 1;
        PREFS_axlon_num_banks = MEMORY_axlon_num_banks;
        }
    else
        MEMORY_axlon_num_banks = 0;
    if (mosaic_enabled) {
        MEMORY_mosaic_num_banks = prefs.mosaicMaxBank + 1;
        PREFS_mosaic_num_banks = MEMORY_mosaic_num_banks;
    }
    else
        MEMORY_mosaic_num_banks = 0;

    strcpy(af80_rom_filename, prefs.af80RomFile);
    strcpy(af80_charset_filename, prefs.af80CharsetFile);
    strcpy(bit3_rom_filename, prefs.bit3RomFile);
    strcpy(ultimate_rom_filename, prefs.ultimate1MBFlashFileName);
    strcpy(ultimate_nvram_filename, prefs.ultimate1MBNVRAMFileName);
    strcpy(side2_rom_filename, prefs.side2FlashFileName);
    strcpy(side2_nvram_filename, prefs.side2NVRAMFileName);
    strcpy(side2_compact_flash_filename, prefs.side2CFFileName);
    strcpy(bit3_charset_filename, prefs.bit3CharsetFile);
    SIDE2_SDX_Mode_Switch = prefs.side2SDXMode;
    SIDE2_Flash_Type = prefs.side2UltimateFlashType;
    ULTIMATE_Flash_Type = prefs.side2UltimateFlashType;
    strcpy(bb_rom_filename, prefs.blackBoxRomFile);
	strcpy(mio_rom_filename, prefs.mioRomFile);
	strcpy(bb_scsi_disk_filename, prefs.blackBoxScsiDiskFile);
	strcpy(mio_scsi_disk_filename, prefs.mioScsiDiskFile);
	
	if (bbRequested != prefs.blackBoxEnabled && prefs.blackBoxEnabled) {
		if (!PBI_BB_enabled) {
			init_bb();
		}
	} else if (prefs.blackBoxEnabled && 
			   ((strcmp(bb_rom_filename, prefs.blackBoxRomFile) != 0) ||
				(strcmp(bb_scsi_disk_filename, prefs.blackBoxScsiDiskFile) != 0))) {
		init_bb();
    }
    if (prefs.af80_enabled &&
               ((strcmp(af80_rom_filename, prefs.af80RomFile) != 0) ||
                (strcmp(af80_charset_filename, prefs.af80CharsetFile) != 0))) {
        init_af80();
    }
    if (prefs.bit3_enabled &&
               ((strcmp(bit3_rom_filename, prefs.af80RomFile) != 0) ||
                (strcmp(bit3_charset_filename, prefs.bit3CharsetFile) != 0))) {
        init_bit3();
    }
	if (mioRequested != prefs.mioEnabled && prefs.mioEnabled) {
		if (!PBI_MIO_enabled) {
			init_mio();
		}
	} else if (prefs.mioEnabled && 
			   ((strcmp(mio_rom_filename, prefs.mioRomFile) != 0) ||
				(strcmp(mio_scsi_disk_filename, prefs.mioScsiDiskFile) != 0))) {
		init_mio();
	}
	bbRequested = prefs.blackBoxEnabled;
	mioRequested = prefs.mioEnabled;
	if (bbRequested) {
		PBI_MIO_enabled = FALSE;
	} else if (mioRequested) {
		PBI_BB_enabled = FALSE;
	} else {
		PBI_BB_enabled = FALSE;
		PBI_MIO_enabled = FALSE;
	}

	machine_switch_type = prefs.atariSwitchType; 
	disable_all_basic = prefs.disableAllBasic;
    Atari800_disable_basic = prefs.disableBasic;
    ESC_enable_sio_patch = prefs.enableSioPatch;
    Devices_enable_h_patch = prefs.enableHPatch;
	Devices_enable_d_patch = prefs.enableDPatch;
    Devices_enable_p_patch = prefs.enablePPatch;
    Devices_enable_r_patch = prefs.enableRPatch;
	RDevice_serial_enabled = prefs.rPatchSerialEnabled;
	strncpy(RDevice_serial_device, prefs.rPatchSerialPort,FILENAME_MAX);
    portnum = prefs.rPatchPort;
    strncpy(Devices_print_command, prefs.printCommand,256);
    PLATFORM_80col = prefs.xep80;
    XEP80_enabled = prefs.xep80_enabled;
    AF80_enabled = prefs.af80_enabled;
    BIT3_enabled = prefs.bit3_enabled;
    if (XEP80_enabled && AF80_enabled)
        AF80_enabled = FALSE;
    if (XEP80_enabled && BIT3_enabled)
        BIT3_enabled = FALSE;
    if (AF80_enabled && BIT3_enabled)
        BIT3_enabled = FALSE;
    COL80_autoswitch = prefs.COL80_autoswitch;
	XEP80_port = prefs.xep80_port;
	XEP80_FONTS_oncolor = prefs.xep80_oncolor;
	XEP80_FONTS_offcolor = prefs.xep80_offcolor;
    Atari800_jumper = prefs.a1200xlJumper;
    Atari800_keyboard_detached = !prefs.xegsKeyboard;
    sound_enabled = prefs.enableSound;
	sound_volume = prefs.soundVolume;
    POKEYSND_stereo_enabled = prefs.enableStereo;
    POKEYSND_console_sound_enabled = prefs.enableConsoleSound;
    POKEYSND_serio_sound_enabled = prefs.enableSerioSound;
    dontMuteAudio = prefs.dontMuteAudio;
    diskDriveSound = prefs.diskDriveSound;
#if 0 /* enableHifiSound is deprecated from 4.2.2 on */
    POKEYSND_enable_new_pokey = prefs.enableHifiSound;
#endif	
	if (firstTime) {
		if (prefs.enable16BitSound) {
			sound_bits = 16;
			sound_flags = POKEYSND_BIT16;
		} else {
			sound_bits = 8;
			sound_flags = 0;
		}
	}
    ignore_header_writeprotect = prefs.ignoreHeaderWriteprotect;
    speed_limit = prefs.speedLimit;
    CASSETTE_hold_start_on_reboot = prefs.bootFromCassette;
    CASSETTE_hold_start = prefs.bootFromCassette;
    strcpy(atari_image_dir, prefs.imageDir);
    strcpy(atari_print_dir, prefs.printDir);
    strcpy(Devices_atari_h_dir[0], prefs.hardDiskDir[0]);
    strcpy(Devices_atari_h_dir[1], prefs.hardDiskDir[1]);
    strcpy(Devices_atari_h_dir[2], prefs.hardDiskDir[2]);
    strcpy(Devices_atari_h_dir[3], prefs.hardDiskDir[3]);
    Devices_h_read_only = prefs.hardDrivesReadOnly;
    strcpy(Devices_h_exe_path, prefs.hPath);
    strcpy(CFG_xegs_filename, prefs.xegsRomFile);
    strcpy(CFG_xegsGame_filename, prefs.xegsGameRomFile);
    strcpy(CFG_1200xl_filename, prefs.a1200XLRomFile);
    strcpy(CFG_osb_filename, prefs.osBRomFile);
    strcpy(CFG_xlxe_filename, prefs.xlRomFile);
    strcpy(CFG_basic_filename, prefs.basicRomFile);
    strcpy(CFG_5200_filename, prefs.a5200RomFile);
    Atari800_useAlitrraXEGSRom = prefs.useAltirraXEGSRom;
    Atari800_useAlitrra1200XLRom = prefs.useAltirra1200XLRom;
    Atari800_useAlitrraOSBRom = prefs.useAltirraOSBRom;
    Atari800_useAlitrraXLRom = prefs.useAltirraXLRom;
    Atari800_useAlitrra5200Rom = prefs.useAltirra5200Rom;
    Atari800_useAlitrraBasicRom = prefs.useAltirraBasicRom;

    strcpy(atari_disk_dirs[0], prefs.diskImageDir);
    strcpy(atari_diskset_dir,prefs.diskSetDir);
    strcpy(atari_rom_dir, prefs.cartImageDir);
    strcpy(atari_cass_dir, prefs.cassImageDir);
    strcpy(atari_exe_dir, prefs.exeFileDir);
    strcpy(atari_state_dir, prefs.savedStateDir);
    strcpy(atari_config_dir, prefs.configDir);
    for (i=0;i<4;i++) {
        JOYSTICK_MODE[i] = prefs.joystickMode[i];
        JOYSTICK_AUTOFIRE[i] = prefs.joystickAutofire[i];
        } 
    INPUT_joy_multijoy = prefs.enableMultijoy;

    INPUT_mouse_mode = prefs.mouseDevice;
    INPUT_mouse_speed = prefs.mouseSpeed;
    INPUT_mouse_pot_min = prefs.mouseMinVal;
    INPUT_mouse_pot_max = prefs.mouseMaxVal;
    INPUT_mouse_pen_ofs_h = prefs.mouseHOffset;
    INPUT_mouse_pen_ofs_v = prefs.mouseVOffset;
    INPUT_mouse_joy_inertia = prefs.mouseInertia;

    for (i=0;i<4;i++) {
        for (j=0;j<24;j++) {
            joystickButtonKey[i][j] = joystickIndexToKey[prefs.buttonAssignment[i][j]];
            joystick5200ButtonKey[i][j] = joystick5200IndexToKey[prefs.button5200Assignment[i][j]];
            }
        }

    joystick0TypePref = prefs.joystick1Type;
    joystick1TypePref = prefs.joystick2Type;
    joystick2TypePref = prefs.joystick3Type;
    joystick3TypePref = prefs.joystick4Type;
    joystick0Num = prefs.joystick1Num;
    joystick1Num = prefs.joystick2Num;
    joystick2Num = prefs.joystick2Num;
    joystick3Num = prefs.joystick4Num;
    paddlesXAxisOnly  = prefs.paddlesXAxisOnly; 
	keyjoyEnable = prefs.keyjoyEnable;
    INPUT_cx85 = prefs.cx85enabled;
	cx85_port = prefs.cx85port;

    
    SDL_TRIG_1 = keyboardStickIndexToKey[prefs.leftJoyFire];
	if (SDL_TRIG_1 == SDLK_LCTRL)
		SDL_TRIG_1_R = SDLK_RCTRL;
	else if (SDL_TRIG_1 == SDLK_LSHIFT)
		SDL_TRIG_1_R = SDLK_RSHIFT;
	else if (SDL_TRIG_1 == SDLK_LALT)
		SDL_TRIG_1_R = SDLK_RALT;
	else
		SDL_TRIG_1_R = SDL_TRIG_1;
    SDL_TRIG_1_B = keyboardStickIndexToKey[prefs.leftJoyAltFire];
	if (SDL_TRIG_1_B == SDLK_LCTRL)
		SDL_TRIG_1_B_R = SDLK_RCTRL;
	else if (SDL_TRIG_1_B == SDLK_LSHIFT)
		SDL_TRIG_1_B_R = SDLK_RSHIFT;
	else if (SDL_TRIG_1_B == SDLK_LALT)
		SDL_TRIG_1_B_R = SDLK_RALT;
	else
		SDL_TRIG_1_B_R = SDL_TRIG_1_B;
    SDL_JOY_1_LEFT = keyboardStickIndexToKey[prefs.leftJoyLeft];
    SDL_JOY_1_RIGHT = keyboardStickIndexToKey[prefs.leftJoyRight];
    SDL_JOY_1_DOWN = keyboardStickIndexToKey[prefs.leftJoyDown];
    SDL_JOY_1_UP = keyboardStickIndexToKey[prefs.leftJoyUp];
    SDL_JOY_1_LEFTUP = keyboardStickIndexToKey[prefs.leftJoyUpLeft];
    SDL_JOY_1_RIGHTUP = keyboardStickIndexToKey[prefs.leftJoyUpRight];
    SDL_JOY_1_LEFTDOWN = keyboardStickIndexToKey[prefs.leftJoyDownLeft];
    SDL_JOY_1_RIGHTDOWN = keyboardStickIndexToKey[prefs.leftJoyDownRight];
    SDL_TRIG_0 = keyboardStickIndexToKey[prefs.padJoyFire];
	if (SDL_TRIG_0 == SDLK_LCTRL)
		SDL_TRIG_0_R = SDLK_RCTRL;
	else if (SDL_TRIG_0 == SDLK_LSHIFT)
		SDL_TRIG_0_R = SDLK_RSHIFT;
	else if (SDL_TRIG_0 == SDLK_LALT)
		SDL_TRIG_0_R = SDLK_RALT;
	else
		SDL_TRIG_0_R = SDL_TRIG_0;
    SDL_TRIG_0_B = keyboardStickIndexToKey[prefs.padJoyAltFire];
	if (SDL_TRIG_0_B == SDLK_LCTRL)
		SDL_TRIG_0_B_R = SDLK_RCTRL;
	else if (SDL_TRIG_0_B == SDLK_LSHIFT)
		SDL_TRIG_0_B_R = SDLK_RSHIFT;
	else if (SDL_TRIG_0_B == SDLK_LALT)
		SDL_TRIG_0_B_R = SDLK_RALT;
	else
		SDL_TRIG_0_B_R = SDL_TRIG_0_B;
    SDL_JOY_0_LEFT = keyboardStickIndexToKey[prefs.padJoyLeft];
    SDL_JOY_0_RIGHT = keyboardStickIndexToKey[prefs.padJoyRight];
    SDL_JOY_0_DOWN = keyboardStickIndexToKey[prefs.padJoyDown];
    SDL_JOY_0_UP = keyboardStickIndexToKey[prefs.padJoyUp];
    SDL_JOY_0_LEFTUP = keyboardStickIndexToKey[prefs.padJoyUpLeft];
    SDL_JOY_0_RIGHTUP = keyboardStickIndexToKey[prefs.padJoyUpRight];
    SDL_JOY_0_LEFTDOWN = keyboardStickIndexToKey[prefs.padJoyDownLeft];
    SDL_JOY_0_RIGHTDOWN = keyboardStickIndexToKey[prefs.padJoyDownRight];
	
	mediaStatusWindowOpen = prefs.mediaStatusDisplayed;
	functionKeysWindowOpen = prefs.functionKeysDisplayed;
	
	currPrinter = prefs.currPrinter;
	
	saveCurrentMedia = prefs.saveCurrentMedia;
	clearCurrentMedia = prefs.clearCurrentMedia;
	useAtariCursorKeys = prefs.useAtariCursorKeys;

    return(TRUE);
}

void reloadMacJoyPrefs() {
   for (int i=0;i<4;i++) {
       JOYSTICK_MODE[i] = prefs.joystickMode[i];
       JOYSTICK_AUTOFIRE[i] = prefs.joystickAutofire[i];
       }
}
