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

static unsigned char *CartImage;
static int CartType;
static int CartSize;
static int CartSizeMask;
static FlashEmu *flash;
static FlashEmu *flash2;
static int CartBank;
static int CartDirty = FALSE;

static void SetCartBank(int bank);
static UBYTE MEGACART_Flash_Read(UWORD addr);
static void MEGACART_Flash_Write(UWORD addr, UBYTE value);

void MEGACART_Init(int type, unsigned char *image, int size)
{
    CartImage = image;
    CartType = type;
    CartSize = size << 10;
    CartSizeMask = CartSize - 1;
    switch(type) {
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
            flash = Flash_Init(image, Flash_TypeAm29F040B);
            break;
        case CARTRIDGE_MEGA_4096:
            flash = Flash_Init(image, Flash_TypeAm29F032B);
            break;
    }
}

void MEGACART_Shutdown(void)
{
    if (flash != NULL)
        Flash_Shutdown(flash);
}

int MEGACART_Is_Dirty(void)
{
    return CartDirty;
}

void MEGACART_Cold_Reset(void)
{
    if (CartType == CARTRIDGE_MEGA_4096)
        CartBank = 254;
    else
        CartBank = 0;
    MEGACART_Update_Cart_Banks();
}

UBYTE MEGACART_Read_Byte(UWORD address)
{
    switch(CartType){
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

void MEGACART_Write_Byte(UWORD address, UBYTE value)
{
    switch(CartType){
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
    MEGACART_Update_Cart_Banks();
}

void MEGACART_Update_Cart_Banks(void)
{
    UWORD base = 0x8000;
    UWORD end = base + 0x4000 - 1;
    
    if (CartBank < 0) {
        MEMORY_CartA0bfDisable();
    } else {
        MEMORY_CartA0bfEnable();
        if (CartType == CARTRIDGE_MEGA_512 || CartType == CARTRIDGE_MEGA_4096) {
            MEMORY_SetFlashRoutines(MEGACART_Flash_Read, MEGACART_Flash_Write);
            MEMORY_SetFlash(base, end);
        }
        MEMORY_CopyFromCart(base, end, CartImage + CartBank*0x4000);
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
        CartDirty = TRUE;
    };
}
