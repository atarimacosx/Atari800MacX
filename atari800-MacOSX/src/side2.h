//
//  side2.h
//  Atari800MacX
//
//  Created by markg on 9/12/20.
//

extern int SIDE2_enabled;

void init_side2(void);
int SIDE2_Initialise(int *argc, char *argv[]);
void SIDE2_Exit(void);
UBYTE SIDE2_D5GetByte(UWORD addr, int no_side_effects);
void SIDE2_D5PutByte(UWORD addr, UBYTE byte);
void SIDE2_ColdStart(void);
