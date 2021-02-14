/*
 * scheduler.c - Emulation of the SIDE2 cartridge
 *
 * Copyright (C) 2021 Mark Grebe
 *
*/

#include "atari.h"
#include "cpu.h"
#include "scheduler.h"
#include <stdlib.h>

static SCHEDULE_ENTRY *scheduleStart = NULL;

SCHEDULE_ENTRY *Schedule_Entry_Add(int id, uint64_t deltaTime, void (*handler)(int))
{
    SCHEDULE_ENTRY* curr; 
    SCHEDULE_ENTRY *entry = 
        (SCHEDULE_ENTRY *)malloc(sizeof(SCHEDULE_ENTRY));

    // Create new entry
    entry->schedID = id;
    entry->schedTime = CPU_cycle_count + deltaTime;
    entry->schedHandler = handler;
    entry->next = NULL;

    // Delete an entry with the same ID if it exists
    Schedule_Entry_Delete(id);

    curr = scheduleStart;

    // If the list is currently empty or new entry fits at head, add
    // it to head of the list.
    if (curr == NULL || 
        ((curr->schedTime - entry->schedTime) < 0x800000000000000ull)) {
        entry->next = curr;
        scheduleStart = entry;
        return(entry);
    } else {
        // Find the entry that the new one should follow
        while(curr) {
            if (curr->next != NULL &&
                (curr->next->schedTime - entry->schedTime) >= 0x800000000000000ull) {
                curr = curr->next;
            }
        // Insert it into the list
        entry->next = curr->next;
        curr->next = entry;
        }
    }

    return(entry);
}

void Schedule_Entry_Delete(int id)
{
    SCHEDULE_ENTRY* curr = scheduleStart; 
    SCHEDULE_ENTRY* prev = NULL; 
      
    // If head node itself holds id to be deleted
    if (curr != NULL && curr->schedID == id) 
    { 
        scheduleStart = curr->next; // Changed head 
        free(curr);             // free old head 
        return; 
    } 
  
    // Else Search for the id to be deleted,  
    // keep track of the previous node as we 
    // need to change 'prev->next' */ 
    while (curr != NULL && curr->schedID != id) 
    { 
        prev = curr; 
        curr = curr->next; 
    } 
  
    // If id was not present in linked list 
    if (curr == NULL) 
        return; 
  
    // Unlink the node from linked list 
    prev->next = curr->next; 
  
    // Free memory 
    free(curr); 
}

void Schedule_Check_And_Handle()
{
    SCHEDULE_ENTRY* curr = scheduleStart; 

    // If there is a schedule entry, and if the CPU count is equal to or
    // has passed it...
    if (curr != NULL &&
        ((curr->schedTime == CPU_cycle_count) ||
        ((curr->schedTime - CPU_cycle_count) >= 0x800000000000000ull))) {
        // Run the handler
        (*curr->schedHandler)(curr->schedID);
        // Delete the entry from the list
        scheduleStart = curr->next;
        free(curr);
    }
}
