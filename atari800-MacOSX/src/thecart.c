/* thecart.c -
   Emulation of The!Cart cartridges for the Atari
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   Copyright (C) 2020-2025 Mark Grebe

   Ported and Adapted from Altirra
   Copyright (C) 2008-2011 Avery Lee
*/

#include "atari.h"
#include "cartridge.h"
#include "eeprom.h"
#include "flash.h"
#include "memory.h"
#include "string.h"
#include "thecart.h"

static SWORD TheCartBankInfo[256];
static int   TheCartBankByAddress;
static UBYTE TheCartOSSBank;
static UBYTE TheCartSICEnables;
static UWORD TheCartBankMask;
static int   TheCartConfigLock;
static UBYTE TheCartRegs[9];
static int   CartBank;
static int   CartBank2;
static unsigned char *CartImage;
static UBYTE CARTRAM[524288];
static int CartSize;
static int CartSizeMask;
static FlashEmu *flash;
static int   CartBankWriteEnable = FALSE;
static int   CartBank2WriteEnable = FALSE;
static uint32_t Bank1_Base;
static uint32_t Bank1_Offset;
static uint32_t Bank2_Base;
static uint32_t Bank2_Offset;

static void  UpdateTheCart(void);
static void  UpdateTheCartBanking(void);
static void  Set_Cart_Banks(int bank, int bank2);
static void  Update_Cart_Banks(void);
static UBYTE THECART_Flash_Read(UWORD addr);
static void  THECART_Flash_Write(UWORD addr, UBYTE value);
static UBYTE THECART_Ram_Read(UWORD addr);
static void  THECART_Ram_Write(UWORD addr, UBYTE value);

enum TheCartBankMode {
    TheCartBankMode_Disabled,
    TheCartBankMode_8K,
    TheCartBankMode_16K,
    TheCartBankMode_Flexi,
    TheCartBankMode_8KFixed_8K,
    TheCartBankMode_OSS,        // 4K fixed + 4K variable
    TheCartBankMode_SIC        // 16K, independently switchable halves
};

enum TheCartBankMode TheCartBankMode;

UWORD Read_Unaligned_LEU16(const void *p) { return *(UWORD *)p; }
void  Write_Unaligned_LEU16(void *p, UWORD v) { *(UWORD *)p = v; }

void THECART_Init(int type, unsigned char *image, int size) {
    CartImage = image;
    CartSize = size << 10;
    CartSizeMask = CartSize - 1;
    switch(type) {
        case CARTRIDGE_THECART_32M:
            flash = Flash_Init(image, Flash_TypeS29GL256P);
            break;
        case CARTRIDGE_THECART_64M:
            flash = Flash_Init(image, Flash_TypeS29GL512P);
            break;
        case CARTRIDGE_THECART_128M:
            flash = Flash_Init(image, Flash_TypeS29GL01P);
            break;
    }

    EEPROM_Init();
}

void THECART_Shutdown(void)
{
    Flash_Shutdown(flash);
}

void THECART_Cold_Reset() {
    static const UBYTE TheCartInitData[]={
        0x00,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x01,
        0x00,
        0x82,    // /CS high, SI high, SCK low
    };
    CartBank = 0;
    CartBank2 = -1;

    CartBankWriteEnable = FALSE;
    CartBank2WriteEnable = FALSE;
    Bank1_Base = 0;
    Bank1_Offset = 0;
    Bank2_Base = 0;
    Bank2_Offset = 0;
    
    memcpy(TheCartRegs, TheCartInitData, sizeof(TheCartRegs));
    TheCartConfigLock = FALSE;
    TheCartSICEnables = 0x40;        // Axxx only
    TheCartOSSBank = 0;

    UpdateTheCartBanking();
    UpdateTheCart();
    Update_Cart_Banks();

    EEPROM_Cold_Reset();
}

// The!Cart registers:
//
// $D5A0: primary bank register low byte (0-255, default: 0)
// $D5A1: primary bank register high byte (0-63, default: 0)
// $D5A2: primary bank enable (1=enable, 0=disable, default: 1)
// $D5A3: secondary bank register low byte (0-255, default: 0)
// $D5A4: secondary bank register high byte (0-63, default: 0)
// $D5A5: secondary bank enable (1=enable, 0=disable, default: 0)
// $D5A6: cart mode select (see section on cartridge modes, default: 1 / 8k)
// $D5A7: flash/ram selection and write enable control (0-15, default: 0)
//   bit 0: primary bank write enable (0 = write protect, 1 = write enable)
//   bit 1: primary bank source (0 = flash, 1 = RAM)
//   bit 2: secondary bank write enable (0 = write protect, 1 = write enable)
//   bit 3: secondary bank source (0 = flash, 1 = RAM)
//
// $D5A8: SPI interface to EEPROM
//   bit 0: SPI CLK
//   bit 1: SPI CS
//   bit 7: SPI data in (on reads), SPI data out (on writes)

UBYTE THECART_Read_Byte(UWORD address) {
    address &= 0xff;

    if (address >= 0xA0 && address <= 0xAF && !TheCartConfigLock) {
        if (address <= 0xA7)
            return TheCartRegs[address - 0xA0];
        else if (address == 0xA8)
            return (TheCartRegs[8] & 0x03) + (EEPROM_Read_State() ? 0x80 : 0x00);
        else
            return 0xFF;
    } else if (TheCartBankMode == TheCartBankMode_SIC && address < 0x20) {
        return ((TheCartRegs[0] >> 1) & 0x1F) + (TheCartRegs[2] ? 0 : 0x40) + (TheCartRegs[5] ? 0x20 : 0);
    } else if (TheCartBankByAddress) {
        THECART_Write_Byte(address, 0);
        return 0xFF;
    }

    return 0xFF;
}

void THECART_Write_Byte(UWORD address, UBYTE value) {
    address &= 0xff;

    if (address >= 0xA0 && address <= 0xAF && !TheCartConfigLock) {
        if (address <= 0xA7) {
            static const UBYTE RegWriteMasks[]={
                0xFF,
                0x3F,
                0x01,
                0xFF,
                0x3F,
                0x01,
                0x3F,
                0x0F,
            };

            const int index = address - 0xA0;
            UBYTE *reg = &TheCartRegs[index];

            value &= RegWriteMasks[index];
 
            int forceUpdate = FALSE;

            switch(index) {
                case 0:
                case 1:
                    // accessing the primary bank registers also enables the primary window
                    if (!TheCartRegs[2]) {
                        TheCartRegs[2] = 1;
                        forceUpdate = TRUE;
                    }
                    break;

                case 3:
                case 4:
                    // accessing the secondary bank registers also enables the secondary window
                    if (!TheCartRegs[5]) {
                        TheCartRegs[5] = 1;
                        forceUpdate = TRUE;
                    }
                    break;
            }

            if (forceUpdate || *reg != value) {
                const UBYTE delta = *reg ^ value;
                *reg = value;

                // check if we updated the banking mode register -- this is particularly expensive
                if (index == 6)
                    UpdateTheCartBanking();
                
                if (index == 7) {
                    // Invalidate and recreate banking if we have an R/W state change:
                    // bit 0 controls primary window read/write
                    // bit 2 controls secondary window read/write

                    if (delta & 0x01)
                        CartBank = -1;

                    if (delta & 0x04)
                        CartBank2 = -1;
                }

                // must come after banking update
                UpdateTheCart();
            }
        } else if (address == 0xA8) {
            value &= 0x83;
            
            if (TheCartRegs[8] != value) {
                TheCartRegs[8] = value;

                EEPROM_Write_State((value & 2) == 0, (value & 1) != 0, (value & 0x80) != 0);
            }
        } else if (address == 0xAF) {
            TheCartConfigLock = TRUE;
        }

        return;
    } else {
        const SWORD bankInfo = TheCartBankInfo[TheCartBankByAddress ? address : value];

        if (bankInfo >= 0) {
            // modify primary bank register
            const UWORD oldBank = Read_Unaligned_LEU16(TheCartRegs);
            const UWORD newBank = oldBank ^ ((oldBank ^ (UWORD)bankInfo) & (TheCartBankMask | 0x4000));
            const UBYTE newEnable = bankInfo & 0x4000 ? 1 : 0;

            // argh... we could almost get away with a generic routine here, if it weren't
            // for SIC! and OSS. :(

            switch(TheCartBankMode) {
                case TheCartBankMode_SIC:
                    if (address < 0x20) {
                        const UBYTE newEnables = (bankInfo & 0x6000) >> 8;
                        if (oldBank != newBank || TheCartSICEnables != newEnables) {
                            Write_Unaligned_LEU16(TheCartRegs, newBank);
                            TheCartSICEnables = newEnables;

                            UpdateTheCart();
                        }
                    }
                    break;

                case TheCartBankMode_OSS:
                    if (TheCartOSSBank != (UBYTE)bankInfo) {
                        TheCartOSSBank = (UBYTE)bankInfo;

                        UpdateTheCart();
                    }
                    break;

                default:
                    if (oldBank != newBank || TheCartRegs[2] != newEnable) {
                        Write_Unaligned_LEU16(TheCartRegs, newBank);
                        TheCartRegs[2] = newEnable;

                        UpdateTheCart();
                    }
                    break;
            }

            return;
        }
    }

    return;
}

static void UpdateTheCartBanking() {
    for (int i=0;i<256;i++)
        TheCartBankInfo[i] = -1;

    // Supported modes:
    //
    // $00: off, cartridge disabled
    // $01: 8k banks at $A000
    // $02: AtariMax 1MBit / 128k
    // $03: Atarimax 8MBit / 1MB
    // $04: OSS M091
    // $08: SDX 64k cart, $D5Ex banking
    // $09: Diamond GOS 64k cart, $D5Dx banking
    // $0A: Express 64k cart, $D57x banking
    // $0C: Atrax 128k cart
    // $0D: Williams 64k cart
    // $20: flexi mode (separate 8k banks at $A000 and $8000)
    // $21: standard 16k cart at $8000-$BFFF
    // $22: MegaMax 16k mode (up to 2MB), AtariMax 8Mbit banking
    // $23: Blizzard 16k
    // $24: Sic!Cart 512k
    // $28: 16k Mega cart
    // $29: 32k Mega cart
    // $2A: 64k Mega cart
    // $2B: 128k Mega cart
    // $2C: 256k Mega cart
    // $2D: 512k Mega cart
    // $2E: 1024k Mega cart
    // $2F: 2048k Mega cart
    // $30: 32k XEGS cart
    // $31: 64k XEGS cart
    // $32: 128k XEGS cart
    // $33: 256k XEGS cart
    // $34: 512k XEGS cart
    // $35: 1024k XEGS cart
    // $38: 32k SWXEGS cart
    // $39: 64k SWXEGS cart
    // $3A: 128k SWXEGS cart
    // $3B: 256k SWXEGS cart
    // $3C: 512k SWXEGS cart
    // $3D: 1024k SWXEGS cart

    const UBYTE mode = TheCartRegs[6] & 0x3f;

    TheCartBankMask = 0;
    TheCartBankByAddress = FALSE;

    // validate mode and determine if we need the bank info
    switch(mode) {
        case 0x00:    // $00: off, cartridge disabled
            TheCartBankMode = TheCartBankMode_Disabled;
            break;

        case 0x01:    // $01: 8k banks at $A000
            TheCartBankMode = TheCartBankMode_8K;
            break;

        case 0x20:    // $20: flexi mode (separate 8k banks at $A000 and $8000)
            TheCartBankMode = TheCartBankMode_Flexi;
            break;

        case 0x21:    // $21: standard 16k cart at $8000-$BFFF
            TheCartBankMode = TheCartBankMode_16K;
            break;

        case 0x02:    // $02: AtariMax 1MBit / 128k
            TheCartBankMode = TheCartBankMode_8K;
            TheCartBankByAddress = TRUE;
            TheCartBankMask = 0x0F;

            for(int i=0; i<0x10; ++i)
                TheCartBankInfo[i] = 0x4000 + i;

            for(int i=0x10; i<0x20; ++i)
                TheCartBankInfo[i] = i;
            break;

        case 0x03:    // $03: Atarimax 8MBit / 1MB
            TheCartBankMode = TheCartBankMode_8K;
            TheCartBankByAddress = TRUE;
            TheCartBankMask = 0x7F;

            for(int i=0; i<0x80; ++i)
                TheCartBankInfo[i] = 0x4000 + i;

            // The!Cart docs say that only $D58x disables in AtariMax 8Mbit emulation mode,
            // even though $D580-FF do so in the real cart.
            for(int i=0x80; i<0x90; ++i)
                TheCartBankInfo[i] = i;
            break;

        case 0x04:    // $04: OSS M091
            {
                static const SBYTE BankLookup[16] = {1, 3, 1, 3, 1, 3, 1, 3, -1, 2};

                TheCartBankMode = TheCartBankMode_OSS;
                TheCartBankByAddress = TRUE;
                TheCartBankMask = 0x01;

                for(int i=0; i<256; ++i) {
                    SBYTE v = BankLookup[i & 9];
                    TheCartBankInfo[i] = v;
                }
            }
            break;

        case 0x08:    // $08: SDX 64k cart, $D5Ex banking
            TheCartBankMode = TheCartBankMode_8K;
            TheCartBankByAddress = TRUE;
            TheCartBankMask = 0x07;

            for(int i=0; i<8; ++i)
                TheCartBankInfo[0xE0+i] = 0x4007 - i;

            for(int i=0; i<8; ++i)
                TheCartBankInfo[0xE8+i] = 7 - i;
            break;

        case 0x09:    // $09: Diamond GOS 64k cart, $D5Dx banking
            TheCartBankMode = TheCartBankMode_8K;
            TheCartBankByAddress = TRUE;
            TheCartBankMask = 0x07;

            for(int i=0; i<8; ++i)
                TheCartBankInfo[0xD0+i] = 0x4007 - i;

            for(int i=0; i<8; ++i)
                TheCartBankInfo[0xD8+i] = 7 - i;
            break;

        case 0x0A:    // $0A: Express 64k cart, $D57x banking
            TheCartBankMode = TheCartBankMode_8K;
            TheCartBankByAddress = TRUE;
            TheCartBankMask = 0x07;

            for(int i=0; i<8; ++i)
                TheCartBankInfo[0x70+i] = 0x4007 - i;

            for(int i=0; i<8; ++i)
                TheCartBankInfo[0x78+i] = 7 - i;
            break;

        case 0x0C:    // $0C: Atrax 128k cart
            TheCartBankMode = TheCartBankMode_8K;
            TheCartBankMask = 0x0F;
            for(int i=0; i<128; ++i)
                TheCartBankInfo[i] = 0x4000 + i;

            for(int i=0; i<128; ++i)
                TheCartBankInfo[0x80+i] = i;
            break;

        case 0x0D:    // $0D: Williams 64k cart
            TheCartBankMode = TheCartBankMode_8K;
            TheCartBankByAddress = TRUE;
            TheCartBankMask = 0x07;

            for(int i=0; i<0x100; ++i)
                TheCartBankInfo[i] = i & 8 ? i : 0x4000 + i;
            break;

        case 0x22:    // $22: MegaMax 16k mode (up to 2MB), AtariMax 8Mbit banking
            TheCartBankMode = TheCartBankMode_16K;
            TheCartBankByAddress = TRUE;
            TheCartBankMask = 0xFE;

            for(int i=0; i<128; ++i)
                TheCartBankInfo[i] = 0x4000 + i*2;

            for(int i=0; i<128; ++i)
                TheCartBankInfo[i+128] = i*2;
            break;

        case 0x23:    // $23: Blizzard 16k
            TheCartBankMode = TheCartBankMode_16K;
            TheCartBankMask = 0;
            for(int i=0; i<0x100; ++i)
                TheCartBankInfo[i] = 0;
            break;

        case 0x24:    // $24: Sic!Cart 512k
            TheCartBankMode = TheCartBankMode_SIC;
            TheCartBankMask = 0x3E;

            for(int i=0; i<0x100; ++i) {
                // Bit 7 controls flash -- which we ignore here.
                // Bit 6 disables $A000-BFFF.
                // Bit 5 enables $8000-9FFF.
                // SIC! uses 16K banks, so we have to go evens.
                TheCartBankInfo[i] = ((i & 0x40) ? 0 : 0x4000) + ((i & 0x20) ? 0x2000 : 0) + (i & 0x1f)*2;
            }
            break;

        case 0x28:    // $28: 16k Mega cart
        case 0x29:    // $29: 32k Mega cart
        case 0x2A:    // $2A: 64k Mega cart
        case 0x2B:    // $2B: 128k Mega cart
        case 0x2C:    // $2C: 256k Mega cart
        case 0x2D:    // $2D: 512k Mega cart
        case 0x2E:    // $2E: 1024k Mega cart
        case 0x2F:    // $2F: 2048k Mega cart
            // 16K banks - data written to $D5xx selects bank, D7=1 disables
            TheCartBankMode = TheCartBankMode_16K;
            TheCartBankMask = (2 << (mode - 0x28)) - 2;
            TheCartBankByAddress = FALSE;

            for(int i=0; i<128; ++i)
                TheCartBankInfo[i] = 0x4000 + i * 2;

            for(int i=0; i<128; ++i)
                TheCartBankInfo[i + 128] = i * 2;
            break;

        case 0x30:    // $30: 32k XEGS cart
        case 0x31:    // $31: 64k XEGS cart
        case 0x32:    // $32: 128k XEGS cart
        case 0x33:    // $33: 256k XEGS cart
        case 0x34:    // $34: 512k XEGS cart
        case 0x35:    // $35: 1024k XEGS cart
            // fixed upper 8K bank, data written to $D5xx selects lower 8K bank
            TheCartBankMode = TheCartBankMode_8KFixed_8K;
            TheCartBankMask = (4 << (mode - 0x30)) - 1;
            TheCartBankByAddress = FALSE;

            for(int i=0; i<256; ++i)
                TheCartBankInfo[i] = 0x4000 + i;
            break;

        case 0x38:    // $38: 32k SWXEGS cart
        case 0x39:    // $39: 64k SWXEGS cart
        case 0x3A:    // $3A: 128k SWXEGS cart
        case 0x3B:    // $3B: 256k SWXEGS cart
        case 0x3C:    // $3C: 512k SWXEGS cart
        case 0x3D:    // $3D: 1024k SWXEGS cart
            // fixed upper 8K bank, data written to $D5xx selects lower 8K bank, D7=1 enables
            TheCartBankMode = TheCartBankMode_8KFixed_8K;
            TheCartBankMask = (4 << (mode - 0x38)) - 1;
            TheCartBankByAddress = FALSE;

            for(int i=0; i<256; ++i)
                TheCartBankInfo[i] = ((i & 0x80) ? 0x4000 : 0) + i;
            break;
    }
}

static void UpdateTheCart() {
    // compute primary bank
    UWORD bank1 = -1;

    if (TheCartRegs[2] & 0x01) {        // check if enabled
        bank1 = Read_Unaligned_LEU16(TheCartRegs + 0) & 0x3FFF;

        if (TheCartRegs[7] & 0x02)        // check if RAM
            bank1 += 0x10000;
    }

    // compute secondary bank
    UWORD bank2 = -1;

    if (TheCartRegs[5] & 0x01) {        // check if enabled
        bank2 = Read_Unaligned_LEU16(TheCartRegs + 3) & 0x3FFF;

        if (TheCartRegs[7] & 0x08)        // check if RAM
            bank2 += 0x10000;
    }

    switch(TheCartBankMode) {
        case TheCartBankMode_Disabled:
        default:
            Set_Cart_Banks(-1, -1);
            break;

        case TheCartBankMode_8K:
            Set_Cart_Banks(bank1, -1);
            break;

        case TheCartBankMode_16K:
            bank1 &= ~1;
            Set_Cart_Banks(bank1 + 1, bank1);
            break;

        case TheCartBankMode_SIC:
            if (TheCartRegs[2]) {
                // recompute primary bank since there are independent enables (arg)
                bank1 = Read_Unaligned_LEU16(TheCartRegs + 0) & 0x3FFF;

                if (TheCartRegs[7] & 0x02)        // check if RAM
                    bank1 += 0x10000;

                bank1 &= ~1;
                Set_Cart_Banks(TheCartSICEnables & 0x40 ? bank1+1 : -1,
                              TheCartSICEnables & 0x20 ? bank1 : -1);
            } else {
                Set_Cart_Banks(-1, -1);
            }
            break;

        case TheCartBankMode_Flexi:
            Set_Cart_Banks(bank1, bank2);
            break;

        case TheCartBankMode_8KFixed_8K:
            Set_Cart_Banks(bank1 | TheCartBankMask, bank1);
            break;

        case TheCartBankMode_OSS:
            Set_Cart_Banks(bank1 | 0x20000,
                          (((bank1 & ~1) * 2) | 0x20000 | TheCartOSSBank));
            break;
    }
}

static void Set_Cart_Banks(int bank, int bank2) {
    if (CartBank == bank && CartBank2 == bank2)
        return;

    CartBank = bank;
    CartBank2 = bank2;
    Update_Cart_Banks();
}

static void Update_Cart_Banks() {
    if (CartBank < 0) {
        Bank1_Base = 0;
        MEMORY_CartA0bfDisable();
    } else {
        UWORD base = 0xA000;
        UWORD size = 0x2000;

        if (CartBank & 0x20000) {
            base = 0xB000;
            size = 0x1000;
        }
        
        UWORD end = (base + size) - 1;
        ULONG offset = (((ULONG) CartBank << 13) & CartSizeMask);
        unsigned char *bankData = CartImage + offset;

        Bank1_Base = base;
        Bank1_Offset = offset;

        CartBankWriteEnable = TheCartRegs[7] & 0x01;
        
        MEMORY_CartA0bfEnable();
        if (!(CartBank & 0x10000)) {
            MEMORY_SetFlashRoutines(THECART_Flash_Read, THECART_Flash_Write);
            MEMORY_SetFlash(base, end);
            MEMORY_CopyFromCart(base, end, bankData);
      } else {
            MEMORY_SetFlashRoutines(THECART_Ram_Read, THECART_Ram_Write);
            MEMORY_SetFlash(base, end);
        }
    }

    if (CartBank2 < 0) {
        MEMORY_Cart809fDisable();
        Bank2_Base = 0;
    } else {
        UWORD base = 0x8000;
        UWORD size = 0x2000;
        UWORD extraOffset = 0;
        ULONG bank2 = CartBank2;

        // adjust for 4K banking
        if (bank2 & 0x20000) {
            bank2 -= 0x20000;

            base = 0xA000;
            size = 0x1000;

            if (bank2 & 1)
                extraOffset = 0x1000;

            bank2 += bank2 & 0x10000;
            bank2 >>= 1;
            MEMORY_Cart809fDisable();
        } else {
            MEMORY_Cart809fEnable();
        }
        
        UWORD end = (base + size) - 1;
        ULONG offset = (((bank2 << 13) + extraOffset) & CartSizeMask);
        unsigned char *bankData = CartImage + offset;

        Bank2_Base = base;
        Bank2_Offset = offset;
        
        // if we are in a 16K mode, we must use the primary bank's read-only flag
        uint8_t roBit = 0x04;

        switch(TheCartBankMode) {
            default:
            case TheCartBankMode_Disabled:
            case TheCartBankMode_8K:
            case TheCartBankMode_Flexi:
                // use secondary bank R/O bit (bit 2)
                break;

            case TheCartBankMode_16K:
            case TheCartBankMode_8KFixed_8K:
            case TheCartBankMode_OSS:
            case TheCartBankMode_SIC:
                // use primary bank R/O bit (bit 0)
                roBit = 0x01;
                break;
        }

        CartBank2WriteEnable = (TheCartRegs[7] & roBit);

        if (!(CartBank & 0x10000)) {
            MEMORY_SetFlashRoutines(THECART_Flash_Read, THECART_Flash_Write);
            MEMORY_SetFlash(base, end);
            MEMORY_CopyFromCart(base, end, bankData);
        } else {
            MEMORY_SetFlashRoutines(THECART_Ram_Read, THECART_Ram_Write);
            MEMORY_SetFlash(base, end);
        }
    }
}

static uint32_t Calc_Full_Address(UWORD addr, int * writeEnable)
{
    uint32_t fullAddr = 0;
    uint32_t base;
    uint32_t offset;

    if (Bank1_Base == 0xB000 && addr >= 0xB000) {
        base = 0xB000;
        offset = Bank1_Offset;
        if (writeEnable)
            *writeEnable = CartBankWriteEnable;
    }
    else if (Bank2_Base == 0xA000 && addr >= 0xA000 && addr < 0xB000) {
        base = 0xA000;
        offset = Bank2_Offset;
        if (writeEnable)
            *writeEnable = CartBank2WriteEnable;
    }
    else if (Bank1_Base == 0xA000 && addr > 0xA000) {
        base = 0xA000;
        offset = Bank1_Offset;
        if (writeEnable)
            *writeEnable = CartBankWriteEnable;
    }
    else {
        base = 0x8000;
        offset = Bank2_Offset;
        if (writeEnable)
            *writeEnable = CartBank2WriteEnable;
    }

    fullAddr = offset + (addr - base);
    
    return fullAddr;
}

static UBYTE THECART_Flash_Read(UWORD addr) {
    uint8_t value;
    Flash_Read_Byte(flash, Calc_Full_Address(addr, NULL), &value);

    return value;
}

static void THECART_Flash_Write(UWORD addr, UBYTE value) {
    int writeEnable;
    uint32_t fullAddr = Calc_Full_Address(addr, &writeEnable);
    
    if (writeEnable) {
        Flash_Write_Byte(flash, fullAddr, value);
    }
}

static UBYTE THECART_Ram_Read(UWORD addr) {
    return CARTRAM[Calc_Full_Address(addr, NULL)];
}

static void THECART_Ram_Write(UWORD addr, UBYTE value) {
    CARTRAM[Calc_Full_Address(addr, NULL)] = value;
}
