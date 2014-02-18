
void VapiInit(void);
int VapiLoadImage(char *filename);
unsigned int VapiSectorRead(int sector, unsigned int time, 
                            unsigned int *count, unsigned char *buffer);
unsigned int VapiDriveStatus(unsigned int time, unsigned int *count, 
                             unsigned char *buffer);
void VapiStopServer(void);

