void CDS1305_Init();
void CDS1305_ColdReset();
void CDS1305_Load(const UBYTE *data);
void CDS1305_Save(UBYTE *data);
int CDS1305_ReadState();
void CDS1305_WriteState(int ce, int clock, int data);
