void *CDS1305_Init(void);
void CDS1305_Exit(void *rtc);
void CDS1305_ColdReset(void *rtc);
void CDS1305_Load(void *rtc, const UBYTE *data);
void CDS1305_Save(void *rtc, UBYTE *data);
int CDS1305_ReadState(void *rtc);
void CDS1305_WriteState(void *rtc, int ce, int clock, int data);
