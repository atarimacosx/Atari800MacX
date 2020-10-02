/*
 * side2.c - Emulation of the SIDE2 cartridge
 *
 * Copyright (C) 2008-2012 Avery Lee
 * Copyright (C) 2020 Mark Grebe
 *
*/

#include "atari.h"
#include "side2.h"
#include "cartridge.h"
#include "cpu.h"
#include "ide.h"
#include "log.h"
#include "memory.h"
#include "rtcds1305.h"
#include "util.h"
#include "pia.h"
#include "flash.h"
#include <stdlib.h>

int SIDE2_enabled = FALSE;

static UBYTE side2_rom[0x80000];
#ifdef ATARI800MACX
char side2_rom_filename[FILENAME_MAX] = Util_FILENAME_NOT_SET; //"/Users/markg/Atari800MacX/Altirra-3.20/side2oss.rom"; 
char side2_nvram_filename[FILENAME_MAX] = Util_FILENAME_NOT_SET; //"/Users/markg/Atari800MacX/Altirra-3.20/side2.nvram";
char side2_compact_flash_filename[FILENAME_MAX] = Util_FILENAME_NOT_SET; //"/Users/markg/Atari800MacX/Altirra-3.20/side2.vhd";
#else
static char side_rom_filename[FILENAME_MAX] = Util_FILENAME_NOT_SET;
static char side2_nvram_filename[FILENAME_MAX] = Util_FILENAME_NOT_SET;
static char side2_compact_flash_filename[FILENAME_MAX] = Util_FILENAME_NOT_SET;
#endif

int SIDE2_SDX_Mode_Switch = TRUE;
int SIDE2_Block_Device = 0;
static int Coldstarted = FALSE;
static int IDE_Enabled = FALSE;
static int IDE_Removed = TRUE;
static int IDE_Reset = FALSE;
static int SDX_Bank = 0;
static int SDX_Bank_Register = 0;
static int SDX_Enabled = FALSE;
static int Top_Bank = 0;
static int Top_Bank_Register = 0;
static int Top_Enable = FALSE;
static int Top_Left_Enable = FALSE;
static int Top_Right_Enable = FALSE;
static int Left_Window_Enabled = FALSE;
static int Right_Window_Enabled = FALSE;
static ULONG Bank_Offset = 0;
static ULONG Bank_Offset2 = 0;
static void *ide;
static void *rtc;
static FlashEmu *flash;

static void LoadNVRAM();
static void Reset_Cart_Bank(void);
static void SaveNVRAM();
static void Set_SDX_Bank(int bank, int topEnable);
static void Set_Top_Bank(int bank, int topLeftEnable, int topRightEnable);
static void Update_Memory_Layers_Cart(void);

#ifdef ATARI800MACX
void init_side2(void)
#else
static void init_side2(void)
#endif
{
    Log_print("Side2 enabled");
    if (!Atari800_LoadImage(side2_rom_filename, side2_rom, 0x80000)) {
        SIDE2_enabled = FALSE;
        Log_print("Couldn't load Side2 ROM image");
        return;
    }
    else {
        Log_print("loaded Side2 rom image");
    }
    ide = IDE_Init_Drive(side2_compact_flash_filename, TRUE);
    if (ide == NULL)
    {
        Log_print("Couldn't attach Side2 CF Image");
        side2_compact_flash_filename[0] = 0;
        SIDE2_Block_Device = FALSE;
    }
    else {
        Log_print("Attached Side2 CF Image");
        SIDE2_Block_Device = TRUE;
    }
    flash = Flash_Init(side2_rom, Flash_TypeAm29F040B);
}

void SIDE2_Remove_Block_Device(void)
{
    if (SIDE2_Block_Device) {
        IDE_Close_Drive(ide);
        side2_compact_flash_filename[0] = 0;
        SIDE2_Block_Device = FALSE;
        IDE_Reset_Device(ide);
    }
}

int SIDE2_Add_Block_Device(char *filename) {
    if (SIDE2_Block_Device)
        SIDE2_Remove_Block_Device();
    ide = IDE_Init_Drive(filename, TRUE);
    if (ide == NULL)
    {
        Log_print("Couldn't attach Side2 CF Image");
        SIDE2_Block_Device = FALSE;
    }
    else {
        Log_print("Attached Side2 CF Image");
        strcpy(side2_compact_flash_filename, filename);
        SIDE2_Block_Device = TRUE;
    }
    return SIDE2_Block_Device;
}

int SIDE2_Change_Rom(char *filename) {
    int romLoaded;

    strcpy(side2_rom_filename, filename);
    romLoaded = Atari800_LoadImage(side2_rom_filename, side2_rom, 0x80000);
    if (!romLoaded) {
        SIDE2_enabled = FALSE;
        side2_compact_flash_filename[0] = 0;
    }
    
    return romLoaded;
}

int SIDE2_Initialise(int *argc, char *argv[])
{
    init_side2();
    rtc = CDS1305_Init();
    LoadNVRAM();

    return TRUE;
}

void SIDE2_Exit(void)
{
    SaveNVRAM();
    CDS1305_Exit(rtc);
}

void SIDE2_SDX_Switch_Change(int state)
{
    SDX_Enabled = state;
    SIDE2_SDX_Mode_Switch = state;
    Update_Memory_Layers_Cart();
}

void SIDE2_Bank_Reset_Button_Change()
{
    Reset_Cart_Bank();
}

int SIDE2_D5GetByte(UWORD addr, int no_side_effects)
{
    int result = -1;
    
    switch(addr) {
        case 0xD5E1:    // SDX bank register
            result = SDX_Bank_Register;
            break;
        case 0xD5E2:    // DS1305 RTC
            result =  CDS1305_ReadState(rtc) ? 0x08 : 0x00;
            break;
        case 0xD5E4:    // top cartridge bank switching
            result = Top_Bank_Register;
            break;
        case 0xD5F0:
        case 0xD5F1:
        case 0xD5F2:
        case 0xD5F3:
        case 0xD5F4:
        case 0xD5F5:
        case 0xD5F6:
        case 0xD5F7:
             result = IDE_Enabled && SIDE2_Block_Device ? IDE_GetByte(ide, addr, FALSE) : 0xFF;
            break;
        case 0xD5F8:
            return 0x32;
            break;
        case 0xD5F9:
            result = IDE_Removed ? 1 : 0;
            break;
        case 0xD5FC:
            result = SDX_Enabled ? 'S' : ' ';
            break;
        case 0xD5FD:
            result = 'I';
            break;
        case 0xD5FE:
            result = 'D';
            break;
        case 0xD5FF:
            result = 'E';
            break;
        default:
            break;
    }

    return result;
}

void SIDE2_D5PutByte(UWORD addr, UBYTE byte)
{
    switch(addr) {
        case 0xD5E1:
            if (SDX_Bank_Register != byte) {
                SDX_Bank_Register = byte;
                Set_SDX_Bank(byte & 0x80 ? -1 : (byte & 0x3f), !(byte & 0x40));
            }
            break;

        case 0xD5E2:    // DS1305 RTC
            CDS1305_WriteState(rtc, (byte & 1) != 0, !(byte & 2), (byte & 4) != 0);
            break;

        case 0xD5E4:    // top cartridge bank switching
            if (Top_Bank_Register != byte) {
                Top_Bank_Register = byte;
                Set_Top_Bank((byte & 0x3f) ^ 0x20, (byte & 0x80) == 0, ((byte & 0x40) != 0));
            }
            break;

        case 0xD5F0:
        case 0xD5F1:
        case 0xD5F2:
        case 0xD5F3:
        case 0xD5F4:
        case 0xD5F5:
        case 0xD5F6:
        case 0xD5F7:
            if (IDE_Enabled && SIDE2_Block_Device)
                IDE_PutByte(ide, addr, byte);
            break;

        case 0xD5F8:    // F8-FB: D0 = /reset
        case 0xD5F9:
        case 0xD5FA:
        case 0xD5FB:
            if (addr == 0xD5F9) {
                // Strobe to clear CARD_REMOVED. This can't be done if there isn't actually a
                // card.
                if (SIDE2_Block_Device)
                    IDE_Removed = FALSE;
            }

            IDE_Enabled = !(byte & 0x80);
            break;
        default:
            break;
        }
}

void SIDE2_ColdStart(void)
{
    Coldstarted = TRUE;
    // Reset RTC Chip
    CDS1305_ColdReset(rtc);

    Reset_Cart_Bank();
    SIDE2_SDX_Switch_Change(SIDE2_SDX_Mode_Switch);

    IDE_Reset = TRUE;
    IDE_Enabled = TRUE;

    // If the CF card is absent, the removed flag is always set and can't be
    // cleared. If it's present, the removed flag is cleared on powerup.
    IDE_Removed = !SIDE2_Block_Device;

    IDE_Reset_Device(ide);
}

void SIDE2_Set_Cart_Enables(int leftEnable, int rightEnable) {
    int changed = FALSE;

    if (Left_Window_Enabled != leftEnable) {
        Left_Window_Enabled = leftEnable;
        changed = TRUE;
    }

    if (Right_Window_Enabled != rightEnable) {
        Right_Window_Enabled = rightEnable;
        changed = TRUE;
    }
    
    if (changed && Coldstarted)
        Update_Memory_Layers_Cart();
}


static void LoadNVRAM()
{
    UBYTE buf[0x72];
    FILE *f;
    int len;
    
    f = fopen(side2_nvram_filename, "rb");
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
    f = fopen(side2_nvram_filename, "wb");
    if (f != NULL) {
        fwrite(buf, 1, 0x72, f);
        fclose(f);
    }
}

static void Set_SDX_Bank(int bank, int topEnable)
{
    if (SDX_Bank == bank && Top_Enable == topEnable)
        return;

    SDX_Bank = bank;
    Top_Enable = topEnable;

    Update_Memory_Layers_Cart();
    //mpCartridgePort->OnLeftWindowChanged(mCartId, IsLeftCartActive());
}

static void Set_Top_Bank(int bank, int topLeftEnable, int topRightEnable)
{
    // If the top cartridge is enabled in 16K mode, the LSB bank bit is ignored.
    // We force the LSB on in that case so the right cart window is in the right
    // place and the left cart window is 8K below that (mask LSB back off).
    if (topRightEnable)
        bank |= 0x01;

    if (Top_Bank == bank && Top_Right_Enable == topRightEnable && Top_Left_Enable == topLeftEnable)
        return;

    Top_Bank = bank;
    Top_Left_Enable = topLeftEnable;
    Top_Right_Enable = topRightEnable;

    Update_Memory_Layers_Cart();
    //mpCartridgePort->OnLeftWindowChanged(mCartId, IsLeftCartActive());
}

static void Reset_Cart_Bank(void)
{
    SDX_Bank_Register = 0x00;
    Set_SDX_Bank(0, TRUE);

    Top_Bank_Register = 0x00;
    Set_Top_Bank(0x20, TRUE, FALSE);
}

void Update_Memory_Layers_Cart() {
    if (SDX_Bank >= 0 && SDX_Enabled)
        Bank_Offset = SDX_Bank << 13;
    else if (Top_Enable || !SDX_Enabled)
        Bank_Offset = Top_Bank << 13;

    Bank_Offset2 = Bank_Offset & ~0x2000;

    // SDX disabled by switch => top cartridge enabled, SDX control bits ignored
    // else   SDX disabled, top cartridge disabled => no cartridge
    //        SDX disabled, top cartridge enabled => top cartridge
    //        other => SDX cartridge

    int sdxRead = (SDX_Enabled && SDX_Bank >= 0);
    int topRead = Top_Enable || !SDX_Enabled;
    int topLeftRead = topRead && Top_Left_Enable;
    int flashRead = Left_Window_Enabled && (sdxRead || topLeftRead);
    int flashReadRight = Right_Window_Enabled && topRead &&
                         !sdxRead && Top_Right_Enable;

    if (flashRead) {
        MEMORY_CartA0bfEnable();
        MEMORY_CopyROM(0xa000, 0xbfff, side2_rom + Bank_Offset);
    } else {
        MEMORY_CartA0bfDisable();
    }
    
    if (flashReadRight) {
        MEMORY_Cart809fEnable();
        MEMORY_CopyROM(0x8000, 0xbfff, side2_rom + Bank_Offset2);
    } else {
        MEMORY_Cart809fDisable();
    }
}

UBYTE SIDE2_Flash_Read(UWORD addr) {
    UBYTE value;

    Flash_Read_Byte(flash, Bank_Offset + (addr - 0xA000), &value);
        
    return(value);
}

void SIDE2_Flash_Write(UWORD addr, UBYTE value) {

    Flash_Write_Byte(flash, Bank_Offset + (addr - 0xA000), value);
}
