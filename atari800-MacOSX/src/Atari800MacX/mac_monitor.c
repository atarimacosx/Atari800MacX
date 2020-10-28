/*
 * mac_monitor.c - Implements a builtin system monitor for debugging
 *
 * Copyright (C) 1995-1998 David Firth
 * Copyright (C) 1998-2005 Atari800 development team (see DOC/CREDITS)
 *
 * This file is part of the Atari800 emulator project which emulates
 * the Atari 400, 800, 800XL, 130XE, and 5200 8-bit computers.
 *
 * Atari800 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atari800 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari800; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "antic.h"
#include "atari.h"
#include "cpu.h"
#include "gtia.h"
#include "memory.h"
#include "monitor.h"
#include "pia.h"
#include "pokey.h"
#include "prompts.h"
#include "sio.h"
#include "util.h"
#include <stdarg.h>
#ifdef STEREO_SOUND
#include "pokeysnd.h"
#endif

#ifdef MACOSX
#define MACOSX_MON_ENHANCEMENTS
#endif

#ifdef PROFILE
extern int instruction_count[256];
#endif
extern int cycles[256];

extern UBYTE *atarixe_memory;
extern ULONG atarixe_memory_size;
extern int ControlManagerMonitorPrintf(const char *format, ...);
extern int ControlManagerMonitorPuts(const char *string);
#ifdef MACOSX_MON_ENHANCEMENTS
extern int BreakpointsControllerGetBreakpointNumForConditionNum(int num);
extern void BreakpointsControllerSetDirty(void);
extern void ControlManagerMonitorSetLabelsDirty();
#endif

static void mon_printf(const char *format,...)
{
	static char string[512];
	va_list arguments;             
    va_start(arguments, format);  
    
	vsprintf(string, format, arguments);
    ControlManagerMonitorPrintf(string);
}

static void mon_puts(const char *string)
{
    mon_printf("%s",string);
}

int MONITOR_histon = FALSE;

unsigned int disassemble(UWORD addr1, UWORD addr2);
UWORD show_instruction(UWORD inad, int wid);
UWORD show_instruction_string(char*buff, UWORD inad, int wid);

#ifdef MONITOR_ASSEMBLER
UWORD assembler(char *input);
int MONITOR_assemblerMode = FALSE;
static UWORD assemblerAddr;
#endif

#ifdef MONITOR_HINTS

 /* Symbol names taken from atari.equ - part of disassembler by Erich Bacher
    and from antic.h, gtia.h, pia.h and pokey.h.
    Symbols must be sorted by address. If the adress has different names
    when reading/writing to it, put the read name first. */
#ifdef MACOSX_MON_ENHANCEMENTS
const symtable_rec symtable_builtin[] = {
#else
static const symtable_rec symtable_builtin[] = {
#endif
	{"NGFLAG",  0x0001}, {"CASINI",  0x0002}, {"CASINI+1",0x0003}, {"RAMLO",   0x0004},
	{"RAMLO+1", 0x0005}, {"TRAMSZ",  0x0006}, {"CMCMD",   0x0007}, {"WARMST",  0x0008},
	{"BOOT",    0x0009}, {"DOSVEC",  0x000a}, {"DOSVEC+1",0x000b}, {"DOSINI",  0x000c},
	{"DOSINI+1",0x000d}, {"APPMHI",  0x000e}, {"APPMHI+1",0x000f}, {"POKMSK",  0x0010},
	{"BRKKEY",  0x0011}, {"RTCLOK",  0x0012}, {"RTCLOK+1",0x0013}, {"RTCLOK+2",0x0014},
	{"BUFADR",  0x0015}, {"BUFADR+1",0x0016}, {"ICCOMT",  0x0017}, {"DSKFMS",  0x0018},
	{"DSKFMS+1",0x0019}, {"DSKUTL",  0x001a}, {"DSKUTL+1",0x001b}, {"ABUFPT",  0x001c},
	{"ABUFPT+1",0x001d}, {"ABUFPT+2",0x001e}, {"ABUFPT+3",0x001f},
	{"ICHIDZ",  0x0020}, {"ICDNOZ",  0x0021}, {"ICCOMZ",  0x0022}, {"ICSTAZ",  0x0023},
	{"ICBALZ",  0x0024}, {"ICBAHZ",  0x0025}, {"ICPTLZ",  0x0026}, {"ICPTHZ",  0x0027},
	{"ICBLLZ",  0x0028}, {"ICBLHZ",  0x0029}, {"ICAX1Z",  0x002a}, {"ICAX2Z",  0x002b},
	{"ICAX3Z",  0x002c}, {"ICAX4Z",  0x002d}, {"ICAX5Z",  0x002e}, {"ICAX6Z",  0x002f},
	{"STATUS",  0x0030}, {"CHKSUM",  0x0031}, {"BUFRLO",  0x0032}, {"BUFRHI",  0x0033},
	{"BFENLO",  0x0034}, {"BFENHI",  0x0035}, {"LTEMP",   0x0036}, {"LTEMP+1", 0x0037},
	{"BUFRFL",  0x0038}, {"RECVDN",  0x0039}, {"XMTDON",  0x003a}, {"CHKSNT",  0x003b},
	{"NOCKSM",  0x003c}, {"BPTR",    0x003d}, {"FTYPE",   0x003e}, {"FEOF",    0x003f},
	{"FREQ",    0x0040}, {"SOUNDR",  0x0041}, {"CRITIC",  0x0042}, {"FMSZPG",  0x0043},
	{"FMSZPG+1",0x0044}, {"FMSZPG+2",0x0045}, {"FMSZPG+3",0x0046}, {"FMSZPG+4",0x0047},
	{"FMSZPG+5",0x0048}, {"FMSZPG+6",0x0049}, {"ZCHAIN",  0x004a}, {"ZCHAIN+1",0x004b},
	{"DSTAT",   0x004c}, {"ATRACT",  0x004d}, {"DRKMSK",  0x004e}, {"COLRSH",  0x004f},
	{"TEMP",    0x0050}, {"HOLD1",   0x0051}, {"LMARGN",  0x0052}, {"RMARGN",  0x0053},
	{"ROWCRS",  0x0054}, {"COLCRS",  0x0055}, {"COLCRS+1",0x0056}, {"DINDEX",  0x0057},
	{"SAVMSC",  0x0058}, {"SAVMSC+1",0x0059}, {"OLDROW",  0x005a}, {"OLDCOL",  0x005b},
	{"OLDCOL+1",0x005c}, {"OLDCHR",  0x005d}, {"OLDADR",  0x005e}, {"OLDADR+1",0x005f},
	{"FKDEF",   0x0060}, {"FKDEF+1", 0x0061}, {"PALNTS",  0x0062}, {"LOGCOL",  0x0063},
	{"ADRESS",  0x0064}, {"ADRESS+1",0x0065}, {"MLTTMP",  0x0066}, {"MLTTMP+1",0x0067},
	{"SAVADR",  0x0068}, {"SAVADR+1",0x0069}, {"RAMTOP",  0x006a}, {"BUFCNT",  0x006b},
	{"BUFSTR",  0x006c}, {"BUFSTR+1",0x006d}, {"BITMSK",  0x006e}, {"SHFAMT",  0x006f},
	{"ROWAC",   0x0070}, {"ROWAC+1", 0x0071}, {"COLAC",   0x0072}, {"COLAC+1", 0x0073},
	{"ENDPT",   0x0074}, {"ENDPT+1", 0x0075}, {"DELTAR",  0x0076}, {"DELTAC",  0x0077},
	{"DELTAC+1",0x0078}, {"KEYDEF",  0x0079}, {"KEYDEF+1",0x007a}, {"SWPFLG",  0x007b},
	{"HOLDCH",  0x007c}, {"INSDAT",  0x007d}, {"COUNTR",  0x007e}, {"COUNTR+1",0x007f},
	{"LOMEM",   0x0080}, {"LOMEM+1", 0x0081}, {"VNTP",    0x0082}, {"VNTP+1",  0x0083},
	{"VNTD",    0x0084}, {"VNTD+1",  0x0085}, {"VVTP",    0x0086}, {"VVTP+1",  0x0087},
	{"STMTAB",  0x0088}, {"STMTAB+1",0x0089}, {"STMCUR",  0x008a}, {"STMCUR+1",0x008b},
	{"STARP",   0x008c}, {"STARP+1", 0x008d}, {"RUNSTK",  0x008e}, {"RUNSTK+1",0x008f},
	{"TOPSTK",  0x0090}, {"TOPSTK+1",0x0091}, {"MEOLFLG", 0x0092}, {"POKADR",  0x0095},
	{"POKADR+1",0x0096}, {"DATAD",   0x00b6}, {"DATALN",  0x00b7}, {"DATALN+1",0x00b8},
	{"STOPLN",  0x00ba}, {"STOPLN+1",0x00bb}, {"SAVCUR",  0x00be}, {"IOCMD",   0x00c0},
	{"IODVC",   0x00c1}, {"PROMPT",  0x00c2}, {"ERRSAVE", 0x00c3}, {"COLOUR",  0x00c8},
	{"PTABW",   0x00c9}, {"LOADFLG", 0x00ca}, {"FR0",     0x00d4}, {"FR0+1",   0x00d5},
	{"FR0+2",   0x00d6}, {"FR0+3",   0x00d7}, {"FR0+4",   0x00d8}, {"FR0+5",   0x00d9},
	{"FRE",     0x00da}, {"FRE+1",   0x00db}, {"FRE+2",   0x00dc}, {"FRE+3",   0x00dd},
	{"FRE+4",   0x00de}, {"FRE+5",   0x00df}, {"FR1",     0x00e0}, {"FR1+1",   0x00e1},
	{"FR1+2",   0x00e2}, {"FR1+3",   0x00e3}, {"FR1+4",   0x00e4}, {"FR1+5",   0x00e5},
	{"FR2",     0x00e6}, {"FR2+1",   0x00e7}, {"FR2+2",   0x00e8}, {"FR2+3",   0x00e9},
	{"FR2+4",   0x00ea}, {"FR2+5",   0x00eb}, {"FRX",     0x00ec}, {"EEXP",    0x00ed},
	{"NSIGN",   0x00ee}, {"ESIGN",   0x00ef}, {"FCHRFLG", 0x00f0}, {"DIGRT",   0x00f1},
	{"CIX",     0x00f2}, {"INBUFF",  0x00f3}, {"INBUFF+1",0x00f4}, {"ZTEMP1",  0x00f5},
	{"ZTEMP1+1",0x00f6}, {"ZTEMP4",  0x00f7}, {"ZTEMP4+1",0x00f8}, {"ZTEMP3",  0x00f9},
	{"ZTEMP3+1",0x00fa}, {"RADFLG",  0x00fb}, {"FLPTR",   0x00fc}, {"FLPTR+1", 0x00fd},
	{"FPTR2",   0x00fe}, {"FPTR2+1", 0x00ff},

	{"VDSLST",  0x0200}, {"VDSLST+1",0x0201}, {"VPRCED",  0x0202}, {"VPRCED+1",0x0203},
	{"VINTER",  0x0204}, {"VINTER+1",0x0205}, {"VBREAK",  0x0206}, {"VBREAK+1",0x0207},
	{"VKEYBD",  0x0208}, {"VKEYBD+1",0x0209}, {"VSERIN",  0x020a}, {"VSERIN+1",0x020b},
	{"VSEROR",  0x020c}, {"VSEROR+1",0x020d}, {"VSEROC",  0x020e}, {"VSEROC+1",0x020f},
	{"VTIMR1",  0x0210}, {"VTIMR1+1",0x0211}, {"VTIMR2",  0x0212}, {"VTIMR2+1",0x0213},
	{"VTIMR4",  0x0214}, {"VTIMR4+1",0x0215}, {"VIMIRQ",  0x0216}, {"VIMIRQ+1",0x0217},
	{"CDTMV1",  0x0218}, {"CDTMV1+1",0x0219}, {"CDTMV2",  0x021a}, {"CDTMV2+1",0x021b},
	{"CDTMV3",  0x021c}, {"CDTMV3+1",0x021d}, {"CDTMV4",  0x021e}, {"CDTMV4+1",0x021f},
	{"CDTMV5",  0x0220}, {"CDTMV5+1",0x0221}, {"VVBLKI",  0x0222}, {"VVBLKI+1",0x0223},
	{"VVBLKD",  0x0224}, {"VVBLKD+1",0x0225}, {"CDTMA1",  0x0226}, {"CDTMA1+1",0x0227},
	{"CDTMA2",  0x0228}, {"CDTMA2+1",0x0229}, {"CDTMF3",  0x022a}, {"SRTIMR",  0x022b},
	{"CDTMF4",  0x022c}, {"INTEMP",  0x022d}, {"CDTMF5",  0x022e}, {"SDMCTL",  0x022f},
	{"SDLSTL",  0x0230}, {"SDLSTH",  0x0231}, {"SSKCTL",  0x0232}, {"SPARE",   0x0233},
	{"LPENH",   0x0234}, {"LPENV",   0x0235}, {"BRKKY",   0x0236}, {"BRKKY+1", 0x0237},
	{"VPIRQ",   0x0238}, {"VPIRQ+1", 0x0239}, {"CDEVIC",  0x023a}, {"CCOMND",  0x023b},
	{"CAUX1",   0x023c}, {"CAUX2",   0x023d}, {"TMPSIO",  0x023e}, {"ERRFLG",  0x023f},
	{"DFLAGS",  0x0240}, {"DBSECT",  0x0241}, {"BOOTAD",  0x0242}, {"BOOTAD+1",0x0243},
	{"COLDST",  0x0244}, {"RECLEN",  0x0245}, {"DSKTIM",  0x0246}, {"PDVMSK",  0x0247},
	{"SHPDVS",  0x0248}, {"PDMSK",   0x0249}, {"RELADR",  0x024a}, {"RELADR+1",0x024b},
	{"PPTMPA",  0x024c}, {"PPTMPX",  0x024d}, {"CHSALT",  0x026b}, {"VSFLAG",  0x026c},
	{"KEYDIS",  0x026d}, {"FINE",    0x026e}, {"GPRIOR",  0x026f}, {"PADDL0",  0x0270},
	{"PADDL1",  0x0271}, {"PADDL2",  0x0272}, {"PADDL3",  0x0273}, {"PADDL4",  0x0274},
	{"PADDL5",  0x0275}, {"PADDL6",  0x0276}, {"PADDL7",  0x0277}, {"STICK0",  0x0278},
	{"STICK1",  0x0279}, {"STICK2",  0x027a}, {"STICK3",  0x027b}, {"PTRIG0",  0x027c},
	{"PTRIG1",  0x027d}, {"PTRIG2",  0x027e}, {"PTRIG3",  0x027f}, {"PTRIG4",  0x0280},
	{"PTRIG5",  0x0281}, {"PTRIG6",  0x0282}, {"PTRIG7",  0x0283}, {"STRIG0",  0x0284},
	{"STRIG1",  0x0285}, {"STRIG2",  0x0286}, {"STRIG3",  0x0287}, {"HIBYTE",  0x0288},
	{"WMODE",   0x0289}, {"BLIM",    0x028a}, {"IMASK",   0x028b}, {"JVECK",   0x028c},
	{"NEWADR",  0x028e}, {"TXTROW",  0x0290}, {"TXTCOL",  0x0291}, {"TXTCOL+1",0x0292},
	{"TINDEX",  0x0293}, {"TXTMSC",  0x0294}, {"TXTMSC+1",0x0295}, {"TXTOLD",  0x0296},
	{"TXTOLD+1",0x0297}, {"TXTOLD+2",0x0298}, {"TXTOLD+3",0x0299}, {"TXTOLD+4",0x029a},
	{"TXTOLD+5",0x029b}, {"CRETRY",  0x029c}, {"HOLD3",   0x029d}, {"SUBTMP",  0x029e},
	{"HOLD2",   0x029f}, {"DMASK",   0x02a0}, {"TMPLBT",  0x02a1}, {"ESCFLG",  0x02a2},
	{"TABMAP",  0x02a3}, {"TABMAP+1",0x02a4}, {"TABMAP+2",0x02a5}, {"TABMAP+3",0x02a6},
	{"TABMAP+4",0x02a7}, {"TABMAP+5",0x02a8}, {"TABMAP+6",0x02a9}, {"TABMAP+7",0x02aa},
	{"TABMAP+8",0x02ab}, {"TABMAP+9",0x02ac}, {"TABMAP+A",0x02ad}, {"TABMAP+B",0x02ae},
	{"TABMAP+C",0x02af}, {"TABMAP+D",0x02b0}, {"TABMAP+E",0x02b1}, {"LOGMAP",  0x02b2},
	{"LOGMAP+1",0x02b3}, {"LOGMAP+2",0x02b4}, {"LOGMAP+3",0x02b5}, {"INVFLG",  0x02b6},
	{"FILFLG",  0x02b7}, {"TMPROW",  0x02b8}, {"TMPCOL",  0x02b9}, {"TMPCOL+1",0x02ba},
	{"SCRFLG",  0x02bb}, {"HOLD4",   0x02bc}, {"DRETRY",  0x02bd}, {"SHFLOC",  0x02be},
	{"BOTSCR",  0x02bf}, {"PCOLR0",  0x02c0}, {"PCOLR1",  0x02c1}, {"PCOLR2",  0x02c2},
	{"PCOLR3",  0x02c3}, {"COLOR0",  0x02c4}, {"COLOR1",  0x02c5}, {"COLOR2",  0x02c6},
	{"COLOR3",  0x02c7}, {"COLOR4",  0x02c8}, {"RUNADR",  0x02c9}, {"RUNADR+1",0x02ca},
	{"HIUSED",  0x02cb}, {"HIUSED+1",0x02cc}, {"ZHIUSE",  0x02cd}, {"ZHIUSE+1",0x02ce},
	{"GBYTEA",  0x02cf}, {"GBYTEA+1",0x02d0}, {"LOADAD",  0x02d1}, {"LOADAD+1",0x02d2},
	{"ZLOADA",  0x02d3}, {"ZLOADA+1",0x02d4}, {"DSCTLN",  0x02d5}, {"DSCTLN+1",0x02d6},
	{"ACMISR",  0x02d7}, {"ACMISR+1",0x02d8}, {"KRPDER",  0x02d9}, {"KEYREP",  0x02da},
	{"NOCLIK",  0x02db}, {"HELPFG",  0x02dc}, {"DMASAV",  0x02dd}, {"PBPNT",   0x02de},
	{"PBUFSZ",  0x02df}, {"RUNAD",   0x02e0}, {"RUNAD+1", 0x02e1}, {"INITAD",  0x02e2},
	{"INITAD+1",0x02e3}, {"RAMSIZ",  0x02e4}, {"MEMTOP",  0x02e5}, {"MEMTOP+1",0x02e6},
	{"MEMLO",   0x02e7}, {"MEMLO+1", 0x02e8}, {"HNDLOD",  0x02e9}, {"DVSTAT",  0x02ea},
	{"DVSTAT+1",0x02eb}, {"DVSTAT+2",0x02ec}, {"DVSTAT+3",0x02ed}, {"CBAUDL",  0x02ee},
	{"CBAUDH",  0x02ef}, {"CRSINH",  0x02f0}, {"KEYDEL",  0x02f1}, {"CH1",     0x02f2},
	{"CHACT",   0x02f3}, {"CHBAS",   0x02f4}, {"NEWROW",  0x02f5}, {"NEWCOL",  0x02f6},
	{"NEWCOL+1",0x02f7}, {"ROWINC",  0x02f8}, {"COLINC",  0x02f9}, {"CHAR",    0x02fa},
	{"ATACHR",  0x02fb}, {"CH",      0x02fc}, {"FILDAT",  0x02fd}, {"DSPFLG",  0x02fe},
	{"SSFLAG",  0x02ff},

	{"DDEVIC",  0x0300}, {"DUNIT",   0x0301}, {"DCOMND",  0x0302}, {"DSTATS",  0x0303},
	{"DBUFLO",  0x0304}, {"DBUFHI",  0x0305}, {"DTIMLO",  0x0306}, {"DUNUSE",  0x0307},
	{"DBYTLO",  0x0308}, {"DBYTHI",  0x0309}, {"DAUX1",   0x030a}, {"DAUX2",   0x030b},
	{"TIMER1",  0x030c}, {"TIMER1+1",0x030d}, {"ADDCOR",  0x030e}, {"CASFLG",  0x030f},
	{"TIMER2",  0x0310}, {"TIMER2+1",0x0311}, {"TEMP1",   0x0312}, {"TEMP1+1", 0x0313},
	{"TEMP2",   0x0314}, {"TEMP3",   0x0315}, {"SAVIO",   0x0316}, {"TIMFLG",  0x0317},
	{"STACKP",  0x0318}, {"TSTAT",   0x0319}, {"HATABS",  0x031a}, /* HATABS 1-34 */
	{"PUTBT1",  0x033d}, {"PUTBT2",  0x033e}, {"PUTBT3",  0x033f},
	{"B0-ICHID",0x0340}, {"B0-ICDNO",0x0341}, {"B0-ICCOM",0x0342}, {"B0-ICSTA",0x0343},
	{"B0-ICBAL",0x0344}, {"B0-ICBAH",0x0345}, {"B0-ICPTL",0x0346}, {"B0-ICPTH",0x0347},
	{"B0-ICBLL",0x0348}, {"B0-ICBLH",0x0349}, {"B0-ICAX1",0x034a}, {"B0-ICAX2",0x034b},
	{"B0-ICAX3",0x034c}, {"B0-ICAX4",0x034d}, {"B0-ICAX5",0x034e}, {"B0-ICAX6",0x034f},
	{"B1-ICHID",0x0350}, {"B1-ICDNO",0x0351}, {"B1-ICCOM",0x0352}, {"B1-ICSTA",0x0353},
	{"B1-ICBAL",0x0354}, {"B1-ICBAH",0x0355}, {"B1-ICPTL",0x0356}, {"B1-ICPTH",0x0357},
	{"B1-ICBLL",0x0358}, {"B1-ICBLH",0x0359}, {"B1-ICAX1",0x035a}, {"B1-ICAX2",0x035b},
	{"B1-ICAX3",0x035c}, {"B1-ICAX4",0x035d}, {"B1-ICAX5",0x035e}, {"B1-ICAX6",0x035f},
	{"B2-ICHID",0x0360}, {"B2-ICDNO",0x0361}, {"B2-ICCOM",0x0362}, {"B2-ICSTA",0x0363},
	{"B2-ICBAL",0x0364}, {"B2-ICBAH",0x0365}, {"B2-ICPTL",0x0366}, {"B2-ICPTH",0x0367},
	{"B2-ICBLL",0x0368}, {"B2-ICBLH",0x0369}, {"B2-ICAX1",0x036a}, {"B2-ICAX2",0x036b},
	{"B2-ICAX3",0x036c}, {"B2-ICAX4",0x036d}, {"B2-ICAX5",0x036e}, {"B2-ICAX6",0x036f},
	{"B3-ICHID",0x0370}, {"B3-ICDNO",0x0371}, {"B3-ICCOM",0x0372}, {"B3-ICSTA",0x0373},
	{"B3-ICBAL",0x0374}, {"B3-ICBAH",0x0375}, {"B3-ICPTL",0x0376}, {"B3-ICPTH",0x0377},
	{"B3-ICBLL",0x0378}, {"B3-ICBLH",0x0379}, {"B3-ICAX1",0x037a}, {"B3-ICAX2",0x037b},
	{"B3-ICAX3",0x037c}, {"B3-ICAX4",0x037d}, {"B3-ICAX5",0x037e}, {"B3-ICAX6",0x037f},
	{"B4-ICHID",0x0380}, {"B4-ICDNO",0x0381}, {"B4-ICCOM",0x0382}, {"B4-ICSTA",0x0383},
	{"B4-ICBAL",0x0384}, {"B4-ICBAH",0x0385}, {"B4-ICPTL",0x0386}, {"B4-ICPTH",0x0387},
	{"B4-ICBLL",0x0388}, {"B4-ICBLH",0x0389}, {"B4-ICAX1",0x038a}, {"B4-ICAX2",0x038b},
	{"B4-ICAX3",0x038c}, {"B4-ICAX4",0x038d}, {"B4-ICAX5",0x038e}, {"B4-ICAX6",0x038f},
	{"B5-ICHID",0x0390}, {"B5-ICDNO",0x0391}, {"B5-ICCOM",0x0392}, {"B5-ICSTA",0x0393},
	{"B5-ICBAL",0x0394}, {"B5-ICBAH",0x0395}, {"B5-ICPTL",0x0396}, {"B5-ICPTH",0x0397},
	{"B5-ICBLL",0x0398}, {"B5-ICBLH",0x0399}, {"B5-ICAX1",0x039a}, {"B5-ICAX2",0x039b},
	{"B5-ICAX3",0x039c}, {"B5-ICAX4",0x039d}, {"B5-ICAX5",0x039e}, {"B5-ICAX6",0x039f},
	{"B6-ICHID",0x03a0}, {"B6-ICDNO",0x03a1}, {"B6-ICCOM",0x03a2}, {"B6-ICSTA",0x03a3},
	{"B6-ICBAL",0x03a4}, {"B6-ICBAH",0x03a5}, {"B6-ICPTL",0x03a6}, {"B6-ICPTH",0x03a7},
	{"B6-ICBLL",0x03a8}, {"B6-ICBLH",0x03a9}, {"B6-ICAX1",0x03aa}, {"B6-ICAX2",0x03ab},
	{"B6-ICAX3",0x03ac}, {"B6-ICAX4",0x03ad}, {"B6-ICAX5",0x03ae}, {"B6-ICAX6",0x03af},
	{"B7-ICHID",0x03b0}, {"B7-ICDNO",0x03b1}, {"B7-ICCOM",0x03b2}, {"B7-ICSTA",0x03b3},
	{"B7-ICBAL",0x03b4}, {"B7-ICBAH",0x03b5}, {"B7-ICPTL",0x03b6}, {"B7-ICPTH",0x03b7},
	{"B7-ICBLL",0x03b8}, {"B7-ICBLH",0x03b9}, {"B7-ICAX1",0x03ba}, {"B7-ICAX2",0x03bb},
	{"B7-ICAX3",0x03bc}, {"B7-ICAX4",0x03bd}, {"B7-ICAX5",0x03be}, {"B7-ICAX6",0x03bf},
	{"PRNBUF",  0x03c0}, /* PRNBUF 1-39 */
	{"SUPERF",  0x03e8}, {"CKEY",    0x03e9}, {"CASSBT",  0x03ea}, {"CARTCK",  0x03eb},
	{"DERRF",   0x03ec}, {"ACMVAR",  0x03ed}, /* ACMVAR 1-10 */
	{"BASICF",  0x03f8}, {"MINTLK",  0x03f9}, {"GINTLK",  0x03fa}, {"CHLINK",  0x03fb},
	{"CHLINK+1",0x03fc}, {"CASBUF",  0x03fd},

	{"M0PF",  0xd000}, {"HPOSP0",0xd000}, {"M1PF",  0xd001}, {"HPOSP1",0xd001},
	{"M2PF",  0xd002}, {"HPOSP2",0xd002}, {"M3PF",  0xd003}, {"HPOSP3",0xd003},
	{"P0PF",  0xd004}, {"HPOSM0",0xd004}, {"P1PF",  0xd005}, {"HPOSM1",0xd005},
	{"P2PF",  0xd006}, {"HPOSM2",0xd006}, {"P3PF",  0xd007}, {"HPOSM3",0xd007},
	{"M0PL",  0xd008}, {"SIZEP0",0xd008}, {"M1PL",  0xd009}, {"SIZEP1",0xd009},
	{"M2PL",  0xd00a}, {"SIZEP2",0xd00a}, {"M3PL",  0xd00b}, {"SIZEP3",0xd00b},
	{"P0PL",  0xd00c}, {"SIZEM", 0xd00c}, {"P1PL",  0xd00d}, {"GRAFP0",0xd00d},
	{"P2PL",  0xd00e}, {"GRAFP1",0xd00e}, {"P3PL",  0xd00f}, {"GRAFP2",0xd00f},
	{"TRIG0", 0xd010}, {"GRAFP3",0xd010}, {"TRIG1", 0xd011}, {"GRAFM", 0xd011},
	{"TRIG2", 0xd012}, {"COLPM0",0xd012}, {"TRIG3", 0xd013}, {"COLPM1",0xd013},
	{"PAL",   0xd014}, {"COLPM2",0xd014}, {"COLPM3",0xd015}, {"COLPF0",0xd016},
	{"COLPF1",0xd017},
	{"COLPF2",0xd018}, {"COLPF3",0xd019}, {"COLBK", 0xd01a}, {"PRIOR", 0xd01b},
	{"VDELAY",0xd01c}, {"GRACTL",0xd01d}, {"HITCLR",0xd01e}, {"CONSOL",0xd01f},

	{"POT0",  0xd200}, {"AUDF1", 0xd200}, {"POT1",  0xd201}, {"AUDC1", 0xd201},
	{"POT2",  0xd202}, {"AUDF2", 0xd202}, {"POT3",  0xd203}, {"AUDC2", 0xd203},
	{"POT4",  0xd204}, {"AUDF3", 0xd204}, {"POT5",  0xd205}, {"AUDC3", 0xd205},
	{"POT6",  0xd206}, {"AUDF4", 0xd206}, {"POT7",  0xd207}, {"AUDC4", 0xd207},
	{"ALLPOT",0xd208}, {"AUDCTL",0xd208}, {"KBCODE",0xd209}, {"STIMER",0xd209},
	{"RANDOM",0xd20a}, {"SKREST",0xd20a}, {"POTGO", 0xd20b},
	{"SERIN", 0xd20d}, {"SEROUT",0xd20d}, {"IRQST", 0xd20e}, {"IRQEN", 0xd20e},
	{"SKSTAT",0xd20f}, {"SKCTL", 0xd20f},

	{"PORTA", 0xd300}, {"PORTB", 0xd301}, {"PACTL", 0xd302}, {"PBCTL", 0xd303},

	{"DMACTL",0xd400}, {"CHACTL",0xd401}, {"DLISTL",0xd402}, {"DLISTH",0xd403},
	{"HSCROL",0xd404}, {"VSCROL",0xd405}, {"PMBASE",0xd407}, {"CHBASE",0xd409},
	{"WSYNC", 0xd40a}, {"VCOUNT",0xd40b}, {"PENH",  0xd40c}, {"PENV",  0xd40d},
	{"NMIEN", 0xd40e}, {"NMIST", 0xd40f}, {"NMIRES",0xd40f},

	{"AFP",   0xd800}, {"FASC",  0xd8e6}, {"IFP",   0xd9aa}, {"FPI",   0xd9d2},
	{"ZPRO",  0xda44}, {"ZF1",   0xda46}, {"FSUB",  0xda60}, {"FADD",  0xda66},
	{"FMUL",  0xdadb}, {"FDIV",  0xdb28}, {"PLYEVL",0xdd40}, {"FLD0R", 0xdd89},
	{"FLD0R", 0xdd8d}, {"FLD1R", 0xdd98}, {"FLD1P", 0xdd9c}, {"FST0R", 0xdda7},
	{"FST0P", 0xddab}, {"FMOVE", 0xddb6}, {"EXP",   0xddc0}, {"EXP10", 0xddcc},
	{"LOG",   0xdecd}, {"LOG10", 0xded1},

	{"DSKINV",0xe453}, {"CIOV",  0xe456}, {"SIOV",  0xe459}, {"SETVBV",0xe45c},
	{"SYSVBV",0xe45f}, {"XITVBV",0xe462}, {"SIOINV",0xe465}, {"SENDEV",0xe468},
	{"INTINV",0xe46b}, {"CIOINV",0xe46e}, {"SELFSV",0xe471}, {"WARMSV",0xe474},
	{"COLDSV",0xe477}, {"RBLOKV",0xe47a}, {"CSOPIV",0xe47d}, {"PUPDIV",0xe480},
	{"SELFTSV",0xe483},{"PENTV", 0xe486}, {"PHUNLV",0xe489}, {"PHINIV",0xe48c},
	{"GPDVV", 0xe48f},

	{NULL,    0x0000}
};

#ifdef MACOSX_MON_ENHANCEMENTS
const symtable_rec symtable_builtin_5200[] = {
#else
static const symtable_rec symtable_builtin_5200[] = {
#endif
{"POKMSK",  0x0000}, {"RTCLOKH",  0x0001}, {"RTCLOKL",0x0002}, {"CRITIC",   0x0003},
{"ATRACT", 0x0004}, {"SDLSTL",  0x0005}, {"SDLSTH",   0x0006}, {"SDMCTL",  0x0007},
{"PCOLR0",    0x0008}, {"PCOLR1",  0x0009}, {"PCOLR2",0x000a}, {"PCOLR3",  0x000b},
{"COLOR0",0x000c}, {"COLOR1",  0x000d}, {"COLOR2",0x000e}, {"COLOR3",  0x000f},
{"COLOR4",  0x0010}, {"PADDL0",  0x0011}, {"PADDL1",0x0012}, {"PADDL2",0x0013},
{"PADDL3",  0x0014}, {"PADDL4",0x0015}, {"PADDL5",  0x0016}, {"PADDL6",  0x0017},
{"PADDL7",0x0018},

{"VIMIRQ",  0x0200}, {"VIMIRQ+1",0x0201}, {"VVBLKI",  0x0202}, {"VVBLKI+1",0x0203},
{"VVBLKD",  0x0204}, {"VVBLKD+1",0x0205}, {"VDSLST",  0x0206}, {"VDSLST+1",0x0207},
{"VKEYBD",  0x0208}, {"VKEYBD+1",0x0209}, {"VKPD",  0x020a}, {"VKPD+1",0x020b},
{"BRKKY",  0x020c}, {"BRKKY+1",0x020d}, {"VBREAK",  0x020e}, {"VBREAK+1",0x020f},
{"VSERIN",  0x0210}, {"VSERIN+1",0x0211}, {"VSEROR",  0x0212}, {"VSEROR+1",0x0213},
{"VSEROC",  0x0214}, {"VSEROC+1",0x0215}, {"VTIMR1",  0x0216}, {"VTIMR1+1",0x0217},
{"VTIMR2",  0x0218}, {"VTIMR2+1",0x0219}, {"VTIMR4",  0x021a}, {"VTIMR4+1",0x021b},

{"M0PF",  0xc000}, {"HPOSP0",0xc000}, {"M1PF",  0xc001}, {"HPOSP1",0xc001},
{"M2PF",  0xc002}, {"HPOSP2",0xc002}, {"M3PF",  0xc003}, {"HPOSP3",0xc003},
{"P0PF",  0xc004}, {"HPOSM0",0xc004}, {"P1PF",  0xc005}, {"HPOSM1",0xc005},
{"P2PF",  0xc006}, {"HPOSM2",0xc006}, {"P3PF",  0xc007}, {"HPOSM3",0xc007},
{"M0PL",  0xc008}, {"SIZEP0",0xc008}, {"M1PL",  0xc009}, {"SIZEP1",0xc009},
{"M2PL",  0xc00a}, {"SIZEP2",0xc00a}, {"M3PL",  0xc00b}, {"SIZEP3",0xc00b},
{"P0PL",  0xc00c}, {"SIZEM", 0xc00c}, {"P1PL",  0xc00d}, {"GRAFP0",0xc00d},
{"P2PL",  0xc00e}, {"GRAFP1",0xc00e}, {"P3PL",  0xc00f}, {"GRAFP2",0xc00f},
{"TRIG0", 0xc010}, {"GRAFP3",0xc010}, {"TRIG1", 0xc011}, {"GRAFM", 0xc011},
{"TRIG2", 0xc012}, {"COLPM0",0xc012}, {"TRIG3", 0xc013}, {"COLPM1",0xc013},
{"PAL",   0xc014}, {"COLPM2",0xc014}, {"COLPM3",0xc015}, {"COLPF0",0xc016},
{"COLPF1",0xc017},
{"COLPF2",0xc018}, {"COLPF3",0xc019}, {"COLBK", 0xc01a}, {"PRIOR", 0xc01b},
{"VDELAY",0xc01c}, {"GRACTL",0xc01d}, {"HITCLR",0xc01e}, {"CONSOL",0xc01f},
{"DMACTL",0xd400}, {"CHACTL",0xd401}, {"DLISTL",0xd402}, {"DLISTH",0xd403},
{"HSCROL",0xd404}, {"VSCROL",0xd405}, {"PMBASE",0xd407}, {"CHBASE",0xd409},
{"WSYNC", 0xd40a}, {"VCOUNT",0xd40b}, {"PENH",  0xd40c}, {"PENV",  0xd40d},
{"NMIEN", 0xd40e}, {"NMIST", 0xd40f}, {"NMIRES",0xd40f},

{"POT0",  0xe800}, {"AUDF1", 0xe800}, {"POT1",  0xe801}, {"AUDC1", 0xe801},
{"POT2",  0xe802}, {"AUDF2", 0xe802}, {"POT3",  0xe803}, {"AUDC2", 0xe803},
{"POT4",  0xe804}, {"AUDF3", 0xe804}, {"POT5",  0xe805}, {"AUDC3", 0xe805},
{"POT6",  0xe806}, {"AUDF4", 0xe806}, {"POT7",  0xe807}, {"AUDC4", 0xe807},
{"ALLPOT",0xe808}, {"AUDCTL",0xe808}, {"KBCODE",0xe809}, {"STIMER",0xe809},
{"RANDOM",0xe80a}, {"SKREST",0xe80a}, {"POTGO", 0xe80b},
{"SERIN", 0xe80d}, {"SEROUT",0xe80d}, {"IRQST", 0xe80e}, {"IRQEN", 0xe80e},
{"SKSTAT",0xe80f}, {"SKCTL", 0xe80f},

{NULL,    0x0000}
};

#ifdef MACOSX
int symtable_builtin_enable = TRUE;
symtable_rec *symtable_user = NULL;
int symtable_user_size = 0;
#else
static int symtable_builtin_enable = TRUE;
static symtable_rec *symtable_user = NULL;
static int symtable_user_size = 0;
#endif

static const char *find_label_name(UWORD addr, int write)
{
	int i;
	for (i = 0; i < symtable_user_size; i++) {
		if (symtable_user[i].addr == addr)
			return symtable_user[i].name;
	}
	if (symtable_builtin_enable) {
		const symtable_rec *p;
		for (p = (Atari800_machine_type == Atari800_MACHINE_5200 ? symtable_builtin_5200 : symtable_builtin); p->name != NULL; p++) {
			if (p->addr == addr) {
				if (write && p[1].addr == addr)
					p++;
				return p->name;
			}
		}
	}
	return NULL;
}

#ifdef MACOSX_MON_ENHANCEMENTS
symtable_rec *find_user_label(const char *name)
#else
static symtable_rec *find_user_label(const char *name)
#endif
{
	int i;
	for (i = 0; i < symtable_user_size; i++) {
		if (Util_stricmp(symtable_user[i].name, name) == 0)
			return &symtable_user[i];
	}
	return NULL;
}

#ifdef MACOSX_MON_ENHANCEMENTS
int find_label_value(const char *name)
#else
static int find_label_value(const char *name)
#endif
{
	const symtable_rec *p = find_user_label(name);
	if (p != NULL)
		return p->addr;
	if (symtable_builtin_enable) {
		for (p = (Atari800_machine_type == Atari800_MACHINE_5200 ? symtable_builtin_5200 : symtable_builtin); p->name != NULL; p++) {
			if (Util_stricmp(p->name, name) == 0)
				return p->addr;
		}
	}
	return -1;
}

#ifdef MACOSX_MON_ENHANCEMENTS
void free_user_labels(void)
#else
static void free_user_labels(void)
#endif
{
	if (symtable_user != NULL) {
		while (symtable_user_size > 0)
			free(symtable_user[--symtable_user_size].name);
		free(symtable_user);
		symtable_user = NULL;
	}
}

#ifdef MACOSX_MON_ENHANCEMENTS
void add_user_label(const char *name, UWORD addr)
#else
static void add_user_label(const char *name, UWORD addr)
#endif
{
#define SYMTABLE_USER_INITIAL_SIZE 128
	if (symtable_user == NULL)
		symtable_user = (symtable_rec *) Util_malloc(SYMTABLE_USER_INITIAL_SIZE * sizeof(symtable_rec));
	else if (symtable_user_size >= SYMTABLE_USER_INITIAL_SIZE
	      && (symtable_user_size & (symtable_user_size - 1)) == 0) {
		/* symtable_user_size is a power of two: allocate twice as much */
		symtable_user = (symtable_rec *) Util_realloc(symtable_user,
			2 * symtable_user_size * sizeof(symtable_rec));
	}
	symtable_user[symtable_user_size].name = Util_strdup(name);
	symtable_user[symtable_user_size].addr = addr;
	symtable_user_size++;
}

#ifdef MACOSX_MON_ENHANCEMENTS
void load_user_labels(const char *filename)
#else
static void load_user_labels(const char *filename)
#endif
{
	FILE *fp;
	char line[256];
	if (filename == NULL) {
		mon_printf("You must specify a filename\n");
		return;
	}
	/* "rb" and not "r", because we strip EOLs ourselves
	   - this is better, because we can use CR/LF files on Unix */
	fp = fopen(filename, "rb");
	if (fp == NULL) {
		perror(filename);
		return;
	}
#ifndef MACOSX_MON_ENHANCEMENTS   // We don't clear labels on a load so you can merge two files
	free_user_labels();
#endif
	while (fgets(line, sizeof(line), fp)) {
		char *p;
		unsigned int value = 0;
		int digits = 0;
		/* Find first 4 hex digits or more. */
		/* We don't support "Cafe Assembler", "Dead Assembler" or "C0de Assembler". ;-) */
		for (p = line; *p != '\0'; p++) {
			if (*p >= '0' && *p <= '9') {
				value = (value << 4) + *p - '0';
				digits++;
			}
			else if (*p >= 'A' && *p <= 'F') {
				value = (value << 4) + *p - 'A' + 10;
				digits++;
			}
			else if (*p >= 'a' && *p <= 'f') {
				value = (value << 4) + *p - 'a' + 10;
				digits++;
			}
			else if (digits >= 4)
				break;
			else if (*p == '-')
				break; /* ignore labels with negative values */
			else {
				/* note that xasm can put "2" before the label value and mads puts "00" */
				value = 0;
				digits = 0;
			}
		}
		if (*p != ' ' && *p != '\t')
			continue;
		if (value > 0xffff || digits > 8)
			continue;
		do
			p++;
		while (*p == ' ' || *p == '\t');
		Util_chomp(p);
		if (*p == '\0')
			continue;
		add_user_label(p, (UWORD) value);
	}
	fclose(fp);
	mon_printf("Loaded %d labels\n", symtable_user_size);
}

#endif

static const char instr6502[256][10] = {
	"BRK", "ORA (1,X)", "CIM", "ASO (1,X)", "NOP 1", "ORA 1", "ASL 1", "ASO 1",
	"PHP", "ORA #1", "ASL", "ANC #1", "NOP 2", "ORA 2", "ASL 2", "ASO 2",

	"BPL 0", "ORA (1),Y", "CIM", "ASO (1),Y", "NOP 1,X", "ORA 1,X", "ASL 1,X", "ASO 1,X",
	"CLC", "ORA 2,Y", "NOP !", "ASO 2,Y", "NOP 2,X", "ORA 2,X", "ASL 2,X", "ASO 2,X",

	"JSR 2", "AND (1,X)", "CIM", "RLA (1,X)", "BIT 1", "AND 1", "ROL 1", "RLA 1",
	"PLP", "AND #1", "ROL", "ANC #1", "BIT 2", "AND 2", "ROL 2", "RLA 2",

	"BMI 0", "AND (1),Y", "CIM", "RLA (1),Y", "NOP 1,X", "AND 1,X", "ROL 1,X", "RLA 1,X",
	"SEC", "AND 2,Y", "NOP !", "RLA 2,Y", "NOP 2,X", "AND 2,X", "ROL 2,X", "RLA 2,X",


	"RTI", "EOR (1,X)", "CIM", "LSE (1,X)", "NOP 1", "EOR 1", "LSR 1", "LSE 1",
	"PHA", "EOR #1", "LSR", "ALR #1", "JMP 2", "EOR 2", "LSR 2", "LSE 2",

	"BVC 0", "EOR (1),Y", "CIM", "LSE (1),Y", "NOP 1,X", "EOR 1,X", "LSR 1,X", "LSE 1,X",
	"CLI", "EOR 2,Y", "NOP !", "LSE 2,Y", "NOP 2,X", "EOR 2,X", "LSR 2,X", "LSE 2,X",

	"RTS", "ADC (1,X)", "CIM", "RRA (1,X)", "NOP 1", "ADC 1", "ROR 1", "RRA 1",
	"PLA", "ADC #1", "ROR", "ARR #1", "JMP (2)", "ADC 2", "ROR 2", "RRA 2",

	"BVS 0", "ADC (1),Y", "CIM", "RRA (1),Y", "NOP 1,X", "ADC 1,X", "ROR 1,X", "RRA 1,X",
	"SEI", "ADC 2,Y", "NOP !", "RRA 2,Y", "NOP 2,X", "ADC 2,X", "ROR 2,X", "RRA 2,X",


	"NOP #1", "STA (1,X)", "NOP #1", "SAX (1,X)", "STY 1", "STA 1", "STX 1", "SAX 1",
	"DEY", "NOP #1", "TXA", "ANE #1", "STY 2", "STA 2", "STX 2", "SAX 2",

	"BCC 0", "STA (1),Y", "CIM", "SHA (1),Y", "STY 1,X", "STA 1,X", "STX 1,Y", "SAX 1,Y",
	"TYA", "STA 2,Y", "TXS", "SHS 2,Y", "SHY 2,X", "STA 2,X", "SHX 2,Y", "SHA 2,Y",

	"LDY #1", "LDA (1,X)", "LDX #1", "LAX (1,X)", "LDY 1", "LDA 1", "LDX 1", "LAX 1",
	"TAY", "LDA #1", "TAX", "ANX #1", "LDY 2", "LDA 2", "LDX 2", "LAX 2",

	"BCS 0", "LDA (1),Y", "CIM", "LAX (1),Y", "LDY 1,X", "LDA 1,X", "LDX 1,Y", "LAX 1,X",
	"CLV", "LDA 2,Y", "TSX", "LAS 2,Y", "LDY 2,X", "LDA 2,X", "LDX 2,Y", "LAX 2,Y",


	"CPY #1", "CMP (1,X)", "NOP #1", "DCM (1,X)", "CPY 1", "CMP 1", "DEC 1", "DCM 1",
	"INY", "CMP #1", "DEX", "SBX #1", "CPY 2", "CMP 2", "DEC 2", "DCM 2",

	"BNE 0", "CMP (1),Y", "ESCRTS #1", "DCM (1),Y", "NOP 1,X", "CMP 1,X", "DEC 1,X", "DCM 1,X",
	"CLD", "CMP 2,Y", "NOP !", "DCM 2,Y", "NOP 2,X", "CMP 2,X", "DEC 2,X", "DCM 2,X",


	"CPX #1", "SBC (1,X)", "NOP #1", "INS (1,X)", "CPX 1", "SBC 1", "INC 1", "INS 1",
	"INX", "SBC #1", "NOP", "SBC #1 !", "CPX 2", "SBC 2", "INC 2", "INS 2",

	"BEQ 0", "SBC (1),Y", "ESCAPE #1", "INS (1),Y", "NOP 1,X", "SBC 1,X", "INC 1,X", "INS 1,X",
	"SED", "SBC 2,Y", "NOP !", "INS 2,Y", "NOP 2,X", "SBC 2,X", "INC 2,X", "INS 2,X"
};

/* Opcode type:
   bits 1-0 = instruction length
   bit 2    = instruction reads from memory (without stack-manipulating instructions)
   bit 3    = instruction writes to memory (without stack-manipulating instructions)
   bits 7-4 = adressing type:
     0 = NONE (implicit)
     1 = ABSOLUTE
     2 = ZPAGE
     3 = ABSOLUTE_X
     4 = ABSOLUTE_Y
     5 = INDIRECT_X
     6 = INDIRECT_Y
     7 = ZPAGE_X
     8 = ZPAGE_Y
     9 = RELATIVE
     A = IMMEDIATE
     B = STACK 2 (RTS)
     C = STACK 3 (RTI)
     D = INDIRECT (JMP () )
     E = ESCRTS
     F = ESCAPE */
const UBYTE MONITOR_optype6502[256] = {
	0x01, 0x56, 0x01, 0x5e, 0x22, 0x26, 0x2e, 0x2e, 0x01, 0xa2, 0x01, 0xa2, 0x13, 0x17, 0x1f, 0x1f,
	0x92, 0x66, 0x01, 0x6e, 0x72, 0x76, 0x7e, 0x7e, 0x01, 0x47, 0x01, 0x4f, 0x33, 0x37, 0x3f, 0x3f,
	0x13, 0x56, 0x01, 0x5e, 0x26, 0x26, 0x2e, 0x2e, 0x01, 0xa2, 0x01, 0xa2, 0x17, 0x17, 0x1f, 0x1f,
	0x92, 0x66, 0x01, 0x6e, 0x72, 0x76, 0x7e, 0x7e, 0x01, 0x47, 0x01, 0x4f, 0x33, 0x37, 0x3f, 0x3f,
	0xc1, 0x56, 0x01, 0x5e, 0x22, 0x26, 0x2e, 0x2e, 0x01, 0xa2, 0x01, 0xa2, 0x13, 0x17, 0x1f, 0x1f,
	0x92, 0x66, 0x01, 0x6e, 0x72, 0x76, 0x7e, 0x7e, 0x01, 0x47, 0x01, 0x4f, 0x33, 0x37, 0x3f, 0x3f,
	0xb1, 0x56, 0x01, 0x5e, 0x22, 0x26, 0x2e, 0x2e, 0x01, 0xa2, 0x01, 0xa2, 0xd3, 0x17, 0x1f, 0x1f,
	0x92, 0x66, 0x01, 0x6e, 0x72, 0x76, 0x7e, 0x7e, 0x01, 0x47, 0x01, 0x4f, 0x33, 0x37, 0x3f, 0x3f,
	0xa2, 0x5a, 0x01, 0x5a, 0x2a, 0x2a, 0x2a, 0x2a, 0x01, 0xa2, 0x01, 0xa2, 0x1b, 0x1b, 0x1b, 0x1b,
	0x92, 0x6a, 0x01, 0x6a, 0x7a, 0x7a, 0x8a, 0x8a, 0x01, 0x4b, 0x01, 0x4b, 0x3b, 0x3b, 0x4b, 0x4b,
	0xa2, 0x56, 0xa2, 0x56, 0x26, 0x26, 0x26, 0x26, 0x01, 0xa2, 0x01, 0xa2, 0x17, 0x17, 0x17, 0x17,
	0x92, 0x66, 0x01, 0x66, 0x76, 0x76, 0x86, 0x86, 0x01, 0x47, 0x01, 0x47, 0x37, 0x37, 0x47, 0x47,
	0xa2, 0x56, 0xa2, 0x5e, 0x26, 0x26, 0x2e, 0x2e, 0x01, 0xa2, 0x01, 0xa2, 0x17, 0x17, 0x1f, 0x1f,
	0x92, 0x66, 0xe2, 0x6e, 0x72, 0x76, 0x7e, 0x7e, 0x01, 0x47, 0x01, 0x4f, 0x33, 0x37, 0x3f, 0x3f,
	0xa2, 0x56, 0xa2, 0x5e, 0x26, 0x26, 0x2e, 0x2e, 0x01, 0xa2, 0x01, 0xa2, 0x17, 0x17, 0x1f, 0x1f,
	0x92, 0x66, 0xf2, 0x6e, 0x72, 0x76, 0x7e, 0x7e, 0x01, 0x47, 0x01, 0x4f, 0x33, 0x37, 0x3f, 0x3f
};

static int pager(void)
{
	return 0;
}

char *get_token(char *string)
{
	static char *s;
	char *t;

	if (string)
		s = string;				/* New String */

	while (*s == ' ')
		s++;					/* Skip Leading Spaces */

	if (*s) {
		t = s;					/* Start of String */
		while (*s != ' ' && *s) {	/* Locate End of String */
			s++;
		}

		if (*s == ' ') {		/* Space Terminated ? */
			*s = '\0';			/* C String Terminator */
			s++;				/* Point to Next Char */
		}
	}
	else {
		t = NULL;
	}

	return t;					/* Pointer to String */
}

int get_dec(char *string, UWORD * decval)
{
	char *t;

	t = get_token(string);
	if (t) {
		return sscanf(t, "%hd", decval);
	}
	return 0;
}

int get_val(char *s, UWORD *hexval)
{
	int x = Util_sscanhex(s);
#ifdef MONITOR_HINTS
	int y = find_label_value(s);
	if (y >= 0) {
		if (x < 0 || x > 0xffff || x == y) {
			*hexval = (UWORD) y;
			return TRUE;
		}
		/* s can be a hex number or a label name */
		mon_printf("%s is ambiguous. Use 0%X or %X instead.\n", s, x, y);
		return FALSE;
	}
#endif
	if (x < 0 || x > 0xffff)
		return FALSE;
	*hexval = (UWORD) x;
	return TRUE;
}

#ifdef MACOSX_MON_ENHANCEMENTS
extern void ControlManagerDualError(char *error1, char *error2);
extern void NSBeep();

int get_val_gui(char *s, UWORD *hexval)
{
	int x,y;
	int labelMark = FALSE;
	char buffer1[81], buffer2[81];
	
	if (s[0]=='$') {
		labelMark = TRUE;
		s++;
	}
	
	x = Util_sscanhex(s);
	y = find_label_value(s);
	
	if (y >= 0) {
		if (labelMark || x < 0 || x > 0xffff || x == y) {
			*hexval = (UWORD) y;
			return TRUE;
		}
		/* s can be a hex number or a label name */
		snprintf(buffer1,80,"%s is ambiguous. ",s);
		snprintf(buffer2,80,"Use $%s or 0%X",s,x);
		ControlManagerDualError(buffer1, buffer2);
		return FALSE;
	}
	if (labelMark || x < 0 || x > 0xffff) {
		NSBeep();
		return FALSE;
	}
	*hexval = (UWORD) x;
	return TRUE;
}
#endif

int get_hex(char *string, UWORD *hexval)
{
	char *t;

	t = get_token(string);
	if (t) {
		return get_val(t, hexval);
	}
	return 0;
}

static UBYTE get_dlist_byte(UWORD *addr)
{
	UBYTE result = MEMORY_SafeGetByte(*addr);
	(*addr)++;
	if ((*addr & 0x03ff) == 0)
		(*addr) -= 0x0400;
	return result;
}

#ifdef MONITOR_TRACE
int MONITOR_tron;
FILE *MONITOR_trace_file;
#endif

/*
 * The following array is used for 6502 instruction profiling
 */
#ifdef MONITOR_PROFILE
int instruction_count[256];
#endif

#ifdef MONITOR_BREAK
MONITOR_breakpoint_cond MONITOR_breakpoint_table[MONITOR_BREAKPOINT_TABLE_MAX];
int MONITOR_breakpoint_table_size=0;
int break_table_on=TRUE;
int MONITOR_breakpoints_enabled=FALSE;

typedef struct break_token {
	char string[7];
	int condition;
} break_token;

break_token break_token_table[] = {
	{"ACCESS", MONITOR_BREAKPOINT_READ | MONITOR_BREAKPOINT_WRITE},
	{"WRITE", MONITOR_BREAKPOINT_WRITE},
	{"READ", MONITOR_BREAKPOINT_READ},
	{"PC", MONITOR_BREAKPOINT_PC}, // 3
	{"A", MONITOR_BREAKPOINT_A},
	{"X", MONITOR_BREAKPOINT_X},
	{"Y", MONITOR_BREAKPOINT_Y},
	{"S", MONITOR_BREAKPOINT_S}, 
	{"MEM", MONITOR_BREAKPOINT_MEM}, // 8
	{"CLRN", MONITOR_BREAKPOINT_CLRN},
	{"SETN", MONITOR_BREAKPOINT_SETN},
	{"CLRV", MONITOR_BREAKPOINT_CLRV},
	{"SETV", MONITOR_BREAKPOINT_SETV},
	{"CLRB", MONITOR_BREAKPOINT_CLRB},
	{"SETB", MONITOR_BREAKPOINT_SETB},
	{"CLRD", MONITOR_BREAKPOINT_CLRD},
	{"SETD", MONITOR_BREAKPOINT_SETD},
	{"CLRI", MONITOR_BREAKPOINT_CLRI},
	{"SETI", MONITOR_BREAKPOINT_SETI},
	{"CLRZ", MONITOR_BREAKPOINT_CLRZ},
	{"SETZ", MONITOR_BREAKPOINT_SETZ},
	{"CLRC", MONITOR_BREAKPOINT_CLRC},
	{"SETC", MONITOR_BREAKPOINT_SETC},
	{"OR", MONITOR_BREAKPOINT_OR}, // 23
};

UWORD MONITOR_break_addr;
UBYTE MONITOR_break_active;
UBYTE MONITOR_break_step=0;
static UBYTE break_over = FALSE;
UBYTE MONITOR_break_run_to_here = FALSE;
UBYTE MONITOR_break_brk=0;
UBYTE MONITOR_break_brk_occured=0;
UBYTE MONITOR_break_ret=0;
int MONITOR_break_fired=0;
int MONITOR_ret_nesting=0;
#endif

void MONITOR_monitorEnter(void)
{
	UWORD value = 0;
	UBYTE optype;
	int i;
	static char prBuff[255];
	  
	CPU_GetStatus();

#ifdef MONITOR_BREAK
	if (break_over) {
		/* "O" command was active */
		MONITOR_break_active = FALSE;
		break_over = FALSE;
	}
	else if (MONITOR_break_run_to_here) {
		/* GUI run to here command was active */
		MONITOR_break_active = FALSE;
		MONITOR_break_run_to_here = FALSE;
	}
    else if (MONITOR_break_brk_occured)
		mon_printf("(Break due to BRK opcode)\n");
    else if (MONITOR_break_ret)
        mon_printf("(Return instruction)\n");
#ifdef MACOSX_MON_ENHANCEMENTS
	else if (MONITOR_break_addr==CPU_regPC && MONITOR_break_active)
		mon_printf("(Breakpoint at %04X)\n", CPU_regPC);
	else if (MONITOR_break_fired ) {
		int guiBreakNum = BreakpointsControllerGetBreakpointNumForConditionNum(check_break_i-1);
		mon_printf("(Breakpoint at %04X Breakpoint #%d GUI Breakpoint #%d)\n", 
				   CPU_regPC,check_break_i-1, guiBreakNum);
	}
#else
	else if ( (MONITOR_break_addr==CPU_regPC && MONITOR_break_active) || MONITOR_break_fired )
		mon_printf("(Breakpoint at %04X)\n", CPU_regPC);
#endif
	else if (CPU_cim_encountered)
		mon_printf("(CIM encountered)\n");

#ifdef NEW_CYCLE_EXACT
	mon_printf("%3d ", ANTIC_ypos);
	mon_printf("%3d ", ANTIC_xpos);
#endif


	show_instruction(CPU_regPC,22);
	optype=MONITOR_optype6502[MEMORY_mem[CPU_regPC]]>>4;
	switch (optype)
	{
	case 0x1:
	    value=MEMORY_mem[CPU_regPC+1]+(MEMORY_mem[CPU_regPC+2]<<8);
	    value=MEMORY_mem[value];
	    break;
	case 0x2:
	    value=MEMORY_mem[MEMORY_mem[CPU_regPC+1]];
	    break;
	case 0x3:
	    value=MEMORY_mem[(MEMORY_mem[CPU_regPC+2]<<8)+MEMORY_mem[CPU_regPC+1]+CPU_regX];
	    break;
	case 0x4:
	    value=MEMORY_mem[(MEMORY_mem[CPU_regPC+2]<<8)+MEMORY_mem[CPU_regPC+1]+CPU_regY];
	    break;
	case 0x5:
	    value=(UBYTE)(MEMORY_mem[CPU_regPC+1]+CPU_regX);
	    value=(MEMORY_mem[(UBYTE)(value+1)]<<8)+MEMORY_mem[value];
	    value=MEMORY_mem[value];
	    break;
	case 0x6:
	    value=MEMORY_mem[CPU_regPC+1];
	    value=(MEMORY_mem[(UBYTE)(value+1)]<<8)+MEMORY_mem[value]+CPU_regY;
	    value=MEMORY_mem[value];
	    break;
	case 0x7:
	    value=MEMORY_mem[(UBYTE)(MEMORY_mem[CPU_regPC+1]+CPU_regX)];
	    break;
	case 0x8:
	    value=MEMORY_mem[(UBYTE)(MEMORY_mem[CPU_regPC+1]+CPU_regX)];
	    break;
	case 0x9:
	    switch(MEMORY_mem[CPU_regPC])
	    {
	        case 0x10:  /*BPL*/
	        if (!(CPU_regP & CPU_N_FLAG)) value=1;
	        break;
	        case 0x30:  /*BMI*/
	        if (CPU_regP & CPU_N_FLAG) value=1;
	        break;
	        case 0x50:  /*BVC*/
	        if (!(CPU_regP & CPU_V_FLAG)) value=1;
	        break;
	        case 0x70:  /*BVS*/
	        if (CPU_regP & CPU_V_FLAG) value=1;
	        break;
	        case 0x90:  /*BCC*/
	        if (!(CPU_regP & CPU_C_FLAG)) value=1;
	        break;
	        case 0xb0:  /*BCS*/
	        if (CPU_regP & CPU_C_FLAG) value=1;
	        break;
	        case 0xd0:  /*BNE*/
	        if (!(CPU_regP & CPU_Z_FLAG)) value=1;
	        break;
	        case 0xf0:  /*BEQ*/
	        if (CPU_regP & CPU_Z_FLAG) value=1;
	        break;
	    }
	    if (value==1) mon_printf("(Y) "); else mon_printf("(N) ");
	    break;
	case 0xb:
	    value=MEMORY_mem[0x100+(UBYTE)(CPU_regS+1)]+(MEMORY_mem[0x100+(UBYTE)(CPU_regS+2)]<<8)+1;
	    break;
	case 0xc:
	    value=MEMORY_mem[0x100+(UBYTE)(CPU_regS+2)]+(MEMORY_mem[0x100+(UBYTE)(CPU_regS+3)]<<8);
	    break;
	case 0xd:
	    value=MEMORY_mem[CPU_regPC+1]+(MEMORY_mem[CPU_regPC+2]<<8);
	    value=MEMORY_mem[value]+(MEMORY_mem[value+1]<<8);
	    break;
	case 0xe:
	    mon_printf("(ESC %02X) ",MEMORY_mem[CPU_regPC+1]);
	    value=MEMORY_mem[0x100+(UBYTE)(CPU_regS+1)]+(MEMORY_mem[0x100+(UBYTE)(CPU_regS+2)]<<8)+1;
	    break;
	case 0xf:
	    mon_printf("(ESC %02X) ",MEMORY_mem[CPU_regPC+1]);
	    break;

	}
	if (optype!=0x0 && optype!=0x9 && optype!=0xa && optype!=0xf &&
	    MEMORY_mem[CPU_regPC]!=0x4c && MEMORY_mem[CPU_regPC]!=0x20)
	{
		mon_printf("(%04X) ",value);
	}
	mon_printf("A=%02x S=%02x X=%02x Y=%02x P=",
		CPU_regA,CPU_regS,CPU_regX,CPU_regY);
	for(i=0;i<8;i++)
		prBuff[i] = (CPU_regP&(0x80>>i)?"NV*BDIZC"[i]:'-');
	prBuff[8] = '\n';
	prBuff[9] = 0;
    mon_printf(prBuff);

	CPU_cim_encountered=0;
	MONITOR_break_brk_occured=0;
	MONITOR_break_step=0;
	MONITOR_break_ret=0;
	MONITOR_break_fired=0;
#endif	
}

int MONITOR_monitorCmd(char *input)
{
    static UWORD addr = 0;
    static char s[128];
    static char old_s[sizeof(s)]=""; /*GOLDA CHANGED*/
    static int i,p;
	static char prBuff[80];

	{
		char *t;

		RemoveLF(input);
        strncpy(s,input,127);
        if (MONITOR_assemblerMode) {
            assemblerAddr = assembler(s);
            return(0);
            }
		if (s[0])
			memcpy(old_s, s, sizeof(s));
		else {
			int i;

			/* if no command is given, restart the last one, but remove all
			 * arguments, so after a 'm 600' we will see 'm 700' ...
			 */
			memcpy(s, old_s, sizeof(s));
			for (i = 0; i < (int) sizeof(s); ++i)
				if (isspace((unsigned char)s[i])) {
					s[i] = '\0';
					break;
				}
		}
		t = get_token(s);
		if (t == NULL) {
			return(0);
		}
		for (p = 0; t[p] != 0; p++)
			if (islower(t[p]))
				t[p] = toupper(t[p]);

		if (strcmp(t, "CONT") == 0) {
#ifdef MONITOR_PROFILE
			int i;

			for (i = 0; i < 256; i++)
				instruction_count[i] = 0;
#endif
			return -1;
		}
#ifdef MONITOR_BREAK
		else if (strcmp(t, "BRKHERE") == 0) {
			char *brkarg;
			brkarg = get_token(NULL);
			if (brkarg) {
                if (strcmp(brkarg, "on") == 0) {
                    MONITOR_break_brk = 1;
                    }
                else if (strcmp(brkarg, "off") == 0) {
                    MONITOR_break_brk = 0;
                    }
                else {
                    mon_printf("invalid argument: usage BRKHERE on|off\n");
                    }
                }
            else {
                mon_printf("BRKHERE is %s\n",MONITOR_break_brk ? "on" : "off");
                }
		}
		else if (strcmp(t, "BREAK") == 0) {
			if (get_hex(NULL, &MONITOR_break_addr))
				MONITOR_break_active=1;
			else if (MONITOR_break_addr)
				mon_printf("BREAK is %04X\n",MONITOR_break_addr);
			else
				mon_printf("BREAK is not active\n");
		}

		else if (strcmp(t, "YBREAK") == 0) {
			UWORD break_temp;
			if (!get_dec(NULL, &break_temp)) {
                mon_printf("YBREAK is %04d\n",ANTIC_break_ypos);
				}
			else
				ANTIC_break_ypos = break_temp;
		}
		else if (strcmp(t, "MONHIST") == 0) {
			char *brkarg;
			brkarg = get_token(NULL);
			if (brkarg) {
				if (strcmp(brkarg, "on") == 0) {
                    MONITOR_histon = TRUE;
                    }
                else if (strcmp(brkarg, "off") == 0) {
                    MONITOR_histon = FALSE;
					}
                else {
                    mon_printf("invalid argument: usage MONHIST on|off\n");
					 }
                }
            else {
					mon_printf("Monitor History is %s.\n",MONITOR_histon ? "on" : "off");
				 }
		}
		else if (strcmp(t, "HISTORY") == 0 || strcmp(t, "H") == 0) {
			if (MONITOR_histon == FALSE) {
                mon_printf("Sorry, monitor history is off\n");
				}
			else {
				for (i = 0; i < CPU_REMEMBER_PC_STEPS; i++) {
					UWORD addr;
					int j;
					addr=CPU_remember_PC[(CPU_remember_PC_curpos+i)%CPU_REMEMBER_PC_STEPS];
#ifdef NEW_CYCLE_EXACT
					mon_printf("%3d  ", (CPU_remember_xpos[(CPU_remember_PC_curpos+i)%CPU_REMEMBER_PC_STEPS]>>8));
					mon_printf("%3d  ", (CPU_remember_xpos[(CPU_remember_PC_curpos+i)%CPU_REMEMBER_PC_STEPS]&0xff));
#endif
					show_instruction(addr, 22);
					mon_printf("; %Xcyc ; ", cycles[MEMORY_SafeGetByte(addr)]);
					mon_printf("A=%02x S=%02x X=%02x Y=%02x P=",
						CPU_remember_A[(CPU_remember_PC_curpos+i)%CPU_REMEMBER_PC_STEPS],
						CPU_remember_S[(CPU_remember_PC_curpos+i)%CPU_REMEMBER_PC_STEPS],
						CPU_remember_X[(CPU_remember_PC_curpos+i)%CPU_REMEMBER_PC_STEPS],
						CPU_remember_Y[(CPU_remember_PC_curpos+i)%CPU_REMEMBER_PC_STEPS]);
					for(j=0;j<8;j++)
						prBuff[j] = (CPU_remember_P[(CPU_remember_PC_curpos+i)%CPU_REMEMBER_PC_STEPS] &
									(0x80>>j)?"NV*BDIZC"[j]:'-');
					prBuff[8] = '\n';
					prBuff[9] = 0;
					mon_printf(prBuff);
					}
				mon_printf("\n");
			}
		}
		else if (strcmp(t, "JUMPS") == 0) {
			int i;
			for (i = 0; i < CPU_REMEMBER_JMP_STEPS; i++)
				mon_printf("%04x  ", CPU_remember_JMP[(CPU_remember_jmp_curpos+i)%CPU_REMEMBER_JMP_STEPS]);
			mon_printf("\n");
		}
#endif

		else if (strcmp(t, "DLIST") == 0) {
			static UWORD tdlist;
			UWORD addr;
            char *token;
            int num;
			int done = FALSE;
			int nlines = 0;
			
            token = get_token(NULL);
            if (token != NULL) {
                if (strcmp("curr", token) == 0) {
                    tdlist = ANTIC_dlist;
                    }
                else {
                    sscanf(token, "%X", &num);
                    tdlist = num;
                    }
                }
			while (!done) {
				UBYTE IR;

				mon_printf("%04X: ", tdlist);

				IR = get_dlist_byte(&tdlist);

				if (IR & 0x80)
					mon_printf("DLI ");

				switch (IR & 0x0f) {
				case 0x00:
					mon_printf("%d BLANK", ((IR >> 4) & 0x07) + 1);
					break;
				case 0x01:
					addr = get_dlist_byte(&tdlist);
					addr |= get_dlist_byte(&tdlist) << 8;
					if (IR & 0x40) {
						mon_printf("JVB %04x ", addr);
						done = TRUE;
					}
					else {
						mon_printf("JMP %04x ", addr);
						tdlist = addr;
					}
					break;
				default:
					if (IR & 0x40) {
						addr = get_dlist_byte(&tdlist);
						addr |= get_dlist_byte(&tdlist) << 8;
						mon_printf("LMS %04x ", addr);
					}
					if (IR & 0x20)
						mon_printf("VSCROL ");

					if (IR & 0x10)
						mon_printf("HSCROL ");

					mon_printf("MODE %X ", IR & 0x0f);
				}

				mon_printf("\n");
				nlines++;

				if (nlines == 29)
                                    done = TRUE;
			}
		}
		else if (strcmp(t, "SETPC") == 0) {
			if (get_hex(NULL, &addr))
				CPU_regPC = addr;
			else
				mon_printf("Usage: SETPC hexval\n");
		}

		else if (strcmp(t, "SETS") == 0) {
			if (get_hex(NULL, &addr))
				CPU_regS = addr & 0xff;
			else
				mon_printf("Usage: SETS hexval\n");
		}

		else if (strcmp(t, "SETA") == 0) {
			if (get_hex(NULL, &addr))
				CPU_regA = addr & 0xff;
			else
				mon_printf("Usage: SETA hexval\n");
		}

		else if (strcmp(t, "SETX") == 0) {
			if (get_hex(NULL, &addr))
				CPU_regX = addr & 0xff;
			else
				mon_printf("Usage SETX hexval\n");
		}

		else if (strcmp(t, "SETY") == 0) {
			if (get_hex(NULL, &addr))
				CPU_regY = addr & 0xff;
			else
				mon_printf("Usage SETY hexval\n");
		}

		else if (strcmp(t, "SETN") == 0) {
			CPU_SetN;
		}

		else if (strcmp(t, "CLRN") == 0) {
			CPU_ClrN;
		}

		else if (strcmp(t, "SETV") == 0) {
			CPU_SetV;
		}

		else if (strcmp(t, "CLRV") == 0) {
			CPU_ClrV;
		}

		else if (strcmp(t, "SETD") == 0) {
			CPU_SetD;
		}

		else if (strcmp(t, "CLRD") == 0) {
			CPU_ClrD;
		}

		else if (strcmp(t, "SETI") == 0) {
			CPU_SetI;
		}

		else if (strcmp(t, "CLRI") == 0) {
			CPU_ClrI;
		}

		else if (strcmp(t, "SETZ") == 0) {
			CPU_SetZ;
		}

		else if (strcmp(t, "CLRZ") == 0) {
			CPU_ClrZ;
		}

		else if (strcmp(t, "SETC") == 0) {
			CPU_SetC;
		}

		else if (strcmp(t, "CLRC") == 0) {
			CPU_ClrC;
		}

#ifdef MONITOR_TRACE
		else if (strcmp(t, "TRON") == 0) {
			char *file;
			
			if (MONITOR_tron)
			{
			    mon_printf("Trace is already on, use TROFF to turn off\n");
			}
			else
			{
				file = get_token(NULL);
				if (file) {
					if ((MONITOR_trace_file = fopen(file,"w")))
						MONITOR_tron = TRUE;
					else {
						mon_printf("Error opening trace file %s\n", file);
						}
					}
				else mon_printf("Usage: TRON file\n");
			}
		}

		else if (strcmp(t, "TROFF") == 0) {
			if (MONITOR_tron) {
				fclose(MONITOR_trace_file);
				MONITOR_tron = FALSE;
			}
		}
#endif

#ifdef MONITOR_PROFILE
		else if (strcmp(t, "PROFILE") == 0) {
			int i;

			for (i = 0; i < 256; i++) {
				if (instruction_count[i])
					mon_printf("%02x has been executed %d times\n",
					    i, instruction_count[i]);
			}
		}
#endif

		else if (strcmp(t, "SHOW") == 0) {
			int i;
#ifdef NEW_CYCLE_EXACT
            mon_printf("%3d ", ANTIC_ypos);
            mon_printf("%3d ", ANTIC_xpos);
#endif
            mon_printf("%04X ", CPU_regPC);
			show_instruction(CPU_regPC, 22);
            mon_printf("A=%02x, S=%02x, X=%02x, Y=%02x, P=",
				CPU_regA,CPU_regS,CPU_regX,CPU_regY);
			for(i=0;i<8;i++) {
                mon_printf("%c",CPU_regP&(0x80>>i)?"NV*BDIZC"[i]:'-');
                }
			mon_printf("\n");
		}
		else if (strcmp(t, "STACK") == 0) {
			UWORD ts,ta;
			for( ts = 0x101+CPU_regS; ts<0x200; ) {
				if( ts<0x1ff ) {
					ta = MEMORY_SafeGetByte(ts) | ( MEMORY_SafeGetByte(ts+1) << 8 );
					if( MEMORY_SafeGetByte(ta-2)==0x20 ) {
						mon_printf("%04X : %02X %02X\t%04X : JSR %04X\n",
						ts, MEMORY_SafeGetByte(ts), MEMORY_SafeGetByte(ts+1), ta-2,
						MEMORY_SafeGetByte(ta-1) | ( MEMORY_SafeGetByte(ta) << 8 ) );
						ts+=2;
						continue;
					}
				}
				mon_printf("%04X : %02X\n", ts, MEMORY_SafeGetByte(ts) );
				ts++;
			}
		}

#ifndef PAGED_ATTRIB
		else if (strcmp(t, "ROM") == 0) {
			UWORD addr1;
			UWORD addr2;

			int status;

			status = get_hex(NULL, &addr1);
			status |= get_hex(NULL, &addr2);

			if (status && addr1 <= addr2) {
				MEMORY_SetROM(addr1, addr2);
				mon_printf("Changed Memory from %4x to %4x into ROM\n",
					   addr1, addr2);
			}
			else {
				mon_printf("*** Memory Unchanged (Bad or missing Parameter) ***\n");
			}
		}

		else if (strcmp(t, "RAM") == 0) {
			UWORD addr1;
			UWORD addr2;

			int status;

			status = get_hex(NULL, &addr1);
			status |= get_hex(NULL, &addr2);

			if (status && addr1 <= addr2) {
				MEMORY_SetRAM(addr1, addr2);
				mon_printf("Changed Memory from %4x to %4x into RAM\n",
					   addr1, addr2);
			}
			else {
				mon_printf("*** Memory Unchanged (Bad or missing Parameter) ***\n");
			}
		}

		else if (strcmp(t, "HARDWARE") == 0) {
			UWORD addr1;
			UWORD addr2;

			int status;

			status = get_hex(NULL, &addr1);
			status |= get_hex(NULL, &addr2);

			if (status && addr1 <= addr2) {
				MEMORY_SetHARDWARE(addr1, addr2);
				mon_printf("Changed Memory from %4x to %4x into HARDWARE\n",
					   addr1, addr2);
			}
			else {
				mon_printf("*** Memory Unchanged (Bad or missing Parameter) ***\n");
			}
		}
		else if (strcmp(t, "BANK") == 0) {
			UWORD bank;

			int status;

			status = get_dec(NULL, &bank);

			if (status) {
				if (bank < (atarixe_memory_size/16384)) {
					memcpy(atarixe_memory + (((long) MEMORY_xe_bank) << 14), MEMORY_mem + 0x4000, 16384);
					memcpy(MEMORY_mem + 0x4000, atarixe_memory + (((long) bank) << 14), 16384);
					MEMORY_xe_bank = bank;
				}
				else {
					mon_printf("Error - Bank number larger than available memory\n");
				}
			}
			else {
				mon_printf("Current Bank: %d\n",MEMORY_xe_bank);
			}
		}
#endif
		else if (strcmp(t, "COLDSTART") == 0) {
			Atari800_Coldstart();
			return -1;	/* perform reboot immediately */
		}
		else if (strcmp(t, "WARMSTART") == 0) {
			Atari800_Warmstart();
			return -1;	/* perform reboot immediately */
		}
#ifndef PAGED_MEM
		else if (strcmp(t, "READ") == 0) {
			char *filename;
			UWORD addr;
			UWORD nbytes;
			int status;

			filename = get_token(NULL);
			if (filename) {
				status = get_hex(NULL, &addr);
				if (status) {
					status = get_hex(NULL, &nbytes);
					if (status) {
						FILE *f;

						if (!(f = fopen(filename, "rb"))) {
                			mon_printf("Error reading file %s\n",filename);
							}
						else {
							if (fread(&MEMORY_mem[addr], 1, nbytes, f) == 0) {
                				mon_printf("Error reading file %s\n",filename);
                				}
							fclose(f);
						}
					}
				}
			}
		}
		else if (strcmp(t, "READSEC") == 0) {
			UWORD driveNo;
			UWORD sector;
			UWORD secCount;
			UWORD addr;
			int i,result;
			int status1, status2, status3, status4;
			int size;
			ULONG offset;

			status1 = get_dec(NULL, &driveNo);
			status2 = get_dec(NULL, &sector);
			status3 = get_dec(NULL, &secCount);
			status4 = get_hex(NULL, &addr);
			
			if (status1 && status2 && status3 && status4) {
				if (driveNo > 8 || driveNo == 0) {
					mon_printf("Drive Number must be 1-8\n");
					}
				else {
					if (SIO_drive_status[driveNo-1] == SIO_OFF || SIO_drive_status[driveNo-1] == SIO_NO_DISK) {
						mon_printf("No Disk in Drive #%d\n",driveNo);
						}
					else {
						for (i=0;i<secCount;i++) {
							if ((result = SIO_ReadSector(driveNo-1, sector+i, &MEMORY_mem[addr])) != 'C') {
								mon_printf("Error reading drive %d sector %d\n",driveNo, sector+i);
								break;
								}
							SIO_SizeOfSector(driveNo-1,sector+i,&size,&offset);
							addr += size;
							}
						}
					}
				}
			else {
				mon_printf("Usage: READSEC drive# count addr\n");
				}
		}
		else if (strcmp(t, "WRITESEC") == 0) {
			UWORD driveNo;
			UWORD sector;
			UWORD secCount;
			UWORD addr;
			int i,result;
			int status1, status2, status3, status4;
			int size;
			ULONG offset;

			status1 = get_dec(NULL, &driveNo);
			status2 = get_dec(NULL, &sector);
			status3 = get_dec(NULL, &secCount);
			status4 = get_hex(NULL, &addr);
			
			if (status1 && status2 && status3 && status4) {
				if (driveNo > 8 || driveNo == 0) {
					mon_printf("Drive Number must be 1-8\n");
					}
				else {
					if (SIO_drive_status[driveNo-1] == SIO_OFF || SIO_drive_status[driveNo-1] == SIO_NO_DISK) {
						mon_printf("No Disk in Drive #%d\n",driveNo);
						}
					else {
						for (i=0;i<secCount;i++) {
							if ((result = SIO_WriteSector(driveNo-1, sector+i, &MEMORY_mem[addr])) != 'C') {
								mon_printf("Error writing drive %d sector %d\n",driveNo, sector+i);
								break;
								}
							SIO_SizeOfSector(driveNo-1,sector+i,&size,&offset);
							addr += size;
							}
						}
					}
				}
			else {
				mon_printf("Usage: READSEC drive# count addr\n");
				}
		}
		else if (strcmp(t, "WRITE") == 0) {
			UWORD addr1;
			UWORD addr2;
			UWORD init;
			UWORD run;
			char *filename;
			int status;
			int status_init;
			int status_run;
			int write_result = TRUE;
			unsigned char header[6];

			status = get_hex(NULL, &addr1);
			status |= get_hex(NULL, &addr2);

			if ( ! (filename = get_token(NULL) )) /* ERU */
				filename="memdump.dat"; 

			status_init = get_hex(NULL, &init);
			status_run = get_hex(NULL, &run);
  
			if (status) {
				FILE *f;

				if (!(f = fopen(filename, "wb"))) {
                  mon_printf("Error writing file %s\n",filename);
				  }
				else {
					if ((status_init) || (status_run))
					{
						header[0] = 0xFF;
						header[1] = 0xFF;
						header[2] = (addr1 & 0xFF);
						header[3] = ((addr1 & 0xFF00) >> 8);
						header[4] = (addr2 & 0xFF);
						header[5] = ((addr2 & 0xFF00) >> 8);
						write_result &= (fwrite(&header[0], 1, 6, f) != -1);
					}

					write_result &= (fwrite(&MEMORY_mem[addr1], 1, addr2 - addr1 + 1, f) != -1);

					if ((status_init) && (init != 0x0000))
					{
						header[0] = 0xE2;
						header[1] = 0x02;
						header[2] = 0xE3;
						header[3] = 0x02;
						header[4] = (init & 0xFF);
						header[5] = ((init & 0xFF00) >> 8);
						write_result &= (fwrite(&header[0], 1, 6, f) != -1);
					}

					if ((status_run) && (run != 0x0000))
					{
						header[0] = 0xE0;
						header[1] = 0x02;
						header[2] = 0xE1;
						header[3] = 0x02;
						header[4] = (run & 0xFF);
						header[5] = ((run & 0xFF00) >> 8);
						write_result &= (fwrite(&header[0], 1, 6, f) != -1);
					}

					if (!write_result) {
						mon_printf("Error writing file %s\n",filename);
						}
 				  fclose(f);
				}
			}
		}
#endif

		else if (strcmp(t, "SUM") == 0) {
			UWORD addr1;
			UWORD addr2;
			int status;

			status = get_hex(NULL, &addr1);
			status |= get_hex(NULL, &addr2);

			if (status) {
				int sum = 0;
				int i;

				for (i = addr1; i <= addr2; i++)
					sum += (UWORD) MEMORY_SafeGetByte(i);
				mon_printf("SUM: %X\n", sum);
			}
		}

		else if (strcmp(t, "M") == 0) {
			UWORD xaddr1;
			int i;
			int count;
            char *linePtr;
			if (get_hex(NULL, &xaddr1))
				addr = xaddr1;
			count = 16;
			while (count) {
                linePtr = prBuff;
				linePtr += sprintf(linePtr,"%04X : ", addr);
				for (i = 0; i < 16; i++)
                    {
                    linePtr += sprintf(linePtr,"%02X ", MEMORY_SafeGetByte((UWORD) (addr + i)));
                    }
				mon_printf("\t");
				for(i=0;i<16;i++) {
					char c;
					c=MEMORY_SafeGetByte((UWORD)(addr+i));
					linePtr += sprintf(linePtr,"%c",c>=' '&&c<='z'&&c!='\x60'?c:'.');
				}
                linePtr += sprintf(linePtr,"\n");
                mon_puts(prBuff);
				addr += 16;
				count--;
			}
		}

#ifndef PAGED_MEM
		else if (strcmp(t, "F") == 0) {
			int addr;
			int addr1;
			int addr2;
			UWORD xaddr1;
			UWORD xaddr2;
			UWORD hexval;

			get_hex(NULL, &xaddr1);
			get_hex(NULL, &xaddr2);
			get_hex(NULL, &hexval);

			addr1 = xaddr1;
			addr2 = xaddr2;

			for (addr = addr1; addr <= addr2; addr++)
				MEMORY_mem[addr] = (UBYTE) (hexval & 0x00ff);
		}
		else if (strcmp(t, "MM") == 0) {
			int addr1;
			int addr2;
			UWORD xaddr1;
			UWORD xaddr2;
			UWORD count;
			int i;

			get_hex(NULL, &xaddr1);
			get_hex(NULL, &xaddr2);
			get_hex(NULL, &count);

			addr1 = xaddr1;
			addr2 = xaddr2;

			if (addr1 < addr2) {
				for (i=count-1;i>=0;i--)
					MEMORY_mem[addr2+i] = MEMORY_mem[addr1+i];
				}
			else {
				for (i=0;i<count;i++)
					MEMORY_mem[addr2+i] = MEMORY_mem[addr1+i];
				}
		}
#endif
		else if (strcmp(t, "S") == 0) {
			static int n = 0;
			static UWORD xaddr1;
			static UWORD xaddr2;
			UWORD hexval;
			static UBYTE tab[64];

			if (get_hex(NULL, &xaddr1) && get_hex(NULL, &xaddr2) && get_hex(NULL, &hexval)) {
				n = 0;
				do {
					tab[n++] = (UBYTE) hexval;
					if (hexval & 0xff00)
						tab[n++] = (UBYTE) (hexval >> 8);
				} while (get_hex(NULL, &hexval));
			}
			if (n) {
				int addr;
				int addr1 = xaddr1;
				int addr2 = xaddr2;

				for (addr = addr1; addr <= addr2; addr++) {
					int i = 0;
					while (MEMORY_SafeGetByte(addr + i) == tab[i]) {
						i++;
						if (i >= n) {
							mon_printf("Found at %04x\n", addr);
							break;
						}
					}
				}
			}
		}

#ifndef PAGED_MEM
		else if (strcmp(t, "C") == 0) {
			UWORD addr;
			UWORD temp;
			addr = 0;
			temp = 0;

			get_hex(NULL, &addr);

			while (get_hex(NULL, &temp)) {
				MEMORY_PutByte(addr, (UBYTE) temp);
				addr++;
				if (temp & 0xff00)
					MEMORY_PutByte(addr++, (UBYTE) (temp >> 8));
			}
		}
#endif

#ifdef MONITOR_BREAK
		else if (strcmp(t, "G") == 0) {
		        MONITOR_break_step=1;
		        return -2;
		}
		else if (strcmp(t, "R") == 0 ) {
		        MONITOR_break_ret=1;
		        MONITOR_ret_nesting=1;
		        return -1;
		}
		else if (strcmp(t, "O") == 0) {
#if 1			
			UBYTE opcode = MEMORY_SafeGetByte(CPU_regPC);
			
			if (opcode == 0x20) {
				/* JSR: set breakpoint after it */
				MONITOR_break_addr = CPU_regPC + (MONITOR_optype6502[MEMORY_SafeGetByte(CPU_regPC)] & 0x3);
				MONITOR_break_active=TRUE;
				break_over = TRUE;
			}
			else
				MONITOR_break_step = TRUE;
			return -2;
#else			
		/* with RTS, RTI, JMP, SKW, ESCRTS we simply do step */
		        if (MEMORY_mem[CPU_regPC]==0x60 || MEMORY_mem[CPU_regPC]==0x40 || MEMORY_mem[CPU_regPC]==0x0c ||
		            MEMORY_mem[CPU_regPC]==0x4c || MEMORY_mem[CPU_regPC]==0x6c || MEMORY_mem[CPU_regPC]==0xd2 ||
		            MEMORY_mem[CPU_regPC]==0x1c || MEMORY_mem[CPU_regPC]==0x3c || MEMORY_mem[CPU_regPC]==0x5c ||
		            MEMORY_mem[CPU_regPC]==0x7c || MEMORY_mem[CPU_regPC]==0xdc || MEMORY_mem[CPU_regPC]==0xfc ||
		            MEMORY_mem[CPU_regPC]==0x9c )
		        {
		          MONITOR_break_step=1;
		          return -1;
		        }
		        else
		        {
		          MONITOR_break_addr=CPU_regPC+(UWORD)(MONITOR_optype6502[MEMORY_mem[CPU_regPC]]&0x3);
				  MONITOR_break_active=TRUE;
				  break_over = TRUE;
		          return -1;
		        }
#endif			
		}
		else if (strcmp(t, "B") == 0) {
			char *arg=NULL;
			UWORD i;
			int bytes;
			int one_break_entry_on = FALSE;

			arg=get_token(NULL);
			if (!arg) {
				if (!MONITOR_breakpoint_table_size)
					mon_printf("Breakpoint table empty.\n");
				else {
					if (break_table_on)
						mon_printf("Breakpoint table is ON.\n");
					else
						mon_printf("Breakpoint table is OFF.\n");
					for (i=0; i<MONITOR_breakpoint_table_size; i++) {
						mon_printf("%2d: ", i);
						if (MONITOR_breakpoint_table[i].on)
							mon_printf("ON ");
						else
							mon_printf("OFF ");
						switch (MONITOR_breakpoint_table[i].condition) {
							case MONITOR_BREAKPOINT_NONE:
								mon_printf("NONE");
								break;
							case MONITOR_BREAKPOINT_OR:
								mon_printf("OR");
								break;
							case MONITOR_BREAKPOINT_CLRN:
								mon_printf("CLRN");
								break;
							case MONITOR_BREAKPOINT_SETN:
								mon_printf("SETN");
								break;
							case MONITOR_BREAKPOINT_CLRV:
								mon_printf("CLRV");
								break;
							case MONITOR_BREAKPOINT_SETV:
								mon_printf("SETV");
								break;
							case MONITOR_BREAKPOINT_CLRB:
								mon_printf("CLRB");
								break;
							case MONITOR_BREAKPOINT_SETB:
								mon_printf("SETB");
								break;
							case MONITOR_BREAKPOINT_CLRD:
								mon_printf("CLRD");
								break;
							case MONITOR_BREAKPOINT_SETD:
								mon_printf("SETD");
								break;
							case MONITOR_BREAKPOINT_CLRI:
								mon_printf("CLRI");
								break;
							case MONITOR_BREAKPOINT_SETI:
								mon_printf("SETI");
								break;
							case MONITOR_BREAKPOINT_CLRZ:
								mon_printf("CLRZ");
								break;
							case MONITOR_BREAKPOINT_SETZ:
								mon_printf("SETZ");
								break;
							case MONITOR_BREAKPOINT_CLRC:
								mon_printf("CLRC");
								break;
							case MONITOR_BREAKPOINT_SETC:
								mon_printf("SETC");
								break;

							default:
								switch (MONITOR_breakpoint_table[i].condition>>3) {
									case MONITOR_BREAKPOINT_PC>>3:
										mon_printf("PC");
										bytes=2;
										break;
									case MONITOR_BREAKPOINT_A>>3:
										mon_printf("A");
										bytes=1;
										break;
									case MONITOR_BREAKPOINT_X>>3:
										mon_printf("X");
										bytes=1;
										break;
									case MONITOR_BREAKPOINT_Y>>3:
										mon_printf("Y");
										bytes=1;
										break;
									case MONITOR_BREAKPOINT_S>>3:
										mon_printf("S");
										bytes=1;
										break;
									case MONITOR_BREAKPOINT_MEM>>3:
										mon_printf("MEM%04X",MONITOR_breakpoint_table[i].addr);
										bytes=1;
										break;
									case MONITOR_BREAKPOINT_READ>>3:
										mon_printf("READ");
										bytes=2;
										break;
									case MONITOR_BREAKPOINT_WRITE>>3:
										mon_printf("WRITE");
										bytes=2;
										break;
									case (MONITOR_BREAKPOINT_READ|MONITOR_BREAKPOINT_WRITE)>>3:
										mon_printf("ACCESS");
										bytes=2;
										break;
									default:
										bytes=0;
								}
								if (bytes) {
									switch (MONITOR_breakpoint_table[i].condition & 7) {
										case MONITOR_BREAKPOINT_LESS:
											mon_printf("<");
											break;
										case MONITOR_BREAKPOINT_EQUAL:
											mon_printf("=");
											break;
										case MONITOR_BREAKPOINT_LESS|MONITOR_BREAKPOINT_EQUAL:
											mon_printf("<=");
											break;
										case MONITOR_BREAKPOINT_GREATER:
											mon_printf(">");
											break;
										case MONITOR_BREAKPOINT_GREATER|MONITOR_BREAKPOINT_LESS:
											mon_printf("<>");
											break;
										case MONITOR_BREAKPOINT_GREATER|MONITOR_BREAKPOINT_EQUAL:
											mon_printf(">=");
											break;
										default:
											break;
									}
									if (bytes==1) mon_printf("%02X", MONITOR_breakpoint_table[i].val);
									else mon_printf("%04X", MONITOR_breakpoint_table[i].addr);
								}
						}
						mon_printf("\n");
					}

				}
			}
			else if (strcmp(arg, "?") == 0) {
				mon_printf("Breakpoint table usage:\n");
				mon_printf("B - print table \n");
				mon_printf("B ? - help\n");
				mon_printf("B C - clear table\n");
				mon_printf("B D num - delete one entry\n");
#ifndef MACOSX_MON_ENHANCEMENTS
				mon_printf("B ON - turn breakpoint table ON\n");
#endif
				mon_printf("B ON num1 num2 num3 - turn particular breakpoint ON\n");
#ifndef MACOSX_MON_ENHANCEMENTS
				mon_printf("B OFF - turn breakpoint table OFF\n");
#endif
				mon_printf("B OFF num1 num2 num3 - turn particular breakpoint OFF\n");
				mon_printf("B [num] cond1 cond2 cond3 ... - insert several breakpoints\n");
				mon_printf("    (in num position in breakpoint table)\n");
				mon_printf("  - Breakpoint will fire when all conditions are meet\n");
				mon_printf("    (they are connected by AND operator)\n");
				mon_printf("  - OR is a special condition which separates\n");
				mon_printf("    several AND-connected breakpoints\n");
				mon_printf("  - conditions are: TYPE OPERATOR VALUE (without spaces)\n");
				mon_printf("    or: MEM ADDR OPERATOR VALUE (without spaces)\n");
				mon_printf("    or: SETFLAG, CLRFLAG where FLAG is: N, V, B, D, I, Z, C\n");
				mon_printf("  - TYPE is: PC, A, X, Y, S, READ, WRITE, ACCESS (read or write)\n");
				mon_printf("  - OPERATOR is: <, <=, =, ==, >, >=, !=, <>\n");
				mon_printf("  - ADDR and VALUE are a one or two bytes hex number\n");
				mon_printf("Examples:\n");
				mon_printf("B PC>=203f A<3a OR PC=3a7f X<>0 - creates table with 5 entries\n");
				mon_printf("B 2 MEM1000=4a                  - add new entry on position 2\n");
				mon_printf("B D 1                           - delete entry on position 1\n");
				mon_printf("B OR SETD                       - adds 2 new entries at the end\n");
			}
			else if (strcasecmp(arg, "D") == 0) {
				if (get_dec(NULL,&i)){
					if (MONITOR_breakpoint_table_size>i) {
						for (;i<MONITOR_breakpoint_table_size-1;i++) {
							MONITOR_breakpoint_table[i]=MONITOR_breakpoint_table[i+1];
						}
						MONITOR_breakpoint_table_size--;
						BreakpointsControllerSetDirty();
						mon_printf("Entry deleted\n");
					}
					else mon_printf("Error: value out of range.\n");
				}
				else mon_printf("Error. Type B ? for help.\n");
			}
			else if (strcasecmp(arg, "C") == 0) {
				MONITOR_breakpoint_table_size=0;
				BreakpointsControllerSetDirty();
				mon_printf("Table cleared\n");
			}
			else if (strcasecmp(arg, "ON") == 0) {
				if (get_dec(NULL,&i)) {
					do {
						if (MONITOR_breakpoint_table_size>i)
							MONITOR_breakpoint_table[i].on=TRUE;
					} while (get_dec(NULL,&i));
					BreakpointsControllerSetDirty();
				}
#ifdef MACOSX_MON_ENHANCEMENTS
				else break_table_on=TRUE;
#endif

			}
			else if (strcasecmp(arg, "OFF") == 0) {
				if (get_dec(NULL,&i)) {
					do {
						if (MONITOR_breakpoint_table_size>i)
							MONITOR_breakpoint_table[i].on=FALSE;
					} while (get_dec(NULL,&i));
					BreakpointsControllerSetDirty();
				}
#ifdef MACOSX_MON_ENHANCEMENTS				
				else break_table_on=FALSE;
#endif
			}
			else {
				i=MONITOR_breakpoint_table_size;
				if (isdigit(arg[0])) {
					int tmp;
					sscanf(arg, "%d", &tmp);
					i = tmp;
					if (i>MONITOR_breakpoint_table_size)
						i=MONITOR_breakpoint_table_size;
					arg=get_token(NULL);
				}
				while (MONITOR_breakpoint_table_size<MONITOR_BREAKPOINT_TABLE_MAX) {
					MONITOR_breakpoint_cond tmp = {MONITOR_BREAKPOINT_NONE, TRUE, 0, 0};
					int j;
					UWORD a;

					for (j=0;j<24;j++) { 
						if (!strncasecmp(arg, break_token_table[j].string, strlen(break_token_table[j].string))) {
							tmp.condition=break_token_table[j].condition;
							arg+=strlen(break_token_table[j].string);
							break;
						}
					}

					if (tmp.condition==MONITOR_BREAKPOINT_NONE) {
						mon_printf("Error processing breakpoints. Type B ? for help.\n");
						break;
					}
					
					if (tmp.condition==MONITOR_BREAKPOINT_MEM) {
						char memaddr[80];
						strcpy(memaddr, arg);
						memaddr[strcspn(arg,"<>=")] = 0;
						if (!get_val(memaddr,&a)) {
							mon_printf("Error processing breakpoints. Type B ? for help.\n");
							break;
							}
						arg+=strcspn(arg,"<>=");
						tmp.addr = a;
						}

					if (j<9) {
						if (!strncmp(arg, "<>", 2) || !strncmp(arg, "!=", 2)) {
							tmp.condition |= (MONITOR_BREAKPOINT_LESS | MONITOR_BREAKPOINT_GREATER);
							arg+=2;
						}
						else if (!strncmp(arg, "<=", 2)) {
							tmp.condition |= (MONITOR_BREAKPOINT_LESS | MONITOR_BREAKPOINT_EQUAL);
							arg+=2;
						}
						else if (!strncmp(arg, ">=", 2)) {
							tmp.condition |= (MONITOR_BREAKPOINT_GREATER | MONITOR_BREAKPOINT_EQUAL);
							arg+=2;
						}
						else if (!strncmp(arg, "==", 2)) {
							tmp.condition |= MONITOR_BREAKPOINT_EQUAL;
							arg+=2;
						}
						else if (!strncmp(arg, "=", 1)) {
							tmp.condition |= MONITOR_BREAKPOINT_EQUAL;
							arg++;
						}
						else if (!strncmp(arg, "<", 1)) {
							tmp.condition |= MONITOR_BREAKPOINT_LESS;
							arg++;
						}
						else if (!strncmp(arg, ">", 1)) {
							tmp.condition |= MONITOR_BREAKPOINT_GREATER;
							arg++;
						}
						else {
							mon_printf("Error processing breakpoints. Type B ? for help.\n");
							break;
						}

						if (!get_val(arg,&a)) {
							mon_printf("Error processing breakpoints. Type B ? for help.\n");
							break;
						}

						if (j<4)
							tmp.addr = a;
						else
							tmp.val = (UBYTE) a;
					}

					for (j=MONITOR_BREAKPOINT_TABLE_MAX-2;j>=i;j--)
						MONITOR_breakpoint_table[j+1]=MONITOR_breakpoint_table[j];
					MONITOR_breakpoint_table[i] = tmp;
					MONITOR_breakpoint_table_size++;
					i++;
					mon_printf("Breakpoint added.\n");
					BreakpointsControllerSetDirty();

					arg=get_token(NULL);
					if (!arg) break;
				}
			}
			for (i=0;i<MONITOR_breakpoint_table_size;i++)
				if (MONITOR_breakpoint_table[i].on)
					one_break_entry_on = TRUE;
			if (one_break_entry_on && break_table_on)
				MONITOR_breakpoints_enabled = TRUE;
			else
				MONITOR_breakpoints_enabled = FALSE;
		}
#endif /* MONITOR_BREAK */

		else if (strcmp(t, "D") == 0) {
			UWORD addr1;
			addr1 = addr;
			get_hex(NULL, &addr1);
			addr = disassemble(addr1,0);
		}
#ifdef MONITOR_HINTS
		else if (strcmp(t, "LABELS") == 0) {
			char *cmd = get_token(NULL);
			if (cmd == NULL) {
				mon_printf("Built-in labels are %sabled.\n", symtable_builtin_enable ? "en" : "dis");
				if (symtable_user_size > 0)
					mon_printf("Using %d user-defined label%s.\n",
						symtable_user_size, (symtable_user_size > 1) ? "s" : "");
				else
					mon_printf("There are no user-defined labels.\n");
				mon_printf("Labels are displayed in disassembly listings.\n");
				mon_printf("You may also use them as command arguments");
#ifdef MONITOR_ASSEMBLER
				mon_printf(" and in the built-in assembler");
#endif
				mon_printf(".\nUsage:\n");
				mon_printf("LABELS OFF            - no labels\n");
				mon_printf("LABELS BUILTIN        - use only built-in labels\n");
				mon_printf("LABELS LOAD filename  - use only labels loaded from file\n");
				mon_printf("LABELS ADD filename   - use built-in and loaded labels\n");
				mon_printf("LABELS SET name value - define a label\n");
				mon_printf("LABELS LIST           - list user-defined labels\n");
				mon_printf("LABELS VALUE name     - lookup label value given name\n");
				mon_printf("LABLES NAME vale      - lookup label name given value\n");
			}
			else {
				Util_strupper(cmd);
				if (strcmp(cmd, "OFF") == 0) {
					symtable_builtin_enable = FALSE;
					free_user_labels();
#ifdef MACOSX_MON_ENHANCEMENTS					
					ControlManagerMonitorSetLabelsDirty();
#endif
				}
				else if (strcmp(cmd, "BUILTIN") == 0) {
					symtable_builtin_enable = TRUE;
					free_user_labels();
#ifdef MACOSX_MON_ENHANCEMENTS					
					ControlManagerMonitorSetLabelsDirty();
#endif
				}
				else if (strcmp(cmd, "LOAD") == 0) {
					symtable_builtin_enable = FALSE;
					load_user_labels(get_token(NULL));
#ifdef MACOSX_MON_ENHANCEMENTS					
					ControlManagerMonitorSetLabelsDirty();
#endif
				}
				else if (strcmp(cmd, "ADD") == 0) {
					symtable_builtin_enable = TRUE;
					load_user_labels(get_token(NULL));
#ifdef MACOSX_MON_ENHANCEMENTS					
					ControlManagerMonitorSetLabelsDirty();
#endif
				}
				else if (strcmp(cmd, "SET") == 0) {
					const char *name = get_token(NULL);
					if (name != NULL && get_hex(NULL, &addr)) {
						symtable_rec *p = find_user_label(name);
						if (p != NULL) {
							if (p->addr != addr) {
								mon_printf("%s redefined (previous value: %04X)\n", name, p->addr);
								p->addr = addr;
							}
						}
						else
							add_user_label(name, addr);
#ifdef MACOSX_MON_ENHANCEMENTS					
						ControlManagerMonitorSetLabelsDirty();
#endif
					}
					else
						mon_printf("Missing or bad arguments\n");
				}
				else if (strcmp(cmd, "LIST") == 0) {
					int i;
					int nlines = 0;
					for (i = 0; i < symtable_user_size; i++) {
						if (++nlines == 24) {
							if (pager())
								break;
							nlines = 0;
						}
						mon_printf("%04X %s\n", symtable_user[i].addr, symtable_user[i].name);
					}
				}
				else if (strcmp(cmd, "VALUE") == 0) {
					const char *name = get_token(NULL);
					if (name != NULL) {
						int value = find_label_value(name);
						if (value != -1)
							mon_printf("LABEL %s = 0x%x\n",name,value);
						else
							mon_printf("LABEL %s not found\n",name);
						}					
					} 
				else if (strcmp(cmd, "NAME") == 0) {
					const char *name_read;
					const char *name_write;
					if (get_hex(NULL, &addr)) {
						name_read = find_label_name(addr,0);
						name_write = find_label_name(addr,0x8);
						if (name_read == NULL && name_write == NULL)
							mon_printf("LABEL with value 0x%x not found\n",addr);
						else if (name_read == name_write)
							mon_printf("LABEL %s = 0x%x\n",name_read,addr);
						else  {
							mon_printf("LABEL %s = 0x%x read\n",name_read,addr);
							mon_printf("LABEL %s = 0x%x write\n",name_write,addr);
							}
						}					
					} 
				else
					mon_printf("Invalid command, type \"LABELS\" for help\n");
			}
		}
#endif

		else if (strcmp(t, "ANTIC") == 0) {
			mon_printf("DMACTL=%02x    CHACTL=%02x    DLISTL=%02x    "
				   "DLISTH=%02x    HSCROL=%02x    VSCROL=%02x\n",
				   ANTIC_DMACTL, ANTIC_CHACTL, (UBYTE)(ANTIC_dlist&0xff), (UBYTE)(ANTIC_dlist>>8), ANTIC_HSCROL, ANTIC_VSCROL);
			mon_printf("PMBASE=%02x    CHBASE=%02x    VCOUNT=%02x    "
				"NMIEN= %02x    ypos= %3d\n",
				ANTIC_PMBASE,ANTIC_CHBASE, ANTIC_ypos >> 1, ANTIC_NMIEN, ANTIC_ypos);
		}

		else if (strcmp(t, "PIA") == 0) {
			mon_printf("PACTL=%02x      PBCTL=%02x     PORTA=%02x     "
				   "PORTB=%02x\n", PIA_PACTL, PIA_PBCTL, PIA_PORTA, PIA_PORTB);
		}

		else if (strcmp(t, "GTIA") == 0) {
			mon_printf("HPOSP0=%02X    HPOSP1=%02X    HPOSP2=%02X    HPOSP3=%02X\n",
				   GTIA_HPOSP0, GTIA_HPOSP1, GTIA_HPOSP2, GTIA_HPOSP3);
			mon_printf("HPOSM0=%02X    HPOSM1=%02X    HPOSM2=%02X    HPOSM3=%02X\n",
				   GTIA_HPOSM0, GTIA_HPOSM1, GTIA_HPOSM2, GTIA_HPOSM3);
			mon_printf("SIZEP0=%02X    SIZEP1=%02X    SIZEP2=%02X    SIZEP3=%02X    SIZEM= %02X\n",
				   GTIA_SIZEP0, GTIA_SIZEP1, GTIA_SIZEP2, GTIA_SIZEP3, GTIA_SIZEM);
			mon_printf("GRAFP0=%02X    GRAFP1=%02X    GRAFP2=%02X    GRAFP3=%02X    GRAFM= %02X\n",
				   GTIA_GRAFP0, GTIA_GRAFP1, GTIA_GRAFP2, GTIA_GRAFP3, GTIA_GRAFM);
			mon_printf("COLPM0=%02X    COLPM1=%02X    COLPM2=%02X    COLPM3=%02X\n",
				   GTIA_COLPM0, GTIA_COLPM1, GTIA_COLPM2, GTIA_COLPM3);
			mon_printf("COLPF0=%02X    COLPF1=%02X    COLPF2=%02X    COLPF3=%02X    COLBK= %02X\n",
				   GTIA_COLPF0, GTIA_COLPF1, GTIA_COLPF2, GTIA_COLPF3, GTIA_COLBK);
			mon_printf("PRIOR= %02X    VDELAY=%02X    GRACTL=%02X\n",
				   GTIA_PRIOR, GTIA_VDELAY, GTIA_GRACTL);
		}

		else if (strcmp(t, "POKEY") == 0) {
			mon_printf("AUDF1= %02x    AUDF2= %02x    AUDF3= %02x    "
				   "AUDF4= %02x    AUDCTL=%02x    KBCODE=%02x\n",
				   POKEY_AUDF[POKEY_CHAN1], POKEY_AUDF[POKEY_CHAN2], POKEY_AUDF[POKEY_CHAN3], 
				   POKEY_AUDF[POKEY_CHAN4], POKEY_AUDCTL[0], POKEY_KBCODE);
			mon_printf("AUDC1= %02x    AUDC2= %02x    AUDC3= %02x    "
				   "AUDC4= %02x    IRQEN= %02x    IRQST= %02x\n",
				   POKEY_AUDC[POKEY_CHAN1], POKEY_AUDC[POKEY_CHAN2], POKEY_AUDC[POKEY_CHAN3], 
				   POKEY_AUDC[POKEY_CHAN4], POKEY_IRQEN, POKEY_IRQST);
			mon_printf("SKSTAT=%02X    SKCTL= %02X\n", POKEY_SKSTAT, POKEY_SKCTL);
#ifdef STEREO_SOUND
			if (POKEYSND_stereo_enabled) {
				mon_printf("second chip:\n");
				mon_printf("AUDF1= %02x    AUDF2= %02x    AUDF3= %02x    "
					   "AUDF4= %02x    AUDCTL=%02x\n",
					   POKEY_AUDF[POKEY_CHAN1 + POKEY_CHIP2], POKEY_AUDF[POKEY_CHAN2 + POKEY_CHIP2], 
					   POKEY_AUDF[POKEY_CHAN3 + POKEY_CHIP2], POKEY_AUDF[POKEY_CHAN4 + POKEY_CHIP2], 
					   POKEY_AUDCTL[1]);
				mon_printf("AUDC1= %02x    AUDC2= %02x    AUDC3= %02x    "
					   "AUDC4= %02x\n",
					   POKEY_AUDC[POKEY_CHAN1 + POKEY_CHIP2], POKEY_AUDC[POKEY_CHAN2 + POKEY_CHIP2], 
					   POKEY_AUDC[POKEY_CHAN3 + POKEY_CHIP2], POKEY_AUDC[POKEY_CHAN4 + POKEY_CHIP2]);
			}
#endif
		}

#ifdef MONITOR_ASSEMBLER
		else if (strcmp(t,"A") == 0) {
			UWORD addr1;
			addr1 = addr;
			get_hex(NULL, &addr1);
			assemblerAddr = addr1;
            MONITOR_assemblerMode = TRUE;
            mon_printf("Simple assembler (enter empty line to exit)\n");
            mon_printf("%X : ",(int)assemblerAddr);
		}
#endif

		else if (strcmp(t, "HELP") == 0 || strcmp(t, "?") == 0) {
			mon_printf("CONT                           - Continue emulation\n");
			mon_printf("SHOW                           - Show registers\n");
			mon_printf("STACK                          - Show stack\n");
			mon_printf("SET{PC,A,S,X,Y} hexval         - Set register value\n");
			mon_printf("SET{N,V,D,I,Z,C}               - Set flag value\n");
			mon_printf("CLR{N,V,D,I,Z,C}               - Clear flag value\n");
			mon_printf("C startaddr hexval...          - Change memory\n");
			mon_printf("D [startaddr]                  - Disassemble memory\n");
			mon_printf("F startaddr endaddr hexval     - Fill memory\n");
			mon_printf("M [startaddr]                  - Memory list\n");
			mon_printf("MM srcaddr destaddr bytecnt    - Move Memory\n");
			mon_printf("S startaddr endaddr hexval...  - Search memory\n");
			mon_printf("ROM startaddr endaddr          - Convert memory block into ROM\n");
			mon_printf("RAM startaddr endaddr          - Convert memory block into RAM\n");
			mon_printf("HARDWARE startaddr endaddr     - Convert memory block into HARDWARE\n");
			mon_printf("BANK [banknum]                 - Switch memory bank (XE systems)\n");
			mon_printf("READ file startaddr nbytes     - Read file into memory\n");
			mon_printf("READSEC drive# sector count addr - Read disk sectors into memory\n");
			mon_printf("WRITE startaddr endaddr [file] [init>0] [run>0]\n");
			mon_printf("                               - Write memory block to a file (memdump.dat)\n");
			mon_printf("WRITESEC drive# sector count addr  - Write disk sectors from memory\n");
			mon_printf("SUM startaddr endaddr          - SUM of specified memory range\n");
#ifdef MONITOR_TRACE
			mon_printf("TRON file                      - Trace on\n");
			mon_printf("TROFF                          - Trace off\n");
#endif
#ifdef MONITOR_BREAK
			mon_printf("BREAK [addr]                   - Set breakpoint at address\n");
			mon_printf("YBREAK [pos], or [1000+pos]    - Break at scanline or flash scanline\n");

 			mon_printf("BRKHERE [on|off]               - Set BRK opcode behaviour\n");
 			mon_printf("MONHIST [on|off]               - Turn monitor history keeping on/off\n");
#ifndef NEW_CYCLE_EXACT
			mon_printf("HISTORY or H                   - Disasm. last %i PC addrs. giving ypos xpos\n",(int)CPU_REMEMBER_PC_STEPS);
#else
			mon_printf("HISTORY or H                   - Disasm. last %i PC addrs. \n",(int)CPU_REMEMBER_PC_STEPS);
#endif /*NEW_CYCLE_EXACT*/

			mon_printf("JUMPS                          - List last %i locations of JMP/JSR\n",(int)CPU_REMEMBER_PC_STEPS);
			mon_printf("G                              - Execute 1 instruction\n");
			mon_printf("O                              - Step over the instruction\n");
			mon_printf("R                              - Execute until return\n");
			mon_printf("B                              - Breakpoint (B ? for help)\n");
#endif			
#ifdef MONITOR_ASSEMBLER
            mon_printf("A [startaddr]                  - Start simple assembler\n");
#endif
			mon_printf("ANTIC, GTIA, PIA, POKEY        - Display hardware registers\n");
			mon_printf("DLIST [startaddr]              - Show Display List\n");
			mon_printf("DLIST CURR                     - Show Current Display List\n");
#ifdef MONITOR_PROFILE
			mon_printf("PROFILE                        - Display profiling statistics\n");
#endif
#ifdef MONITOR_HINTS
			mon_printf("LABELS file                    - Load labels from file (xasm format)\n");
#endif
			mon_printf("COLDSTART, WARMSTART           - Perform system coldstart/warmstart\n");
#if 0                        
			mon_printf("!command                       - Execute shell command\n");
#endif                        
			mon_printf("QUIT                           - Quit emulator\n");
			mon_printf("HELP or ?                      - This text\n");
		}
		else if (strcmp(t, "QUIT") == 0) {
			return 1;
		}
		else {
			mon_printf("Invalid command.\n");
		}
        return(0);
	}
}

unsigned int disassemble(UWORD addr1, UWORD addr2)
{
	int count = 24;
	do
#ifdef MACOSX_MON_ENHANCEMENTS
		addr1 = show_instruction(addr1, 31);
#else
		addr1 = show_instruction(addr1, 22);
#endif
	while (--count > 0);
	return addr1;
}

UWORD show_instruction(UWORD inad, int wid)
{
	char prBuff[256];
	UWORD retVal;
	
	retVal = show_instruction_string(prBuff,inad,wid);
	mon_printf(prBuff);
	return(retVal);
}

UWORD MONITOR_get_disasm_start(UWORD addr, UWORD target) 
{
	int i;
	UWORD baseAddr = addr;
	UWORD currentInstr;
	char prBuff[256];
	
	for(i=0; i<4; ++i) {
		currentInstr = baseAddr;
		for(;;) {
			if (currentInstr == target)
				return baseAddr;

#ifdef MACOSX_MON_ENHANCEMENTS			
			currentInstr = show_instruction_string(prBuff, currentInstr, 31);
#else
			currentInstr = show_instruction_string(prBuff, currentInstr, 22);
#endif			
			
			if (currentInstr > target)
				break;
		}
		
		++baseAddr;
		if (baseAddr == target)
			break;
	}
	
	return baseAddr;
}

UWORD MONITOR_show_instruction_file(FILE* file, UWORD inad, int wid)
{
	char prBuff[256];
	UWORD retVal;
	
	retVal = show_instruction_string(prBuff,inad,wid);
	fprintf(file,"%s",prBuff);
	return(retVal);
}

UWORD show_instruction_string(char*buff, UWORD pc, int wid)
{
	UWORD addr = pc;
	UBYTE insn;
	const char *mnemonic;
	const char *p;
	int value = 0;
	int nchars = 0;
#ifdef MACOSX_MON_ENHANCEMENTS
	const char *mylabel;
		
	/* get the label for the pc location, if we can find it */
	mylabel = find_label_name(pc,0);
	if (mylabel == NULL)
		mylabel = "        ";
#endif
	insn = MEMORY_SafeGetByte(pc++);
	mnemonic = instr6502[insn];
	for (p = mnemonic + 3; *p != '\0'; p++) {
		if (*p == '1') {
			value = MEMORY_SafeGetByte(pc++);
#ifdef MACOSX_MON_ENHANCEMENTS
			nchars = sprintf(buff, "%04X:%02X %02X    %-8s " /*"%Xcyc  "*/ "%.*s$%02X%s",
			                 addr, insn, value,  mylabel,/*cycles[insn],*/ (int) (p - mnemonic), mnemonic, value, p + 1);
#else
			nchars = sprintf(buff, "%04X:%02X %02X    " /*"%Xcyc  "*/ "%.*s$%02X%s",
			                 addr, insn, value, /*cycles[insn],*/ (int) (p - mnemonic), mnemonic, value, p + 1);
#endif
			buff += nchars;
			break;
		}
		if (*p == '2') {
			value = MEMORY_GetWord(pc);
#ifdef MACOSX_MON_ENHANCEMENTS
			nchars = sprintf(buff, "%04X:%02X %02X %02X %-8s " /*"%Xcyc  "*/ "%.*s$%04X%s",
			                 addr, insn, value & 0xff, value >> 8, mylabel, /*cycles[insn],*/ (int) (p - mnemonic), mnemonic, value, p + 1);
#else
			nchars = sprintf(buff, "%04X:%02X %02X %02X " /*"%Xcyc  "*/ "%.*s$%04X%s",
			                 addr, insn, value & 0xff, value >> 8, /*cycles[insn],*/ (int) (p - mnemonic), mnemonic, value, p + 1);
#endif
			buff += nchars;
			pc += 2;
			break;
		}
		if (*p == '0') {
			UBYTE op = MEMORY_SafeGetByte(pc++);
			value = (UWORD) (pc + (SBYTE) op);
#ifdef MACOSX_MON_ENHANCEMENTS
			nchars = sprintf(buff, "%04X:%02X %02X    %-8s " /*"3cyc  "*/ "%.4s$%04X", addr, insn, op, mylabel, mnemonic, value);
#else
			nchars = sprintf(buff, "%04X:%02X %02X    " /*"3cyc  "*/ "%.4s$%04X", addr, insn, op, mnemonic, value);
#endif
			buff += nchars;
			break;
		}
	}
	if (*p == '\0') {
#ifdef MACOSX_MON_ENHANCEMENTS
		nchars = sprintf(buff, "%04X:%02X       %-8s " /*"%Xcyc  "*/ "%s\n", addr, insn, mylabel, /*cycles[insn],*/ mnemonic);
#else
		nchars = sprintf(buff, "%04X:%02X       " /*"%Xcyc  "*/ "%s\n", addr, insn, /*cycles[insn],*/ mnemonic);
#endif
		buff += nchars;
		return pc;
	}
#ifdef MONITOR_HINTS
	if (p[-1] != '#') {
		/* different names when reading/writing memory */
		const char *label = find_label_name((UWORD) value, (MONITOR_optype6502[insn] & 0x08) != 0);
		if (label != NULL) {
#ifdef MACOSX_MON_ENHANCEMENTS
			nchars += sprintf(buff, "%*s;%s\n", 35 - nchars, "", label);
#else
			nchars += sprintf(buff, "%*s;%s\n", 26 - nchars, "", label);
#endif
			buff += nchars;
			return pc;
		}
	}
#endif
	buff += sprintf(buff,"\n");
	return pc;
}

#ifdef MONITOR_ASSEMBLER
UWORD assembler(char *input)
{
		char s[128];  /* input string */
		char c[128];  /* converted input */
		char *sp;     /* input pointer */
		char *cp;     /* converted input pointer */
		char *vp;     /* value pointer (the value is stored in s) */
		char *tp;     /* type pointer (points at type character '0', '1' or '2' in converted input) */
		int i;
		int isa;      /* the operand is "A" */
		UWORD value = 0;
  UWORD addr = assemblerAddr;
  
    strncpy(s, input, 127);
    
    RemoveLF(s);
    if (s[0]=='\0') {
      MONITOR_assemblerMode = FALSE;
      return addr;
      }

		Util_strupper(s);

		sp = s;
		cp = c;
		/* copy first three characters */
		for (i = 0; i < 3 && *sp != '\0'; i++)
			*cp++ = *sp++;
		/* insert space before operands */
		*cp++ = ' ';

		tp = NULL;
		isa = FALSE;

		/* convert input to format of instr6502[] table */
		while (*sp != '\0') {
			switch (*sp) {
			case ' ':
			case '\t':
			case '$':
			case '@':
				sp++;
				break;
			case '#':
			case '(':
			case ')':
			case ',':
				isa = FALSE;
				*cp++ = *sp++;
				break;
			default:
				if (tp != NULL) {
					if (*sp == 'X' || *sp == 'Y') {
						*cp++ = *sp++;
						break;
					}
					goto invalid_instr;
				}
				vp = s;
				do
					*vp++ = *sp++;
				while (strchr(" \t$@#(),", *sp) == NULL && *sp != '\0');
				/* If *sp=='\0', strchr() should return non-NULL,
				   but we do an extra check to be on safe side. */
				*vp++ = '\0';
				tp = cp++;
				*tp = '0';
				isa = (s[0] == 'A' && s[1] == '\0');
				break;
			}
		}
		if (cp[-1] == ' ')
			cp--;    /* no arguments (e.g. NOP or ASL @) */
		*cp = '\0';

		/* if there's an operand, get its value */
		if (tp != NULL && !get_val(s, &value)) {
			printf("Invalid operand!\n");
			return(addr);
		}

		for (;;) {
			/* search table for instruction */
			for (i = 0; i < 256; i++) {
				if (strcmp(instr6502[i], c) == 0) {
					if (tp == NULL) {
						MEMORY_PutByte(addr, (UBYTE) i);
						addr++;
					}
					else if (*tp == '0') {
						value -= (addr + 2);
						if ((SWORD) value < -128 || (SWORD) value > 127)
							mon_printf("Branch out of range!\n");
						else {
							MEMORY_PutByte(addr, (UBYTE) i);
							addr++;
							MEMORY_PutByte(addr, (UBYTE) value);
							addr++;
						}
					}
					else if (*tp == '1') {
						c[3] = '\0';
						if (isa && (strcmp(c, "ASL") == 0 || strcmp(c, "LSR") == 0 ||
						            strcmp(c, "ROL") == 0 || strcmp(c, "ROR") == 0)) {
							mon_printf("\"%s A\" is ambiguous.\n"
							       "Use \"%s\" for accumulator mode or \"%s 0A\" for zeropage mode.\n", c, c, c);
						}
						else {
							MEMORY_PutByte(addr, (UBYTE) i);
							addr++;
							MEMORY_PutByte(addr, (UBYTE) value);
							addr++;
						}
					}
					else { /* *tp == '2' */
						MEMORY_PutByte(addr, (UBYTE) i);
						addr++;
						MEMORY_PutWord(addr, value);
						addr += 2;
					}
					mon_printf("%X : ",(int)addr);
					return(addr);
				}
			}
			/* not found */
			if (tp == NULL || *tp == '2')
				break;
			if (++*tp == '1' && value > 0xff)
				*tp = '2';
		}
		mon_printf("%X : ",(int)addr);
		return(addr);
	invalid_instr:
		mon_printf("Invalid instruction!\n");
		return(addr);
    
}
#endif

/*
$Log: monitor.c,v $
Revision 1.15  2003/02/24 09:33:04  joy
header cleanup

Revision 1.14  2003/02/19 14:07:46  joy
configure stuff cleanup

Revision 1.13  2003/02/10 10:00:59  joy
include added

Revision 1.12  2003/01/27 14:13:02  joy
Perry's cycle-exact ANTIC/GTIA emulation

Revision 1.11  2003/01/27 13:14:53  joy
Jason's changes: either PAGED_ATTRIB support (mostly), or just clean up.

Revision 1.10  2002/07/04 12:40:19  pfusik
fixed optype6502[] to match unofficial instructions

Revision 1.9  2002/03/19 13:17:02  joy
Piotrs changes

Revision 1.8  2001/10/29 17:56:05  fox
"DLIST" didn't stopped on JVB if Display List had multiple of 15 instructions

Revision 1.7  2001/07/20 19:57:07  fox
not displaying rom_inserted in "PIA" command

Revision 1.3  2001/03/25 06:57:36  knik
open() replaced by fopen()

Revision 1.2  2001/03/18 06:34:58  knik
WIN32 conditionals removed

*/
