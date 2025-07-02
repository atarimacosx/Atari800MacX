#ifndef MONITOR_H_
#define MONITOR_H_

#include "config.h"
#include <stdio.h>

#include "atari.h"

int MONITOR_Run(void);

#ifdef MONITOR_HINTS
void MONITOR_PreloadLabelFile(char *filename);
#endif

#ifdef MONITOR_TRACE
extern FILE *MONITOR_trace_file;
#endif

#ifdef MONITOR_BREAK
void MONITOR_BBRK_on(void);
void MONITOR_BPC(char *arg);
extern UWORD MONITOR_break_addr;
extern UBYTE MONITOR_break_step;
extern UBYTE MONITOR_break_ret;
extern UBYTE MONITOR_break_brk;
extern int MONITOR_ret_nesting;
#endif

extern const UBYTE MONITOR_optype6502[256];

void MONITOR_Exit(void);
void MONITOR_ShowState(FILE *fp, UWORD pc, UBYTE a, UBYTE x, UBYTE y, UBYTE s,
                char n, char v, char z, char c);

#ifdef MONITOR_BREAKPOINTS

/* Breakpoint conditions */

#define MONITOR_BREAKPOINT_OR          1
#define MONITOR_BREAKPOINT_FLAG_CLEAR  2
#define MONITOR_BREAKPOINT_FLAG_SET    3

/* these three may be ORed together and must be ORed with MONITOR_BREAKPOINT_PC .. MONITOR_BREAKPOINT_WRITE */
#define MONITOR_BREAKPOINT_LESS        1
#define MONITOR_BREAKPOINT_EQUAL       2
#define MONITOR_BREAKPOINT_GREATER     4

#define MONITOR_BREAKPOINT_PC          8
#define MONITOR_BREAKPOINT_A           16
#define MONITOR_BREAKPOINT_X           32
#define MONITOR_BREAKPOINT_Y           40
#define MONITOR_BREAKPOINT_S           48
#define MONITOR_BREAKPOINT_READ        64
#define MONITOR_BREAKPOINT_WRITE       128
#define MONITOR_BREAKPOINT_MEMORY      256
#define MONITOR_BREAKPOINT_ACCESS      (MONITOR_BREAKPOINT_READ | MONITOR_BREAKPOINT_WRITE)

typedef struct {
	UBYTE enabled;
	UWORD condition;
	UWORD value;
	UWORD m_addr; /* only for MEM: */
} MONITOR_breakpoint_cond;

#define MONITOR_BREAKPOINT_TABLE_MAX  20
extern MONITOR_breakpoint_cond MONITOR_breakpoint_table[MONITOR_BREAKPOINT_TABLE_MAX];
extern int MONITOR_breakpoint_table_size;
extern int MONITOR_breakpoints_enabled;

#endif /* MONITOR_BREAKPOINTS */

#ifdef MONITOR_PROFILE
typedef struct {
	unsigned long count; /* number of times executed since last reset */
	unsigned long cycles; /* number of cycles executed since last reset */
} MONITOR_coverage_rec;
extern MONITOR_coverage_rec MONITOR_coverage[0x10000];
extern unsigned long MONITOR_coverage_insns;
extern unsigned long MONITOR_coverage_cycles;

#endif /* MONITOR_PROFILE */

/* Mac GUI compatibility constants for legacy breakpoint system */
#ifdef ATARI800MACX
/* Mac compatibility function declarations */
int get_hex_gui(const char *buffer, UWORD *hexval);
int get_val_gui(const char *buffer, UWORD *val);

/* Mac compatibility symbol table declarations */
#ifndef SYMTABLE_REC_DEFINED
typedef struct {
    char *name;
    UWORD addr;
} symtable_rec;
#define SYMTABLE_REC_DEFINED
#endif

extern symtable_rec *symtable_user;
extern int symtable_user_size;
extern int symtable_builtin_enable;
extern int break_table_on;
extern const symtable_rec symtable_builtin[];
extern const symtable_rec symtable_builtin_5200[];

#define MONITOR_BREAKPOINT_NONE   0x8000
#define MONITOR_BREAKPOINT_CLRN   0x8001
#define MONITOR_BREAKPOINT_SETN   0x8002
#define MONITOR_BREAKPOINT_CLRV   0x8003
#define MONITOR_BREAKPOINT_SETV   0x8004
#define MONITOR_BREAKPOINT_CLRB   0x8005
#define MONITOR_BREAKPOINT_SETB   0x8006
#define MONITOR_BREAKPOINT_CLRD   0x8007
#define MONITOR_BREAKPOINT_SETD   0x8008
#define MONITOR_BREAKPOINT_CLRI   0x8009
#define MONITOR_BREAKPOINT_SETI   0x800A
#define MONITOR_BREAKPOINT_CLRZ   0x800B
#define MONITOR_BREAKPOINT_SETZ   0x800C
#define MONITOR_BREAKPOINT_CLRC   0x800D
#define MONITOR_BREAKPOINT_SETC   0x800E
/* Use MONITOR_BREAKPOINT_MEMORY from new system instead of custom MEM */
#endif

#endif /* MONITOR_H_ */
