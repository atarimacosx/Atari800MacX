/*
 * ultimate1mb.c - Emulation of the Ultimate 1MB Expansion
 *
 * Adapted from Altirra
 * Copyright (C) 2008-2012 Avery Lee
 * Copyright (C) 2020 Mark Grebe
 *
*/

#include "atari.h"
#include "ultimate1mb.h"
#include "cartridge.h"
#include "util.h"
#include "log.h"
#include "memory.h"
#include "pia.h"
#include "pbi.h"
#include "rtcds1305.h"
#include "cpu.h"
#include "flash.h"
#include "gtia.h"
#include "esc.h"
#include "sysrom.h"
#include <stdlib.h>

static UBYTE ultimate_rom[0x80000];
#ifdef ATARI800MACX
char ultimate_rom_filename[FILENAME_MAX] = "/Users/markg/Atari800MacX/Altirra-3.20/ultimate.rom";
char ultimate_nvram_filename[FILENAME_MAX] = "/Users/markg/Atari800MacX/Altirra-3.20/ultimate.nvram";
#else
static char ultimate_rom_filename[FILENAME_MAX] = Util_FILENAME_NOT_SET;
#endif

int ULTIMATE_enabled = TRUE;
int ULTIMATE_have_rom = FALSE;
int ULTIMATE_Flash_Type = 0;

/* Ultimate1MB information from Altirra Hardware Reference Manual */
static int config_lock = FALSE;
static UBYTE pbi_bank = 0;
static int IO_RAM_enable = FALSE;
static int SDX_module_enable = TRUE;
static int SDX_enable = FALSE;
static int cart_bank_offset = 0;
static int external_cart_enable = FALSE;
static int external_cart_active = FALSE;
static int OS_ROM_select = 0;
static int ultimate_mem_config = 3;
static int flash_write_enable = FALSE;
static int soundBoard_enable = FALSE;
static int VBXE_enabled = FALSE;
static UWORD VBXE_address = 0;
static UBYTE signal_outputs = 0;
static UBYTE game_rom_select = 0;
static UBYTE basic_rom_select = 0;
static int pbi_emulation_enable = FALSE;
static int pbi_selected = FALSE;
static int pbi_button_enable = FALSE;
static UBYTE pbi_device_id = 0;
static UBYTE cold_reset_flag = 0x80;
static UBYTE pbi_ram[0x1000];
static void *rtc;
static FlashEmu *flash;

void CreateWindowCaption(void);
static void Select_PBI_Device(int enable);
static void Set_Kernel_Bank(UBYTE bank);
static void Set_Mem_Mode(void);
static void Set_SDX_Bank(UBYTE bank);
static void Set_SDX_Enabled(int enabled);
static void Set_SDX_Module_Enabled(int enabled);
static void Set_PBI_Bank(UBYTE bank);
static void Update_Kernel_Bank(void);
static void LoadNVRAM();
static void SaveNVRAM();

#ifdef ATARI800MACX
void init_ultimate(void)
#else
static void init_ultimate(void)
#endif
{
    Log_print("Ultimate1MB enabled");
    if (!Atari800_LoadImage(ultimate_rom_filename, ultimate_rom, 0x80000)) {
        ULTIMATE_enabled = FALSE;
        Log_print("Couldn't load Ultimate1MB ROM image");
        ULTIMATE_have_rom = FALSE;
        return;
    }
    else {
        Log_print("loaded Ultimate1MB rom image");
        ULTIMATE_have_rom = TRUE;
    }
    if (ULTIMATE_Flash_Type == 0)
        flash = Flash_Init(ultimate_rom, Flash_TypeAm29F040B);
    else
        flash = Flash_Init(ultimate_rom, Flash_TypeSST39SF040);
}

int ULTIMATE_Initialise(int *argc, char *argv[])
{
    init_ultimate();
    rtc = CDS1305_Init();
    LoadNVRAM();
    return TRUE;
}

void ULTIMATE_Exit(void)
{
    SaveNVRAM();
    CDS1305_Exit(rtc);
}

UBYTE ULTIMATE_D1GetByte(UWORD addr, int no_side_effects)
{
    int result = 0xff;
    if ( addr <= 0xD1BF && (pbi_selected || !config_lock))
        result = pbi_ram[addr & 0xFFF];
    return result;
}

void ULTIMATE_D1PutByte(UWORD addr, UBYTE byte)
{
    if (addr <= 0xD1BF) {
        if (pbi_selected || !config_lock) {
        if (addr == 0xD1BF)
            Set_PBI_Bank(byte & 3);
        pbi_ram[addr & 0xFFF] = byte;
        }
    }
}

void Set_PBI_Bank(UBYTE bank)
{
    if (pbi_selected) {
        memcpy(MEMORY_mem + 0xd800,
               ultimate_rom + 0x59800 + ((UWORD) bank << 13), 0x800);
    }
    pbi_bank = bank;
}

int ULTIMATE_D1ffPutByte(UBYTE byte)
{
    int result = 0; /* handled */
    if (ULTIMATE_enabled && pbi_emulation_enable && config_lock && byte == pbi_device_id) {
        if (pbi_selected)
            return 0;
        Select_PBI_Device(TRUE);
    }
    else {
        Select_PBI_Device(FALSE);
        result = PBI_NOT_HANDLED;
    }
    return result;
}

void ULTIMATE_Subcart_Left_Active(int active)
{
    external_cart_active = active;
}

UBYTE ULTIMATE_D3GetByte(UWORD addr, int no_side_effects)
{
    int result = 0xff;
    if (addr == 0xD3E2)
        result = CDS1305_ReadState(rtc) ? 0x08 : 0x00;
    else if (addr == 0xD383) {
        result = cold_reset_flag;
    }
    else if (addr == 0xD384) {
        UBYTE v = 0;
        
        if (pbi_button_enable)
            v += 0x80;
        
        if (external_cart_active)
            v += 0x40;
        
        return v;
    }
    else if (addr < 0xD380) {
        return PIA_GetByte(addr, no_side_effects);
    }
    
    return result;
}

void ULTIMATE_D3PutByte(UWORD addr, UBYTE byte)
{
    if (addr < 0xD380) {
        PIA_PutByte(addr, byte);
    }
    else {
        if (!config_lock) {
            if (addr == 0xD380) {
                ultimate_mem_config = byte & 0x03;
                Set_Mem_Mode();
                Set_Kernel_Bank((byte >> 2) & 0x03);
                Set_SDX_Module_Enabled(!(byte & 0x10));
                IO_RAM_enable = (byte & 0x40) != 0;
                if (byte & 0x80) {
                    config_lock = TRUE;
                    Update_Kernel_Bank();
                    ULONG osbase =  0x70000 + (OS_ROM_select << 14);
                    Atari800_os_version = SYSROM_FindImageType(&ultimate_rom[osbase]);
                    if (Atari800_os_version > 0)
                        ESC_UpdatePatches();
                }
            }
            else if (addr == 0xD381) {
                if (byte & 0x20)
                    VBXE_enabled = FALSE;
                else {
                    VBXE_enabled = TRUE;
                    if (byte & 0x10)
                        VBXE_address = 0xD740;
                    else
                        VBXE_address = 0xD640;
                }
                soundBoard_enable = (byte & 0x40) != 0;
                flash_write_enable = !(byte & 0x80);
                signal_outputs = byte & 0x0F;
            }
            else if (addr == 0xD382) {
                pbi_device_id = 0x01 << (2 * (byte & 3));
                pbi_emulation_enable = (byte & 0x04) != 0;
                pbi_button_enable = (byte & 0x08) != 0;
                basic_rom_select = (byte >> 4) & 3;
                game_rom_select = (byte >> 6) & 3;
                Update_Kernel_Bank();
            }
            else if (addr == 0xD383) {
                cold_reset_flag = byte;
            }
        }
        
        if (addr == 0xD3E2) {
            CDS1305_WriteState(rtc, (byte & 1) != 0, !(byte & 2), (byte & 4) != 0);
        }

    }

}

int ULTIMATE_D5GetByte(UWORD addr, int no_side_effects)
{
    int result = -1;
    if ( addr <= 0xD5BF && (pbi_selected || !config_lock))
        return pbi_ram[addr & 0xFFF];
    return result;
}

void ULTIMATE_D5PutByte(UWORD addr, UBYTE byte)
{
    if (addr <= 0xD5BF && (pbi_selected || !config_lock)) {
        pbi_ram[addr & 0xFFF] = byte;
    } else if (SDX_module_enable) {
        if (addr == 0xD5E0) {
            // SDX control (write only)
            //
            // D7 = 0    => Enable SDX
            // D6 = 0    => Enable external cartridge (only if SDX disabled)
            // D0:D5    => SDX bank
            //
            // Forced to 10000000 if SDX module is disabled.
            external_cart_enable = ((byte & 0xc0) == 0x80);
            Set_SDX_Bank(byte & 63);
            Set_SDX_Enabled(!(byte & 0x80));

            //Update_External_Cart();
            // Pre-control lock, the SDX bank is also used for the
            // BASIC and GAME banks (!).
            if (!config_lock)
                Update_Kernel_Bank();
        }
    }
}

UBYTE ULTIMATE_D6D7GetByte(UWORD addr, int no_side_effects)
{
    int result = 0xff;
    if ( addr <= 0xD7FF && (pbi_selected || IO_RAM_enable))
        return pbi_ram[addr & 0xFFF];
    return result;
}

void ULTIMATE_D6D7PutByte(UWORD addr, UBYTE byte)
{
    if (addr <= 0xD7FF && (pbi_selected || IO_RAM_enable)) {
        pbi_ram[addr & 0xFFF] = byte;
    }
}

void ULTIMATE_ColdStart(void)
{
    Flash_Cold_Reset(flash);
    
    cold_reset_flag = 0x80;        // ONLY set by cold reset, not warm reset.

    // The SDX module is enabled on warm reset, but the cart enables and bank
    // are only affected by cold reset.
    cart_bank_offset = 1;
    SDX_enable = FALSE;
    Set_SDX_Bank(0);
    Set_SDX_Enabled(TRUE);
    external_cart_enable = FALSE;
    flash_write_enable = TRUE;
    
    VBXE_address = 0xD640;
    VBXE_enabled = FALSE;
    soundBoard_enable = FALSE;
    signal_outputs = 0;
    
    // Clear PBI Ram
    memset(pbi_ram, 0, sizeof(pbi_ram));
    IO_RAM_enable = TRUE;

    MEMORY_SetFlashRoutines(ULTIMATE_Flash_Read, ULTIMATE_Flash_Write);

    ULTIMATE_WarmStart();
}

void ULTIMATE_WarmStart(void)
{
    // Reset RTC Chip
    CDS1305_ColdReset(rtc);

    // Default to all of extended memory
    ultimate_mem_config = 3;
    Set_Mem_Mode();

    // Allow configuration changes
    config_lock = FALSE;

    pbi_emulation_enable = FALSE;
    pbi_button_enable = FALSE;
    pbi_device_id = 0;
    pbi_selected = FALSE;
    Select_PBI_Device(FALSE);

    OS_ROM_select = 0;
    game_rom_select = 0;
    basic_rom_select = 0;

    Update_Kernel_Bank();
    Set_PBI_Bank(0);

    Set_SDX_Module_Enabled(TRUE);
}

void ULTIMATE_LoadRoms(void)
{
    Update_Kernel_Bank();
}

int ULTIMATE_Change_Rom(char *filename, int new) {
    int romLoaded;

    SaveNVRAM();
    romLoaded = Atari800_LoadImage(filename, ultimate_rom, 0x80000);
    if (!romLoaded) {
        ULTIMATE_have_rom = FALSE;
    } else {
        ULTIMATE_have_rom = TRUE;
        if (new)
            strcpy(ultimate_rom_filename, filename);
        strcpy(ultimate_nvram_filename, ultimate_rom_filename);
        UTIL_strip_ext(ultimate_nvram_filename);
        strcat(ultimate_nvram_filename,".nvram");
        LoadNVRAM();
        if (flash == NULL) {
            if (ULTIMATE_Flash_Type == 0)
                flash = Flash_Init(ultimate_rom, Flash_TypeAm29F040B);
            else
                flash = Flash_Init(ultimate_rom, Flash_TypeSST39SF040);
        }
    }

    return romLoaded;
}

int ULTIMATE_Save_Rom(char *filename)
{
    FILE *f;

    f = fopen(filename, "wb");
    if (f != NULL) {
        fwrite(ultimate_rom, 1, 0x80000, f);
        fclose(f);
        return 0;
    } else {
        return -1;
    }
}

static void Set_SDX_Bank(UBYTE bank) {
    ULONG offset = (ULONG)bank << 13;

    if (cart_bank_offset == offset)
        return;

    cart_bank_offset = offset;
    if (SDX_enable)
        MEMORY_CopyROM(0xa000, 0xbfff, ultimate_rom + cart_bank_offset);
}

static void Set_SDX_Enabled(int enabled) {
    if (SDX_enable == enabled)
        return;

    SDX_enable = enabled;
    if (SDX_enable) {
        if (CARTRIDGE_main.type == CARTRIDGE_NONE)
            CARTRIDGE_Insert_Ultimate_1MB();
        else {
            CARTRDIGE_Switch_To_Main();
        }
        MEMORY_SetFlashRoutines(ULTIMATE_Flash_Read, ULTIMATE_Flash_Write);
        MEMORY_SetFlash(0xa000, 0xbfff);
        MEMORY_CopyROM(0xa000, 0xbfff, ultimate_rom + cart_bank_offset);
    }
    else {
        MEMORY_SetROM(0xa000, 0xbfff);
        if (external_cart_enable)
            CARTRDIGE_Switch_To_Piggyback(pbi_button_enable);
        else
            MEMORY_CartA0bfDisable();
    }
}

static void Set_SDX_Module_Enabled(int enabled) {
    if (SDX_module_enable == enabled)
        return;

    if (enabled) {
        Set_SDX_Enabled(TRUE);
    } else {
        external_cart_enable = TRUE;
        Set_SDX_Bank(0);
        Set_SDX_Enabled(FALSE);
    }

}

static void Set_Kernel_Bank(UBYTE bank)
{
    if (OS_ROM_select != bank) {
        OS_ROM_select = bank;
    
        Update_Kernel_Bank();
    }
}

static void Update_Kernel_Bank(void)
{
    // Prior to control lock, the kernel bank is locked to $50000 instead
    // of $7x000. The BASIC, GAME, and PBI selects also act weirdly in this
    // mode (they are mapped with the SDX mapping register!).

    ULONG kernelbase = config_lock ? 0x70000 + (OS_ROM_select << 14) : 0x50000;
    ULONG basicbase = config_lock ? 0x60000 + (basic_rom_select << 13) : cart_bank_offset;
    ULONG gamebase = config_lock ? 0x68000 + (game_rom_select << 13) : cart_bank_offset;

    memcpy(MEMORY_os, ultimate_rom + kernelbase, 0x4000);
    memcpy(MEMORY_mem + 0xc000, ultimate_rom + kernelbase, 0x4000);
    if (MEMORY_selftest_enabled)
        memcpy(MEMORY_mem + 0x5000, ultimate_rom + kernelbase + 0x1000, 0x800);
    memcpy(MEMORY_basic, ultimate_rom + basicbase, 0x2000);


    memcpy(MEMORY_xegame, ultimate_rom + gamebase, 0x2000);
}

static void LoadNVRAM()
{
    UBYTE buf[0x72];
    FILE *f;
    int len;
    
    f = fopen(ultimate_nvram_filename, "rb");
    if (f == NULL) {
        memset(buf, 0, sizeof(buf));
    } else {
        len = fread(buf, 1, 0x72, f);
        fclose(f);
        if (len != 0x72) {
            memset(buf, 0, sizeof(buf));
        }
    }
    CDS1305_Load(rtc, buf);
}

static void SaveNVRAM()
{
    UBYTE buf[0x72];
    FILE *f;

    CDS1305_Save(rtc, buf);
    f = fopen(ultimate_nvram_filename, "wb");
    if (f != NULL) {
        fwrite(buf, 1, 0x72, f);
        fclose(f);
    }
}

static void Set_Mem_Mode(void)
{
    switch(ultimate_mem_config) {
        case 0:
            MEMORY_ram_size = 64;
            break;
        case 1:
            MEMORY_ram_size = 320;
            break;
        case 2:
            MEMORY_ram_size = 576;
            break;
        case 3:
            MEMORY_ram_size = 1088;
            break;
    }
    CreateWindowCaption();
}

static void Select_PBI_Device(int selected)
{
    if (pbi_selected == selected)
        return;
    
    pbi_selected = selected;
    
    if (selected) {
        MEMORY_StartPBIOverlay();
        Set_PBI_Bank(pbi_bank);
    }
    else {
        Set_PBI_Bank(0);
        MEMORY_StopPBIOverlay();
        ULONG kernelbase = config_lock ? 0x70000 + (OS_ROM_select << 14) : 0x50000;

        if ((PIA_PORTB | PIA_PORTB_mask) & 0x01)
            memcpy(MEMORY_mem + 0xd800,
                   ultimate_rom + kernelbase + 0x1800, 0x800);
    }
}

UBYTE ULTIMATE_Flash_Read(UWORD addr) {
    UBYTE value;

    Flash_Read_Byte(flash, cart_bank_offset + (addr - 0xA000), &value);
        
    return(value);
}

void ULTIMATE_Flash_Write(UWORD addr, UBYTE value) {
    if (flash_write_enable)
        Flash_Write_Byte(flash, cart_bank_offset + (addr - 0xA000), value);
}
