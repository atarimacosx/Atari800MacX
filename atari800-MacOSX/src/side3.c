/*
 * side3.c - Emulation of the SIDE2 cartridge
 *
 * Adapted from Altirra
 * Copyright (C) 2009-2020 Avery Lee
 * Copyright (C) 2021 Mark Grebe
 *
*/

#include "atari.h"
#include "side3.h"
#include "cartridge.h"
#include "cpu.h"
#include "ide.h"
#include "log.h"
#include "memory.h"
#include "rtccmp7951x.h"
#include "util.h"
#include "pia.h"
#include "flash.h"
#include "ultimate1mb.h"
#include "scheduler.h"
#include <stdlib.h>

int SIDE3_enabled = FALSE;
int SIDE3_have_rom = FALSE;

static UBYTE side3_rom[0x80000];
static UBYTE side3_RAM[0x200000];

#ifdef ATARI800MACX
char side3_rom_filename[FILENAME_MAX] = Util_FILENAME_NOT_SET;
char side3_nvram_filename[FILENAME_MAX] = Util_FILENAME_NOT_SET;
char side3_sd_card_filename[FILENAME_MAX] = Util_FILENAME_NOT_SET;
#else
static char side3_rom_filename[FILENAME_MAX] = Util_FILENAME_NOT_SET;
static char side3_nvram_filename[FILENAME_MAX] = Util_FILENAME_NOT_SET;
static char side3_sd_card_filename[FILENAME_MAX] = Util_FILENAME_NOT_SET;
#endif

int SIDE3_SDX_Mode_Switch = TRUE;
int SIDE3_Block_Device = 0;
static int Recovery_Mode = FALSE;
static int SDX_Enable = FALSE;
static int Firmware_Usable = FALSE;
static int Aperture_Enable = FALSE;           // Enable mapping SRAM $1FF500-1FF57F to $D500-D57F
static UBYTE Bank_Flash_A = 0;
static UBYTE Bank_Flash_B = 0;
static UBYTE Bank_RAM_A = 0;
static UBYTE Bank_RAM_B = 0;
static UBYTE Bank_Control = 0;
static UBYTE Bank_RAM_Write_Protect = 0;       // bits 0-2 valid in hardware; bit 2 is unknown but is read/write
static ULONG Flash_Offset_A = 0;              // latched flash offset for native banking
static ULONG Flash_Offset_B = 0;              // latched flash offset for native banking
static ULONG Window_Offset_A1 = 0;            // effective flash offset for $8000-8FFF
static ULONG Window_Offset_A2 = 0;            // effective flash offset for $9000-9FFF
static ULONG Window_Offset_B = 0;             // effective flash offset for $A000-BFFF
static int Window_Using_Flash_A = FALSE;

static int Activity_Indicator_Enable = TRUE;
static UBYTE LED_Brightness = 0;         // $D5F0 primary set
static ULONG LED_Green_Color = 0;

static int LED_Green_Enabled = FALSE;
static int LED_Green_Manual_Enabled = FALSE;

static ULONG LED_Xfer_Off_Time = 0;

static int Button_Pressed = FALSE;
static int Button_PBI_Mode = FALSE;
static int Cold_Start_Flag = FALSE;

enum RegisterMode {
    Register_Mode_Primary,
    Register_Mode_DMA,
    Register_Mode_CartridgeEmu,
    Register_Mode_Reserved
}

static enum RegisterMode Register_Mode = Register_Mode_Primary;

static UBYTE SD_Status = 0x00;       // $D5F3 primary set
static UBYTE SD_CRC7 = 0;
static uint64 SD_Next_Transfer_Time = 0;
static UBYTE SD_Prev_Read = 0;
static UBYTE SD_Next_Read = 0;

static UBYTE SD_Command_Frame[6] = {0, 0, 0, 0, 0, 0};
static UBYTE SD_Command_State = 0;
static int SD_HC = TRUE;
static int SD_SPI_Mode = FALSE;
static int SD_App_Command = FALSE;
static int SD_CRC_Enabled = TRUE;
static ULONG SD_Response_Index = 0;
static ULONG SD_Response_Length = 0;
static ULONG SD_Send_Index = 0;

enum SDActiveCommandMode {
    SD_Mode_None,
    SD_Mode_ReadMultiple,
    SD_Mode_Write,
    SD_Mode_WriteMultiple
};

static enum SDActiveCommandMode SD_Active_Command_Mode = 
    SD_Mode_None;
static ULONG SD_Active_Command_LBA = 0;

static int DMA_Active = FALSE;
static int DMA_From_SD = FALSE;    // inverted mirror of control bit 5
static ULONG DMA_Src_Address = 0;
static ULONG DMA_Dst_Address = 0;
static UWORD DMA_Counter = 0;
static ULONG DMA_Active_Src_Address = 0;
static ULONG DMA_Active_Dst_Address = 0;
static ULONG DMA_Active_Bytes_Left = 0;
static ULONG DMA_Src_Step = 0;
static ULONG DMA_Dst_Step = 0;
static UBYTE DMA_And_Mask = 0;
static UBYTE DMA_Xor_Mask = 0;
static UBYTE DMA_Latch = 0;
static uint64 DMA_Last_Cycle = 0;
static ULONG DMA_State = 0;

static UBYTE Emu_CCTL_Base = 0;
static UBYTE Emu_CCTL_Mask = 0;
static UBYTE Emu_AddressMask = 0;
static UBYTE Emu_DataMask = 0;
static UBYTE Emu_DisableMask_A = 0;
static UBYTE Emu_DisableMask_B = 0;
static int Emu_Disabled_A = FALSE;
static int Emu_Disabled_B = FALSE;
static int Emu_Locked = FALSE;
static int Emu_Cart_Enable_Requested = FALSE;
static int Emu_Cart_Enable_Effective = FALSE;

enum {
    EF_UseAddress = 0x80,
    EF_EnableDisA = 0x10,
    EF_EnableDisB = 0x08,
    EF_DisableByData = 0x04,
    EF_EnableCCTLMask = 0x02,
    EF_BountyBob = 0x01
};

static UBYTE Emu_Feature = 0;

enum {
    EC_Enable = 0x80,
    EC_InvertDisableA = 0x40,
    EC_InvertDisableB = 0x20,
    EC_Lock = 0x10,
    EC_Mode = 0x07,
};

static UBYTE Emu_Control = 0;

enum {
    EC2_DisableWinB = 0x02,
    EC2_DisableWinA = 0x01
};

static UBYTE Emu_Control2 = 0;
static UBYTE Emu_BankA = 0;
static UBYTE Emu_BankB = 0;
static UBYTE Emu_Bank = 0;
static UBYTE Emu_Data = 0;

static UWORD Cart_Id = 0;
static int Left_Window_Active = FALSE;
static int Left_Window_Enabled = FALSE;
static int Right_Window_Enabled = FALSE;
static int CCTL_Enabled = FALSE;

static UBYTE Send_Buffer[520];
static UBYTE Response_Buffer[520];

static IDEEmu *ide;
static void *rtc;
static FlashEmu *flash;

static void LoadNVRAM();
static void Reset_Cart_Bank(void);
static void SaveNVRAM();
static void Set_SDX_Bank(int bank, int topEnable);
static void Set_Top_Bank(int bank, int topLeftEnable, int topRightEnable);
static void Update_IDE_Reset(void);
static void Update_Memory_Layers_Cart(void);

#ifdef ATARI800MACX
void init_side3(void)
#else
static void init_side3(void)
#endif
{
    Log_print("Side3 enabled");
    if (!Atari800_LoadImage(side3_rom_filename, side3_rom, 0x80000)) {
        SIDE2_have_rom = FALSE;
        Log_print("Couldn't load Side3 ROM image");
        return;
    }
    else {
        SIDE3_have_rom = TRUE;
        Log_print("loaded Side3 rom image");
    }
    
    ide = IDE_Init();
    IDE_Open_Image(ide, side3_sd_card_filename);
    if (ide->Disk == NULL)
    {
        Log_print("Couldn't attach Side3 SD Image");
        side3_sd_card_filename[0] = 0;
        SIDE3_Block_Device = FALSE;
    }
    else {
        Log_print("Attached Side3 SD Image");
        SIDE2_Block_Device = TRUE;
    }
        
    flash = Flash_Init(side3_rom, Flash_TypeMX29LV640DT);
}

void SIDE2_Remove_Block_Device(void)
{
    if (SIDE3_Block_Device) {
        IDE_Close_Image(ide);
        side3_compact_flash_filename[0] = 0;
        SIDE3_Block_Device = FALSE;
        Update_IDE_Reset();
    }
}

int SIDE3_Add_Block_Device(char *filename) {
    if (ide == NULL ) {
        ide = IDE_Init();
    }
    if (SIDE2_Block_Device)
        SIDE2_Remove_Block_Device();
    IDE_Open_Image(ide, filename);
    if (ide->Disk == NULL)
    {
        Log_print("Couldn't attach Side2 CF Image");
        SIDE2_Block_Device = FALSE;
    }
    else {
        Log_print("Attached Side2 CF Image");
        strcpy(side2_compact_flash_filename, filename);
        SIDE2_Block_Device = TRUE;
        Update_IDE_Reset();
    }
    return SIDE2_Block_Device;
}

int SIDE3_Change_Rom(char *filename, int new) {
    int romLoaded;

    SaveNVRAM();
    romLoaded = Atari800_LoadImage(filename, side3_rom, 0x80000);
    if (!romLoaded) {
        SIDE3_have_rom = FALSE;
        side3_compact_flash_filename[0] = 0;
    } else {
        SIDE3_have_rom = TRUE;
        if (new)
            strcpy(side3_rom_filename, filename);
        strcpy(side3_nvram_filename, side3_rom_filename);
        UTIL_strip_ext(side3_nvram_filename);
        strcat(side3_nvram_filename,".nvram");
        LoadNVRAM();
        if (flash == NULL) {
            flash = Flash_Init(side3_rom, Flash_TypeMX29LV640DT);
        }
    }
    
    return romLoaded;
}

int SIDE3_Save_Rom(char *filename)
{
    FILE *f;

    f = fopen(filename, "wb");
    if (f != NULL) {
        fwrite(side3_rom, 1, 0x80000, f);
        fclose(f);
        return 0;
    } else {
        return -1;
    }
}

int SIDE3_Initialise(int *argc, char *argv[])
{
    init_side3();
    rtc = MCP7951X_Init();
    LoadNVRAM();

    return TRUE;
}

void SIDE3_Exit(void)
{
    SaveNVRAM();
    MCP7951X_Init(rtc);
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
    UBYTE addr8 = (UBYTE)addr;
    int rval = -1;

    // If the CCTL RAM aperture and cartridge emulation are both active, they
    // can both respond to the read, by returning data from RAM but also switching
    // banks.
    if (addr8 < 0x80 && Aperture_Enable)
        rval = RAM[0x1FF500 + addr8];

    // Locked register state only affects writes; it does not block reads
    // from the SIDE3 register set.
    if ((Emu_Control & 0x80) && (addr8 < 0xF0 || Emu_Locked)) {
        OnEmuBankAccess(addr, -1);
    }

    if (addr8 < 0xF0)
        return rval;

    // === $D5FC-D5FF: Common registers ===
    if (addr8 >= 0xFC) {
        switch(addr8 & 0x03) {
            // === $D5FC: Misc status/control ===
            case 0:
                return (UBYTE)Register_Mode
                    + (Emu_Cart_Enable_Requested ? 0x04 : 0)
                    + (Button_PBI_Mode ? 0x08 : 0)
                    + (Cold_Start_Flag ? 0x10 : 0)
                    + (LED_Green_Manual_Enabled ? 0x20 : 0)
                    + (SDX_Enable ? 0x40 : 0)
                    + (Button_Pressed ? 0x80 : 0)
                    ;

            // === $D5FD-D5FF: Signature ===
            case 1: return Aperture_Enable ? 'R' : 'S';
            case 2: return 'S';
            case 3: return 'D';
        }
    }

    switch(Register_Mode) {
        case Register_Mode_Primary:
            switch(addr8 & 0x0F) {
                case 0x0:   return LED_Brightness;

                // $D5F1 primary set - open bus
                // $D5F2 primary set - open bus

                // === $D5F3-D5F5: SD/RTC card control ===
                case 0x3:
                    return SD_Status + (CPU_cycle_count >= SD_Next_Transfer_Time ? 0x00 : 0x02);

                case 0x4:
                    if (CPU_cycle_count >= SD_Next_Transfer_Time) {
                        UBYTE v = SD_Next_Read;

                        if (SD_Status & 0x08) {
                            AdvanceSPIRead();
                        }

                        return v;
                    } else {
                        // We are reading the shifter while it is operating, so compute the number of
                        // bits to shift and blend the previous and next bytes. In fast mode, this is
                        // only possible for an offset of 0, so we can just use the same calc as for
                        // slow mode. The shift direction is left (MSB-first), but we are computing
                        // the opposite shift.

                        int bitsRemaining = ((ULONG)(SD_Next_Transfer_Time - t) + 3) >> 2;

                        return (((ULONG)SD_Prev_Read << 8) + SD_Next_Read) >> bitsRemaining;
                    }
                    break;

                case 0x5:   return SD_CRC7 | 1;

                // === $D5F6-D5FB: Banking registers ===
                case 0x6:   return Bank_RAM_A;
                case 0x7:   return Bank_RAM_B;
                case 0x8:   return Bank_Flash_A;
                case 0x9:   return Bank_Flash_B;
                case 0xA:   return Bank_Control;
                case 0xB:   return Bank_RAM_Write_Protect;
            }
            break;

        case Register_Mode_DMA:
            switch(addr8 & 0x0F) {
                case 0x0: return ((DMA_Src_Address >> 16) & 0x1F) + 
                                  (DMA_From_SD ? 0x00 : 0x20) + 
                                  (DMA_Active ? 0x80 : 0x00);
                case 0x1: return (DMA_Src_Address >> 8) & 0xFF;
                case 0x2: return (DMA_Src_Address >> 0) & 0xFF;
                case 0x3: return (DMA_Dst_Address >> 16) & 0x1F;
                case 0x4: return (DMA_Dst_Address >> 8) & 0xFF;
                case 0x5: return (DMA_Dst_Address >> 0) & 0xFF;
                case 0x6: return (DMA_Counter >> 8) & 0xFF;
                case 0x7: return (DMA_Counter >> 0) & 0xFF;
                case 0x8: return DMA_Src_Step & 0xFF;
                case 0x9: return DMA_Dst_Step & 0xFF;
                case 0xA: return DMA_And_Mask;
                case 0xB: return DMA_Xor_Mask;
            }
            break;

        case Register_Mode_CartridgeEmu:
            switch(addr8 & 0xF) {
                case 0x0: return Emu_CCTL_Base;
                case 0x1: return Emu_CCTL_Mask;
                case 0x2: return Emu_Address_Mask;
                case 0x3: return Emu_Data_Mask;
                case 0x4: return Emu_Disable_Mask_A;
                case 0x5: return Emu_Disable_Mask_B;
                case 0x6: return Emu_Feature;
                case 0x7: return Emu_Control;
                case 0x8: return Emu_BankA;
                case 0x9: return Emu_BankB;
                case 0xA: return Emu_Control2;
                case 0xB: return Emu_Data;

                default:
                    break;
            }
            break;

        default:
            // all registers in the reserved set return $00, even on floating
            // bus systems
            return 0x00;
    }

    return result;
}

void SIDE2_D5PutByte(UWORD addr, UBYTE byte)
{
    UBTYE addr8 = (UBTYE)addr;

    // If both the CCTL RAM aperture and cartridge emulation are enabled, both
    // must be able to handle the access.
    if (addr8 < 0x80 && Aperture_Enable) {
        if (!(Bank_RAM_Write_Protect & 0x04))
            RAM[0x1FF500 + addr8] = value;
    }

    // The register file and the cartridge emulator are mutally exclusive for
    // the $D5F0-D5FF range. If the lock is disabled, only the register file
    // will respond to the write, and if the lock is enabled, only the
    // cartridge emulator will.
    if ((Emu_Control & 0x80) && (addr8 < 0xF0 || Emu_Locked)) {
        OnEmuBankAccess(addr, value);
        return true;
    }

    if (addr8 < 0xF0)
        return false;

    // === $D5FC-D5FF: Common registers ===
    if (addr8 >= 0xFC) {
        switch(addr8 & 3) {
            case 0:
                // D7: button status (1 = pressed; can only be cleared)
                // D6: SDX switch state (1 = down)
                // D5: Green LED manual control (1 = on if SD power off, 0 = on if SD power on)
                // D4: Cold start flag (1 on power-up, can only be changed to 0)
                // D3: PBI button mode enabled (1 = inhibit reset)
                // D2: Internal cartridge pass-through enable (1 = enabled; SDX enabled only)
                // D1-D0: register mode for $D1F0-D1FC

                Register_Mode = (RegisterMode)(value & 3);

                Button_PBI_Mode = (value & 0x08) != 0;

                if (!(value & 0x80))
                    Button_Pressed = FALSE;

                if (!(value & 0x10))
                    Cold_Start_Flag = FALSE;

                if (LED_Green_Manual_Enabled != ((value & 0x20) != 0)) {
                    LED_Green_Manual_Enabled = led;

                    UpdateLEDGreen();
                }

                Emu_Cart_Enable_Requested = (value & 0x04) != 0;
                if (UpdateEmuCartEnabled())
                    UpdateWindows();
                break;

            case 1:
                Aperture_Enable = (value & 0x40) != 0;
                break;

            default:
                break;
        }

        return true;
    }

    switch(RegisterMode) {
        case Register_Mode_Primary:
            switch(addr8 & 0x0F) {

                case 0x0:
                    if (LED_Brightness != value) {
                        LED_Brightness = value;
                        UpdateLEDIntensity();
                    }
                    break;

                // $D5F3: SD card control
                // Bits that can be changed:
                //  - D7=1 SD card present (can't be changed if no SD card)
                //  - D5=1 enable SD power
                //  - D4=1 use fast SD transfer clock
                //  - D3=1 SD SPI freerun
                //  - D2=1 RTC select
                //  - D0=1 SD select
                case 0x3:
                    {
                        // If there is no SD card, bit 7 can't be set. Doesn't matter if
                        // SD is powered. Can't be reset by a write.
                        if (!Block_Device)
                            value &= 0x7F;
                        else
                            value |= (SD_Status & 0x80);

                        UBTYE delta = SD_Status ^ value;

                        if (delta & 0x04)
                            MCP7951X_Reselect(rtc);

                        SD_Status = (SD_Status & 0x42) + (value & 0xBD);

                        // The SD checksum register is cleared on any write to this
                        // register, even if nothing has changed and SD power is off.
                        SD_CRC7 = 0;

                        if (delta & 0x20)
                            UpdateLEDGreen();
                    }
                    break;

                case 0x4:
                    // The shift register is shared for both input and output, with
                    // the input byte being shifted in as the output byte is shifted out.
                    // Where this makes a difference is in free-running mode, where the
                    // output is ignored and forced to 1, but the CRC-7 logic does see
                    // the now input bytes being shifted through.
                    SD_Next_Read = value;
                    WriteSPI(value);
                    break;

                // === $D5F6-D5FB: Banking registers ===
                case 0x6:
                    if (Bank_RAM_A != value) {
                        Bank_RAM_A = value;

                        if ((Bank_Control & BC_EnableAnyA) == BC_EnableMemA)
                            UpdateWindowA();
                    }
                    break;

                case 0x7:
                    if (Bank_RAM_B != value) {
                        Bank_RAM_B = value;

                        if ((Bank_Control & BC_EnableAnyB) == BC_EnableMemB)
                            UpdateWindowB();
                    }
                    break;

                case 0x8:
                    Bank_Flash_A = value;
                    ULONG flash_Offset_A = ((ULONG)Bank_Flash_A << 13) + ((Bank_Control & BC_HighBankA) << 17);
                    if (Flash_Offset_A != flash_Offset_A)
                    {
                        Flash_Offset_A = flash_Offset_A;

                        if (Bank_Control & BC_EnableFlashA)
                            UpdateWindowA();
                    }
                    break;

                case 0x9:
                    Bank_Flash_B = value;
                    ULONG flash_Offset_B = ((ULONG)Bank_Flash_B << 13) + ((Bank_Control & BC_HighBankB) << 15);
            
                    if (Flash_Offset_B != flash_Offset_B)
                    {
                        Flash_Offset_B = flash_Offset_B;

                        if (Bank_Control & BC_EnableFlashB)
                            UpdateWindowB();
                    }
                    break;

                case 0xA:
                    if (Bank_Control != value) {
                        Bank_Control = value;

                        UpdateWindows();
                    }
                    break;

                case 0xB:
                    value &= 0x07;

                    UBYTE delta = Bank_RAM_Write_Protect ^ value
                    if (delta) {
                        Bank_RAM_Write_Protect = value;

                        // only need window updates for A and B, as the CCTL window is part of the control
                        // layer that is always enabled
                        if (delta & 3) {
                            if (BankControl & (BC_EnableMemA | BC_EnableMemB))
                                UpdateWindows();
                        }
                    }
                    break;
            }
            break;

        case Register_Mode_DMA:
            AdvanceDMA();

            switch(addr8 & 0x0F) {
                case 0x0:
                    DMA_Src_Address = ((ULONG)(value & 0x1F) << 16) + (DMA_Src_Address & 0x00FFFF);
                    DMA_From_SD = !(value & 0x20);

                    if (value & 0x80) {
                        if (!DMA_Active) {
                            DMA_Active = TRUE;
                            DMA_Last_Cycle = CPU_cycle_count;
                            DMA_State = 0;

                            // These registers are double-buffered. The step and AND/XOR values are not.
                            DMA_Active_Src_Address = DMA_Src_Address;
                            DMA_Active_Dst_Address = DMA_Dst_Address;
                            DMA_Active_Bytes_Left = (ULONG)DMA_Counter + 1;

                            if (g_ATLCSIDE3DMA.IsEnabled()) {
                                if (DMA_From_SD) {
                                    printf("$%06X [%c$%02X] <- SD Len=$%05X And=$%02X Xor=$%02X\n"
                                        , DMA_Dst_Address
                                        , (SLONG)DMA_Dst_Step < 0 ? '-' : '+'
                                        , abs((SLONG)DMA_Dst_Step)
                                        , (ULONG)DMA_Counter + 1
                                        , DMA_And_Mask
                                        , DMA_Xor_Mask
                                    );
                                } else {
                                    printf("$%06X [%c$%02X] <- $%06X [%c$%02X] Len=$%05X And=$%02X Xor=$%02X\n"
                                        , DMA_Dst_Address
                                        , (SLONG)DMA_Dst_Step < 0 ? '-' : '+'
                                        , abs((SLONG)DMA_Dst_Step)
                                        , DMA_Src_Address
                                        , (SLONG)DMA_Src_Step < 0 ? '-' : '+'
                                        , abs((SLONG)DMA_Src_Step)
                                        , (ULONG)DMA_Counter + 1
                                        , DMA_And_Mask
                                        , DMA_Xor_Mask
                                    );
                                }
                            }
                        }
                    } else {
                        DMA_Active = FALSE;
                    }

                    break;
                case 0x1: DMA_Src_Address = ((ULONG)(value       ) <<  8) + (DMA_Src_Address & 0x1F00FF); break;
                case 0x2: DMA_Src_Address = ((ULONG)(value       ) <<  0) + (DMA_Src_Address & 0x1FFF00); break;
                case 0x3: DMA_Dst_Address = ((ULONG)(value & 0x1F) << 16) + (DMA_Dst_Address & 0x00FFFF); break;
                case 0x4: DMA_Dst_Address = ((ULONG)(value       ) <<  8) + (DMA_Dst_Address & 0x1F00FF); break;
                case 0x5: DMA_Dst_Address = ((ULONG)(value       ) <<  0) + (DMA_Dst_Address & 0x1FFF00); break;
                case 0x6:
                    DMA_Counter = ((ULONG)value <<  8) + (DMA_Counter & 0x00FF);
                    break;
                case 0x7:
                    DMA_Counter = ((ULONG)value <<  0) + (DMA_Counter & 0xFF00);
                    break;
                case 0x8: DMA_Src_Step = (ULONG)(SBYTE)value; break;
                case 0x9: DMA_Dst_Step = (ULONG)(SBYTE)value; break;
                case 0xA: DMA_And_Mask = value; break;
                case 0xB: DMA_Xor_Mask = value; break;
            }

            RescheduleDMA();
            break;

        case Register_Mode_CartridgeEmu:
            switch(addr8 & 0xF) {
                case 0x0:
                    Emu_CCTL_Base = value;
                    break;

                case 0x1:
                    Emu_CCTL_Mask = value;
                    break;

                case 0x2:
                    Emu_Address_Mask = value;
                    break;

                case 0x3:
                    Emu_Data_Mask = value;
                    break;

                case 0x4:
                    Emu_Disable_Mask_A = value;
                    break;

                case 0x5:
                    Emu_Disable_Mask_B = value;
                    break;

                case 0x6:
                    if (Emu_Feature != value) {
                        Emu_Feature = value;
                        UpdateWindows();
                    }
                    break;

                case 0x7:
                    // bit 3 doesn't exist and is always zero
                    value &= 0xF7;

                    // bit 4 cannot be set unless cart emu is enabled
                    if (!(value & EC_Enable))
                        value &= ~EC_Lock;

                    UBYTE delta = Emu_Control ^ value;
                    if (delta) {
                        // if we are turning on cart emu, we need to reset banking state
                        if (delta & value & EC_Enable) {
                            Emu_Bank = Emu_Bank_A;
                            mbEmuDisabledA = (mEmuControl2 & kEC2_DisableWinA) != 0;
                            mbEmuDisabledB = (mEmuControl2 & kEC2_DisableWinB) != 0;
                        }

                        Emu_Locked = (value & EC_Lock) != 0;

                        Emu_Control = value;
                        UpdateWindows();
                    }
                    break;

                case 0x8:
                    if (Emu_Bank_A != value) {
                        Emu_Bank_A = value;
                        UpdateWindows();
                    }
                    break;

                case 0x9:
                    if (Emu_Bank_B != value) {
                        Emu_Bank_B = value;
                        UpdateWindows();
                    }
                    break;

                case 0xA:
                    value &= 0x03;
                    if (Emu_Control2 != value) {
                        Emu_Control2 = value;
                        UpdateWindows();
                    }
                    break;

                case 0xB:
                    Emu_Data = value;
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }

    return TRUE;

}

void UpdateWindows() {
    UpdateWindowA();
    UpdateWindowB();
}

void UpdateWindowA() {
    // compute RD4 enable
    int enabled = FALSE;

    if (Right_Window_Enabled) {
        if ((Emu_Control & EC_Enable) && Emu_Cart_Enable_Effective) {
            switch(Emu_Control & EC_Mode) {
                case 0:     // 8/16K mode
                    enabled = Emu_Disabled_A;
                    break;

                case 1:     // Williams
                default:
                    enabled = FALSE;
                    break;

                case 2:     // XEGS/BB mode
                case 4:     // MegaCart mode
                    enabled = Emu_Disabled_A;

                    if (EmuControl & EC_Invert_Disable_A)
                        enabled = !enabled;
                    break;
            }
        } else {
            enabled = (Bank_Control & BC_EnableAnyA) != 0;
        }
    }

    // upstream cartridge mask
    if (!enabled) {
        mpMemMan->EnableLayer(mpMemLayerWindowA, false);
        mpMemMan->EnableLayer(mpMemLayerWindowA2, false);
        mpMemMan->EnableLayer(mpMemLayerFlashControlA, false);
        mpMemMan->EnableLayer(mpMemLayerSpecialBank1, false);
        mpMemMan->EnableLayer(mpMemLayerSpecialBank2, false);

        const bool bbsbSnoop = (mEmuControl & kEC_Enable) && (mEmuFeature & kEF_BountyBob);
        const ATMemoryAccessMode bbsbLayerMode = bbsbSnoop ? kATMemoryAccessMode_W : kATMemoryAccessMode_0;
        mpMemMan->SetLayerModes(mpMemLayerSpecialBank1, bbsbLayerMode);
        mpMemMan->SetLayerModes(mpMemLayerSpecialBank2, bbsbLayerMode);
        return;
    }

    int useFlash = (Bank_Control & BC_EnableFlashA) != 0;
    int splitBanks = false;

    if (Emu_Control & EC_Enable) {
        mpMemMan->EnableLayer(mpMemLayerFlashControlA, false);

        UBYTE effectiveBank = 0;

        switch(Emu_Control & EC_Mode) {
            case 0:     // 8/16K mode
            default:
                effectiveBank = EmuBankA;
                break;
            case 1:     // Williams mode
                effectiveBank = 0;
                break;
            case 2:     // XEGS/BB mode
                effectiveBank = EmuBank;
                break;
            case 4:     // MegaCart mode
                effectiveBank = EmuBank << 1;
                break;
        }

        const bool readOnly = (BankRAMWriteProtect & 1) != 0;
        splitBanks = (EmuControl & EC_Mode) == 2 && (EmuFeature & EF_BountyBob);

        if (splitBanks) {
            WindowOffsetA2 = ((ULONG)(effectiveBank >> 4) << 13) + 0x21000;

            effectiveBank &= 15;
        }

        WindowOffsetA1 = (ULONG)effectiveBank << 13;

        if (!splitBanks)
            WindowOffsetA2 = WindowOffsetA1 + 0x1000;

        const ATMemoryAccessMode bbsbLayerMode = (mEmuFeature & kEF_BountyBob) ? kATMemoryAccessMode_ARW : kATMemoryAccessMode_0;
        mpMemMan->SetLayerModes(mpMemLayerSpecialBank1, bbsbLayerMode);
        mpMemMan->SetLayerModes(mpMemLayerSpecialBank2, bbsbLayerMode);
    } else {
        if (useFlash) {
            WindowOffsetA1 = FlashOffsetA;
        } else {
            WindowOffsetA1 = (ULONG)BankRAMA << 13;
        }

        WindowOffsetA2 = WindowOffsetA1 + 0x1000;
        mpMemMan->EnableLayer(mpMemLayerSpecialBank1, false);
        mpMemMan->EnableLayer(mpMemLayerSpecialBank2, false);
    }

    if (!splitBanks) {
        mpMemMan->EnableLayer(mpMemLayerWindowA2, false);
    }

    WindowUsingFlashA = useFlash;
    if (useFlash) {
        const bool enableFlashControlReadA = mFlashCtrl.IsControlReadEnabled();

        mpMemMan->SetLayerModes(mpMemLayerFlashControlA, enableFlashControlReadA ? kATMemoryAccessMode_ARW : kATMemoryAccessMode_W);
        mpMemMan->SetLayerMemory(mpMemLayerWindowA, Flash + WindowOffsetA1);

        if (splitBanks)
            mpMemMan->SetLayerMemory(mpMemLayerWindowA2, Flash + WindowOffsetA2);
    } else {
        mpMemMan->EnableLayer(mpMemLayerFlashControlA, false);
        mpMemMan->SetLayerMemory(mpMemLayerWindowA, RAM + WindowOffsetA1);

        if (splitBanks)
            mpMemMan->SetLayerMemory(mpMemLayerWindowA2, RAM + WindowOffsetA2);
    }

    const bool readOnly = (BankRAMWriteProtect & 1) != 0;

    if (splitBanks) {
        mpMemMan->EnableLayer(mpMemLayerWindowA2, true);
        mpMemMan->SetLayerReadOnly(mpMemLayerWindowA2, readOnly);
    }

    mpMemMan->SetLayerReadOnly(mpMemLayerWindowA, readOnly);

    mpMemMan->EnableLayer(mpMemLayerWindowA, true);
}

void UpdateWindowB() {
    // compute RD5 and RAM/ROM enables
    int enabled = FALSE;
    if ((mEmuControl & kEC_Enable) && mbEmuCartEnableEffective) {
        enabled = !mbEmuDisabledB;

        if (mEmuControl & 7) {
            if (mEmuControl & kEC_InvertDisableB)
                enabled = !enabled;
        }
    } else {
        enabled = (mBankControl & kBC_EnableAnyB) != 0;
    }

    // these must not be gated by mbLeftWindowEnabled
    mbLeftWindowActive = enabled;
    mpCartridgePort->OnLeftWindowChanged(mCartId, enabled);

    // upstream cartridge mask
    if (!enabled) {
        mpMemMan->EnableLayer(mpMemLayerWindowB, false);
        mpMemMan->EnableLayer(mpMemLayerFlashControlB, false);
        return;
    }

    const bool useFlash = (mBankControl & kBC_EnableFlashB) != 0;

    if (mEmuControl & kEC_Enable) {
        mpMemMan->EnableLayer(mpMemLayerFlashControlB, false);

        uint8 effectiveBank = 0;

        switch(mEmuControl & 7) {
            case 0:     // 8/16K mode
            default:
                effectiveBank = mEmuBankB;
                break;
            case 1:     // Williams mode
                effectiveBank = mEmuBank;
                break;
            case 2:     // XEGS/BB mode
                effectiveBank = mEmuBankB;
                break;
            case 4:     // MegaCart mode
                effectiveBank = (mEmuBank << 1) + 1;
                break;
        }

        mpMemMan->SetLayerReadOnly(mpMemLayerWindowB, (mBankRAMWriteProtect & 2) != 0);

        const uint32 offset = (uint32)effectiveBank << 13;
        mWindowOffsetB = offset;
    } else {
        if (useFlash)
            mWindowOffsetB = mFlashOffsetB;
        else
            mWindowOffsetB = (uint32)mBankRAMB << 13;
    }

    if (useFlash) {
        const bool enableFlashControlReadB = mFlashCtrl.IsControlReadEnabled();

        mpMemMan->SetLayerModes(mpMemLayerFlashControlB, enableFlashControlReadB ? kATMemoryAccessMode_ARW : kATMemoryAccessMode_W);
        mpMemMan->SetLayerMemory(mpMemLayerWindowB, mFlash + mWindowOffsetB);
    } else {
        mpMemMan->EnableLayer(mpMemLayerFlashControlB, false);
        mpMemMan->SetLayerMemory(mpMemLayerWindowB, mRAM + mWindowOffsetB);
        mpMemMan->SetLayerReadOnly(mpMemLayerWindowB, (mBankRAMWriteProtect & 2) != 0);
    }

    mpMemMan->EnableLayer(mpMemLayerWindowB, true);
}

SLONG OnSpecialReadByteA1(ULONG addr) {
    UBYTE addr8 = (UBYTE)addr;
    UBYTE v;

    if (Window_Using_Flash_A) {
        if (Flash_Is_Control_Read_Enabled(flash)) {
            v = OnFlashReadA(addr);
        } else
            v = side3_rom[Window_Offset_A1 + 0x0F00 + addr8];
    } else {
        v = side3_RAM[(Window_Offset_A1 + 0x0F00 + addr8) & ((sizeof (RAM)) - 1)];
    }

    if (addr8 >= 0xF6 && addr8 < 0xFA)
        OnEmuBankAccess(addr, v);

    return v;
}

int OnSpecialWriteByteA1(ULONG addr, UBYTE value) {
    UBYTE addr8 = (UBYTE)addr;

    if (Window_Using_Flash_A)
        OnFlashWriteA(addr, value);
    else if (!(Bank_RAM_Write_Protect & 1))
        side_RAM[(Window_Offset_A1 & ((sizeof RAM)-1)) + 0xF00 + addr8] = value;

    if (addr8 >= 0xF6 && addr8 < 0xFA)
        OnEmuBankAccess(addr, value);

    return TRUE;
}

SLONG OnSpecialReadByteA2(ULONG addr)  {
    UBYTE addr8 = (UBYTE)addr;
    UBYTE v = side3_RAM[(Window_Offset_A2 + 0xF00 + addr8) & ((sizeof(RAM)) - 1)];

    if (Window_Using_Flash_A) {
        if (Flash_Is_Control_Read_Enabled(flash)) {
            v = OnFlashReadA(addr);
        } else
            v = side3_rom[Window_Offset_A2 + 0x0F00 + addr8];
    }

    if (addr8 >= 0xF6 && addr8 < 0xFA)
        OnEmuBankAccess(addr, v);

    return v;
}

int OnSpecialWriteByteA2(uint32 addr, uint8 value) {
    UBYTE addr8 = (UBYTE)addr;

    if (Window_Using_Flash_A)
        OnFlashWriteA(addr, value);
    else if (!(Bank_RAM_Write_Protect & 1))
        RAM[(Window_Offset_A2 & ((sizeof (RAM))-1)) + 0xF00 + addr8] = value;

    if (addr8 >= 0xF6 && addr8 < 0xFA)
        OnEmuBankAccess(addr, value);

    return TRUE;
}

void OnEmuBankAccess(ULONG addr, SLONG value) {
    // apply CCTL address mask
    if (addr >= 0xD500 && (Emu_Feature & EF_EnableCCTLMask)) {
        if ((addr & Emu_CCTL_Mask) != Emu_CCTL_Mask)
            return;
    }

    // check if we are responding to addresses or data
    UBTYE bank;
    UBTYE bankMask;
    if (Emu_Feature & EF_UseAddress) {
        bank = addr;
        bankMask = Emu_Address_Mask;
    } else {
        // drop reads
        if (value < 0)
            return;

        bank = (UBYTE)value;
        bankMask = Emu_Data_Mask;
    }

    int disabledA = (bank & Emu_Disable_Mask_A) && (EmuFeature & EF_EnableDisA);
    int disabledB;

    if (EmuFeature & EF_DisableByData) {
        disabledB = (Emu_Feature & EF_EnableDisB) && bank == bankMask;
    } else {
        disabledB = (bank & Emu_Disable_Mask_B) && (Emu_Feature & EF_EnableDisB);
    }
    
    bank &= bankMask;

    if (Emu_Feature & EF_BountyBob) {
        if (addr & 0x1000)
            bank = (Emu_Bank & 0x0F) + ((bank & 0x0F) << 4);
        else
            bank = (Emu_Bank & 0xF0) + (bank & 0x0F);
    }

    if (Emu_Bank != bank || Emu_Disabled_A != disabledA || Emu_Disabled_B != disabledB) {
        Emu_Disabled_A = disabledA;
        Emu_Disabled_B = disabledB;
        Emu_Bank = bank;

        UpdateWindows();
    }
}

SLONG OnFlashReadA(ULONG addr) {
    ULONG flashAddr = addr < 0x9000 ? Window_Offset_A1 + (addr - 0x8000) : Window_Offset_A2 + (addr - 0x9000);

    UBYTE value;
    if (Flash_Read_Byte(flash, flashAddr, value)) {
        UpdateWindows();
    }

    return value;
}

SLONG OnFlashReadB(ULONG addr) {
    UBYTE value;
    if (Flash_Read_Byte(flash, Window_Offset_B + (addr - 0xA000), value)) {
        UpdateWindows();
    }

    return value;
}

int OnFlashWriteA(ULONG addr, UWORD value) {
    UWORD flashAddr = addr < 0x9000 ? Window_Offset_A1 + (addr - 0x8000) : Window_Offset_A2 + (addr - 0x9000);

    if (Flash_Write_Byte(flash, flashAddr, value)) {
        tUpdateWindows();
    }

    return TRUE;
}

int OnFlashWriteB(ULONG addr, UBYTE value) {
    if (Flash_Write_Byte(flash, Window_Offset_B + (addr - 0xA000), value)) {
        UpdateWindows();
    }

    return TRUE;
}

void AdvanceSPIRead() {
    // queue next read if freerunning
    if (SD_Status & 0x08)
        WriteSPI(0xFF);
}

typedef struct crc7_table {
    UBYTE v[256];       // v[i] = ((i*x^8) mod (x^7 + x^3 + 1))*x
} CRC7Table;

typedef struct crc16_table {
    UWORD v[256];       // v[i] = (i*x^16) mod (x^16 + x^12 + x^5 + 1)
} CRC16Table;

static int crc7TableInited = FALSE;
static CRC7Table Crc7_Table;
static int crc16TableInited = FALSE;
static CRC16Table Crc16_Table;

void InitCRC7()
{
    for(ULONG i = 0; i < 256; ++i) {
        UBTYE v = i;

        for(ULONG j = 0; j < 8; ++j) {
            if (v >= 0x80)
                v ^= 0x89;

            v += v;
        }

        Crc7_Table.v[i] = v;
    }
}

void InitCRC16()
{
    for(int i=0; i<256; ++i) {
        UWORD crc = i << 8;

        for(int j=0; j<8; ++j)
            crc = (crc + crc) ^ (crc & 0x8000 ? 0x1021 : 0);

        Crc16_Table.v[i] = crc;
    }
}

UBYTE AdvanceCRC7(UBYTE crc, UBYTE v) {
    if (!crc7TableInited) {
        InitCRC7();
        crc7TableInited = TRUE;
    }

    return Crc7_Table.v[crc ^ v];
}

UBYTE ComputeCRC7(UBYTE crc, const void *data, size_t len) {
    const UBYTE *data8 = (const UBYTE *)data;

    if (!crc7TableInited) {
        InitCRC7();
        crc7TableInited = TRUE;
    }

    while(len--)
        crc = Crc7_Table.v[crc ^ *data8++];

    return crc;
}

UWORD ComputeCRC16(UWORD crc, const void *data, size_t len) {
    const UBYTE *data8 = (const UBYTE *)data;

    if (!crc16TableInited) {
        InitCRC16();
        crc16TableInited = TRUE;
    }

    while(len--)
        crc = (crc << 8) ^ Crc16_Table.v[(crc >> 8) ^ *data8++];

    return crc;
}

void WriteSPINoTimingUpdate(UBYTE v) {
    SD_Prev_Read = SD_Next_Read;

    if (SD_Status & 0x01)
        SD_Next_Read = TransferSD(v);
    else
        SD_Next_Read = TransferRTC(v);

    SD_CRC7 = AdvanceCRC7(SD_CRC7, SD_Prev_Read);
}

void WriteSPI(UBYTE v) {
    WriteSPINoTimingUpdate(v);

    ULONG transferDelay = SD_Status & 0x10 ? 1 : 32;
    SD_Next_Transfer_Time = CPU_cycle_count + transferDelay;

    EnableXferLED();
}

UBTYE TransferSD(UBTYE v) {
    if (!Block_Device)
        return 0xFF;

    UBTYE reply = 0xFF;

    if (SD_Response_Index < SD_Response_Length) {
        reply = Response_Buffer[SD_Response_Index++];

        if (SD_Response_Index >= SD_Response_Length) {
            switch(SD_Active_Command_Mode) {
                case SD_Mode_None:
                    break;

                case SD_Mode_ReadMultiple:
                    if (SD_Active_Command_LBA + 1 >= Block_Device->GetSectorCount()) {
                        Response_Buffer[0] = 0x08;
                        SD_Response_Index = 0;
                        SD_Response_Length = 1;
                    } else {
                        if (DMA_Active && DMA_From_SD)
                            printf("Read multiple LBA $%04X (DMA length remaining $%04X)\n", SD_Active_Command_LBA, DMA_Active_Bytes_Left);
                        else
                            printf("Read multiple LBA $%04X\n", SD_Active_Command_LBA);

                        if (SetupRead(SD_Active_Command_LBA, FALSE))
                            ++SD_Active_Command_LBA;
                        else
                            SD_Active_Command_Mode = SD_Mode_None;
                    }
                    break;
            }
        }
    }

    switch(SD_Active_Command_Mode) {
        case SD_Mode_None:
        case SD_Mode_ReadMultiple:
            if (SD_Command_State < 6) {
                // for the first byte, check the start and transmitter bits (01)
                if (SD_Command_State > 0 || (v & 0xC0) == 0x40)
                    SD_Command_Frame[SD_Command_State++] = v;

                // for the last byte, check if we got a stop bit, and toss the frame
                // if not
                if (SD_Command_State == 6 && !(v & 0x01))
                    SD_Command_State = 0;
            }

            if (SD_Command_State >= 6) {
                SD_Command_State = 0;

                // verify CRC7 if we are in SD mode still or CMD8 is being sent, as
                // CMD8 is always CRC checked
                bool commandValid = true;

                if (SD_CRC_Enabled || (SD_Command_Frame[0] & 0x3F) == 8) {
                    uint8 crc7 = 0;

                    for(uint32 i = 0; i < 5; ++i)
                        crc7 = AdvanceCRC7(crc7, SD_Command_Frame[i]);

                    crc7 |= 1;

                    if (crc7 != SD_Command_Frame[5]) {
                        printf("Dropping command frame with bad CRC7: %02X %02X %02X %02X %02X %02X (expected %02X)\n"
                            , SD_Command_Frame[0]
                            , SD_Command_Frame[1]
                            , SD_Command_Frame[2]
                            , SD_Command_Frame[3]
                            , SD_Command_Frame[4]
                            , SD_Command_Frame[5]
                            , crc7);

                        commandValid = false;
                    }
                }

                if (commandValid) {
#ifdef SIDE3_DEBUG
                        int cmd = SD_Command_Frame[0] & 0x3F;
                        char *cmdname = NULL;

                        if (SD_App_Command) {
                            switch(cmd) {
                                case 41: cmdname = "SD send op cond"; break;
                                case 55: cmdname = "App cmd"; break;
                            }
                        } else {
                            switch(cmd) {
                                case  0: cmdname = "Go idle state"; break;
                                case  8: cmdname = "Send IF cond"; break;
                                case  9: cmdname = "Send CSD"; break;
                                case 10: cmdname = "Send CID"; break;
                                case 12: cmdname = "Stop transmission"; break;
                                case 17: cmdname = "Read single block"; break;
                                case 18: cmdname = "Read multiple block"; break;
                                case 23: cmdname = "Set block count"; break;
                                case 24: cmdname = "Write block"; break;
                                case 25: cmdname = "Write multiple block"; break;
                                case 55: cmdname = "App cmd"; break;
                                case 58: cmdname = "Read OCR"; break;
                                case 59: cmdname = "CRC on/off"; break;
                            }
                        }

                        printf("Command: %02X %02X %02X %02X %02X %02X (%sCMD%-2d%s%s%s)\n"
                            , SD_Command_Frame[0]
                            , SD_Command_Frame[1]
                            , SD_Command_Frame[2]
                            , SD_Command_Frame[3]
                            , SD_Command_Frame[4]
                            , SD_Command_Frame[5]
                            , SD_App_Command && cmd != 55 ? "A" : ""
                            , cmd
                            , mbSDAppCommand && cmd != 55 ? "" : " "
                            , cmdname ? " " : ""
                            , cmdname ? cmdname : ""
                        );
#endif

                    const UBYTE commandId = SD_Command_Frame[0] & 0x3F;

                    if (!SD_SPI_Mode) {
                        if (commandId == 0) {
                            printf("CMD0 received -- switching to SPI mode.\n");
                            SD_SPI_Mode = TRUE;
                            SD_CRC_Enabled = FALSE;

                            ResponseBuffer[0] = 0xFF;
                            ResponseBuffer[1] = 0x01;
                            SD_Response_Index = 0;
                            SD_Response_Length = 2;
                        }
                    } else if (SD_App_Command) {
                        SD_App_Command = false;

                        switch(commandId) {
                            case 41:    // ACMD41 (send op cond)
                                ResponseBuffer[0] = 0x00;
                                SD_Response_Index = 0;
                                SD_Response_Length = 1;
                                break;

                            case 55:    // CMD55 (app command) (even if already shifted)
                                SD_App_Command = TRUE;
                                ResponseBuffer[0] = 0x00;
                                SD_Response_Index = 0;
                                SD_Response_Length = 1;
                                break;

                            default:
                                // return illegal command
                                ResponseBuffer[0] = 0x04;
                                SD_Response_Index = 0;
                                SD_Response_Length = 1;
                                break;
                        }
                    } else if (SD_Active_Command_Mode == SD_Mode_None || commandId == 12) {
                        switch(commandId) {
                            case 0:     // CMD0 (go idle state)
                                ResponseBuffer[0] = 0x01;
                                SD_Response_Index = 0;
                                SD_Response_Length = 1;
                                break;

                            case 8:     // CMD8 (send interface condition)
                                ResponseBuffer[0] = 0x00;
                                ResponseBuffer[1] = 0x00;
                                ResponseBuffer[2] = 0x00;
                                ResponseBuffer[3] = SD_Command_Frame[3];    // voltage
                                ResponseBuffer[4] = SD_Command_Frame[4];    // check pattern
                                {
                                    uint8 crc7 = 0;

                                    for(int i=0; i<5; ++i)
                                        crc7 = AdvanceCRC7(crc7, ResponseBuffer[i]);

                                    ResponseBuffer[5] = crc7 | 1;
                                }
                                SD_Response_Index = 0;
                                SD_Response_Length = 6;
                                break;

                            case 9:     // CMD9 (send CSD)
                                // send R1 response
                                ResponseBuffer[0] = 0x00;

                                // start with a CSD 2.0 block for SDHC/SDXC
                                ResponseBuffer[ 1] = 0xFE;
                                ResponseBuffer[ 2] = 0b01000000;   // [127:120] CSD structure (01), reserved (000000)
                                ResponseBuffer[ 3] = 0b00001010;   // [119:112] TAAC = 0Eh
                                ResponseBuffer[ 4] = 0b00000000;   // [111:104] NSAC = 00h
                                ResponseBuffer[ 5] = 0b00110010;   // [103: 96] TRAN_SPEED = 32h
                                ResponseBuffer[ 6] = 0b01011011;   // [ 95: 88] CCC = 010110110101
                                ResponseBuffer[ 7] = 0b01011001;   // [ 87: 80] CCC con't, READ_BL_LEN = 9
                                ResponseBuffer[ 8] = 0b00000000;   // [ 79: 72] READ_BL_PARTIAL=0, WRITE_BLK_MISALIGN=0, READ_BLK_MISALIGN=0, DSR_IMP=0, reserved (4)
                                ResponseBuffer[ 9] = 0b00000000;   // [ 71: 64] reserved(2), C_SIZE=0 (TBD)
                                ResponseBuffer[10] = 0b00000000;   // [ 63: 56] C_SIZE cont'd
                                ResponseBuffer[11] = 0b00000000;   // [ 55: 48] C_SIZE cont'd
                                ResponseBuffer[12] = 0b01111111;   // [ 47: 40] reserved(1), SECTOR_SIZE=7Fh
                                ResponseBuffer[13] = 0b10000000;   // [ 39: 32] SECTOR_SIZE cont'd, WP_GRP_SIZE=0
                                ResponseBuffer[14] = 0b00001010;   // [ 31: 24] WP_GRP_ENABLE=0, reserved(2), R2W_FACTOR=010b, WRITE_BL_LEN=9
                                ResponseBuffer[15] = 0b01000000;   // [ 23: 16] WRITE_BL_LEN con'td, WRITE_BL_PARTIAL=0, reserved(5)
                                ResponseBuffer[16] = 0b00000000;   // [ 15:  8] FILE_FORMAT_GRP=0, COPY=0, PERM_WRITE_PROTECT=0, TMP_WRITE_PROTECT=0, FILE_FORMAT=0, reserved=0
                                ResponseBuffer[17] = 0b00000000;   // [  7:  0] CRC (TBD), 1-bit
                                
                                ULONG blockCount = BlockDevice->GetSectorCount(); 
                                if (SD_HC) {
                                    // set device size (units of 512KB - 1)
                                    ULONG sizeBlocks = ((blockCount + 1023) >> 10) - 1;
                                    ResponseBuffer[ 9] = (UBYTE)(sizeBlocks >> 16);
                                    ResponseBuffer[10] = (UBYTE)(sizeBlocks >>  8);
                                    ResponseBuffer[11] = (UBYTE)(sizeBlocks >>  0);
                                } else {
                                    // change CSD version to 1.0
                                    ResponseBuffer[2] = 0;

                                    // compute C_SIZE and C_SIZE_MULT
                                    // block count = (C_SIZE + 1) * 2^(C_SIZE_MULT + 2)
                                    // This means that minimum size = 8 blocks, maximum size = 4096*2^9 = 2097152 blocks
                                    ULONG base = blockCount;
                                    ULONG exp = 2;

                                    // if the card exceeds 1GB in size, we must specify the capacity in 1KB blocks instead
                                    if (blockCount > 2097152) {
                                        ++Response_Buffer[7];
                                        blockCount >>= 1;
                                    }

                                    while(base > 4096 && exp < 9) {
                                        base >>= 1;
                                        ++exp;
                                    }

                                    // C_SIZE is in [73:62], so we encode it <<6 and OR it into [79:56]
                                    const ULONG encodedCSize = (((base -1) < 4095) ? (base -1) : 4095) << 6;
                                    Response_Buffer[ 8] |= (UBYTE)(encodedCSize >> 16);
                                    Response_Buffer[ 9] |= (UBYTE)(encodedCSize >>  8);
                                    Response_Buffer[10] |= (UBYTE)(encodedCSize >>  0);

                                    // C_SIZE_MULT is in [49:47], so encode it <<7 and OR it into [55:40]
                                    const UBYTE encodedCSizeMult = (exp - 2) << 7;
                                    Response_Buffer[11] |= (UBYTE)(encodedCSizeMult >> 8);
                                    Response_Buffer[12] |= (UBYTE)encodedCSizeMult;
                                }

                                // set CRC7
                                Response_Buffer[17] = ComputeCRC7(0, Response_Buffer + 2, 15) | 1;

                                // set data frame CRC16
                                VDWriteUnalignedBEU16(&Response_Buffer[18], ComputeCRC16(0, Response_Buffer + 2, 16));

                                SD_Response_Index = 0;
                                SD_Response_Length = 20;
                                break;

                            case 10:    // CMD10 (send CID)
                                // send R1 response
                                Response_Buffer[0] = 0x00;

                                // start with a CSD 2.0 block for SDHC/SDXC
                                Response_Buffer[ 1] = 0xFE;
                                Response_Buffer[ 2] = 0x00;         // [127:120] MID=0
                                Response_Buffer[ 3] = 'X';          // [119:112] OID
                                Response_Buffer[ 4] = 'X';          // [111:104] OID cont'd
                                Response_Buffer[ 5] = 'S';          // [103: 96] PNM
                                Response_Buffer[ 6] = 'D';          // [ 95: 88] PNM cont'd
                                Response_Buffer[ 7] = 'E';          // [ 87: 80] PNM cont'd
                                Response_Buffer[ 8] = 'M';          // [ 79: 72] PNM cont'd
                                Response_Buffer[ 9] = 'U';          // [ 71: 64] PNM cont'd
                                Response_Buffer[10] = 0b00010000;   // [ 63: 56] PRV = 1.0
                                Response_Buffer[11] = 0b00000000;   // [ 55: 48] PSN = 0
                                Response_Buffer[12] = 0b00000000;   // [ 47: 40] PSN cont'd
                                Response_Buffer[13] = 0b00000000;   // [ 39: 32] PSN cont'd
                                Response_Buffer[14] = 0b00000000;   // [ 31: 24] PSN cont'd
                                Response_Buffer[15] = 0b00000001;   // [ 23: 16] reserved(), MDT = January 2020 (141h)
                                Response_Buffer[16] = 0b01000001;   // [ 15:  8] MDT cont'd
                                Response_Buffer[17] = 0b00000000;   // [  7:  0] CRC (TBD), 1-bit

                                // set CRC7
                                Response_Buffer[17] = ComputeCRC7(0, Response_Buffer + 1, 15) | 1;

                                // set CRC16
                                VDWriteUnalignedBEU16(&Response_Buffer[19], ComputeCRC16(0, Response_Buffer + 2, 16));

                                SD_Response_Index = 0;
                                SD_Response_Length = 20;
                                break;

                            case 12:    // CMD12 (stop transmission)
                                Response_Buffer[0] = 0xFF;
                                Response_Buffer[1] = 0x00;
                                SD_Response_Index = 0;
                                SD_Response_Length = 2;
                                SD_Active_Command_Mode = SD_Mode_None;
                                break;

                            case 17:    // CMD17 (read sector)
                                {
                                    UBYTE error = SetupLBA();

                                    if (error) {
                                        Response_Buffer[0] = 0xFF;
                                        Response_Buffer[1] = error;
                                        SD_Response_Index = 0;
                                        SD_Response_Length = 2;
                                    } else
                                        SetupRead(SD_Active_Command_LBA, TRUE);
                                }
                                break;

                            case 18:    // CMD18 (read multiple)
                                {
                                    UBYTE error = SetupLBA();

                                    Response_Buffer[0] = 0xFF;
                                    Response_Buffer[1] = error;
                                    SD_Response_Index = 0;
                                    SD_Response_Length = 2;

                                    if (!error)
                                        SD_Active_Command_Mode = SD_Mode_ReadMultiple;
                                }
                                break;

                            case 24:    // CMD24 (write block)
                                {
                                    UBYTE error = SetupLBA();

                                    Response_Buffer[0] = 0xFF;
                                    Response_Buffer[1] = error;
                                    SD_Response_Index = 0;
                                    SD_Response_Length = 2;

                                    if (!error)
                                        SD_Active_Command_Mode = SD_Mode_Write;
                                }
                                break;

                            case 25:    // CMD25 (write multiple block)
                                {
                                    UBYTE error = SetupLBA();

                                    Response_Buffer[0] = 0xFF;
                                    Response_Buffer[1] = error;
                                    SD_Response_Index = 0;
                                    SD_Response_Length = 2;

                                    if (!error)
                                        SD_Active_Command_Mode = SD_Mode_WriteMultiple;
                                }
                                break;

                            case 55:    // CMD55 (app command)
                                Response_Buffer[0] = 0x00;
                                SD_Response_Index = 0;
                                SD_Response_Length = 1;
                                SD_App_Command = TRUE;
                                break;

                            case 58:    // CMD58 (read OCR)
                                Response_Buffer[0] = 0x00;
                                Response_Buffer[1] = SD_HC ? 0x40 : 0x00;
                                Response_Buffer[2] = 0x00;
                                Response_Buffer[3] = 0x00;
                                Response_Buffer[4] = 0x00;
                                SD_Response_Index = 0;
                                SD_Response_Length = 5;
                                break;

                            case 59:    // CMD59 (CRC on/off)
                                SD_CRC_Enabled = (SD_Command_Frame[4] & 0x01) != 0;
                                break;

                            default:
                                // return illegal command
                                Response_Buffer[0] = 0x04;
                                SD_Response_Index = 0;
                                SD_Response_Length = 1;
                                break;
                        }
                    }
                }
            }
            break;

        case SDActiveCommandMode::Write:
            if (mSDSendIndex == 0) {
                if (v == 0xFE)
                    mSDSendIndex = 1;
            } else if (mSDSendIndex > 0) {
                mSendBuffer[mSDSendIndex++ - 1] = v;

                if (mSDSendIndex == 515) {
                    mSDSendIndex = 0;

                    SetupWrite();

                    mSDActiveCommandMode = SDActiveCommandMode::None;
                }
            }
            break;

        case SDActiveCommandMode::WriteMultiple:
            if (mSDSendIndex == 0) {
                if (v == 0xFE)
                    mSDSendIndex = 1;
            } else if (mSDSendIndex > 0) {
                mSendBuffer[mSDSendIndex++ - 1] = v;

                if (mSDSendIndex == 515) {
                    mSDSendIndex = 0;

                    SetupWrite();
                    ++mSDActiveCommandLBA;
                }
            }
            break;
    }

    return reply;
}

UBYTE SetupLBA() {
    ULONG lba = VDReadUnalignedBEU32(&SD_Command_Frame[1]);

    if (!SD_HC) {
        if (lba & 0x1FF)
            return 0x20;

        lba >>= 9;
    }

    if (lba >= BlockDevice->GetSectorCount())
        return 0x40;

    SD_Active_Command_LBA = lba;
    return 0;
}

int SetupRead(ULONG lba, int addOK) {
    if (!Block_Device || lba >= Block_Device->GetSectorCount()) {
        Response_Buffer[0] = 0xFF;
        Response_Buffer[1] = 0x40;
        SD_Response_Index = 0;
        SD_Response_Length = 2;
        return FALSE;
    }

    Response_Buffer[0] = 0xFF;
    Response_Buffer[1] = addOK ? 0x00 : 0xFF;
    Response_Buffer[2] = 0xFE;

    BlockDevice->ReadSectors(Response_Buffer + 3, lba, 1);

    VDWriteUnalignedBEU16(&Response_Buffer[515], ComputeCRC16(0, Response_Buffer + 3, 512));

    SD_Response_Index = 0;
    SD_Response_Length = 516;
    return true;
}

void SetupWrite() {
    if (!Block_Device || SD_Active_Command_LBA >= BlockDevice->GetSectorCount()) {
        Response_Buffer[0] = 0xFF;
        Response_Buffer[1] = 0x40;
        SD_Response_Index = 0;
        SD_Response_Length = 2;
        return;
    }

    if (SD_CRC_Enabled) {
        const UWORD expectedCRC = ComputeCRC16(0, Send_Buffer, 512);
        const UWORD receivedCRC = VDReadUnalignedBEU16(&Send_Buffer[512]);

        if (expectedCRC != receivedCRC) {
            Response_Buffer[0] = 0xFF;
            Response_Buffer[1] = 0x0B;      // data rejected due to CRC error
            SD_Response_Index = 0;
            SD_Response_Length = 2;
            return;
        }
    }

    Response_Buffer[0] = 0xFF;
    Response_Buffer[1] = 0x0D;      // data rejected due to write error
    SD_Response_Index = 0;
    SD_Response_Length = 2;

    if (!BlockDevice->IsReadOnly()) {
        BlockDevice->WriteSectors(Send_Buffer, SD_Active_Command_LBA, 1);
        Response_Buffer[1] = 0x05;      // data accepted
    }
}

void ResetSD() {
    SD_Active_Command_Mode = SD_Mode_None;
    SD_Command_State = 0;
    SD_Send_Index = 0;
    SD_SPI_Mode = false;
    SD_CRC_Enabled = true;
}

UBYTE TransferRTC(UBYTE v) {
    return MCP7951X_Transfer(rtc,v);
}

void DMAScheduleEvent(int id) {
    AdvanceDMA();
    RescheduleDMA();
}

void AdvanceDMA() {
    if (!DMA_Active)
        return;

    uint64_t ticks = CPU_cycle_count - DMA_Last_Cycle;
    ULONG maxCount = (ULONG)DMA_Active_Bytes_Left;
    ULONG count = 0;

    if (DMA_From_SD) {
        maxCount = ((maxCount < ticks) ? maxCount : ticks) >> 1;

        if (count < maxCount)
            EnableXferLED();

        for(; count < maxCount; ++count) {
            UBYTE v = SD_Next_Read;
            WriteSPINoTimingUpdate(0xFF);

            // Because DMA transfers are intended for SD, after 512 bytes the DMA engine will
            // discard two bytes for the CRC and then wait for a new Start Block token ($FE).
            // No Start Block is needed for the first sector.
            if (DMA_State == 514) {
                if (v == 0xFE)
                    DMA_State = 0;
            } else if (DMA_State++ < 512) {
                RAM[DMA_Active_Dst_Address] = v;
                DMA_Active_Dst_Address = (DMA_Active_Dst_Address + DMA_Dst_Step) & 0x1FFFFF;
                --DMA_Active_Bytes_Left;
            }
        }

        DMA_Last_Cycle += count*2;
    } else {
        maxCount = (maxCount < ticks) ? maxCount : ticks;

        for(; count < maxCount; ++count) {
            RAM[DMA_Active_Dst_Address] = (RAM[DMA_Active_Src_Address] & DMA_And_Mask) ^ DMA_Xor_Mask;

            DMA_Active_Dst_Address = (DMA_Active_Dst_Address + DMA_Dst_Step) & 0x1FFFFF;
            DMA_Active_Src_Address = (DMA_Active_Src_Address + DMA_Src_Step) & 0x1FFFFF;
        }

        DMA_Last_Cycle += count;
        DMA_Active_Bytes_Left -= count;
    }

    if (!DMA_Active_Bytes_Left)
        DMA_Active = FALSE;
}

void RescheduleDMA() {
    if (DMA_Active) {
        ULONG ticks = (ULONG)DMA_Counter + 1;

        if (DMA_From_SD)
            ticks += ticks;

        uint64_t t = CPU_cycle_count;
        uint64_t tNext = DMA_Last_Cycle + ticks;
        uint64_t dt = tNext <= t ? 1 : tNext - t;

        if (dt > 128)
            dt = 128;

        Scheduler_Entry_Add(EventID_DMA, dt, DMAScheduleEvent);
    } else {
        Scheduler_Entry_Delete(EventID_DMA);
    }
}


void SIDE2_ColdStart(void)
{
    mFlashCtrl.ColdReset();

    MCP7951X_ColdReset(rtc);

    memset(ram, 0xFF, sizeof(ram));

    DMA_From_SD = FALSE;
    DMA_Active = FALSE;
    DMA_Src_Address = 0;
    DMA_Dst_Address = 0;
    DMA_Counter = 0;
    DMA_Src_Step = 1;
    DMA_Dst_Step = 1;
    DMA_And_Mask = 0xFF;
    DMA_Xor_Mask = 0;
    Scheduler_Entry_Delete(EventID_DMA);

    Emu_CCTL_Base = 0;
    Emu_CCTL_Mask = 0;
    Emu_Address_Mask = 0;
    Emu_Data_Mask = 0;
    Emu_Disable_Mask_A = 0;
    Emu_Disable_Mask_B = 0;
    Emu_Feature = 0;
    Emu_Control = 0;
    Emu_Control2 = 0;
    Emu_Bank_A = 0;
    Emu_Bank_B = 0;
    Emu_Bank = 0;
    Emu_Data = 0;
    Emu_Disabled_A = FALSE;
    Emu_Disabled_B = FALSE;
    Emu_Locked = FALSE;

    LED_Brightness = 80;

    Emu_Cart_Enable_Requested = FALSE;
    UpdateEmuCartEnabled();

    // The ROM/flash bank A register is not reset by the push button, so we
    // must do it explicitly for cold reset.
    Bank_Flash_A = 0;
    Flash_Offset_A = 0;

    ResetCartBank();

    Register_Mode = Primary;
    Aperture_Enable = FALSE;
    LED_Green_Manual_Enabled = FALSE;
    Cold_Start_Flag = TRUE;
    Button_Pressed = FALSE;
    SD_Status &= 0x40;
    SD_Status |= 0x05;

    // The hardware resets bit 7 = 1 and doesn't clear it if an SD card is inserted.
    // SIDE3.SYS 4.03 relies on this or it reports No Media on boot.
    if (SIDE3_Block_Device)
        SD_Status |= 0x80;

    ResetSD();

    SD_Next_Transfer_Time = 0;
    UpdateLEDIntensity();
    UpdateLEDGreen();
    UpdateLED();

    MEMORY_SetFlashRoutines(SIDE3_Flash_Read, SIDE3_Flash_Write);


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
    
    Update_Memory_Layers_Cart();
}


static void LoadNVRAM()
{
    MCP7951X_NVState nvstate;
    FILE *f;
    int len;
    
    f = fopen(side3_nvram_filename, "rb");
    if (f == NULL) {
        memset(&nvstate, 0, sizeof(nvstate));
    } else {
        len = fread(&nvstate, 1, sizeof(nvstate), f);
        fclose(f);
        if (len != sizeof(nvstate)) {
            memset(&nvstate, 0, sizeof(nvstate));
        }
    }
    MCP7951X_Load(rtc, buf);
}

static void SaveNVRAM()
{
    MCP7951X_NVState nvstate;
    FILE *f;

    MCP7951X_SAVE(rtc, &nvstate);
    f = fopen(side3_nvram_filename, "wb");
    if (f != NULL) {
        fwrite(&nvstate, 1, sizeof(nvstate), f);
        fclose(f);
    }
}

static void Update_Memory_Layers_Cart(void) {
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

    if (ULTIMATE_enabled) {
        ULTIMATE_Subcart_Left_Active(sdxRead || topLeftRead);
    }

    if (flashRead) {
        MEMORY_CartA0bfEnable();
        MEMORY_SetFlashRoutines(SIDE2_Flash_Read, SIDE2_Flash_Write);
        MEMORY_SetFlash(0xa000, 0xbfff);
        MEMORY_CopyROM(0xa000, 0xbfff, side2_rom + Bank_Offset);
    } else {
        //MEMORY_SetROM(0xa000, 0xbfff);
        MEMORY_CartA0bfDisable();
    }
    
    if (flashReadRight) {
        MEMORY_Cart809fEnable();
        MEMORY_CopyROM(0x8000, 0xbfff, side2_rom + Bank_Offset2);
    } else {
        MEMORY_Cart809fDisable();
    }
}

UBYTE SIDE3_Flash_Read(UWORD addr) {
    UBYTE value;

    Flash_Read_Byte(flash, Bank_Offset + (addr - 0xA000), &value);
        
    return(value);
}

void SIDE3_Flash_Write(UWORD addr, UBYTE value) {

    Flash_Write_Byte(flash, Bank_Offset + (addr - 0xA000), value);
}
