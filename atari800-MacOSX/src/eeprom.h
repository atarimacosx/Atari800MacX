//
//  eeprom.h
//  Atari800MacX
//
//  Created by markg on 11/14/20.
//

#ifndef eeprom_h
#define eeprom_h

void EEPROM_Init(void);
void EEPROM_Cold_Reset(void);
void EEPROM_Load(const UBYTE *data);
void EEPROM_Save(UBYTE *data);
int  EEPROM_State(void);
int  EEPROM_Read_State(void);
void EEPROM_Write_State(int chipEnable, int clock, int data);

#endif /* eeprom_h */
