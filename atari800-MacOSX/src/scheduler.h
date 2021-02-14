/*
 * scheduler.c - Emulation of the SIDE2 cartridge
 *
 * Copyright (C) 2021 Mark Grebe
 *
*/

#include "atari.h"

typedef struct schedule {
    int             schedID;
    uint64_t        schedTime;
    void            (*schedHandler)(int);
    struct schedule *next;
} SCHEDULE_ENTRY;

SCHEDULE_ENTRY *Schedule_Entry_Add(int id, uint64_t deltaTime, void (*handler)(int));

void Schedule_Entry_Delete(int id);

void Schedule_Check_And_Handle();
