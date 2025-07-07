#ifndef MAC_MONITOR_H_
#define MAC_MONITOR_H_

#include "atari.h"

/* Mac-specific monitor GUI compatibility functions */
int get_hex(const char *buffer, UWORD *hexval);
int get_val_gui(const char *buffer, UWORD *val);

/* Mac-specific label management functions */
void load_user_labels(const char *filename);
void free_user_labels(void);
char *find_user_label(const char *label);
void add_user_label(const char *label, int addr);

/* Mac-specific disassembly functions */
UWORD MONITOR_get_disasm_start(UWORD start_addr, UWORD pc_addr);

/* Mac-specific monitor types */
#ifndef SYMTABLE_REC_DEFINED
typedef struct {
    char *name;
    UWORD addr;
} symtable_rec;
#define SYMTABLE_REC_DEFINED
#endif

/* Mac-specific monitor variables */
extern int symtable_builtin_enable;
extern int symtable_user_size;
extern symtable_rec *symtable_user;
extern const symtable_rec symtable_builtin[];
extern const symtable_rec symtable_builtin_5200[];
extern int break_table_on;
extern int MONITOR_break_active;

/* Mac-specific breakpoint constants for flag conditions */
#define MONITOR_BREAKPOINT_CLRN 2
#define MONITOR_BREAKPOINT_SETN 3
#define MONITOR_BREAKPOINT_CLRV 4
#define MONITOR_BREAKPOINT_SETV 5
#define MONITOR_BREAKPOINT_CLRB 6
#define MONITOR_BREAKPOINT_SETB 7
#define MONITOR_BREAKPOINT_CLRD 8
#define MONITOR_BREAKPOINT_SETD 9
#define MONITOR_BREAKPOINT_CLRI 10
#define MONITOR_BREAKPOINT_SETI 11
#define MONITOR_BREAKPOINT_CLRZ 12
#define MONITOR_BREAKPOINT_SETZ 13
#define MONITOR_BREAKPOINT_CLRC 14
#define MONITOR_BREAKPOINT_SETC 15

#endif /* MAC_MONITOR_H_ */