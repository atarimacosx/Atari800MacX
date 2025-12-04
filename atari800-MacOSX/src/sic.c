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
#include "sic.h"

static CARTRIDGE_image_t *Cart;
static int CartSize;
static int CartSizeMask;
static FlashEmu *flash;
static FlashEmu *flash2;
static int CartBank;
static int CartBankMask;

static void SetCartBank(int bank);
static void Map_Cart_Sic();
static void Map_Cart_SicPlus();
static UBYTE SIC_Flash_Read(UWORD addr);
static void SIC_Flash_Write(UWORD addr, UBYTE value);
static UBYTE SICPLUS_Flash_Read(UWORD addr);
static void SICPLUS_Flash_Write(UWORD addr, UBYTE value);

static void  SIC_Shutdown(void);
static int SIC_Is_Dirty(void);
static void  SIC_Cold_Reset(void);
static UBYTE SIC_Read_Byte(UWORD address);
static void  SIC_Write_Byte(UWORD address, UBYTE value);
static void SIC_Map_Cart(void);

static CARTRIDGE_funcs_type funcs = {
    SIC_Shutdown,
    SIC_Is_Dirty,
    SIC_Cold_Reset,
    SIC_Read_Byte,
    SIC_Write_Byte,
    SIC_Map_Cart
};

void SIC_Init(CARTRIDGE_image_t *cart)
{
    Cart = cart;
    Cart->funcs = &funcs;
    Cart->dirty = FALSE;
    CartSize = cart->size << 10;
    CartSizeMask = CartSize - 1;
    switch(cart->type) {
        case CARTRIDGE_SIC_128:
            flash = Flash_Init(Cart->image, Flash_TypeAm29F010B);
            flash2 = NULL;
            CartBankMask = 0x7;
            break;
        case CARTRIDGE_SIC_256:
            flash = Flash_Init(Cart->image, Flash_TypeAm29F002BT);
            flash2 = NULL;
            CartBankMask = 0xF;
            break;
        case CARTRIDGE_SIC_512:
            flash = Flash_Init(Cart->image, Flash_TypeAm29F040B);
            flash2 = NULL;
            CartBankMask = 0x1F;
            break;
        case CARTRIDGE_SICPLUS_1024:
            flash = Flash_Init(Cart->image, Flash_TypeAm29F040B);
            flash2 = Flash_Init(Cart->image + 0x80000, Flash_TypeAm29F040B);
            break;
    }
}

static void SIC_Shutdown(void)
{
    Flash_Shutdown(flash);
    if (flash2 != NULL)
        Flash_Shutdown(flash2);
}

static int SIC_Is_Dirty(void)
{
    return Cart->dirty;
}

static void SIC_Cold_Reset(void)
{
    CartBank = 0;
}

static UBYTE SIC_Read_Byte(UWORD address)
{
    switch(Cart->type){
        case CARTRIDGE_SIC_128:
        case CARTRIDGE_SIC_256:
        case CARTRIDGE_SIC_512:
            if ((UBYTE) address >= 0x20)
                return -1;
            return (UBYTE)CartBank;
        case CARTRIDGE_SICPLUS_1024:
            if ((UBYTE) address >= 0x40)
                return -1;
            return (UBYTE)CartBank;
        default:
            return -1;
    }
}

static void SIC_Write_Byte(UWORD address, UBYTE value)
{
    switch(Cart->type){
        case CARTRIDGE_SIC_128:
        case CARTRIDGE_SIC_256:
        case CARTRIDGE_SIC_512:
            if ((UBYTE)address < 0x20)
                SetCartBank(value);
            return;
        case CARTRIDGE_SICPLUS_1024:
            if ((UBYTE)address < 0x40)
                SetCartBank(value);
            return;
    }
}

static void SetCartBank(int bank) {
    if (CartBank == bank)
        return;

    CartBank = bank;
}

static void SIC_Map_Cart(void)
{
    if (Cart->type == CARTRIDGE_SICPLUS_1024)
        Map_Cart_SicPlus();
    else
        Map_Cart_Sic();
}

static void Map_Cart_Sic() {
    if (!(CartBank & 0x20))
        MEMORY_Cart809fDisable();
    else {
        MEMORY_Cart809fEnable();
        MEMORY_SetFlash(0x8000, 0x9FFF);
        MEMORY_SetFlashRoutines(SIC_Flash_Read, SIC_Flash_Write);
        MEMORY_CopyFromCart(0x8000, 0x9FFF,
                            Cart->image + (CartBank & CartBankMask) * 0x4000);
    }
    if (CartBank & 0x40)
        MEMORY_CartA0bfDisable();
    else {
        MEMORY_CartA0bfEnable();
        MEMORY_SetFlash(0xA000, 0xBFFF);
        MEMORY_SetFlashRoutines(SIC_Flash_Read, SIC_Flash_Write);
        MEMORY_CopyFromCart(0xA000, 0xBFFF,
                            Cart->image + (CartBank & CartBankMask) * 0x4000 + 0x2000);
    }
}

static void Map_Cart_SicPlus() {
    uint32_t upperBankAddr = (CartBank & 0x80 ? 512*1024 : 0);
    if (!(CartBank & 0x20))
        MEMORY_Cart809fDisable();
    else {
        MEMORY_Cart809fEnable();
        MEMORY_SetFlash(0x8000, 0x9FFF);
        MEMORY_SetFlashRoutines(SICPLUS_Flash_Read, SICPLUS_Flash_Write);
        MEMORY_CopyFromCart(0x8000, 0x9FFF,
                            Cart->image + (CartBank & CartBankMask) * 0x4000 + upperBankAddr);
    }
    if (CartBank & 0x40)
        MEMORY_CartA0bfDisable();
    else {
        MEMORY_CartA0bfEnable();
        MEMORY_SetFlash(0xA000, 0xBFFF);
        MEMORY_SetFlashRoutines(SICPLUS_Flash_Read, SICPLUS_Flash_Write);
        MEMORY_CopyFromCart(0xA000, 0xBFFF,
                            Cart->image + (CartBank & CartBankMask) * 0x4000 + 0x2000 + upperBankAddr);
    }
}

static UBYTE SIC_Flash_Read(UWORD addr) {
    uint32_t fullAddr;
    UBYTE value;

    fullAddr = (CartBank & CartBankMask) * 0x4000 + (addr & 0x3fff);

    Flash_Read_Byte(flash, fullAddr, &value);

    return value;
}

static void SIC_Flash_Write(UWORD addr, UBYTE value) {
    uint32_t fullAddr;

    if (CartBank & 0x80) {
        fullAddr = (CartBank & CartBankMask) * 0x4000 + (addr & 0x3fff);
        
        if (Flash_Write_Byte(flash, fullAddr, value)) {
            Cart->dirty = TRUE;
        }
    }
}

static UBYTE SICPLUS_Flash_Read(UWORD addr) {
    UBYTE value;
    uint32_t fullAddr;
    FlashEmu *flashEmu;

    fullAddr = (CartBank & 0x1f) * 0x4000 + (addr & 0x3fff);
    flashEmu = (CartBank & 0x80 ? flash2 : flash);

    Flash_Read_Byte(flashEmu, fullAddr, &value);

    return value;
}

static void SICPLUS_Flash_Write(UWORD addr, UBYTE value) {
    uint32_t fullAddr;
    FlashEmu *flashEmu;

    fullAddr = (CartBank & 0x1f) * 0x4000 + (addr & 0x3fff);
    flashEmu = (CartBank & 0x80 ? flash2 : flash);
    
    if (Flash_Write_Byte(flashEmu, fullAddr & 0x7FFFF, value)) {
        Cart->dirty = TRUE;
    }
}
