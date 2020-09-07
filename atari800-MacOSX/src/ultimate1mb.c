/*
 * ultimate1mb.c - Emulation of the Ultimate 1MB Expansion
 *
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
#include <stdlib.h>

static UBYTE ultimate_rom[0x80000];
#ifdef ATARI800MACX
char ultimate_rom_filename[FILENAME_MAX] = "/Users/markg/Atari800MacX/Altirra-3.20/ultimate.rom"; //Util_FILENAME_NOT_SET;
char ultimate_nvram_filename[FILENAME_MAX] = "/Users/markg/Atari800MacX/Altirra-3.20/ultimate.nvram"; //Util_FILENAME_NOT_SET;
#else
static char ultimate_rom_filename[FILENAME_MAX] = Util_FILENAME_NOT_SET;
#endif

int ULTIMATE_enabled = TRUE;

/* Ultimate1MB information from Altirra Hardware Reference Manual */
static int config_lock = FALSE;
static UBYTE pbi_bank = 0;
static int IO_RAM_enable = FALSE;
static int SDX_module_enable = TRUE;
static int SDX_enable = FALSE;
static int cart_bank_offset = 0;
static int external_cart_enable = FALSE;
static int OS_ROM_select = 0;
static int ultimate_mem_config = 0;
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
static int pbi_button_pressed = FALSE;
static UBYTE pbi_device_id = 0;
static UBYTE cold_reset_flag = 0x80;
static int ext_cart_rd5_sense = FALSE;
static UBYTE pbi_ram[0x1000];

static void Select_PBI_Device(int enable);
static void Set_Kernel_Bank(UBYTE bank);
static void Set_Mem_Mode(void);
static void Set_SDX_Bank(UBYTE bank);
static void Set_SDX_Enabled(int enabled);
static void Set_SDX_Module_Enabled(int enabled);
static void Set_PBI_Bank(UBYTE bank);
static void Update_External_Cart(void);
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
        return;
    }
    else {
        Log_print("loaded Ultimate1MB rom image");
    }
}

int ULTIMATE_Initialise(int *argc, char *argv[])
{
    init_ultimate();
    CDS1305_Init();
    LoadNVRAM();
    return TRUE;
}

void ULTIMATE_Exit(void)
{
    SaveNVRAM();
}

UBYTE ULTIMATE_D1GetByte(UWORD addr, int no_side_effects)
{
    int result = 0xff;
    if ( addr <= 0xD1BF && (pbi_selected || !config_lock))
        return pbi_ram[addr & 0xFFF];
    return result;
}

void ULTIMATE_D1PutByte(UWORD addr, UBYTE byte)
{
    if (addr <= 0xD1BF) {
        if (addr == 0xD1BF)
            Set_PBI_Bank(byte & 3);
        if (pbi_selected || !config_lock)
            pbi_ram[addr & 0xFFF] = byte;
    }
}

void Set_PBI_Bank(UBYTE bank)
{
    if (pbi_selected) {
        if (PIA_PORTB & 0x01)
            memcpy(MEMORY_mem + 0xd800,
                   ultimate_rom + 0x59800 + ((UWORD) bank << 13), 0x800);
        memcpy(MEMORY_os + 0x1800,
               ultimate_rom + 0x59800 + ((UWORD) bank << 13), 0x800);
    }
    pbi_bank = bank;
}

int ULTIMATE_D1ffPutByte(UBYTE byte)
{
    int result = 0; /* handled */
    if (ULTIMATE_enabled && pbi_emulation_enable && byte == pbi_device_id) {
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

UBYTE ULTIMATE_D3GetByte(UWORD addr, int no_side_effects)
{
    int result = 0xff;
    if (addr == 0xD3E2)
        return CDS1305_ReadState() ? 0x08 : 0x00;
    else if (addr == 0xD383) {
        result = cold_reset_flag;
    }
    else if (addr == 0xD384) {
        UBYTE v = 0;
        
        if (pbi_button_pressed)
            v += 0x80;
        
        if (external_cart_enable)
            v += 0x40;
        
        return v;
    }
    else if (addr < 0xD380) {
        return PIA_GetByte(addr, no_side_effects);
    }
    
    return 0xFF;
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
                    // TBD Update_PBI_Device();
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
                // TBD UpdatePBIDevice();
                // TBD UpdateExternalCart();
                Update_Kernel_Bank();
            }
            else if (addr == 0xD383) {
                cold_reset_flag = byte;
            }
        }
        
        if (addr == 0xD3E2) {
            CDS1305_WriteState((byte & 1) != 0, !(byte & 2), (byte & 4) != 0);
        }

    }

}

UBYTE ULTIMATE_D5GetByte(UWORD addr, int no_side_effects)
{
    int result = 0xff;
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

            // TBD - What do we do here?
            Update_External_Cart();
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
    cold_reset_flag = 0x80;        // ONLY set by cold reset, not warm reset.

    // Allow configuration changes
    config_lock = FALSE;

    // The SDX module is enabled on warm reset, but the cart enables and bank
    // are only affected by cold reset.
    cart_bank_offset = 1;
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

    ULTIMATE_WarmStart();
}

void ULTIMATE_WarmStart(void)
{
    // Reset RTC Chip
    CDS1305_ColdReset();

    // Default to all of extended memory
    ultimate_mem_config = 3;
    Set_Mem_Mode();

    pbi_emulation_enable = FALSE;
    pbi_button_enable = FALSE;
    pbi_device_id = 0;
    pbi_selected = FALSE;
    // TBD Update PBI Device

    OS_ROM_select = 0;
    game_rom_select = 0;
    basic_rom_select = 0;

    Update_Kernel_Bank();
    Set_PBI_Bank(0);

    Set_SDX_Module_Enabled(TRUE);

    //TBD UpdateExternalCart();
    //UpdateCartLayers();
    //UpdateFlashShadows();
}

void ULTIMATE_LoadRoms(void)
{
    Update_Kernel_Bank();
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
        //if (CARTRIDGE_main.type == CARTRIDGE_NONE)
        CARTRIDGE_Insert_Ultimate_1MB();
        //MEMORY_Cart809fDisable();
        //MEMORY_CartA0bfEnable();
        MEMORY_CopyROM(0xa000, 0xbfff, ultimate_rom + cart_bank_offset);
    }
    else {
        MEMORY_CartA0bfDisable();
        if (external_cart_enable)
            CARTRDIGE_Switch_To_Piggyback();
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
        // TBD UpdateExternalCart();
    }

    // TBD mpCartridgePort->OnLeftWindowChanged(mCartId, IsLeftCartActive());
}

static void Update_External_Cart(void)
{
    // TBD
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
    // TBD - What about self test area? And what about getting the
    // below into their parts of MEMORY_mem.
    memcpy(MEMORY_basic, ultimate_rom + basicbase, 0x2000);
    memcpy(MEMORY_xegame, ultimate_rom + gamebase, 0x2000);
    //ESC_UpdatePatches(); // TBD - Should we do this???
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
    CDS1305_Load(buf);
}

static void SaveNVRAM()
{
    UBYTE buf[0x72];
    FILE *f;

    CDS1305_Save(buf);
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
    MEMORY_AllocXEMemory();
}

static void Select_PBI_Device(int selected)
{
    if (pbi_selected == selected)
        return;
    
    pbi_selected = selected;
    
    if (selected) {
        //pbi_bank = 255;  // Force reload
        Set_PBI_Bank(0);
    }
    else {
        ULONG kernelbase = config_lock ? 0x70000 + (OS_ROM_select << 14) : 0x50000;

        if (PIA_PORTB & 0x01)
            memcpy(MEMORY_mem + 0xd800,
                   ultimate_rom + kernelbase + 0x1800, 0x800);
        memcpy(MEMORY_os + 0x1800, ultimate_rom + kernelbase + 0x1800, 0x800);

        //Set_PBI_Bank(0);
    }
}
