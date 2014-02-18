/* copydata.h - copy function module
 For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 
 */


#include "config.h"
#include "atari.h"
#include "antic.h"
#include "memory.h"

#define MAX_SCREEN_LINES 500

static struct screen_lines_type {
	unsigned int start_y;
	unsigned int end_y;
	unsigned int type;
	unsigned int data_ptr;
} screen_lines[MAX_SCREEN_LINES];

static int mode_scan_lines[16] = {0,0,8,10,8,16,8,16,8,4,4,2,1,2,1,1};
static int mode_memory_bytes[16] = {0,0,40,40,40,40,20,20,10,10,20,20,20,40,40,40};

static int screen_line_count = 0;

static UBYTE get_dlist_byte(UWORD *addr)
{
	UBYTE result = MEMORY_dGetByte(*addr);
	(*addr)++;
	if ((*addr & 0x03ff) == 0)
		(*addr) -= 0x0400;
	return result;
}

static void parseDL(void)
{
	UWORD tdlist;
	int done = FALSE;
	int screen_ypos;
	UWORD screen_data = 0;
	
	screen_line_count = 0;
	screen_ypos = 0;
	tdlist = ANTIC_dlist;
	while (!done) {
		UBYTE IR;
		UWORD addr;
		
		IR = get_dlist_byte(&tdlist);
		
		switch (IR & 0x0f) {
			case 0x00: // Blank lines
				screen_ypos += ((IR >> 4) & 0x07) + 1;
				break;
			case 0x01: // Jump instructions
				addr = get_dlist_byte(&tdlist);
				addr |= get_dlist_byte(&tdlist) << 8;
				if (IR & 0x40) {
					done = TRUE;
				}
				else {
					tdlist = addr;
				}
				break;
			default:
				if (IR & 0x40) {  // Load data pointer
					addr = get_dlist_byte(&tdlist);
					addr |= get_dlist_byte(&tdlist) << 8;
					screen_data = addr;
				}
				
#if 0 // Note, horizontal and vertical scrolling is not handled by copy at the moment.				
				if (IR & 0x20)
					mon_printf("VSCROL ");
				
				if (IR & 0x10)
					mon_printf("HSCROL ");
#endif				
				screen_lines[screen_line_count].start_y = screen_ypos;
				screen_lines[screen_line_count].end_y = screen_ypos + mode_scan_lines[IR & 0x0f] - 1;
				screen_lines[screen_line_count].type = IR & 0x0f;
				screen_lines[screen_line_count].data_ptr = screen_data;
				
				screen_ypos += mode_scan_lines[IR & 0x0f];
				screen_data += mode_memory_bytes[IR & 0x0f];
				screen_line_count++;
				if (screen_line_count == MAX_SCREEN_LINES)
					done = TRUE;
		}
	}
}
	
int Atari800GetCopyData(int startx, int endx, int starty, int endy, unsigned char *data)
{
	int i,x;
	int char_count = 0;
	UWORD data_ptr;
	UBYTE character;
	int startcol40, endcol40, startcol20, endcol20;
	int startline = 0;
	int endline = 0;
	
	parseDL();
	
	// Find the starting line
	for (i=0;i<screen_line_count;i++) {
		if (starty <= screen_lines[i].start_y) {
			startline = i;
			break;
		}
		if (starty > screen_lines[i].start_y && starty <= screen_lines[i].end_y) {
			if (i == screen_line_count-1)
				return 0;
			startline = i+1;
			break;
		}
	}
	
	// Check if no full lines are selected
	if (endy < screen_lines[startline].end_y) {
		*data = 0;
		return(0);
	}
	
	// Find the ending line
	for (i=startline; i<screen_line_count;i++) {
		if (endy == screen_lines[i].end_y) {
			endline = i;
			break;
		}
		if (endy < screen_lines[i].end_y) {
			endline = i-1;
			break;
		}
	}
	if (i==screen_line_count)
		endline = screen_line_count - 1;
	
	// Find the starting column
	if (startx % 8 ==0)
		startcol40 = startx/8;
	else
		startcol40 = startx/8 + 1;
	// Find the ending column
	if (endx % 8 == 7)
		endcol40 = endx/8;
	else
		endcol40 = endx/8 -1;
	
	// If no full columns are selected
	if (startcol40 > endcol40) {
		*data = 0;
		return(0);
	}
	
	startcol20 = startcol40/2;
	endcol20 = endcol40/2;
	
	for (i=startline;i<=endline;i++) {
		int startcol=0;
		int endcol=0;
		
		if (screen_lines[i].type > 7) {
			continue;
		} else if (screen_lines[i].type >= 2 && screen_lines[i].type <= 5) {
			startcol = startcol40;
			endcol = endcol40;
		} else if (screen_lines[i].type >= 6 && screen_lines[i].type <= 7) {
			startcol = startcol20;
			endcol = endcol20;
		}
		data_ptr = screen_lines[i].data_ptr + startcol;
		for (x=startcol;x<=endcol;x++,data_ptr++) {
			char_count++;
			character = MEMORY_dGetByte(data_ptr) & 0x7F;
			if (character <= 0x3F)
				*data++ = character +0x20;
			else if (character >= 'a' && character <= 'z')
				*data++ = character;
			else
				*data++ = ' ';
		}
		if (i!=endline) {
			*data++ = '\n';
			char_count++;
		}
	}
	*data = 0;
	return char_count;
}
