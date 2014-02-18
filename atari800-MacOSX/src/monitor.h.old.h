#ifndef _MONITOR_H_
#define _MONITOR_H_

#ifdef MONITOR_TRACE
extern int tron;
extern FILE *trace_file;
#endif

/*
 * The following array is used for 6502 instruction profiling
 */
#ifdef MONITOR_PROFILE
extern int instruction_count[256];
#endif

#ifdef MONITOR_BREAK
#define NONE        0
#define OR_BREAK    1

#define CLR_FLAG    0 
#define SET_FLAG    1 
#define CLRN_BREAK  2 
#define SETN_BREAK  3 
#define CLRV_BREAK  4 
#define SETV_BREAK  5 
#define CLRB_BREAK  6 
#define SETB_BREAK  7 
#define CLRD_BREAK  8 
#define SETD_BREAK  9 
#define CLRI_BREAK  10 
#define SETI_BREAK  11 
#define CLRZ_BREAK  12 
#define SETZ_BREAK  13 
#define CLRC_BREAK  14 
#define SETC_BREAK  15 
	
#define BREAK_LESS  1
#define BREAK_EQUAL 2
#define BREAK_GREAT 4

#define PC_BREAK	16
#define A_BREAK		24 
#define X_BREAK		32
#define Y_BREAK		40
#define S_BREAK		48
#define MEM_BREAK	56

#define READ_BREAK 64
#define WRITE_BREAK 128

typedef struct break_struct {
	int condition;
	int on;
	UWORD addr;
	UBYTE val;
} break_struct;

#define BREAK_TABLE_SIZE 20
extern break_struct break_table[BREAK_TABLE_SIZE];
extern int break_table_pos;
extern int break_table_on;
extern int break_table_active;

typedef struct break_token {
	char string[7];
	int condition;
} break_token;
extern break_token break_token_table[];

extern int histon;

#define REMEMBER_PC_STEPS 32
extern UWORD remember_PC[REMEMBER_PC_STEPS];
extern UBYTE remember_A[REMEMBER_PC_STEPS];
extern UBYTE remember_X[REMEMBER_PC_STEPS];
extern UBYTE remember_Y[REMEMBER_PC_STEPS];
extern UBYTE remember_S[REMEMBER_PC_STEPS];
extern UBYTE remember_P[REMEMBER_PC_STEPS];
extern int remember_PC_curpos;
#ifdef NEW_CYCLE_EXACT
extern int remember_xpos[REMEMBER_PC_STEPS];
#endif

#define REMEMBER_JMP_STEPS 32
extern UWORD remember_JMP[REMEMBER_JMP_STEPS];
extern int remember_jmp_curpos;

extern UWORD break_addr;
extern UBYTE break_active;
extern UBYTE break_step;
extern UBYTE break_ret;
extern UBYTE break_cim;
extern UBYTE break_here;
extern int break_fired;
extern int ret_nesting;
extern int brkhere;
#endif

int monitor(void);
unsigned int disassemble(UWORD addr1, UWORD addr2);
UWORD show_instruction_file(FILE *file, UWORD inad, int wid);
extern const UBYTE optype6502[];

#endif /* _MONITOR_H_ */
