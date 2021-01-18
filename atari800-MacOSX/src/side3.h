/*
 * side3.h - Emulation of the SIDE3 cartridge
 *
 * Adapted from Altirra
 * Copyright (C) 2009-2021 Avery Lee
 * Copyright (C) 2021 Mark Grebe
 *
*/
extern int SIDE3_enabled;
extern int SIDE3_have_rom;
extern int SIDE3_SDX_Mode_Switch;
extern int SIDE3_Flash_Type;
extern int SIDE3_Block_Device;
extern char side3_rom_filename[FILENAME_MAX];
extern char side3_nvram_filename[FILENAME_MAX];
extern char side3_sd_card_filename[FILENAME_MAX];

void init_side3(void);
int SIDE3_Initialise(int *argc, char *argv[]);
void SIDE3_Exit(void);
int SIDE3_D5GetByte(UWORD addr, int no_side_effects);
void SIDE3_D5PutByte(UWORD addr, UBYTE byte);
void SIDE3_ColdStart(void);
void SIDE3_Set_Cart_Enables(int leftEnable, int rightEnable);
int SIDE3_Add_Block_Device(char *filename);
void SIDE3_Remove_Block_Device(void);
void SIDE3_SDX_Switch_Change(int state);
int SIDE3_Change_Rom(char *filename, int new);
int SIDE3_Save_Rom(char *filename);
void SIDE3_Bank_Reset_Button_Change();
UBYTE SIDE3_Flash_Read(UWORD addr);
void SIDE3_Flash_Write(UWORD addr, UBYTE value);
