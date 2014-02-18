#ifndef MONITOR_H_
#define MONITOR_H_

#include "config.h"
#include <stdio.h>

#include "atari.h"

int MONITOR_Run(void);

#ifdef MONITOR_TRACE
extern FILE *MONITOR_trace_file;
#endif

typedef struct {
	char *name;
	UWORD addr;
} symtable_rec;

#ifdef MONITOR_BREAK
extern UWORD MONITOR_break_addr;
extern UBYTE MONITOR_break_step;
extern UBYTE MONITOR_break_ret;
extern UBYTE MONITOR_break_brk;
extern int MONITOR_ret_nesting;
#ifdef MACOSX
extern int MONITOR_histon;
extern UBYTE MONITOR_break_active;
extern UBYTE MONITOR_break_brk_occured;
extern int MONITOR_break_fired;
extern int MONITOR_tron;
extern int check_break_i;
extern int break_table_on;
extern const UBYTE MONITOR_optype6502[];
extern int symtable_builtin_enable;
extern symtable_rec *symtable_user;
extern int symtable_user_size;
extern const symtable_rec symtable_builtin[];
extern const symtable_rec symtable_builtin_5200[];

UWORD MONITOR_show_instruction_file(FILE* file, UWORD inad, int wid);
UWORD MONITOR_get_disasm_start(UWORD addr, UWORD target); 
void load_user_labels(const char *filename);
void add_user_label(const char *name, UWORD addr);
void free_user_labels(void);
symtable_rec *find_user_label(const char *name);
int get_val_gui(char *s, UWORD *hexval);
int get_hex(char *string, UWORD *hexval);
#endif /* MACOSX */
#endif /* MONITOR_BREAK */

extern const UBYTE MONITOR_optype6502[256];

#ifndef MACOSX
void MONITOR_ShowState(FILE *fp, UWORD pc, UBYTE a, UBYTE x, UBYTE y, UBYTE s,
                char n, char v, char z, char c);
#endif

#ifdef MONITOR_BREAKPOINTS

/* Breakpoint conditions */
#ifdef MACOSX
#define MONITOR_BREAKPOINT_NONE        0
#endif
#define MONITOR_BREAKPOINT_OR          1
#define MONITOR_BREAKPOINT_FLAG_CLEAR  2
#define MONITOR_BREAKPOINT_FLAG_SET    3

/* these three may be ORed together and must be ORed with MONITOR_BREAKPOINT_PC .. MONITOR_BREAKPOINT_WRITE */
#define MONITOR_BREAKPOINT_LESS        1
#define MONITOR_BREAKPOINT_EQUAL       2
#define MONITOR_BREAKPOINT_GREATER     4

#ifdef MACOSX
#define MONITOR_BREAKPOINT_CLR_FLAG    0 
#define MONITOR_BREAKPOINT_SET_FLAG    1 
#define MONITOR_BREAKPOINT_CLRN        2 
#define MONITOR_BREAKPOINT_SETN        3 
#define MONITOR_BREAKPOINT_CLRV        4 
#define MONITOR_BREAKPOINT_SETV        5 
#define MONITOR_BREAKPOINT_CLRB        6 
#define MONITOR_BREAKPOINT_SETB        7 
#define MONITOR_BREAKPOINT_CLRD        8 
#define MONITOR_BREAKPOINT_SETD        9 
#define MONITOR_BREAKPOINT_CLRI        10 
#define MONITOR_BREAKPOINT_SETI        11 
#define MONITOR_BREAKPOINT_CLRZ        12 
#define MONITOR_BREAKPOINT_SETZ        13 
#define MONITOR_BREAKPOINT_CLRC        14 
#define MONITOR_BREAKPOINT_SETC        15 
#endif

//#ifdef MACOSX
//#define MONITOR_BREAKPOINT_PC          8
//#define MONITOR_BREAKPOINT_A           16
//#else
#define MONITOR_BREAKPOINT_PC          16
#define MONITOR_BREAKPOINT_A           24
//#endif
#define MONITOR_BREAKPOINT_X           32
#define MONITOR_BREAKPOINT_Y           40
#define MONITOR_BREAKPOINT_S           48
#ifdef MACOSX
#define MONITOR_BREAKPOINT_MEM         56
#endif
#define MONITOR_BREAKPOINT_READ        64
#define MONITOR_BREAKPOINT_WRITE       128
#define MONITOR_BREAKPOINT_ACCESS      (MONITOR_BREAKPOINT_READ | MONITOR_BREAKPOINT_WRITE)

typedef struct {
#ifdef MACOSX
	int condition;
	int on;
	UWORD addr;
	UBYTE val;
#else
	UBYTE enabled;
	UBYTE condition;
	UWORD value;
#endif
} MONITOR_breakpoint_cond;

#define MONITOR_BREAKPOINT_TABLE_MAX  200
extern MONITOR_breakpoint_cond MONITOR_breakpoint_table[MONITOR_BREAKPOINT_TABLE_MAX];
extern int MONITOR_breakpoint_table_size;
extern int MONITOR_breakpoints_enabled;

#endif /* MONITOR_BREAKPOINTS */

#endif /* MONITOR_H_ */
