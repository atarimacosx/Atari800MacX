/*
 * ultimate1mb.c - Emulation of the Austin Franklin 80 column card.
 *
 * Copyright (C) 2020 Mark Grebe
 *
*/

#include "af80.h"
#include "atari.h"
#include "util.h"
#include "log.h"
#include "memory.h"
#include "cpu.h"
#include <stdlib.h>

static UBYTE ultimate_rom[0x80000];
#ifdef ATARI800MACX
char ultimate_rom_filename[FILENAME_MAX] = Util_FILENAME_NOT_SET;
#else
static char ultimate_rom_filename[FILENAME_MAX] = Util_FILENAME_NOT_SET;
#endif

int ultimate_enabled = FALSE;

/* Ultimate1MB information from Altirra Hardware Reference Manual */
static int config_lock = FALSE;
static UBYTE pbi_bank = 0;
static int PBI_ROM_active = FALSE;
static int IO_RAM_enable = FALSE;
static int SDX_module_enable = FALSE;
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
static int pbi_button_enable = FALSE;
static UBYTE pbi_device_id = 0;
static UBYTE cold_reset_flag = 0x80;

#ifdef ATARI800MACX
void init_ultimate(void)
#else
static void init_ultimate(void)
#endif
{
    Log_print("Ultimate1MB enabled");
    if (!Atari800_LoadImage(ultimate_rom_filename, ultimate_rom, 0x80000)) {
        AF80_enabled = FALSE;
        SetDisplayManagerDisableAF80();
        Log_print("Couldn't load Ultimate1MB ROM image");
        return;
    }
    else {
        Log_print("loaded Ultimate1MB rom image");
    }
}

int Ultimate_Initialise(int *argc, char *argv[])
{
    init_ultimate();
	return TRUE;
}

void Ultimate_Exit(void)
{
}

int Ultimate_D1GetByte(UWORD addr, int no_side_effects)
{
    int result = 0xff;
    return result;
}

void Ultimate_D1PutByte(UWORD addr, UBYTE byte)
{
    if (addr == 0xD1BF && PBI_ROM_active) {
        pbi_bank = byte & 0x03;
    }
}

int Ultimate_D3GetByte(UWORD addr, int no_side_effects)
{
    int result = 0xff;
    if (addr < 0xD380) {
        return PIA_GetByte(add, no_side_effects);
    }
    else if (addr == 0xD384) {
        return (pbi_button_pressed << 7) |
               (ext_cart_rd5_sense << 6);
    }
    else if (addr == 0xD3E2) {
        return CDS1305_ReadState();
    }
    return result;
}

void Ultimate_D3PutByte(UWORD addr, UBYTE byte)
{
    if (addr < 0xD380) {
        PIA_PutByte(addr, byte);
    }
    else if (addr == 0xD380 && !config_lock) {
        if (byte & 0x80)
            config_lock = TRUE;
        else
            config_lock = FALSE;
        if (byte & 0x40)
            IO_RAM_enable = TRUE:
        else
            IO_RAM_enable = FALSE;
        if (byte & 0x10)
            SDX_module_enable = TRUE;
        else
            SDX_module_enable = FALSE;
        OS_ROM_select = (byte & 0x0C) >> 2;
        ultimate_mem_config = byte & 0x03;
    }
    else if (addr == 0xD381 && !config_lock) {
        if (byte & 0x80)
            flash_write_enable = TRUE;
        else
            flash_write_enable = FALSE;
        if (byte & 0x40)
            soundBoard_enable = TRUE:
        else
            soundBoard_enable = FALSE;
        if (byte & 0x20)
            VBXE_enabled = FALSE;
        else {
            VBXE_enabled = TRUE;
            if (byte & 0x10)
                VBXE_address = 0xD740;
            else
                VBXE_address = 0xD640;
        }
        signal_outputs = byte & 0x0F;
    }
    else if (addr == 0xD382 && !config_lock) {
        game_rom_select = (byte & 0xC0) >> 6;
        basic_rom_select = (byte & 0xe0) >> 4;
        if (byte & 0x08)
            pbi_button_enable = TRUE;
        else
            pbi_button_enable = FALSE;
        if (byte & 0x04)
            pbi_emulation_enable = TRUE;
        else
            pbi_emulation_enable = FALSE;
        pbi_device_id = byte & 0x03;
    }
    else if (addr == 0xD383 && !config_lock) {
        cold_reset_flag = byte;
    }
    else if (addr == 0xD3E2) {
        CDS1305_WriteState((value & 1) != 0, !(value & 2), (value & 4) != 0);
    }
}

void Ultimate_ColdStart(void)
{
}

void Ultimate_WarmStart(void)
{
}

