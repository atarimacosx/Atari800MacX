/* eeprom.c -
   Emulation of SPI EEPROM for the Atari
   Macintosh OS X SDL port of Atari800
   Mark Grebe <atarimacosx@gmail.com>
   Copyright (C) 2020 Mark Grebe

   Ported and Adapted from Altirra
   Copyright (C) 2009-2014 Avery Lee
*/

#include "atari.h"
#include <stdlib.h>
#include <string.h>
#include "eeprom.h"

UBYTE    Phase;
UBYTE    Address;
UBYTE    Status;
UBYTE    ValueIn;
UBYTE    ValueOut;
int      State;
int      ChipEnable;
int      SPIClock;
UBYTE    Memory[0x100];

enum CommandState {
    CommandState_Initial,
    CommandState_CommandCompleted,
    CommandState_WriteStatus,
    CommandState_WriteMemoryAddress,
    CommandState_WriteMemoryNext,
    CommandState_ReadMemoryAddress,
    CommandState_ReadMemoryNext
} CommandState;

void Read_Register(void);
void Write_Register(void);
void On_Next_Byte(void);

void EEPROM_Init() {
    memset(Memory, 0, sizeof (Memory));

    EEPROM_Cold_Reset();
}

void EEPROM_Cold_Reset() {
    Phase = 0;
    Address = 0;
    Status = 0;
    ValueIn = 0xFF;
    ValueOut = 0xFF;
    CommandState = CommandState_Initial;

    State = TRUE;
    ChipEnable = FALSE;
    SPIClock = FALSE;
}

void EEPROM_Load(const UBYTE *data) {
    memcpy(Memory, data, 0x100);
}

void EEPROM_Save(UBYTE *data)  {
    memcpy(data, Memory, 0x100);
}

int EEPROM_Read_State() {
    return State;
}

void EEPROM_Write_State(int chipEnable, int clock, int data) {
    if (ChipEnable && SPIClock != clock) {
        SPIClock = clock;

        // Data in is sampled on a rising (0 -> 1) clock transition.
        // Data out is changed on a falling (1 -> 0) clock transition.

        if (clock) {    // read (0 -> 1 clock transition)
            // shift in next bit
            ValueIn += ValueIn;
            if (data)
                ++ValueIn;
        } else {        // write (1 -> 0 clock transition)
            // increment phase
            if (++Phase >= 8) {
                Phase = 0;

                On_Next_Byte();
            }

            // shift out next bit
            State = (ValueOut >= 0x80);
            ValueOut += ValueOut;
            ++ValueOut;
        }
    }

    // The!Cart menu drops CS as the same time as SCK, so we can't reset
    // everything until we've processed the clock transition above.
    if (ChipEnable != chipEnable) {
        ChipEnable = chipEnable;

        if (!chipEnable) {
            CommandState = CommandState_Initial;
            Phase = 0;
            State = TRUE;
            SPIClock = TRUE;
        }
    }
}

void On_Next_Byte() {
    switch(CommandState) {
        case CommandState_Initial:
            switch(ValueIn & 0xF7) {
                case 0x01:    // write status register
                    CommandState = CommandState_WriteStatus;
                    break;

                case 0x02:    // write memory
                    CommandState = CommandState_WriteMemoryAddress;
                    break;

                case 0x03:    // read memory
                    CommandState = CommandState_ReadMemoryAddress;
                    break;

                case 0x04:    // reset write enable latch (disable writes)
                    Status &= ~0x02;
                    CommandState = CommandState_CommandCompleted;
                    break;

                case 0x05:    // read status register
                    ValueOut = Status;
                    CommandState = CommandState_CommandCompleted;
                    break;

                case 0x06:    // set write enable latch (enable writes)
                    Status |= 0x02;
                    CommandState = CommandState_CommandCompleted;
                    break;

                default:
                    // ignore command
                    break;
            }
            break;

        case CommandState_CommandCompleted:
            // nothing to do
            break;

        case CommandState_WriteStatus:
            // change write protection bits
            Status = (Status & 0x03) + (ValueIn & 0x0c);
            CommandState = CommandState_CommandCompleted;
            break;

        case CommandState_WriteMemoryAddress:
            Address = ValueIn;
            CommandState = CommandState_WriteMemoryNext;
            break;

        case CommandState_WriteMemoryNext:
            Write_Register();
            break;

        case CommandState_ReadMemoryAddress:
            Address = ValueIn;
            CommandState = CommandState_ReadMemoryNext;
            // fall through to read first memory location
        case CommandState_ReadMemoryNext:
            // read commands wrap through entire 256-byte address space
            Read_Register();
            break;
    }
}

void Read_Register() {
    ValueOut = Memory[Address];
    ++Address;
}

void Write_Register() {
    // check if this write is allowed by current write protection state
    int writeAllowed = FALSE;

    switch(Status & 0x0C) {
        case 0x00:    // all writes allowed
            writeAllowed = TRUE;
            break;

        case 0x04:    // $C0-FF protected
            if (Address < 0xC0)
                writeAllowed = TRUE;
            break;

        case 0x08:    // $80-FF protected
            if (Address < 0x80)
                writeAllowed = TRUE;
            break;

        case 0x0C:    // all writes blocked
            writeAllowed = TRUE;
            break;
    }

    if (writeAllowed) {
        Memory[Address] = ValueIn;
    }

    // write commands only wrap within the current 16-byte page
    Address = ((Address + 1) & 0x0F) + (Address & 0xF0);
}
