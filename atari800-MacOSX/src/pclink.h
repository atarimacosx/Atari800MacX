//
//  pclink.h
//  Atari800MacX
//
//  Created by markg on 12/15/20.
//

#ifndef pclink_h
#define pclink_h

#define LINK_DEVICE_NUM_DEVS 5
extern char PCLink_base_dir[4][FILENAME_MAX];
extern int  PCLinkEnable[4];
extern int  PCLinkReadOnly[4];
extern int  PCLinkTimestamps[4];
extern int  PCLinkTranslate[4];

void Link_Device_Cold_Reset();
void Link_Device_Init(void);
void Link_Device_Init_Devices(void);
UBYTE Link_Device_On_Serial_Begin_Command(UBYTE *commandFrame,
                                          int *read, int *ExpectedBytes,
                                          char *buffer);
int Link_Device_WriteFrame(char *data);
#endif /* pclink_h */
