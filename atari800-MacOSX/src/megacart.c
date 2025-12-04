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
#include "megacart.h"

static CARTRIDGE_image_t *Cart;
static int CartSize;
static int CartSizeMask;
static FlashEmu *flash;
static FlashEmu *flash2;
static int CartBank;

static void SetCartBank(int bank);
static UBYTE MEGACART_Flash_Read(UWORD addr);
static void MEGACART_Flash_Write(UWORD addr, UBYTE value);

static void  MEGACART_Shutdown(void);
static int MEGACART_Is_Dirty(void);
static void  MEGACART_Cold_Reset(void);
static UBYTE MEGACART_Read_Byte(UWORD address);
static void  MEGACART_Write_Byte(UWORD address, UBYTE value);
static void MEGACART_Map_Cart(void);

static CARTRIDGE_funcs_type funcs = {
    MEGACART_Shutdown,
    MEGACART_Is_Dirty,
    MEGACART_Cold_Reset,
    MEGACART_Read_Byte,
    MEGACART_Write_Byte,
    MEGACART_Map_Cart
};

void MEGACART_Init(CARTRIDGE_image_t *cart)
{
    Cart = cart;
    Cart->funcs = &funcs;
    Cart->dirty = FALSE;
    CartSize = cart->size << 10;
    CartSizeMask = CartSize - 1;
    switch(Cart->type) {
        case CARTRIDGE_MEGA_16:
        case CARTRIDGE_MEGA_32:
        case CARTRIDGE_MEGA_64:
        case CARTRIDGE_MEGA_128:
        case CARTRIDGE_MEGA_256:
        case CARTRIDGE_MEGA_1024:
        case CARTRIDGE_MEGA_2048:
            flash = NULL;
            break;
        case CARTRIDGE_MEGA_512:
            flash = Flash_Init(Cart->image, Flash_TypeAm29F040B);
            break;
        case CARTRIDGE_MEGA_4096:
            flash = Flash_Init(Cart->image, Flash_TypeAm29F032B);
            break;
    }
}

static void MEGACART_Shutdown(void)
{
    if (flash != NULL)
        Flash_Shutdown(flash);
}

static int MEGACART_Is_Dirty(void)
{
    return Cart->dirty;
}

static void MEGACART_Cold_Reset(void)
{
    if (Cart->type == CARTRIDGE_MEGA_4096)
        CartBank = 254;
    else
        CartBank = 0;
}

static UBYTE MEGACART_Read_Byte(UWORD address)
{
    switch(Cart->type){
        case CARTRIDGE_MEGA_16:
        case CARTRIDGE_MEGA_32:
        case CARTRIDGE_MEGA_64:
        case CARTRIDGE_MEGA_128:
        case CARTRIDGE_MEGA_256:
        case CARTRIDGE_MEGA_1024:
        case CARTRIDGE_MEGA_2048:
            return -1;
        case CARTRIDGE_MEGA_512:
        case CARTRIDGE_MEGA_4096:
            return CartBank;
        default:
            return -1;
    }
}

static void MEGACART_Write_Byte(UWORD address, UBYTE value)
{
    switch(Cart->type){
        case CARTRIDGE_MEGA_16:
            SetCartBank(value & 0x80 ? -1 : value & 0x0);
            return;
        case CARTRIDGE_MEGA_32:
            SetCartBank(value & 0x80 ? -1 : value & 0x1);
            return;
        case CARTRIDGE_MEGA_64:
            SetCartBank(value & 0x80 ? -1 : value & 0x3);
            return;
        case CARTRIDGE_MEGA_128:
            SetCartBank(value & 0x80 ? -1 : value & 0x7);
            return;
        case CARTRIDGE_MEGA_256:
            SetCartBank(value & 0x80 ? -1 : value & 0xf);
            return;
        case CARTRIDGE_MEGA_1024:
            SetCartBank(value & 0x80 ? -1 : value & 0x1f);
            return;
        case CARTRIDGE_MEGA_2048:
            SetCartBank(value & 0x80 ? -1 : value & 0x3f);
            return;
        case CARTRIDGE_MEGA_512:
            SetCartBank(value & 0x80 ? -1 : value & 0x1F);
            return;
        case CARTRIDGE_MEGA_4096:
            SetCartBank(value == 0xFF ? -1 : value);
            return;
        default:
            return;
    }
}

static void SetCartBank(int bank) {
    if (CartBank == bank)
        return;

    CartBank = bank;
}

static void MEGACART_Map_Cart(void)
{
    UWORD base = 0x8000;
    UWORD end = base + 0x4000 - 1;
    
    if (CartBank < 0) {
        MEMORY_CartA0bfDisable();
    } else {
        MEMORY_CartA0bfEnable();
        if (Cart->type == CARTRIDGE_MEGA_512 || Cart->type == CARTRIDGE_MEGA_4096) {
            MEMORY_SetFlashRoutines(MEGACART_Flash_Read, MEGACART_Flash_Write);
            MEMORY_SetFlash(base, end);
        }
        MEMORY_CopyFromCart(base, end, Cart->image + CartBank*0x4000);
    }
}

static UBYTE MEGACART_Flash_Read(UWORD addr) {
    uint32_t fullAddr;
    UBYTE value;
    
    if (CartBank < 0)
        return -1;

    fullAddr = (CartBank*0x4000 + (addr & 0x3fff)) & (CartSize - 1);

    Flash_Read_Byte(flash, fullAddr, &value);

    return value;
}

static void MEGACART_Flash_Write(UWORD addr, UBYTE value) {
    uint32_t fullAddr;

    if (CartBank < 0)
        return;

    fullAddr = (CartBank*0x4000 + (addr & 0x3fff)) & (CartSize - 1);

    if (Flash_Write_Byte(flash, fullAddr, value)) {
        Cart->dirty = TRUE;
    };
}
