//
//  side2.h
//  Atari800MacX
//
//  Created by markg on 9/12/20.
//

extern int SIDE2_enabled;
extern int SIDE2_have_rom;
extern int SIDE2_SDX_Mode_Switch;
extern int SIDE2_Flash_Type;
extern int SIDE2_Block_Device;
extern char side2_rom_filename[FILENAME_MAX];
extern char side2_nvram_filename[FILENAME_MAX];
extern char side2_compact_flash_filename[FILENAME_MAX];

void init_side2(void);
int SIDE2_Initialise(int *argc, char *argv[]);
void SIDE2_Exit(void);
int SIDE2_D5GetByte(UWORD addr, int no_side_effects);
void SIDE2_D5PutByte(UWORD addr, UBYTE byte);
void SIDE2_ColdStart(void);
void SIDE2_Set_Cart_Enables(int leftEnable, int rightEnable);
int SIDE2_Add_Block_Device(char *filename);
void SIDE2_Remove_Block_Device(void);
void SIDE2_SDX_Switch_Change(int state);
int SIDE2_Change_Rom(char *filename, int new);
int SIDE2_Save_Rom(char *filename);
void SIDE2_Bank_Reset_Button_Change();
UBYTE SIDE2_Flash_Read(UWORD addr);
void SIDE2_Flash_Write(UWORD addr, UBYTE value);
