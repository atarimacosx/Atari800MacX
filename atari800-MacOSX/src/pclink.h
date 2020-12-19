//
//  pclink.h
//  Atari800MacX
//
//  Created by markg on 12/15/20.
//

#ifndef pclink_h
#define pclink_h

#define LINK_DEVICE_NUM_DEVS 9

void Link_Device_Cold_Reset();
void Link_Device_Init(void);
UBYTE Link_Device_On_Serial_Begin_Command(UBYTE *commandFrame,
                                          int *read, int *ExpectedBytes,
                                          char *buffer);
int Link_Device_WriteFrame(char *data);
#endif /* pclink_h */
