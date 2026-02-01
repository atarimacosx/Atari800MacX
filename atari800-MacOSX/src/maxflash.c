/* maxflash.c -
   Emulation of MAXFLASH cartridges for the Atari
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   Copyright (C) 2020-2025 Mark Grebe

   Ported and Adapted from Altirra
   Copyright (C) 2008-2011 Avery Lee
*/

#include "atari.h"
#include "cartridge.h"
#include "cartridge_info.h"
#include "flash.h"
#include "memory.h"
#include "string.h"
#include "maxflash.h"

static CARTRIDGE_image_t *Cart;
static int CartSize;
static int CartSizeMask;
static FlashEmu *flash;
static FlashEmu *flash2;
static int CartBank;
static int CartMiniBank;
static int CartWriteEnable = TRUE;

static void SetCartBank(int bank);
static void SetDCartBank(int bank, int miniBank);
static UBYTE MAXFLASH_Flash_Read(UWORD addr);
static void MAXFLASH_Flash_Write(UWORD addr, UBYTE value);

static void  MAXFLASH_Shutdown(void);
static int MAXFLASH_Is_Dirty(void);
static void  MAXFLASH_Cold_Reset(void);
static UBYTE MAXFLASH_Read_Byte(UWORD address);
static void  MAXFLASH_Write_Byte(UWORD address, UBYTE value);
static void MAXFLASH_Map_Cart(void);

static CARTRIDGE_funcs_type funcs = {
    MAXFLASH_Shutdown,
    MAXFLASH_Is_Dirty,
    MAXFLASH_Cold_Reset,
    MAXFLASH_Read_Byte,
    MAXFLASH_Write_Byte,
    MAXFLASH_Map_Cart
};

void MAXFLASH_Init(CARTRIDGE_image_t *cart)
{
    Cart = cart;
    Cart->funcs = &funcs;
    Cart->dirty = FALSE;
    CartSize = cart->size << 10;
    CartSizeMask = CartSize - 1;
    switch(Cart->type) {
        case CARTRIDGE_ATMAX_128:
            flash = Flash_Init(Cart->image, Flash_TypeAm29F010);
            flash2 = NULL;
            break;
        case CARTRIDGE_ATMAX_OLD_1024:
            flash = Flash_Init(Cart->image, Flash_TypeAm29F040B);
            flash2 = Flash_Init(Cart->image + 0x80000, Flash_TypeAm29F040B);
            break;
        case CARTRIDGE_ATMAX_NEW_1024:
            flash = Flash_Init(Cart->image, Flash_TypeAm29F040B);
            flash2 = Flash_Init(Cart->image + 0x80000, Flash_TypeAm29F040B);
            break;
        case CARTRIDGE_JACART_128:
            flash = Flash_Init(Cart->image, Flash_TypeSST39SF010);
            flash2 = NULL;
            break;
        case CARTRIDGE_JACART_256:
            flash = Flash_Init(Cart->image, Flash_TypeSST39SF020);
            flash2 = NULL;
            break;
        case CARTRIDGE_JACART_512:
            flash = Flash_Init(Cart->image, Flash_TypeSST39SF040);
            flash2 = NULL;
            break;
        case CARTRIDGE_JACART_1024:
            flash = Flash_Init(Cart->image, Flash_TypeSST39SF040);
            flash2 = Flash_Init(Cart->image + 0x80000, Flash_TypeSST39SF040);
            break;
        case CARTRIDGE_DCART:
            flash = Flash_Init(Cart->image, Flash_TypeSST39SF040);
            flash2 = NULL;
    }
}

static void  MAXFLASH_Shutdown(void)
{
    Flash_Shutdown(flash);
    if (flash2 != NULL)
        Flash_Shutdown(flash2);
}

static int MAXFLASH_Is_Dirty(void)
{
    return Cart->dirty;
}

static void MAXFLASH_Cold_Reset(void)
{
    if (Cart->type == CARTRIDGE_ATMAX_OLD_1024)
        CartBank = 127;
    else
        CartBank = 0;
    CartMiniBank = 0;
}

static UBYTE MAXFLASH_Read_Byte(UWORD address)
{
    uint32_t flashOffset;
    UBYTE value;
    
    switch(Cart->type){
        case CARTRIDGE_ATMAX_128:
            if (address < 0xD520) {
                if (address < 0xD510) {
                    SetCartBank(address & 15);
                }
                else {
                    SetCartBank(-1);
                }
            }
            return -1;
       case CARTRIDGE_JACART_128:
            SetCartBank(address & 0x80 ? -1 : (UBYTE)address & 0x0F);
            return -1;
        case CARTRIDGE_JACART_256:
            SetCartBank(address & 0x80 ? -1 : (UBYTE)address & 0x1F);
            return -1;
        case CARTRIDGE_JACART_512:
            SetCartBank(address & 0x80 ? -1 : (UBYTE)address & 0x3F);
            return -1;
        case CARTRIDGE_ATMAX_OLD_1024:
        case CARTRIDGE_ATMAX_NEW_1024:
        case CARTRIDGE_JACART_1024:
            SetCartBank(address & 0x80 ? -1 : (UBYTE)address & 0x7F);
            return -1;
        case CARTRIDGE_DCART:
            // read from 0x1500 within current bank, regardless of whether cart window is enabled
            flashOffset = address - 0xD500 + 0x1500 + ((uint32_t)(CartMiniBank & 0x3F) << 13);
            Flash_Read_Byte(flash, flashOffset, &value);
            return value;
        default:
            return -1;
    }
}

static void MAXFLASH_Write_Byte(UWORD address, UBYTE value)
{
    switch(Cart->type){
        case CARTRIDGE_ATMAX_128:
            if (address < 0xD520) {
                if (address < 0xD510) {
                    SetCartBank(address & 15);
                }
                else {
                    SetCartBank(-1);
                }
            }
            break;
        case CARTRIDGE_JACART_128:
            SetCartBank(address & 0x80 ? -1 : (UBYTE)address & 0x0F);
            return;
        case CARTRIDGE_JACART_256:
            SetCartBank(address & 0x80 ? -1 : (UBYTE)address & 0x1F);
            return;
        case CARTRIDGE_JACART_512:
            SetCartBank(address & 0x80 ? -1 : (UBYTE)address & 0x3F);
            return;
        case CARTRIDGE_ATMAX_OLD_1024:
        case CARTRIDGE_ATMAX_NEW_1024:
        case CARTRIDGE_JACART_1024:
            SetCartBank(address & 0x80 ? -1 : (UBYTE)address & 0x7F);
            return;
        case CARTRIDGE_DCART:
            SetDCartBank(address & 0xBF, address & 0xBF);
            return;
    }
}

static void SetCartBank(int bank) {
    if (CartBank == bank)
        return;

    CartBank = bank;
}

static void SetDCartBank(int bank, int miniBank) {
    if (CartBank == bank && CartMiniBank == miniBank)
        return;

    CartMiniBank = miniBank;
    CartBank = bank;
}

static void MAXFLASH_Map_Cart(void)
{
    UWORD base = 0xA000;
    UWORD end = base + 0x2000 - 1;
    
    if (CartBank < 0) {
        MEMORY_CartA0bfDisable();
    } else {
        MEMORY_CartA0bfEnable();
        MEMORY_SetFlashRoutines(MAXFLASH_Flash_Read, MAXFLASH_Flash_Write);
        MEMORY_SetFlash(base, end);
        MEMORY_CopyFromCart(base, end, Cart->image + CartBank*0x2000);
        if (Cart->type == CARTRIDGE_DCART)
            MEMORY_CopyFromCart(0xD500, 0xd5FF, Cart->image + CartBank*0x2000 + 0X1500);
    }
}

static UBYTE MAXFLASH_Flash_Read(UWORD addr) {
    uint32_t fullAddr;
    UBYTE value;
    FlashEmu *flashEmu;
    
    if (CartBank < 0)
        return -1;

    fullAddr = (CartBank*0x2000 + (addr & 0x1fff)) & (CartSize - 1);
    flashEmu = (fullAddr >= 0x80000 ? flash2 : flash);

    Flash_Read_Byte(flashEmu, fullAddr & 0x7FFFF, &value);

    return value;
}

static void MAXFLASH_Flash_Write(UWORD addr, UBYTE value) {
    uint32_t fullAddr;
    FlashEmu *flashEmu;

    if (CartBank < 0)
        return;

    fullAddr = (CartBank*0x2000 + (addr & 0x1fff)) & (CartSize - 1);
    flashEmu = (fullAddr >= 0x80000 ? flash2 : flash);
    if (CartWriteEnable) {
        if (Flash_Write_Byte(flashEmu, fullAddr & 0x7FFFF, value)) {
            Cart->dirty = TRUE;
        }
    }
}
