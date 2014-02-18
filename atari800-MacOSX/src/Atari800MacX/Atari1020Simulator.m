/* Atari1020Simulator.m - 
 Atari1020Simulator class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 */

#import "Atari1020Simulator.h"
#import "PrintOutputController.h"

// States of the simulated printer state machine
#define ATARI1020_TEXT_IDLE												0
#define ATARI1020_GRAPHICS_IDLE											1
#define ATARI1020_GOT_ESC												2
#define ATARI1020_WAIT_CR												3
#define ATARI1020_GOT_RESET                                             4
#define ATARI1020_GOT_CHANGE_COLOR                                      5
#define ATARI1020_GOT_DRAW_ABSOLUTE                                     6
#define ATARI1020_GOT_DRAW_ABSOLUTE_Y                                   7
#define ATARI1020_GOT_DRAW_RELATIVE                                     8
#define ATARI1020_GOT_DRAW_RELATIVE_Y                                   9
#define ATARI1020_GOT_MOVE_ABSOLUTE									   10
#define ATARI1020_GOT_MOVE_ABSOLUTE_Y								   11
#define ATARI1020_GOT_LINE_TYPE										   12
#define ATARI1020_GOT_PRINT_TEXT                                       13
#define ATARI1020_GOT_CHAR_SIZE                                        14
#define ATARI1020_GOT_ROTATE_PRINT                                     15
#define ATARI1020_GOT_MOVE_RELATIVE                                    16
#define ATARI1020_GOT_MOVE_RELATIVE_Y                                  17
#define ATARI1020_GOT_DRAW_XY_AXIS                                     18
#define ATARI1020_GOT_DRAW_XY_AXIS_STEP                                19
#define ATARI1020_GOT_DRAW_XY_AXIS_INTERVAL                            20

ATARI1020_PREF prefs1020;

// Other constants
#define POINTS_PER_STEP						((0.02/2.5472*72.0) * scaleFactor)
#define ATARI1020_LEFT_PRINT_EDGE			((8.3*72 - (480 * POINTS_PER_STEP))/2)

@implementation Atari1020Simulator
static Atari1020Simulator *sharedInstance = nil;
static float lineTypes[16][2];
static NSColor *colors[4];
static NSBezierPath *fontPaths[0x60];
static NSBezierPath *interFontPaths[26];
static int fontTemplates[0x60][20][3] =
{
	{   {1,0,0},{0,6,7}   },  /* space */
	{   {11,0,0},{0,3,6},{1,2,7},{1,4,7},{1,3,6},{0,3,5},{1,4,4},{1,4,0},
		{1,2,0},{1,2,4},{1,3,5},{0,6,0} },  /* exclaim */
	{   {5,0,0},{0,2,2},{1,2,0},{0,4,2},{1,4,0},{0,6,7} },  /* doubquote */
	{   {9,0,0},{0,2,6},{1,2,0},{0,4,6},{1,4,0},{0,1,2},{1,5,2},{0,1,4},
		{1,5,4},{0,6,7} },  /* hash */
	{   {13,0,0},{0,1,4},{1,2,5},{1,4,5},{1,5,4},{1,4,3},{1,2,3},{1,1,2},
		{1,2,1},{1,4,1},{1,5,2},{0,3,0},{1,3,6},{0,6,7} },  /* dollar */
	{   {13,0,0},{0,1,5},{1,5,1},{0,3,2},{1,3,1},{1,2,1},{1,2,2},{1,3,2},
		{0,3,4},{1,3,5},{1,4,5},{1,4,4},{1,3,4},{0,6,0} },  /* percent */
	{   {12,0,0},{0,5,6},{1,1,2},{1,1,1},{1,2,0},{1,3,1},{1,3,2},{1,1,4},
		{1,1,5},{1,2,6},{1,3,6},{1,5,4},{0,6,7} },  /* ampersand */
	{   {7,0,0},{0,2,3},{1,3,2},{1,3,0},{1,2,0},{1,2,1},{1,3,1},
		{0,6,7} },  /* singquote */
	{   {7,0,0},{0,4,0},{1,3,0},{1,2,1},{1,2,5},{1,3,6},{1,4,6},
		{0,6,7} },  /* lparen*/
	{   {7,0,0},{0,2,0},{1,3,0},{1,4,1},{1,4,5},{1,3,6},{1,2,6},
		{0,6,7} },  /* rparen*/
	{   {7,0,0},{0,1,5},{1,5,1},{0,3,0},{1,3,6},{0,5,5},{1,1,1},
		{0,6,7} },  /* asterix */
	{   {5,0,0},{0,3,5},{1,3,1},{0,1,3},{1,5,3},{0,6,7} },  /* plus */
	{   {7,0,0},{0,2,7},{1,3,6},{1,3,4},{1,2,4},{1,2,5},{1,3,5},
		{0,6,7} },  /* comma */
	{   {3,0,0},{0,1,3},{1,5,3},{0,6,7} },  /* dash */
	{   {6,0,0},{0,2,5},{1,2,6},{1,3,6},{1,3,5},{1,2,5},{0,6,7} },  /* period */
	{   {3,0,0},{0,1,5},{1,5,1},{0,6,7} },  /* fslash */
	{   {12,0,0},{0,2,6},{1,1,5},{1,1,1},{1,2,0},{1,4,0},{1,5,1},{1,5,5},
		{1,4,6},{1,2,6},{0,1,5},{1,5,1},{0,6,7} },  /* 0 */
	{   {6,0,0},{0,2,6},{1,4,6},{0,3,6},{1,3,0},{1,2,1},{0,6,7} },  /* 1 */
	{   {8,0,0},{0,1,1},{1,2,0},{1,4,0},{1,5,1},{1,5,2},{1,1,6},{1,5,6},
		{0,6,7} },  /* 2 */
	{   {12,0,0},{0,1,5},{1,2,6},{1,4,6},{1,5,5},{1,5,4},{1,4,3},{1,5,2},
		{1,5,1},{1,4,0},{1,2,0},{1,1,1},{0,6,7} },  /* 3 */
	{   {6,0,0},{0,4,6},{1,4,0},{1,1,3},{1,1,4},{1,5,4},{0,6,7} },  /* 4 */
	{   {10,0,0},{0,1,5},{1,2,6},{1,4,6},{1,5,5},{1,5,3},{1,4,2},{1,1,2},
		{1,1,0},{1,5,0},{0,6,7} },  /* 5 */
	{   {13,0,0},{0,5,1},{1,4,0},{1,2,0},{1,1,1},{1,1,5},{1,2,6},{1,4,6},
		{1,5,5},{1,5,3},{1,4,2},{1,2,2},{1,1,3},{0,6,7} },  /* 6 */
	{   {5,0,0},{0,1,6},{1,5,2},{1,5,0},{1,1,0},{0,6,7} },  /* 7 */
	{   {18,0,0},{0,2,6},{1,4,6},{1,5,5},{1,5,4},{1,4,3},{1,5,2},{1,5,1},
		{1,4,0},{1,2,0},{1,1,1},{1,1,2},{1,2,3},{1,4,3},{0,2,3},{1,1,4},
		{1,1,5},{1,2,6},{0,6,7} },  /* 8 */
	{   {13,0,0},{0,1,5},{1,2,6},{1,4,6},{1,5,5},{1,5,1},{1,4,0},{1,2,0},{1,1,1},
		{1,1,2},{1,2,3},{1,4,3},{1,5,2},{0,6,7} },  /* 9 */
	{   {11,0,0},{0,2,1},{1,3,1},{1,3,2},{1,2,2},{1,2,1},{0,2,4},{1,3,4},{1,3,5},
		{1,2,5},{1,2,4},{0,6,7} },  /* colon */
	{   {12,0,0},{0,2,1},{1,3,1},{1,3,2},{1,2,2},{1,2,1},{0,3,5},{1,2,5},{1,2,4},
		{1,3,4},{1,3,6},{1,2,7},{0,6,7} },  /* semicolon */
	{   {4,0,0},{0,5,6},{1,2,3},{1,5,0},{0,6,7} },  /* less */
	{   {5,0,0},{0,1,4},{1,4,4},{0,1,2},{1,4,2},{0,6,7} },  /* equals */
	{   {4,0,0},{0,1,6},{1,4,3},{1,1,0},{0,6,7} },  /* greater */
	{   {12,0,0},{0,3,6},{1,3,5},{0,3,4},{1,3,3},{1,4,3},{1,5,2},{1,5,1},{1,4,0},
		{1,2,0},{1,1,1},{1,1,2},{0,6,7} },  /* question */
	{   {15,0,0},{0,5,6},{1,2,6},{1,1,5},{1,1,1},{1,2,0},{1,4,0},{1,5,1},{1,5,4},
		{1,3,5},{1,2,4},{1,2,3},{1,3,2},{1,4,2},{1,4,4},{0,6,7} },  /* at */
	{   {8,0,0},{0,1,6},{1,1,2},{1,3,0},{1,5,2},{1,5,6},{0,5,3},{1,1,3},{0,6,7} },  /* A */
	{   {13,0,0},{0,1,0},{1,4,0},{1,5,1},{1,5,2},{1,4,3},{1,5,4},{1,5,5},{1,4,6},{1,1,6},{1,1,0},{0,1,3},{1,4,3},{0,6,7} },  /* B */
	{   {9,0,0},{0,5,1},{1,4,0},{1,2,0},{1,1,1},{1,1,5},{1,2,6},{1,4,6},{1,5,5},{0,6,7} },  /* C */
	{   {8,0,0},{0,1,0},{1,4,0},{1,5,1},{1,5,5},{1,4,6},{1,1,6},{1,1,0},{0,6,7} },  /* D */
	{   {7,0,0},{0,5,0},{1,1,0},{1,1,6},{1,5,6},{0,1,3},{1,4,3},{0,6,7} },  /* E */
	{   {6,0,0},{0,5,0},{1,1,0},{1,1,6},{0,1,3},{1,4,3},{0,6,7} },  /* F */
	{   {11,0,0},{0,5,1},{1,4,0},{1,2,0},{1,1,1},{1,1,5},{1,2,6},{1,4,6},{1,5,5},{1,5,3},{1,3,3},{0,6,7} },  /* G */
	{   {7,0,0},{0,1,0},{1,1,6},{0,5,0},{1,5,6},{0,1,3},{1,5,3},{0,6,7} },  /* H */
	{   {7,0,0},{0,2,0},{1,4,0},{0,3,0},{1,3,6},{0,2,6},{1,4,6},{0,6,7} },  /* I */
	{   {6,0,0},{0,5,0},{1,5,5},{1,4,6},{1,2,6},{1,1,5},{0,6,7} },  /* J */
	{   {7,0,0},{0,1,0},{1,1,6},{0,1,4},{1,5,0},{0,2,3},{1,5,6},{0,6,7} },  /* K */
	{   {4,0,0},{0,1,0},{1,1,6},{1,5,6},{0,6,7} },  /* L */
	{   {6,0,0},{0,1,6},{1,1,0},{1,3,2},{1,5,0},{1,5,6},{0,6,7} },  /* M */
	{   {7,0,0},{0,1,0},{1,1,6},{0,5,0},{1,5,6},{0,1,1},{1,5,5},{0,6,7} },  /* N */
	{   {10,0,0},{0,2,0},{1,4,0},{1,5,1},{1,5,5},{1,4,6},{1,2,6},{1,1,5},{1,1,1},{1,2,0},{0,6,7} },  /* O */
	{   {9,0,0},{0,1,0},{1,4,0},{1,5,1},{1,5,2},{1,4,3},{1,1,3},{0,1,0},{1,1,6},{0,6,7} },  /* P */
	{   {12,0,0},{0,2,0},{1,4,0},{1,5,1},{1,5,5},{1,4,6},{1,2,6},{1,1,5},{1,1,1},{1,2,0},{0,5,6},{1,3,4},{0,6,7} },  /* Q */
	{   {11,0,0},{0,1,0},{1,4,0},{1,5,1},{1,5,2},{1,4,3},{1,1,3},{0,1,0},{1,1,6},{0,5,6},{1,2,3},{0,6,7} },  /* R */
	{   {13,0,0},{0,5,1},{1,4,0},{1,2,0},{1,1,1},{1,1,2},{1,2,3},{1,4,3},{1,5,4},{1,5,5},{1,4,6},{1,2,6},{1,1,5},{0,6,7} },  /* S */
	{   {5,0,0},{0,1,0},{1,5,0},{1,3,0},{1,3,6},{0,6,7} },  /* T */
	{   {7,0,0},{0,1,0},{1,1,5},{1,2,6},{1,4,6},{1,5,5},{1,5,0},{0,6,7} },  /* U */
	{   {6,0,0},{0,1,0},{1,1,4},{1,3,6},{1,5,4},{1,5,0},{0,6,7} },  /* V */
	{   {6,0,0},{0,1,0},{1,1,6},{1,3,4},{1,5,6},{1,5,0},{0,6,7} },  /* W */
	{   {9,0,0},{0,1,0},{1,1,1},{1,5,5},{1,5,6},{0,1,6},{1,1,5},{1,5,1},{1,5,0},{0,6,7} },  /* X */
	{   {6,0,0},{0,1,0},{1,3,2},{1,5,0},{0,3,2},{1,3,6},{0,6,7} },  /* Y */
	{   {9,0,0},{0,1,0},{1,4,0},{1,5,1},{1,1,5},{1,2,6},{1,5,6},{0,2,2},{1,4,4},{0,6,7} },  /* Z */
	{   {5,0,0},{0,4,0},{1,2,0},{1,2,6},{1,4,6},{0,6,7} },  /* lbracket */
	{   {3,0,0},{0,1,1},{1,5,5},{0,6,7} },  /* backslash */
	{   {5,0,0},{0,2,0},{1,4,0},{1,4,6},{1,2,6},{0,6,7} },  /* rbracket */
	{   {4,0,0},{0,1,3},{1,3,1},{1,5,3},{0,6,7} },  /* caret */
	{   {2,0,0},{0,0,7},{1,6,7} },  /* underline */
	{   {7,0,0},{0,2,1},{1,3,1},{1,3,0},{1,2,0},{1,2,2},{1,3,3},{0,6,7} },  /* backquote */
	{   {14,0,0},{0,1,2},{1,3,2},{1,4,3},{1,4,5},{1,5,6},{0,4,5},{1,3,6},{1,2,6},{1,1,5},{1,1,4},{1,2,3},{1,3,3},{1,4,4},{0,6,7} },  /* a */
	{   {8,0,0},{0,1,0},{1,1,6},{1,3,6},{1,4,5},{1,4,3},{1,3,2},{1,1,2},{0,6,7} },  /* b */
	{   {9,0,0},{0,4,3},{1,3,2},{1,2,2},{1,1,3},{1,1,5},{1,2,6},{1,3,6},{1,4,5},{0,6,7} },  /* c */
	{   {8,0,0},{0,4,0},{1,4,6},{1,2,6},{1,1,5},{1,1,3},{1,2,2},{1,4,2},{0,6,7} },  /* d */
	{   {10,0,0},{0,1,4},{1,4,4},{1,4,3},{1,3,2},{1,2,2},{1,1,3},{1,1,5},{1,2,6},{1,4,6},{0,6,7} },  /* e */
	{   {6,0,0},{0,3,0},{1,2,0},{1,2,6},{0,1,3},{1,3,3},{0,6,7} },  /* f */
	{   {12,0,0},{0,1,7},{1,3,7},{1,4,6},{1,4,3},{1,3,2},{1,2,2},{1,1,3},{1,1,4},{1,2,5},{1,3,5},{1,4,4},{0,6,7} },  /* g */
	{   {7,0,0},{0,1,0},{1,1,6},{0,1,2},{1,3,2},{1,4,3},{1,4,6},{0,6,7} },  /* h */
	{   {8,0,0},{0,3,1},{1,3,2},{0,2,3},{1,3,3},{1,3,6},{0,2,6},{1,4,6},{0,6,7} },  /* i */
	{   {8,0,0},{0,4,1},{1,4,2},{0,4,3},{1,4,6},{1,3,7},{1,2,7},{1,1,6},{0,6,7} },  /* j */
	{   {8,0,0},{0,1,0},{1,1,6},{0,1,5},{0,4,2},{1,1,5},{0,2,4},{1,4,6},{0,6,7} },  /* k */
	{   {3,0,0},{0,3,0},{1,3,6},{0,6,7} },  /* l */
	{   {11,0,0},{0,1,6},{1,1,2},{0,1,3},{1,2,2},{1,3,3},{1,3,6},{0,3,3},{1,4,2},{1,5,3},{1,5,6},{0,6,7} },  /* m */
	{   {8,0,0},{0,1,6},{1,1,2},{0,1,3},{1,2,2},{1,3,2},{1,4,3},{1,4,6},{0,6,7} },  /* n */
	{   {10,0,0},{0,1,3},{1,2,2},{1,3,2},{1,4,3},{1,4,5},{1,3,6},{1,2,6},{1,1,5},{1,1,3},{0,6,7} },  /* o */
	{   {8,0,0},{0,1,7},{1,1,2},{1,3,2},{1,4,3},{1,4,4},{1,3,5},{1,1,5},{0,6,7} },  /* p */
	{   {9,0,0},{0,4,5},{1,2,5},{1,1,4},{1,1,3},{1,2,2},{1,4,2},{1,4,7},{1,5,7},{0,6,7} },  /* q */
	{   {7,0,0},{0,1,2},{1,2,3},{1,2,6},{0,2,3},{1,3,2},{1,4,2},{0,6,7} },  /* r */
	{   {11,0,0},{0,1,5},{1,2,6},{1,3,6},{1,4,5},{1,3,4},{1,2,4},{1,1,3},{1,2,2},{1,3,2},{1,4,3},{0,6,7},{0,6,7} },  /* s */
	{   {7,0,0},{0,2,1},{1,2,6},{1,3,6},{1,4,5},{0,1,2},{1,3,2},{0,6,7} },  /* t */
	{   {9,0,0},{0,1,2},{1,1,5},{1,2,6},{1,3,6},{1,4,5},{1,5,6},{0,4,5},{1,4,2},{0,6,7} },  /* u */
	{   {6,0,0},{0,1,2},{1,1,4},{1,3,6},{1,5,4},{1,5,2},{0,6,7} },  /* v */
	{   {10,0,0},{0,1,2},{1,1,5},{1,2,6},{1,3,5},{1,3,4},{0,3,5},{1,4,6},{1,5,5},{1,5,2},{0,6,7} },  /* w */
	{   {5,0,0},{0,1,2},{1,5,6},{0,1,6},{1,5,2},{0,6,7} },  /* x */
	{   {7,0,0},{0,1,2},{1,1,3},{1,3,5},{0,5,2},{1,5,3},{1,1,7},{0,6,7} },  /* y */
	{   {5,0,0},{0,1,2},{1,5,2},{1,1,6},{1,5,6},{0,6,7} },  /* z */
	{   {7,0,0},{0,4,0},{1,3,1},{1,3,5},{1,4,6},{0,2,3},{1,3,3},{0,6,7} },  /* lcurbrack */
	{   {5,0,0},{0,3,0},{1,3,2},{0,3,3},{1,3,5},{0,6,7} },  /* vertline */
	{   {8,0,0},{0,2,0},{1,3,1},{1,3,5},{1,2,6},{0,3,3},{1,4,3},{0,6,7} },  /* rcurbrack */
	{   {5,0,0},{0,1,3},{1,2,2},{1,4,4},{1,5,3},{0,6,7} },  /* squiggle */
	{   {10,0,0},{0,1,0},{1,5,0},{1,5,6},{1,1,6},{1,1,0},{0,1,1},{1,5,5},{0,1,5},{1,5,1},{0,6,7} }   /* delete */
};
		
static int interFontTemplates[26][25][3] = 
{
	{   {11,0,0},{0,1,2},{1,1,5},{1,2,6},{1,3,6},{1,4,5},{1,5,6},{0,4,5},{1,4,2},{0,3,1},{1,2,0},{0,6,7} },  /* u backslash*/
	{   {11,0,0},{0,1,2},{1,1,6},{0,5,2},{1,5,6},{0,1,2},{1,5,6},{0,1,1},{1,2,0},{1,3,1},{1,4,0},{0,6,7} },  /* N squiggle*/
	{   {9,0,0},{0,5,2},{1,1,2},{1,1,6},{1,5,6},{0,1,4},{1,4,4},{0,2,1},{1,3,0},{0,6,7} },  /* E slash */
	{   {13,0,0},{0,4,1},{1,3,0},{1,2,0},{1,1,1},{1,1,3},{1,2,4},{1,3,4},{1,4,3},{0,2,4},{1,2,5},{1,3,5},{1,2,6},{0,6,7} },  /* c bottom squiggle */
	{   {13,0,0},{0,1,3},{1,2,2},{1,3,2},{1,4,3},{1,4,5},{1,3,6},{1,2,6},{1,1,5},{1,1,3},{0,3,1},{1,2,0},{1,1,1},{0,6,7} },  /* o carret */
	{   {12,0,0},{0,1,3},{1,2,2},{1,3,2},{1,4,3},{1,4,5},{1,3,6},{1,2,6},{1,1,5},{1,1,3},{0,3,1},{1,2,0},{0,6,7} },  /* o backslash */
	{   {10,0,0},{0,2,2},{1,3,2},{0,2,3},{1,3,3},{1,3,6},{0,2,6},{1,4,6},{0,3,1},{1,2,0},{0,6,7} },  /* i backslash */
	{   {12,0,0},{0,4,1},{1,3,0},{1,2,0},{1,1,1},{1,2,3},{1,2,6},{0,1,6},{1,4,6},{1,5,5},{0,1,3},{1,3,3},{0,6,7} },  /* pound */
	{   {18,0,0},{0,3,1},{1,3,2},{0,2,3},{1,3,3},{1,3,6},{0,2,6},{1,4,6},{0,4,1},{1,4,0},{1,5,0},{1,5,1},{1,4,1},{0,2,1},{1,1,1},{1,1,0},{1,2,0},{1,2,1},{0,6,7} },  /* i umlaut */
	{   {19,0,0},{0,1,2},{1,1,5},{1,2,6},{1,3,6},{1,4,5},{1,5,6},{0,4,5},{1,4,2},{0,4,1},{1,4,0},{1,5,0},{1,5,1},{1,4,1},{0,2,1},{1,1,1},{1,1,0},{1,2,0},{1,2,1},{0,6,7} },  /* u umlaut*/
	{   {24,0,0},{0,1,2},{1,3,2},{1,4,3},{1,4,5},{1,5,6},{0,4,5},{1,3,6},{1,2,6},{1,1,5},{1,1,4},{1,2,3},{1,3,3},{1,4,4},{0,4,1},{1,4,0},{1,5,0},{1,5,1},{1,4,1},{0,2,1},{1,1,1},{1,1,0},{1,2,0},{1,2,1},{0,6,7} },  /* a umlaut */
	{   {20,0,0},{0,2,2},{1,4,2},{1,5,3},{1,5,5},{1,4,6},{1,2,6},{1,1,5},{1,1,3},{1,2,2},{0,4,1},{1,4,0},{1,5,0},{1,5,1},{1,4,1},{0,2,1},{1,1,1},{1,1,0},{1,2,0},{1,2,1},{0,6,7} },  /* O umlaut */
	{   {11,0,0},{0,1,2},{1,1,5},{1,2,6},{1,3,6},{1,4,5},{1,5,6},{0,4,5},{1,4,2},{0,2,1},{1,3,0},{0,6,7} },  /* u slash*/
	{   {12,0,0},{0,1,3},{1,2,2},{1,3,2},{1,4,3},{1,4,5},{1,3,6},{1,2,6},{1,1,5},{1,1,3},{0,2,1},{1,3,0},{0,6,7} },  /* o slash */
	{   {22,0,0},{0,1,3},{1,2,2},{1,3,2},{1,4,3},{1,4,5},{1,3,6},{1,2,6},{1,1,5},{1,1,3},{0,4,1},{1,4,0},{1,5,0},{1,5,1},{1,4,1},{0,2,1},{1,1,1},{1,1,0},{1,2,0},{1,2,1},{0,6,7} },  /* o umlaut */
	{   {17,0,0},{0,1,2},{1,1,5},{1,2,6},{1,4,6},{1,5,5},{1,5,2},{0,4,1},{1,4,0},{1,5,0},{1,5,1},{1,4,1},{0,2,1},{1,1,1},{1,1,0},{1,2,0},{1,2,1},{0,6,7} },  /* U umlaut*/
	{   {17,0,0},{0,1,2},{1,3,2},{1,4,3},{1,4,5},{1,5,6},{0,4,5},{1,3,6},{1,2,6},{1,1,5},{1,1,4},{1,2,3},{1,3,3},{1,4,4},{0,4,1},{1,3,0},{1,2,1},{0,6,7} },  /* a caret */
	{   {12,0,0},{0,1,2},{1,1,5},{1,2,6},{1,3,6},{1,4,5},{1,5,6},{0,4,5},{1,4,2},{0,4,1},{1,3,0},{1,2,1},{0,6,7} },  /* u caret */
	{   {11,0,0},{0,3,1},{1,3,2},{0,2,3},{1,3,3},{1,3,6},{0,2,6},{1,4,6},{0,4,1},{1,3,0},{1,2,1},{0,6,7} },  /* i caret */
	{   {12,0,0},{0,1,4},{1,4,4},{1,4,3},{1,3,2},{1,2,2},{1,1,3},{1,1,5},{1,2,6},{1,4,6},{0,2,1},{1,3,0},{0,6,7} },  /* e slash */
	{   {12,0,0},{0,1,4},{1,4,4},{1,4,3},{1,3,2},{1,2,2},{1,1,3},{1,1,5},{1,2,6},{1,4,6},{0,3,1},{1,2,0},{0,6,7} },  /* e backslash */
	{   {12,0,0},{0,1,6},{1,1,2},{0,1,3},{1,2,2},{1,3,2},{1,4,3},{1,4,6},{0,1,1},{1,2,0},{1,3,1},{1,4,0},{0,6,7} },  /* n squiggle */
	{   {13,0,0},{0,1,4},{1,4,4},{1,4,3},{1,3,2},{1,2,2},{1,1,3},{1,1,5},{1,2,6},{1,4,6},{0,3,1},{1,2,0},{1,1,1},{0,6,7} },  /* e caret */
	{   {19,0,0},{0,1,2},{1,3,2},{1,4,3},{1,4,5},{1,5,6},{0,4,5},{1,3,6},{1,2,6},{1,1,5},{1,1,4},{1,2,3},{1,3,3},{1,4,4},{0,3,1},{1,2,1},{1,2,0},{1,3,0},{1,3,1},{0,6,7} },  /* a dot */
	{   {16,0,0},{0,1,2},{1,3,2},{1,4,3},{1,4,5},{1,5,6},{0,4,5},{1,3,6},{1,2,6},{1,1,5},{1,1,4},{1,2,3},{1,3,3},{1,4,4},{0,3,1},{1,2,0},{0,6,7} },  /* a backslash */
	{   {13,0,0},{0,1,6},{1,1,4},{1,3,2},{1,5,4},{1,5,6},{0,5,5},{1,1,5},{0,3,1},{1,2,1},{1,2,0},{1,3,0},{1,3,1},{0,6,7} },  /* A dot */
};

+ (Atari1020Simulator *)sharedInstance {
    return sharedInstance ? sharedInstance : [[self alloc] init];
}

- (id)init {
    if (sharedInstance) {
		[self dealloc];
    } else {
        [super init];
	}
	
    sharedInstance = self;
	
	printPath = [[PrintablePath alloc] init];
    [printPath retain];
			
    return sharedInstance;
}

- (void)printChar:(char) character
{
    unsigned char thechar = (unsigned char) character;

    thechar &= 0x7F;

	switch (state)
	{
		case ATARI1020_TEXT_IDLE:
			[self textIdleState:thechar];
			break;
		case ATARI1020_GRAPHICS_IDLE:
			[self graphicsIdleState:thechar];
			break;
		case ATARI1020_GOT_ESC:                                   
			[self gotEscState:thechar];
			break;
		case ATARI1020_WAIT_CR:                                   
			[self waitCrState:thechar];
			break;
		case ATARI1020_GOT_RESET:                                   
			[self gotResetState:thechar];
			break;
		case ATARI1020_GOT_CHANGE_COLOR:    
			[self gotChangeColorState:thechar];
			break;
		case ATARI1020_GOT_DRAW_ABSOLUTE:   
			[self gotDrawAbsoluteState:thechar];
			break;
		case ATARI1020_GOT_DRAW_ABSOLUTE_Y:   
			[self gotDrawAbsoluteYState:thechar];
			break;
		case ATARI1020_GOT_DRAW_RELATIVE:   
			[self gotDrawRelativeState:thechar];
			break;
		case ATARI1020_GOT_DRAW_RELATIVE_Y:   
			[self gotDrawRelativeYState:thechar];
			break;
		case ATARI1020_GOT_MOVE_ABSOLUTE:   
			[self gotMoveAbsoluteState:thechar];
			break;
		case ATARI1020_GOT_MOVE_ABSOLUTE_Y:   
			[self gotMoveAbsoluteYState:thechar];
			break;
		case ATARI1020_GOT_LINE_TYPE:       
			[self gotLineTypeState:thechar];
			break;
		case ATARI1020_GOT_PRINT_TEXT:      
			[self gotPrintTextState:thechar];
			break;
		case ATARI1020_GOT_CHAR_SIZE:       
			[self gotCharSizeState:thechar];
			break;
		case ATARI1020_GOT_ROTATE_PRINT:    
			[self gotRotatePrintState:thechar];
			break;
		case ATARI1020_GOT_MOVE_RELATIVE:   
			[self gotMoveRelativeState:thechar];
			break;
		case ATARI1020_GOT_MOVE_RELATIVE_Y:   
			[self gotMoveRelativeYState:thechar];
			break;
		case ATARI1020_GOT_DRAW_XY_AXIS:        
			[self gotDrawAxisState:thechar];
			break;
		case ATARI1020_GOT_DRAW_XY_AXIS_STEP:        
			[self gotDrawAxisStepState:thechar];
			break;
		case ATARI1020_GOT_DRAW_XY_AXIS_INTERVAL:        
			[self gotDrawAxisIntervalState:thechar];
			break;
	}
}

- (void)addChar:(unsigned char)character
{
	NSAffineTransform *transform;
	NSBezierPath *newChar;
	PrintablePath *printableChar;
	float horizOffset = 0.0, vertOffset = 0.0;
	
	if (state == ATARI1020_TEXT_IDLE)
		vertOffset -= 7*(charSize+1)*POINTS_PER_STEP;
	else
		{
		switch(rotation)
			{
			case 0:
				vertOffset -= 7*(charSize+1)*POINTS_PER_STEP;
				break;
			case 1:
				horizOffset += 7*(charSize+1)*POINTS_PER_STEP;
				break;
			case 2:
				vertOffset += 7*(charSize+1)*POINTS_PER_STEP;
				break;
			case 3:
				horizOffset -= 7*(charSize+1)*POINTS_PER_STEP;
				break;
			}
		}

	transform = [NSAffineTransform transform];
	// Translate the glyph the apporpriate amount
	[transform translateXBy:horizPosition+horizOffset yBy:vertPosition+vertOffset];
	// Scale it to the correct character size
	[transform scaleBy:(float)(charSize + 1)];
	// Rotate it to the proper orientation
	[transform rotateByDegrees:(rotation * 90.0)];

	if (internationalMode) {
		newChar = [transform transformBezierPath:interFontPaths[character-0x20]];
		}
	else
		newChar = [transform transformBezierPath:fontPaths[character-0x20]];
	printableChar = [newChar copy];
	[printableChar setColor:colors[color]];
	
	[[PrintOutputController sharedInstance] addToPrintArray:printableChar];
	[printableChar release];
	[newChar release];
	[transform release];
	
	if (state == ATARI1020_TEXT_IDLE)
		horizPosition += 6*(charSize+1)*POINTS_PER_STEP;
	else
		{
		switch(rotation)
			{
			case 0:
				horizPosition += 6*(charSize+1)*POINTS_PER_STEP;
				break;
			case 1:
				vertPosition += 6*(charSize+1)*POINTS_PER_STEP;
				break;
			case 2:
				horizPosition -= 6*(charSize+1)*POINTS_PER_STEP;
				break;
			case 3:
				vertPosition -= 6*(charSize+1)*POINTS_PER_STEP;
				break;
			}
		}
		
	[self checkPositions];

	if (state == ATARI1020_TEXT_IDLE && 
	    (horizPosition + ( 6*(charSize+1)*POINTS_PER_STEP) > rightMargin)) 
		{
			[self executeLineFeed:lineSpacing];            // move to next line
			horizPosition = leftMargin;		//  move to left margin;
		}
			
}

- (void)textIdleState:(unsigned char) character
{
	if ((character >= 0x20) && (character <= 0x7F))
		{
		if (internationalMode)
			{
			if ((character >= 'A') && (character <= 'Z'))
				[self addChar:(character-'A'+0x20)];
			}
		else
			{
            [self addChar:character];
			}
        }
	else 
		{
		switch(character)
		{
			case 0x08: // BS
				horizPosition -= 6*(charSize+1)*POINTS_PER_STEP;
				if (horizPosition < leftMargin)
					horizPosition = leftMargin;
				break;
			case 0x0A: // LF
				[self executeLineFeed:lineSpacing];            // move to next line
				break;
			case 0x0B: // Rev LF
				[self executeLineFeed:-lineSpacing];            // move to next line
				break;
			case 0x0D: // CR
				if (autoLineFeed == YES)
					{
					[self executeLineFeed:lineSpacing];            // move to next line
					}
				horizPosition = leftMargin;		//  move to left margin;
				break;
			case 0x11: // DC1 - Text Mode
				break;
			case 0x12: // DC2 - Graphic Mode
				horizOrigin = leftMargin;
				vertOrigin = vertPosition;
                state = ATARI1020_GRAPHICS_IDLE;
				break;
			case 0x1B: // ESC
				state = ATARI1020_GOT_ESC;
				break;
			case 0x1D: // Next Color
                color++;
                color %= 4;
				break;
			default:
				break;
			}
		}
}

- (void)gotEscState:(unsigned char) character
{
		switch(character)
		{
			case 0x07: // Graphics Mode
				horizOrigin = leftMargin;
				vertOrigin = vertPosition;
                state = ATARI1020_GRAPHICS_IDLE;
				break;
			case 0x10: // 20 Char Text Mode
				charSize = 3;
				lineSpacing = 6*2*POINTS_PER_STEP*(charSize+1);
				state = ATARI1020_TEXT_IDLE;
				break;
			case 0x0E: // 40 Char Text Mode
				charSize = 1;
				lineSpacing = 6*2*POINTS_PER_STEP*(charSize+1);
				state = ATARI1020_TEXT_IDLE;
				break;
			case 0x13: // 80 Char Text Mode
				charSize = 0;
				lineSpacing = 6*2*POINTS_PER_STEP*(charSize+1);
				state = ATARI1020_TEXT_IDLE;
				break;
			case 0x17: // International Mode
				internationalMode = YES;
				state = ATARI1020_TEXT_IDLE;
				break;
			case 0x18: // US Mode
				internationalMode = NO;
				state = ATARI1020_TEXT_IDLE;
				break;
			default:
				break;
		}
}

- (void)graphicsIdleState:(unsigned char) character
{
	NSPoint point;
	
	switch(character)
	{
		case 0x11: // DC1 - Text Mode
			state = ATARI1020_TEXT_IDLE;
			break;
		case 'A':
			horizPosition = leftMargin;
			horizOrigin = horizPosition;
			vertOrigin = vertPosition;
            state = ATARI1020_GOT_RESET;
            break;
        case 'C':
            state = ATARI1020_GOT_CHANGE_COLOR;
            break;
                case 'D':
                        charCount  = 0;
						point = NSMakePoint(horizPosition, vertPosition);
						[printPath moveToPoint:point];
                        state = ATARI1020_GOT_DRAW_ABSOLUTE;
                        break;
                case 'H':
                        horizPosition = horizOrigin;
						vertPosition = vertOrigin;
                        state = ATARI1020_WAIT_CR;
                        break;
                case 'I':
                        horizOrigin = horizPosition;
						vertOrigin = vertPosition;
                        state = ATARI1020_WAIT_CR;
                        break;
                case 'J':
                        charCount  = 0;
						point = NSMakePoint(horizPosition, vertPosition);
						[printPath moveToPoint:point];
                        state = ATARI1020_GOT_DRAW_RELATIVE;
                        break;
                case 'M':
                        charCount  = 0;
                        state = ATARI1020_GOT_MOVE_ABSOLUTE;
                        break;
                case 'L':
                        lineType = 0;
                        state = ATARI1020_GOT_LINE_TYPE;
                        break;
                case 'P':
                        state = ATARI1020_GOT_PRINT_TEXT;
                        break;
                case 'S':
                        charSize = 0;
                        state = ATARI1020_GOT_CHAR_SIZE;
                        break;
                case 'Q':
                        state = ATARI1020_GOT_ROTATE_PRINT;
                        break;
                case 'R':
                        charCount  = 0;
                        state = ATARI1020_GOT_MOVE_RELATIVE;
                        break;
                case 'X':
						axis = 0;
						point = NSMakePoint(horizPosition, vertPosition);
						[printPath moveToPoint:point];
                        state = ATARI1020_GOT_DRAW_XY_AXIS;
                        break;
		default:
			break;
	}

}
 
- (void)waitCrState:(unsigned char) character
{
	if (character == 0x0D || character == '*')
		{
		state = ATARI1020_GRAPHICS_IDLE;
		}
}

- (void)gotResetState:(unsigned char) character
{
	if (character == 0x0D)
		{
		state = ATARI1020_TEXT_IDLE;
		}
}

- (void)gotChangeColorState:(unsigned char) character
{
	switch (character)
        {
                case 0x0D:
				case '*':
                        color = 0;
                        state = ATARI1020_GRAPHICS_IDLE;
                        break;
                case '0':
                case '1':
                case '2':
                case '3':
                        color = character - '0';
                        state = ATARI1020_WAIT_CR;
                        break;
        }
}

- (void)gotRotatePrintState:(unsigned char) character
{
	switch (character)
        {
                case 0x0D:
				case '*':
                        rotation = 0;
                        state = ATARI1020_GRAPHICS_IDLE;
                        break;
                case '0':
                case '1':
                case '2':
                case '3':
                        rotation = character - '0';
                        state = ATARI1020_WAIT_CR;
                        break;
        }
}

- (void)gotLineTypeState:(unsigned char) character
{
	switch (character)
        {
                case 0x0D:
				case '*':
                        if (lineType > 15)
                                lineType = 0;
                        state = ATARI1020_GRAPHICS_IDLE;
                        break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                        lineType *= 10;
                        lineType += character - '0';
                        break;
                default:
                        lineType = 0;
                        state = ATARI1020_WAIT_CR;
        }
}

- (void)gotCharSizeState:(unsigned char) character
{
	switch (character)
        {
		case 0x0D:
		case '*':
			if (charSize > 63)
				charSize = 0;
			lineSpacing = 6*2*POINTS_PER_STEP*(charSize+1);
			state = ATARI1020_GRAPHICS_IDLE;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			charSize *= 10;
			charSize += character - '0';
			break;
		default:
			charSize = 0;
			state = ATARI1020_WAIT_CR;
        }
}

- (void)checkAccums
{
        if (xAccum < -999)
                xAccum = 999;
        if (xAccum > 999)
                xAccum = 999;
        if (yAccum < -999)
                yAccum = -999;
        if (yAccum > 999)
                yAccum = 999;
}

- (void)checkPositions
{
	[self checkPoints:&horizPosition:&vertPosition];
}

- (void)checkPoints:(float *)horiz:(float *)vert
{
	if (*horiz < leftMargin)
		*horiz = leftMargin;
	else if (horizPosition > rightMargin)
		*horiz = rightMargin;
	
	if (*vert < 0)
		*vert = 0;
}

- (void)gotDrawAbsoluteState:(unsigned char) character
{
	switch (character)
        {
                case ',':
						charBuffer[charCount] = 0;
						xAccum = atof(charBuffer);
						charCount = 0;
                        state = ATARI1020_GOT_DRAW_ABSOLUTE_Y;
                        break;
                case '-':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
				case '.':
                        charBuffer[charCount++] = character;
                        break;
                default:
                        state = ATARI1020_WAIT_CR;
						[self clearPrintPath];
        }
}

- (void)gotDrawAbsoluteYState:(unsigned char) character
{
	NSPoint point;
	float horizNew;
	float vertNew;
	
	switch (character)
        {
                case 0x0D:
				case '*':
                case ',':
						charBuffer[charCount] = 0;
						yAccum = atof(charBuffer);
                        [self checkAccums];
						horizNew = horizOrigin + xAccum * POINTS_PER_STEP;
						vertNew = vertOrigin - yAccum * POINTS_PER_STEP;
						[self checkPoints:&horizNew:&vertNew];
						point = NSMakePoint(horizNew, vertNew);
						[printPath lineToPoint:point];
						horizPosition = horizNew;
						vertPosition = vertNew;

                        if (character == 0x0D || character == '*')
                                {
								[self emptyPrintPath];
                                state = ATARI1020_GRAPHICS_IDLE;
                                }
                        else
                                {
                                state = ATARI1020_GOT_DRAW_ABSOLUTE;
                                }
                        break;
                case '-':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
				case '.':
                        charBuffer[charCount++] = character;
                        break;
                default:
						[self clearPrintPath];
                        state = ATARI1020_WAIT_CR;
        }
}

- (void)gotDrawRelativeState:(unsigned char) character
{
	switch (character)
        {
                case ',':
						charBuffer[charCount] = 0;
						xAccum = atof(charBuffer);
						charCount = 0;
                        state = ATARI1020_GOT_DRAW_RELATIVE_Y;
                        break;
                case '-':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
				case '.':
                        charBuffer[charCount++] = character;
                        break;
                default:
						[self clearPrintPath];
                        state = ATARI1020_WAIT_CR;
        }
}

- (void)gotDrawRelativeYState:(unsigned char) character
{
	float horizNew;
	float vertNew;
	NSPoint point;
	
	switch (character)
        {
                case 0x0D:
                case ',':
				case '*':
						charBuffer[charCount] = 0;
						yAccum = atof(charBuffer);
                        [self checkAccums];
						horizNew = horizPosition + xAccum * POINTS_PER_STEP;
						vertNew = vertPosition - yAccum * POINTS_PER_STEP;
						[self checkPoints:&horizNew:&vertNew];
						point = NSMakePoint(horizNew, vertNew);
						[printPath lineToPoint:point];
						horizPosition = horizNew;
						vertPosition = vertNew;

                        if (character == 0x0D || character == '*')
                                {
								[self emptyPrintPath];
                                state = ATARI1020_GRAPHICS_IDLE;
                                }
                        else
                                {
                                state = ATARI1020_GOT_DRAW_RELATIVE;
                                }
                        break;
                case '-':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
				case '.':
                        charBuffer[charCount++] = character;
                        break;
                default:
						[self clearPrintPath];
                        state = ATARI1020_WAIT_CR;
        }
}

- (void)gotMoveAbsoluteState:(unsigned char) character
{
	switch (character)
        {
                case 0x0D:
				case '*':
                        state = ATARI1020_GRAPHICS_IDLE;
                        break;
                case ',':
						charBuffer[charCount] = 0;
						xAccum = atof(charBuffer);
						charCount = 0;
                        state = ATARI1020_GOT_MOVE_ABSOLUTE_Y;
                        break;
                case '-':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
				case '.':
                        charBuffer[charCount++] = character;
                        break;
                default:
                        state = ATARI1020_WAIT_CR;
        }
}

- (void)gotMoveAbsoluteYState:(unsigned char) character
{
	switch (character)
        {
                case 0x0D:
				case '*':
						charBuffer[charCount] = 0;
						yAccum = atof(charBuffer);
                        [self checkAccums];
                        // Move to the point
						horizPosition = horizOrigin + xAccum * POINTS_PER_STEP;
						vertPosition = vertOrigin - yAccum * POINTS_PER_STEP;
						[self checkPositions];
						
                        state = ATARI1020_GRAPHICS_IDLE;
                        break;
                case '-':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
				case '.':
                        charBuffer[charCount++] = character;
                        break;
                default:
                        state = ATARI1020_WAIT_CR;
        }
}

- (void)gotMoveRelativeState:(unsigned char) character
{
	switch (character)
        {
                case 0x0D:
				case '*':
                        state = ATARI1020_GRAPHICS_IDLE;
                        break;
                case ',':
						charBuffer[charCount] = 0;
						xAccum = atof(charBuffer);
						charCount = 0;
                        state = ATARI1020_GOT_MOVE_RELATIVE_Y;
                        break;
                case '-':
				case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
				case '.':
                        charBuffer[charCount++] = character;
                        break;
                default:
                        state = ATARI1020_WAIT_CR;
        }
}

- (void)gotMoveRelativeYState:(unsigned char) character
{
	switch (character)
        {
                case 0x0D:
						charBuffer[charCount] = 0;
						yAccum = atof(charBuffer);
                        [self checkAccums];
                        // Move to the point
						horizPosition += xAccum * POINTS_PER_STEP;
						vertPosition -= yAccum * POINTS_PER_STEP;
						[self checkPositions];
						
                        state = ATARI1020_GRAPHICS_IDLE;
                        break;
                case '-':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
				case '.':
                        charBuffer[charCount++] = character;
                        break;
                default:
                        state = ATARI1020_WAIT_CR;
        }
}

-(void) gotPrintTextState:(unsigned char) character
{
        if (character == 0x0D)
                {
                state = ATARI1020_GRAPHICS_IDLE;
                }
        else if (character >= 0x20 && character <= 0x7e)
                {
                [self addChar:character];
                }
}

- (void)gotDrawAxisState:(unsigned char) character
{
	switch (character)
        {
                case 0x0D:
				case '*':
						[self clearPrintPath];
                        state = ATARI1020_GRAPHICS_IDLE;
                case ',':
						charCount = 0;
						state = ATARI1020_GOT_DRAW_XY_AXIS_STEP;
						break;
                case '0':
						axis = 0;
						break;
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
						axis = 1;
						break;
                default:
						[self clearPrintPath];
                        state = ATARI1020_WAIT_CR;
        }
}

- (void)gotDrawAxisStepState:(unsigned char) character
{
	switch (character)
        {
                case 0x0D:
				case '*':
						[self clearPrintPath];
                        state = ATARI1020_GRAPHICS_IDLE;
                        break;
                case ',':
						charBuffer[charCount] = 0;
						stepAccum = atof(charBuffer);
						charCount = 0;
                        state = ATARI1020_GOT_DRAW_XY_AXIS_INTERVAL;
                        break;
                case '-':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
				case '.':
                        charBuffer[charCount++] = character;
                        break;
                default:
						[self clearPrintPath];
                        state = ATARI1020_WAIT_CR;
        }
}

- (void)gotDrawAxisIntervalState:(unsigned char) character
{
	int steps;
	float horizNext = 0.0, vertNext = 0.0;
	NSPoint endPoint;

	switch (character)
        {
		case 0x0D:
		case '*':
			charBuffer[charCount] = 0;
			intervalAccum = atoi(charBuffer);
			if (axis)
				{
				endPoint = NSMakePoint(horizPosition, vertPosition - POINTS_PER_STEP);
				[self checkPoints:&endPoint.x:&endPoint.y];
				[printPath moveToPoint:endPoint];
										   
				endPoint = NSMakePoint(horizPosition, vertPosition + POINTS_PER_STEP);
				[self checkPoints:&endPoint.x:&endPoint.y];
				[printPath lineToPoint:endPoint];
					
				endPoint = NSMakePoint(horizPosition, vertPosition);
				[self checkPoints:&endPoint.x:&endPoint.y];
				[printPath moveToPoint:endPoint];
				
				for (steps=1;steps<(intervalAccum+1);steps++)
					{					
					horizNext = horizPosition + steps * stepAccum * POINTS_PER_STEP;
					endPoint = NSMakePoint(horizNext,vertPosition);
					[self checkPoints:&endPoint.x:&endPoint.y];
					[printPath lineToPoint:endPoint];

					endPoint = NSMakePoint(horizNext, vertPosition - POINTS_PER_STEP);
					[self checkPoints:&endPoint.x:&endPoint.y];
					[printPath moveToPoint:endPoint];
										   
					endPoint = NSMakePoint(horizNext, vertPosition + POINTS_PER_STEP);
					[self checkPoints:&endPoint.x:&endPoint.y];
					[printPath lineToPoint:endPoint];
					
					endPoint = NSMakePoint(horizNext, vertPosition);
					[self checkPoints:&endPoint.x:&endPoint.y];
					[printPath moveToPoint:endPoint];
					}
					horizPosition = horizNext;
				}
			else
				{
				endPoint = NSMakePoint(horizPosition - POINTS_PER_STEP, vertPosition);
				[self checkPoints:&endPoint.x:&endPoint.y];
				[printPath moveToPoint:endPoint];
										   
				endPoint = NSMakePoint(horizPosition + POINTS_PER_STEP, vertPosition);
				[self checkPoints:&endPoint.x:&endPoint.y];
				[printPath lineToPoint:endPoint];
					
				endPoint = NSMakePoint(horizPosition, vertPosition);
				[self checkPoints:&endPoint.x:&endPoint.y];
				[printPath moveToPoint:endPoint];
				
				for (steps=1;steps<(intervalAccum+1);steps++)
					{
					vertNext = vertPosition - steps * stepAccum * POINTS_PER_STEP;
					endPoint = NSMakePoint(horizPosition,vertNext);
					[self checkPoints:&endPoint.x:&endPoint.y];
					[printPath lineToPoint:endPoint];
					
					endPoint = NSMakePoint(horizPosition - POINTS_PER_STEP, vertNext);
					[self checkPoints:&endPoint.x:&endPoint.y];
					[printPath moveToPoint:endPoint];
										   
					endPoint = NSMakePoint(horizPosition + POINTS_PER_STEP, vertNext);
					[self checkPoints:&endPoint.x:&endPoint.y];
					[printPath lineToPoint:endPoint];
					
					endPoint = NSMakePoint(horizPosition, vertNext);
					[self checkPoints:&endPoint.x:&endPoint.y];
					[printPath moveToPoint:endPoint];
					}
					vertPosition = vertNext;
				}
			// Draw Axis
			[self emptyPrintPath];
			state = ATARI1020_GRAPHICS_IDLE;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '.':
            charBuffer[charCount++] = character;
			break;
		default:
			state = ATARI1020_WAIT_CR;
        }
}

-(void)reset
{
    int i, numStrokes, stroke;
	NSPoint strokePoint;

	scaleFactor	= prefs1020.printWidth + 1;
	autoPageAdjust = prefs1020.autoPageAdjust;

	state = ATARI1020_TEXT_IDLE;
	rightMargin = 480*POINTS_PER_STEP + ATARI1020_LEFT_PRINT_EDGE;
	leftMargin = 0.0 + ATARI1020_LEFT_PRINT_EDGE;
	horizPosition = leftMargin;
	
	for (i=0;i<4;i++)
		if (colors[i] != nil)
			[colors[i] release];
		
	colors[0] = [NSColor colorWithCalibratedRed:prefs1020.pen1Red
						 green:prefs1020.pen1Green
						 blue:prefs1020.pen1Blue
						 alpha:prefs1020.pen1Alpha];
	colors[1] = [NSColor colorWithCalibratedRed:prefs1020.pen2Red
						 green:prefs1020.pen2Green
						 blue:prefs1020.pen2Blue
						 alpha:prefs1020.pen2Alpha];
	colors[2] = [NSColor colorWithCalibratedRed:prefs1020.pen3Red
						 green:prefs1020.pen3Green
						 blue:prefs1020.pen3Blue
						 alpha:prefs1020.pen3Alpha];
	colors[3] = [NSColor colorWithCalibratedRed:prefs1020.pen4Red
						 green:prefs1020.pen4Green
						 blue:prefs1020.pen4Blue
						 alpha:prefs1020.pen4Alpha];

	for (i=0;i<4;i++)
		[colors[i] retain];
						 
	for (i=0;i<16;i++)
		{
		lineTypes[i][0] = i * POINTS_PER_STEP;
		lineTypes[i][1] = i * POINTS_PER_STEP;
		}
		
	[NSBezierPath setDefaultMiterLimit:50.0];
	[NSBezierPath setDefaultLineCapStyle:NSRoundLineCapStyle];
	[NSBezierPath setDefaultLineJoinStyle:NSRoundLineJoinStyle];
	[NSBezierPath setDefaultLineWidth:(0.5 * scaleFactor)];
	
	for (i=0;i<96;i++)
		{
		if (fontPaths[i] != nil)
			[fontPaths[i] release];
		fontPaths[i] = [[PrintablePath alloc] init];
		numStrokes = fontTemplates[i][0][0];
		strokePoint = NSMakePoint(0,0);
		[fontPaths[i] moveToPoint:strokePoint];

		for (stroke = 1;stroke < (numStrokes + 1);stroke++)
			{
				strokePoint = NSMakePoint(fontTemplates[i][stroke][1] * POINTS_PER_STEP,
										  fontTemplates[i][stroke][2] * POINTS_PER_STEP);
				if (fontTemplates[i][stroke][0])
					{
					[fontPaths[i] lineToPoint:strokePoint];
					}
				else
					{
					[fontPaths[i] moveToPoint:strokePoint];
					}
			}
		}

	for (i=0;i<26;i++)
		{
		if (interFontPaths[i] != nil)
			[interFontPaths[i] release];
		interFontPaths[i] = [[PrintablePath alloc] init];
		numStrokes = interFontTemplates[i][0][0];
		strokePoint = NSMakePoint(0,0);
		[interFontPaths[i] moveToPoint:strokePoint];

		for (stroke = 1;stroke < (numStrokes + 1);stroke++)
			{
				strokePoint = NSMakePoint(interFontTemplates[i][stroke][1] * POINTS_PER_STEP,
										  interFontTemplates[i][stroke][2] * POINTS_PER_STEP);
				if (interFontTemplates[i][stroke][0])
					{
					[interFontPaths[i] lineToPoint:strokePoint];
					}
				else
					{
					[interFontPaths[i] moveToPoint:strokePoint];
					}
			}
		}

	color = 0;
	rotation = 0;
	lineType = 0;
	charSize = 1;
	
	if (prefs1020.autoLinefeed)
		autoLineFeed = YES;
	else
		autoLineFeed = NO;
	lineSpacing = 6*2*POINTS_PER_STEP*(charSize+1);
	formLength = 72.0*prefs1020.formLength;
	vertPosition = 0.0;
                
	horizOrigin = horizPosition;
	vertOrigin = vertPosition;
	
	internationalMode = NO;
        
}

-(void)emptyPrintPath
{
	if (lineType)
		[printPath setLineDash:lineTypes[lineType] count:2 phase:0.0];
	[printPath setColor:colors[color]];
	[[PrintOutputController sharedInstance] addToPrintArray:printPath];
	[printPath release];
	printPath = [[PrintablePath alloc] init];
}

-(void)clearPrintPath
{
	[printPath release];
	printPath = [[PrintablePath alloc] init];
}

-(float)getVertPosition
{
	return(vertPosition);
}

-(float)getFormLength
{
	return(formLength);
}

-(void)executeLineFeed
{
	[self executeLineFeed:lineSpacing];
}

-(void)executeRevLineFeed
{
	[self executeLineFeed:-lineSpacing];
}

-(void)executeLineFeed:(float) amount
{
	vertPosition += amount;
	if (vertPosition < 0.0)
		vertPosition = 0.0;	
}

-(void)executeFormFeed
{
	vertPosition = (((int) (vertPosition/formLength)) + 1) * formLength;
}

-(void)executePenChange
{
	color = (color + 1) % 4;
}

-(NSColor *)getPenColor
{
	return(colors[color]);
}

-(void)topBlankForm
{
	vertPosition = 0.0;
	horizPosition = leftMargin;
	horizOrigin = horizPosition;
	vertOrigin = vertPosition;
}

-(bool)isAutoPageAdjustEnabled
{
	return(autoPageAdjust);
}

@end

