/*
 * atari.c - main high-level routines
 *
 * Copyright (c) 1995-1998 David Firth
 * Copyright (c) 1998-2008 Atari800 development team (see DOC/CREDITS)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "afile.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SIGNAL_H
biteme!
#include <signal.h>
#endif
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# elif defined(HAVE_TIME_H)
#  include <time.h>
# endif
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif
#ifdef __EMX__
#define INCL_DOS
#include <os2.h>
#endif
#ifdef __BEOS__
#include <OS.h>
#endif
#ifdef HAVE_LIBZ
#include <zlib.h>
#endif
#ifdef R_SERIAL
#include <sys/stat.h>
#endif
#ifdef SDL
#include "SDL.h"
#endif

#include "af80.h"
#include "altirra_basic.h"
#include "altirraos_800.h"
#include "altirraos_xl.h"
#include "altirra_5200_os.h"
#include "akey.h"
#include "antic.h"
#include "atari.h"
#include "binload.h"
#include "bit3.h"
#include "cartridge.h"
#include "cassette.h"
#include "cfg.h"
#include "cpu.h"
#include "devices.h"
#include "esc.h"
#include "gtia.h"
#include "input.h"
#include "log.h"
#include "memory.h"
#include "monitor.h"
#include "pia.h"
#include "platform.h"
#include "pokey.h"
#include "rtime.h"
#include "pbi.h"
#include "sio.h"
#include "side2.h"
#include "ui.h"
#include "ultimate1mb.h"
#include "util.h"
#include "colours.h"
#include "screen.h"
#include "statesav.h"
#include "sysrom.h"

extern double emulationSpeed;
void loadMacPrefs(int firstTime);
void MacSoundReset(void);
void MacCapsLockStateReset(void);
int MediaManagerCartSelect(int nbytes);
#if defined(SOUND) && !defined(__PLUS)
#include "pokeysnd.h"
#include "sndsave.h"
#include "sound.h"
#endif
#ifdef R_IO_DEVICE
#include "rdevice.h"
#endif
#ifdef PBI_BB
#include "pbi_bb.h"
#endif
#ifdef PBI_XLD
#include "pbi_xld.h"
#endif
#ifdef XEP80_EMULATION
#include "xep80.h"
#endif

int machine_switch_type = 7;
int Atari800_machine_type = Atari800_MACHINE_XLXE;
int Atari800_tv_mode = Atari800_TV_PAL;
int Atari800_disable_basic = TRUE;
int Atari800_useAlitrraXEGSRom;
int Atari800_useAlitrra1200XLRom;
int Atari800_useAlitrraOSBRom;
int Atari800_useAlitrraXLRom;
int Atari800_useAlitrra5200Rom;
int Atari800_useAlitrraBasicRom;
int Atari800_os_version = -1;
int Atari800_basic_version = -1;
int Atari800_builtin_basic = TRUE;

int verbose = FALSE;

int Atari800_keyboard_leds = FALSE;
int Atari800_f_keys = FALSE;
int Atari800_jumper = FALSE;
int Atari800_jumper_present = FALSE;
int Atari800_builtin_game = FALSE;
int Atari800_keyboard_detached = FALSE;

int Atari800_display_screen = FALSE;
int Atari800_nframes = 0;
int Atari800_refresh_rate = 1;
int Atari800_collisions_in_skipped_frames = FALSE;

double deltatime;
double fps;
int percent_atari_speed = 100;

int emuos_mode = 1;	/* 0 = never use EmuOS, 1 = use EmuOS if real OS not available, 2 = always use EmuOS */

void Atari800_Warmstart(void)
{
#ifdef MACOSX
	MacCapsLockStateReset();
    if (XEP80_enabled)
        XEP80_Reset();
    if (XEP80_enabled && COL80_autoswitch) {
        XEP80_sent_count = 0;
        XEP80_last_sent_count = 0;
        if (PLATFORM_80col)
            PLATFORM_Switch80Col();
    }
#endif
	if (Atari800_machine_type == Atari800_MACHINE_800) {
		/* A real Axlon homebanks on reset */
		/* XXX: what does Mosaic do? */
		if (MEMORY_axlon_num_banks > 0) MEMORY_PutByte(0xcfff, 0);
		/* RESET key in 400/800 does not reset chips,
		   but only generates RNMI interrupt */
		ANTIC_NMIST = 0x3f;
		CPU_NMI();
	}
	else {
        if (ULTIMATE_enabled && (Atari800_machine_type == Atari800_MACHINE_XLXE)) {
            ULTIMATE_WarmStart();
        }
		PBI_Reset();
        PIA_Reset();
		ANTIC_Reset();
		/* CPU_Reset() must be after PIA_Reset(),
		   because Reset routine vector must be read from OS ROM */
		CPU_Reset();
		/* note: POKEY and GTIA have no Reset pin */

	}
	Devices_WarmCold_Start();
}

void Atari800_Coldstart(void)
{
#ifdef MACOSX
	ANTIC_screenline_cpu_clock = 0;
	MacSoundReset();
	MacCapsLockStateReset();
#endif	
    if (SIDE2_enabled) {
        SIDE2_ColdStart();
        }
    if (ULTIMATE_enabled && (Atari800_machine_type == Atari800_MACHINE_XLXE)) {
        ULTIMATE_ColdStart();
        }
	PBI_Reset();
	PIA_Reset();
	ANTIC_Reset();
	/* CPU_Reset() must be after PIA_Reset(),
	   because Reset routine vector must be read from OS ROM */
	CPU_Reset();
    CARTRIDGE_ColdStart();
    /* set Atari OS Coldstart flag */
    MEMORY_dPutByte(0x244, 1);
    /* handle Option key (disable BASIC in XL/XE)
       and Start key (boot from cassette) */
    GTIA_consol_override = 2;
	/* note: POKEY and GTIA have no Reset pin */
    if (XEP80_enabled)
        XEP80_Reset();
    if (XEP80_enabled && COL80_autoswitch) {
        XEP80_sent_count = 0;
        XEP80_last_sent_count = 0;
    }
    if ((XEP80_enabled || BIT3_enabled) && COL80_autoswitch) {
        if (PLATFORM_80col)
            PLATFORM_Switch80Col();
    }
    if (AF80_enabled && COL80_autoswitch) {
        if (!PLATFORM_80col)
            PLATFORM_Switch80Col();
    }
#ifdef AF80
	if (AF80_enabled) {
		AF80_Reset();
		AF80_InsertRightCartridge();
	}
#endif
#ifdef BIT3
	if (BIT3_enabled) {
		BIT3_Reset();
	}
#endif
    Devices_WarmCold_Start();
}

int Atari800_LoadImage(const char *filename, UBYTE *buffer, int nbytes)
{
	FILE *f;
	int len;

	f = fopen(filename, "rb");
	if (f == NULL) {
		Log_print("Error loading ROM image: %s", filename);
		return FALSE;
	}
	len = fread(buffer, 1, nbytes, f);
	fclose(f);
	if (len != nbytes) {
		Log_print("Error reading %s", filename);
		return FALSE;
	}
	return TRUE;
}

#include "emuos.h"

#define COPY_EMUOS(padding) do { \
		memset(MEMORY_os, 0, padding); \
		memcpy(MEMORY_os + (padding), emuos_h, 0x2000); \
	} while (0)


static int load_roms(void)
{
    char OSType[FILENAME_MAX];
    char *osFilename;
    int osSize;
    int loadBasic;
    int defaultType;
    int altirraType;
    int useAltira;
    const unsigned char *altirraRom;
    char *altirraString;
    char *machineString;

    if (ULTIMATE_enabled && (Atari800_machine_type == Atari800_MACHINE_XLXE)) {
        ULTIMATE_LoadRoms();
        return TRUE;
    }
    switch (Atari800_machine_type) {
        case Atari800_MACHINE_800:
        default:
            osFilename = CFG_osb_filename;
            defaultType = SYSROM_800_CUSTOM;
            altirraType = SYSROM_ALTIRRA_800;
            osSize = 0x2800;
            loadBasic = TRUE;
            useAltira = Atari800_useAlitrraOSBRom;
            altirraRom = ROM_altirraos_800;
            altirraString = "Using Alitrra 800 OS";
            machineString = "Loading ROMS for OSB Machine";
            break;
        case Atari800_MACHINE_XLXE:
            if (Atari800_builtin_game) {
                osFilename = CFG_xegs_filename;
                useAltira = Atari800_useAlitrraXEGSRom;
                machineString = "Loading ROMS for XEGS Machine";
            } else if (Atari800_keyboard_leds) {
                osFilename = CFG_1200xl_filename;
                useAltira = Atari800_useAlitrra1200XLRom;
                machineString = "Loading ROMS for 1200XL Machine";
            } else {
                osFilename = CFG_xlxe_filename;
                useAltira = Atari800_useAlitrraXLRom;
                machineString = "Loading ROMS for XL/XE Machine";
            }
            defaultType = SYSROM_XL_CUSTOM;
            altirraType = SYSROM_ALTIRRA_XL;
            osSize = 0x4000;
            loadBasic = TRUE;
            altirraRom = ROM_altirraos_xl;
            altirraString = "Using Alitrra XL/XE OS";
            break;
        case Atari800_MACHINE_5200:
            osFilename = CFG_5200_filename;
            defaultType = SYSROM_5200_CUSTOM;
            altirraType = SYSROM_ALTIRRA_5200;
            osSize = 0x800;
            loadBasic = FALSE;
            useAltira = Atari800_useAlitrra5200Rom;
            altirraRom = ROM_altirra_5200_os;
            altirraString = "Using Alitrra 5200 OS";
            machineString = "Loading ROMS for 5200 Machine";
            break;
    }
    
    Log_print(machineString);
    if (useAltira) {
        memcpy(MEMORY_os, altirraRom, osSize);
        Atari800_os_version = altirraType;
        Log_print(altirraString);
        }
    else if (Atari800_LoadImage(osFilename, MEMORY_os, osSize)) {
        Atari800_os_version = SYSROM_FindType(defaultType, osFilename, OSType);
        Log_print("Loaded OS: %s",OSType);
        }
    else {
        memcpy(MEMORY_os, altirraRom, osSize);
        Atari800_os_version = altirraType;
        Log_print(altirraString);
    }

    if (Atari800_builtin_game) {
        /* Try loading built-in XEGS game. */
        if (Atari800_LoadImage(CFG_xegsGame_filename, MEMORY_xegame, 0x2000))
            Log_print("Loaded XEGS Game ROM");
        else {
            memcpy(MEMORY_xegame, ROM_altirra_basic, 0x2000);
            Log_print("Error Loading XEGS Game ROM - Substituting Altirra Basic");
        }
    }
    
    if (!loadBasic) {
        MEMORY_have_basic = FALSE;
        return TRUE;
    }
    
    if (Atari800_useAlitrraBasicRom) {
        memcpy(MEMORY_basic, ROM_altirra_basic, 0x2000);
        Atari800_basic_version = SYSROM_ALTIRRA_BASIC;
        Log_print("Using Alitrra BASIC");
        }
    else if (Atari800_LoadImage(CFG_basic_filename, MEMORY_basic, 0x2000)) {
        Atari800_basic_version = SYSROM_FindType(SYSROM_BASIC_CUSTOM, CFG_basic_filename, OSType);
        Log_print("Loaded BASIC: %s",OSType);
        }
    else {
        memcpy(MEMORY_basic, ROM_altirra_basic, 0x2000);
        Atari800_basic_version = SYSROM_ALTIRRA_BASIC;
        Log_print("Using Alitrra BASIC");
    }
    
    MEMORY_have_basic = TRUE;
    return TRUE;
}


int Atari800_InitialiseMachine(void)
{
#if !defined(BASIC) && !defined(CURSES_BASIC) && !defined(ATARI800MACX)
	Colours_InitialiseMachine();
#endif
	ESC_ClearAll();
	if (!load_roms())
		return FALSE;
    Atari800_UpdateKeyboardDetached();
    if (Atari800_jumper_present)
        Atari800_UpdateJumper();
    MEMORY_InitialiseMachine();
    Atari800_Coldstart();
	Devices_UpdatePatches();
	return TRUE;
}


int Atari800_Initialise(int *argc, char *argv[])
{
	int i, j;
	const char *rom_filename = NULL;
	const char *rom2_filename = NULL;
	const char *run_direct = NULL;
#ifndef BASIC
	const char *state_file = NULL;
#endif

	for (i = j = 1; i < *argc; i++) {
		if (strcmp(argv[i], "-atari") == 0) {
			if (Atari800_machine_type != Atari800_MACHINE_800) {
				Atari800_machine_type = Atari800_MACHINE_800;
				MEMORY_ram_size = 48;
			}
		}
		else if (strcmp(argv[i], "-xl") == 0) {
			Atari800_machine_type = Atari800_MACHINE_XLXE;
			MEMORY_ram_size = 64;
		}
		else if (strcmp(argv[i], "-xe") == 0) {
			Atari800_machine_type = Atari800_MACHINE_XLXE;
			MEMORY_ram_size = 128;
		}
		else if (strcmp(argv[i], "-320xe") == 0) {
			Atari800_machine_type = Atari800_MACHINE_XLXE;
			MEMORY_ram_size = MEMORY_RAM_320_COMPY_SHOP;
		}
		else if (strcmp(argv[i], "-rambo") == 0) {
			Atari800_machine_type = Atari800_MACHINE_XLXE;
			MEMORY_ram_size = MEMORY_RAM_320_RAMBO;
		}
		else if (strcmp(argv[i], "-5200") == 0) {
			Atari800_machine_type = Atari800_MACHINE_5200;
			MEMORY_ram_size = 16;
		}
		else if (strcmp(argv[i], "-nobasic") == 0)
			Atari800_disable_basic = TRUE;
		else if (strcmp(argv[i], "-basic") == 0)
			Atari800_disable_basic = FALSE;
		else if (strcmp(argv[i], "-nopatch") == 0)
			ESC_enable_sio_patch = FALSE;
		else if (strcmp(argv[i], "-nopatchall") == 0)
#ifdef D_PATCH			
			ESC_enable_sio_patch = Devices_enable_h_patch = Devices_enable_d_patch = Devices_enable_p_patch = Devices_enable_r_patch = FALSE;
#else
			ESC_enable_sio_patch = Devices_enable_h_patch = Devices_enable_p_patch = Devices_enable_r_patch = FALSE;
#endif
		else if (strcmp(argv[i], "-pal") == 0)
			Atari800_tv_mode = Atari800_TV_PAL;
		else if (strcmp(argv[i], "-ntsc") == 0)
			Atari800_tv_mode = Atari800_TV_NTSC;
		else if (strcmp(argv[i], "-a") == 0) {
			Atari800_machine_type = Atari800_MACHINE_800;
			MEMORY_ram_size = 48;
		}
		else if (strcmp(argv[i], "-b") == 0) {
			Atari800_machine_type = Atari800_MACHINE_800;
			MEMORY_ram_size = 48;
		}
		else if (strcmp(argv[i], "-emuos") == 0)
			emuos_mode = 2;
		else if (strcmp(argv[i], "-c") == 0) {
			if (MEMORY_ram_size == 48)
				MEMORY_ram_size = 52;
		}
		else {
			/* parameters that take additional argument follow here */
			int i_a = (i + 1 < *argc);		/* is argument available? */
			int a_m = FALSE;			/* error, argument missing! */

            if (strcmp(argv[i], "-osb_rom") == 0) {
                if (i_a) Util_strlcpy(CFG_osb_filename, argv[++i], sizeof(CFG_osb_filename)); else a_m = TRUE;
            }
#if 0  /* TBD what to do with R device */
#ifdef R_IO_DEVICE
			else if (strcmp(argv[i], "-rdevice") == 0) {
				Devices_enable_r_patch = TRUE;
#ifdef R_SERIAL
				if (i_a && i + 2 < *argc && *argv[i + 1] != '-') {  /* optional serial device name */
					struct stat statbuf;
					if (! stat(argv[i + 1], &statbuf)) {
						if (S_ISCHR(statbuf.st_mode)) { /* only accept devices as serial device */
							Util_strlcpy(RDevice_serial_device, argv[++i], FILENAME_MAX);
							RDevice_serial_enabled = TRUE;
						}
					}
				}
#endif /* R_SERIAL */
			}
#endif
#endif
			else if (strcmp(argv[i], "-osb_rom") == 0) {
				if (i_a) Util_strlcpy(CFG_osb_filename, argv[++i], sizeof(CFG_osb_filename)); else a_m = TRUE;
			}
			else if (strcmp(argv[i], "-xlxe_rom") == 0) {
				if (i_a) Util_strlcpy(CFG_xlxe_filename, argv[++i], sizeof(CFG_xlxe_filename)); else a_m = TRUE;
			}
			else if (strcmp(argv[i], "-5200_rom") == 0) {
				if (i_a) Util_strlcpy(CFG_5200_filename, argv[++i], sizeof(CFG_5200_filename)); else a_m = TRUE;
			}
			else if (strcmp(argv[i], "-basic_rom") == 0) {
				if (i_a) Util_strlcpy(CFG_basic_filename, argv[++i], sizeof(CFG_basic_filename)); else a_m = TRUE;
			}
			else if (strcmp(argv[i], "-cart") == 0) {
				if (i_a) rom_filename = argv[++i]; else a_m = TRUE;
			}
			else if (strcmp(argv[i], "-cart2") == 0) {
				if (i_a) rom2_filename = argv[++i]; else a_m = TRUE;
			}
			else if (strcmp(argv[i], "-run") == 0) {
				if (i_a) run_direct = argv[++i]; else a_m = TRUE;
			}
			else if (strcmp(argv[i], "-mosaic") == 0) {
				int total_ram = Util_sscandec(argv[++i]);
				MEMORY_mosaic_num_banks = (total_ram - 48)/4;
				if (((total_ram - 48) % 4 != 0) || (MEMORY_mosaic_num_banks > 0x3f) || (MEMORY_mosaic_num_banks < 0)) {
					Log_print("Invalid Mosaic total RAM size");
					return FALSE;
				}
				if (MEMORY_axlon_num_banks > 0) {
					Log_print("Axlon and Mosaic can not both be enabled, because they are incompatible");
					return FALSE;
				}
			}
			else if (strcmp(argv[i], "-axlon") == 0) {
				int total_ram = Util_sscandec(argv[++i]);
				int banks = ((total_ram) - 32) / 16;
				if (((total_ram - 32) % 16 != 0) || ((banks != 8) && (banks != 16) && (banks != 32) && (banks != 64) && (banks != 128) && (banks != 256))) {
					Log_print("Invalid Axlon total RAM size");
					return FALSE;
				}
				if (MEMORY_mosaic_num_banks > 0) {
					Log_print("Axlon and Mosaic can not both be enabled, because they are incompatible");
					return FALSE;
				}
				MEMORY_axlon_num_banks = banks;
			}
			else if (strcmp(argv[i], "-axlon0f") == 0) {
				MEMORY_axlon_0f_mirror = TRUE;
			}
#ifndef BASIC
			/* The BASIC version does not support state files, because:
			   1. It has no ability to save state files, because of lack of UI.
			   2. It uses a simplified emulation, so the state files would be
			      incompatible with other versions.
			   3. statesav is not compiled in to make the executable smaller. */
			else if (strcmp(argv[i], "-state") == 0) {
				if (i_a) state_file = argv[++i]; else a_m = TRUE;
			}
			else if (strcmp(argv[i], "-refresh") == 0) {
				if (i_a) {
					Atari800_refresh_rate = Util_sscandec(argv[++i]);
					if (Atari800_refresh_rate < 1) {
						Log_print("Invalid refresh rate, using 1");
						Atari800_refresh_rate = 1;
					}
				}
				else
					a_m = TRUE;
			}
#endif /* BASIC */
#ifdef STEREO_SOUND
			else if (strcmp(argv[i], "-stereo") == 0) {
				POKEYSND_stereo_enabled = TRUE;
			}
			else if (strcmp(argv[i], "-nostereo") == 0) {
				POKEYSND_stereo_enabled = FALSE;
			}
#endif /* STEREO_SOUND */
			else {
				/* all options known to main module tried but none matched */

				if (strcmp(argv[i], "-help") == 0) {
					Log_print("\t-atari           Emulate Atari 800");
					Log_print("\t-xl              Emulate Atari 800XL");
					Log_print("\t-xe              Emulate Atari 130XE");
					Log_print("\t-320xe           Emulate Atari 320XE (COMPY SHOP)");
					Log_print("\t-rambo           Emulate Atari 320XE (RAMBO)");
					Log_print("\t-5200            Emulate Atari 5200 Games System");
					Log_print("\t-nobasic         Turn off Atari BASIC ROM");
					Log_print("\t-basic           Turn on Atari BASIC ROM");
					Log_print("\t-pal             Enable PAL TV mode");
					Log_print("\t-ntsc            Enable NTSC TV mode");
					Log_print("\t-osa_rom <file>  Load OS A ROM from file");
					Log_print("\t-osb_rom <file>  Load OS B ROM from file");
					Log_print("\t-xlxe_rom <file> Load XL/XE ROM from file");
					Log_print("\t-5200_rom <file> Load 5200 ROM from file");
					Log_print("\t-basic_rom <fil> Load BASIC ROM from file");
					Log_print("\t-cart <file>     Install cartridge (raw or CART format)");
					Log_print("\t-run <file>      Run Atari program (COM, EXE, XEX, BAS, LST)");
#ifndef BASIC
					Log_print("\t-state <file>    Load saved-state file");
					Log_print("\t-refresh <rate>  Specify screen refresh rate");
#endif
					Log_print("\t-nopatch         Don't patch SIO routine in OS");
					Log_print("\t-nopatchall      Don't patch OS at all, H: device won't work");
					Log_print("\t-a               Use OS A");
					Log_print("\t-b               Use OS B");
					Log_print("\t-c               Enable RAM between 0xc000 and 0xcfff in Atari 800");
					Log_print("\t-axlon <n>       Use Atari 800 Axlon memory expansion: <n> k total RAM");
					Log_print("\t-axlon0f         Use Axlon shadow at 0x0fc0-0x0fff");
					Log_print("\t-mosaic <n>      Use 400/800 Mosaic memory expansion: <n> k total RAM");
#ifdef R_IO_DEVICE
					Log_print("\t-rdevice [<dev>] Enable R: emulation (using serial device <dev>)");
#endif
					Log_print("\t-v               Show version/release number");
				}

				/* copy this option for platform/module specific evaluation */
				argv[j++] = argv[i];
			}

			/* this is the end of the additional argument check */
			if (a_m) {
				printf("Missing argument for '%s'\n", argv[i]);
				return FALSE;
			}
		}
	}

	*argc = j;

#ifdef MACOSX
	loadMacPrefs(TRUE);
	if (Atari800_tv_mode == Atari800_TV_PAL)
		deltatime = (1.0 / Atari800_FPS_PAL) / emulationSpeed;
	else
		deltatime = (1.0 / Atari800_FPS_NTSC) / emulationSpeed;
#endif

	Colours_Initialise(argc, argv);
	Devices_Initialise(argc, argv);
	RTIME_Initialise(argc, argv);
	SIO_Initialise (argc, argv);
	CASSETTE_Initialise(argc, argv);
	PBI_Initialise(argc,argv);
	INPUT_Initialise(argc, argv);
#ifdef XEP80_EMULATION
    XEP80_Initialise(argc, argv);
#endif
#ifdef AF80_EMULATION
    AF80_Initialise(argc, argv);
#endif
#ifdef BIT3_EMULATION
    BIT3_Initialise(argc, argv);
#endif
#ifdef ULTIMATE_1MB
    ULTIMATE_Initialise(argc, argv);
#endif
#ifdef SIDE2
    SIDE2_Initialise(argc, argv);
#endif
	/* Platform Specific Initialisation */
	PLATFORM_Initialise(argc, argv);
	Screen_Initialise(argc, argv);
	/* Initialise Custom Chips */
	ANTIC_Initialise(argc, argv);
	GTIA_Initialise(argc, argv);
	PIA_Initialise(argc, argv);
	POKEY_Initialise(argc, argv);

	/* Configure Atari System */
	Atari800_InitialiseMachine();

	/* Auto-start files left on the command line */
	j = 1; /* diskno */
	for (i = 1; i < *argc; i++) {
		if (j > 8) {
			/* The remaining arguments are not necessary disk images, but ignore them... */
			Log_print("Too many disk image filenames on the command line (max. 8).");
			break;
		}
		switch (AFILE_OpenFile(argv[i], i == 1, j, FALSE)) {
			case AFILE_ERROR:
				Log_print("Error opening \"%s\"", argv[i]);
				break;
			case AFILE_ATR:
			case AFILE_XFD:
			case AFILE_ATR_GZ:
			case AFILE_XFD_GZ:
			case AFILE_DCM:
			case AFILE_PRO:
				j++;
				break;
			default:
				break;
		}
	}

	/* Install requested ROM cartridge */
	if (rom_filename) {
		int r = CARTRIDGE_InsertAutoReboot(rom_filename);
		if (r < 0) {
			Log_print("Error inserting cartridge \"%s\": %s", rom_filename,
			r == CARTRIDGE_CANT_OPEN ? "Can't open file" :
			r == CARTRIDGE_BAD_FORMAT ? "Bad format" :
			r == CARTRIDGE_BAD_CHECKSUM ? "Bad checksum" :
			"Unknown error");
		}
		if (r > 0) {
            CARTRIDGE_SetType(&CARTRIDGE_main, MediaManagerCartSelect(CARTRIDGE_main.size));
		}
	}

	/* Install requested second ROM cartridge, if first is SpartaX */
    if (((CARTRIDGE_main.type == CARTRIDGE_SDX_64) || (CARTRIDGE_main.type == CARTRIDGE_SDX_128) || (CARTRIDGE_main.type == CARTRIDGE_ATRAX_SDX_64) || (CARTRIDGE_main.type == CARTRIDGE_ATRAX_SDX_128)) && rom2_filename) {
		int r = CARTRIDGE_Insert_Second(rom2_filename);
		if (r < 0) {
			Log_print("Error inserting cartridge \"%s\": %s", rom2_filename,
			r == CARTRIDGE_CANT_OPEN ? "Can't open file" :
			r == CARTRIDGE_BAD_FORMAT ? "Bad format" :
			r == CARTRIDGE_BAD_CHECKSUM ? "Bad checksum" :
			"Unknown error");
		}
		if (r > 0) {
            CARTRIDGE_SetType(&CARTRIDGE_piggyback, MediaManagerCartSelect(CARTRIDGE_piggyback.size));
		}
	}
	
	/* Load Atari executable, if any */
	if (run_direct != NULL)
		BINLOAD_Loader(run_direct);

	/* Load state file */
	if (state_file != NULL) {
		if (StateSav_ReadAtariState(state_file, "rb"))
			/* Don't press Start nor Option */
			GTIA_consol_override = 0;
	}

	return TRUE;
}

UNALIGNED_STAT_DEF(Screen_atari_write_long_stat)
UNALIGNED_STAT_DEF(pm_scanline_read_long_stat)
UNALIGNED_STAT_DEF(memory_read_word_stat)
UNALIGNED_STAT_DEF(memory_write_word_stat)
UNALIGNED_STAT_DEF(memory_read_aligned_word_stat)
UNALIGNED_STAT_DEF(memory_write_aligned_word_stat)

int Atari800_Exit(int run_monitor)
{
	int restart;

	restart = PLATFORM_Exit(run_monitor);
	if (!restart) {
		SIO_Exit();	/* umount disks, so temporary files are deleted */
		INPUT_Exit();	/* finish event recording */
#ifdef R_IO_DEVICE
		RDevice_Exit(); /* R: Device cleanup */
#endif
#ifdef SOUND
		SndSave_CloseSoundFile();
#endif
        AF80_Exit();
        BIT3_Exit();
        if (ULTIMATE_enabled)
            ULTIMATE_Exit();
        if (SIDE2_enabled)
            SIDE2_Exit();
	}
	return restart;
}

static double Atari_time(void)
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return tp.tv_sec + 1e-6 * tp.tv_usec;
}

void Atari_sleep(double s)
{
	if (s > 0) {
		usleep(s * 1e6);
	}
}

void Atari800_Sync(void)
{
	static double lasttime = 0;
	double local_deltatime;
	double curtime;
	double sleeptime;
	local_deltatime = deltatime * PLATFORM_AdjustSpeed();
	lasttime += local_deltatime;
	sleeptime = lasttime - Atari_time();
	if (sleeptime > 1.0) {
		Log_print("Large positive sleeptime (%f), correcting to 1.0",sleeptime);
		sleeptime = 1.0;
	}
	
	Atari_sleep(sleeptime);
	curtime = Atari_time();
	
	if ((lasttime + local_deltatime) < curtime)
		lasttime = curtime;
}

void Atari800_Frame(void)
{
#ifndef BASIC
	static int refresh_counter = 0;
	switch (INPUT_key_code) {
	case AKEY_COLDSTART:
		Atari800_Coldstart();
		break;
	case AKEY_WARMSTART:
		Atari800_Warmstart();
		break;
	case AKEY_EXIT:
		Atari800_Exit(FALSE);
		exit(0);
	case AKEY_SCREENSHOT:
		Screen_SaveNextScreenshot(FALSE);
		break;
	case AKEY_SCREENSHOT_INTERLACE:
		Screen_SaveNextScreenshot(TRUE);
		break;
	case AKEY_PBI_BB_MENU:
#ifdef PBI_BB
		PBI_BB_Menu();
#endif
		break;
	default:
		break;
	}
#endif /* BASIC */

#ifdef PBI_BB
	PBI_BB_Frame(); /* just to make the menu key go up automatically */
#endif
#ifdef PBI_XLD
	PBI_XLD_VFrame(); /* for the Votrax */
#endif
	Devices_Frame();
	INPUT_Frame();
	GTIA_Frame();

	if (++refresh_counter >= Atari800_refresh_rate) {
		refresh_counter = 0;
		ANTIC_Frame(TRUE);
		INPUT_DrawMousePointer();
		Screen_DrawAtariSpeed(Atari_time());
        Screen_DrawDiskLED();
        Screen_DrawHDDiskLED();
		Atari800_display_screen = TRUE;
	}
	else {
		ANTIC_Frame(Atari800_collisions_in_skipped_frames);
		Atari800_display_screen = FALSE;
	}
	POKEY_Frame();
#ifdef SOUND
	Sound_Update();
#endif
	Atari800_nframes++;

    Atari800_Sync();
}

void Atari800_SetTVMode(int mode)
{
    if (mode != Atari800_tv_mode) {
        Atari800_tv_mode = mode;
   }
}

void Atari800_StateSave(void)
{
    UBYTE temp = Atari800_tv_mode == Atari800_TV_PAL;
    StateSav_SaveUBYTE(&temp, 1);
    temp = Atari800_machine_type;
    StateSav_SaveUBYTE(&temp, 1);
    if (Atari800_machine_type == Atari800_MACHINE_XLXE) {
        temp = Atari800_builtin_basic;
        StateSav_SaveUBYTE(&temp, 1);
        temp = Atari800_keyboard_leds;
        StateSav_SaveUBYTE(&temp, 1);
        temp = Atari800_f_keys;
        StateSav_SaveUBYTE(&temp, 1);
        temp = Atari800_jumper;
        StateSav_SaveUBYTE(&temp, 1);
        temp = Atari800_builtin_game;
        StateSav_SaveUBYTE(&temp, 1);
        temp = Atari800_keyboard_detached;
        StateSav_SaveUBYTE(&temp, 1);
    }
}

void Atari800_StateRead(UBYTE version)
{
    if (version >= 7) {
        UBYTE temp;
        StateSav_ReadUBYTE(&temp, 1);
        Atari800_SetTVMode(temp ? Atari800_TV_PAL : Atari800_TV_NTSC);
        StateSav_ReadUBYTE(&temp, 1);
        if (temp >= Atari800_MACHINE_SIZE) {
            temp = Atari800_MACHINE_XLXE;
            Log_print("Warning: Bad machine type read in from state save, defaulting to XL/XE");
        }
        Atari800_SetMachineType(temp);
        if (Atari800_machine_type == Atari800_MACHINE_XLXE) {
            StateSav_ReadUBYTE(&temp, 1);
            Atari800_builtin_basic = temp != 0;
            StateSav_ReadUBYTE(&temp, 1);
            Atari800_keyboard_leds = temp != 0;
            StateSav_ReadUBYTE(&temp, 1);
            Atari800_f_keys = temp != 0;
            StateSav_ReadUBYTE(&temp, 1);
            Atari800_jumper = temp != 0;
            if (Atari800_jumper_present)
                Atari800_UpdateJumper();
            StateSav_ReadUBYTE(&temp, 1);
            Atari800_builtin_game = temp != 0;
            StateSav_ReadUBYTE(&temp, 1);
            Atari800_keyboard_detached = temp != 0;
            Atari800_UpdateKeyboardDetached();
        }
    }
    else { /* savestate from version 2.2.1 or earlier */
        int new_tv_mode;
        /* these are all for compatibility with previous versions */
        UBYTE temp;
        int default_tv_mode;
        int os;
        int default_system;
        int pil_on;

        StateSav_ReadUBYTE(&temp, 1);
        new_tv_mode = (temp == 0) ? Atari800_TV_PAL : Atari800_TV_NTSC;
        Atari800_SetTVMode(new_tv_mode);

        StateSav_ReadUBYTE(&temp, 1);
        StateSav_ReadINT(&os, 1);
        switch (temp) {
        case 0:
            Atari800_machine_type = Atari800_MACHINE_800;
            MEMORY_ram_size = 48;
            break;
        case 1:
            Atari800_machine_type = Atari800_MACHINE_XLXE;
            MEMORY_ram_size = 64;
            break;
        case 2:
            Atari800_machine_type = Atari800_MACHINE_XLXE;
            MEMORY_ram_size = 128;
            break;
        case 3:
            Atari800_machine_type = Atari800_MACHINE_XLXE;
            MEMORY_ram_size = MEMORY_RAM_320_COMPY_SHOP;
            break;
        case 4:
            Atari800_machine_type = Atari800_MACHINE_5200;
            MEMORY_ram_size = 16;
            break;
        case 5:
            Atari800_machine_type = Atari800_MACHINE_800;
            MEMORY_ram_size = 16;
            break;
        case 6:
            Atari800_machine_type = Atari800_MACHINE_XLXE;
            MEMORY_ram_size = 16;
            break;
        case 7:
            Atari800_machine_type = Atari800_MACHINE_XLXE;
            MEMORY_ram_size = 576;
            break;
        case 8:
            Atari800_machine_type = Atari800_MACHINE_XLXE;
            MEMORY_ram_size = 1088;
            break;
        case 9:
            Atari800_machine_type = Atari800_MACHINE_XLXE;
            MEMORY_ram_size = 192;
            break;
        default:
            Atari800_machine_type = Atari800_MACHINE_XLXE;
            MEMORY_ram_size = 64;
            Log_print("Warning: Bad machine type read in from state save, defaulting to 800 XL");
            break;
        }

        StateSav_ReadINT(&pil_on, 1);
        StateSav_ReadINT(&default_tv_mode, 1);
        StateSav_ReadINT(&default_system, 1);
        Atari800_SetMachineType(Atari800_machine_type);
    }
    load_roms();
    /* XXX: what about patches? */
}

void Atari800_SetMachineType(int type)
{
    Atari800_machine_type = type;
    if (Atari800_machine_type != Atari800_MACHINE_XLXE) {
        Atari800_builtin_basic = FALSE;
        Atari800_keyboard_leds = FALSE;
        Atari800_f_keys = FALSE;
        Atari800_jumper = FALSE;
        Atari800_builtin_game = FALSE;
        Atari800_keyboard_detached = FALSE;
    }
}

void Atari800_UpdateKeyboardDetached(void)
{
    if (Atari800_machine_type == Atari800_MACHINE_XLXE) {
        GTIA_TRIG[2] = !Atari800_keyboard_detached;
        if (Atari800_keyboard_detached && (GTIA_GRACTL & 4))
                GTIA_TRIG_latch[2] = 0;
    }
}
void Atari800_UpdateJumper(void)
{
    if (Atari800_machine_type == Atari800_MACHINE_XLXE)
        POKEY_POT_input[4] = Atari800_jumper ? 0 : 228;
}

