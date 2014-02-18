/* EpsonFX80Simulator.m - 
 EpsonFX80Simulator class for the
 Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 */

#import "EpsonFX80Simulator.h"
#import "PrintOutputController.h"
#import "Preferences.h"

// States of the simulated printer state machine
#define FX_IDLE						0
#define FX_GOT_ESC					1
#define FX_GOT_MASTER_MODE			2
#define FX_GOT_GRAPH_MODE			3
#define FX_GOT_GRAPH_MODE_TYPE		4
#define FX_GOT_NINE_PIN_GRAPH		5
#define FX_GOT_NINE_PIN_GRAPH_TYPE  6
#define FX_GOT_GRAPH_CNT1			7
#define FX_IN_GRAPH_MODE			8
#define FX_GOT_UNDER_MODE			9
#define FX_GOT_VERT_TAB_CHAN		10
#define FX_GOT_SUPER_LINE_SPACE		11
#define FX_GOT_GRAPH_REDEF			12
#define FX_GOT_GRAPH_REDEF_CODE		13
#define FX_GOT_LINE_SPACE			14
#define FX_GOT_VERT_TAB_SET			15
#define FX_GOT_FORM_LENGTH_LINE		16
#define FX_GOT_FORM_LENGTH_INCH		17
#define FX_GOT_HORIZ_TAB_SET		18
#define FX_GOT_PRINT_CTRL_CODES		19
#define FX_GOT_IMMED_LF				20
#define FX_GOT_SKIP_OVER_PERF		21
#define FX_GOT_RIGHT_MARGIN			22
#define FX_GOT_INTER_CHAR_SET		23
#define FX_GOT_SCRIPT_MODE			24
#define FX_GOT_EXPAND_MODE			25
#define FX_GOT_VERT_TAB_CHAN_SET	26
#define FX_GOT_VERT_TAB_CHAN_NUM	27
#define FX_GOT_IMMED_MODE			28
#define FX_GOT_IMMED_REV_LF			29
#define FX_GOT_LEFT_MARGIN			30
#define FX_GOT_PROPORTIONAL_MODE	31
#define FX_GOT_UNIMP_ESC_1			32

// Print Styles

#define STYLE_EXPANDED_BIT								4
#define STYLE_SCRIPT_BIT								8
#define STYLE_ITALIC_BIT								16
#define STYLE_EMPHASIZED_BIT							32
#define STYLE_BASE_MASK									3

#define STYLE_PICA										0
#define STYLE_ELITE										1
#define STYLE_COMPRESSED								2
#define STYLE_PROPORTIONAL								3
#define STYLE_EXPANDED_PICA								(STYLE_PICA | STYLE_EXPANDED_BIT)
#define STYLE_EXPANDED_ELITE							(STYLE_ELITE | STYLE_EXPANDED_BIT)
#define STYLE_EXPANDED_COMPRESSED						(STYLE_COMPRESSED | STYLE_EXPANDED_BIT)
#define STYLE_EXPANDED_PROPORTIONAL						(STYLE_PROPORTIONAL | STYLE_EXPANDED_BIT)
#define STYLE_PICA_SCRIPT								(STYLE_PICA | STYLE_SCRIPT_BIT)
#define STYLE_ELITE_SCRIPT								(STYLE_ELITE | STYLE_SCRIPT_BIT)
#define STYLE_COMPRESSED_SCRIPT							(STYLE_COMPRESSED | STYLE_SCRIPT_BIT)
#define STYLE_EXPANDED_PICA_SCRIPT						(STYLE_EXPANDED_PICA | STYLE_SCRIPT_BIT)
#define STYLE_EXPANDED_ELITE_SCRIPT						(STYLE_EXPANDED_ELITE | STYLE_SCRIPT_BIT)
#define STYLE_EXPANDED_COMPRESSED_SCRIPT				(STYLE_EXPANDED_COMPRESSED | STYLE_SCRIPT_BIT)
#define STYLE_PICA_ITALIC								(STYLE_PICA | STYLE_ITALIC_BIT)
#define STYLE_ELITE_ITALIC								(STYLE_ELITE | STYLE_ITALIC_BIT)
#define STYLE_COMPRESSED_ITALIC							(STYLE_COMPRESSED | STYLE_ITALIC_BIT)
#define STYLE_PROPORTIONAL_ITALIC						(STYLE_PROPORTIONAL | STYLE_ITALIC_BIT)
#define STYLE_EXPANDED_PICA_ITALIC						(STYLE_EXPANDED_PICA | STYLE_ITALIC_BIT)
#define STYLE_EXPANDED_ELITE_ITALIC						(STYLE_EXPANDED_ELITE | STYLE_ITALIC_BIT)
#define STYLE_EXPANDED_COMPRESSED_ITALIC				(STYLE_EXPANDED_COMPRESSED | STYLE_ITALIC_BIT)
#define STYLE_EXPANDED_PROPORTIONAL_ITALIC				(STYLE_EXPANDED_PROPORTIONAL | STYLE_ITALIC_BIT)
#define STYLE_PICA_SCRIPT_ITALIC						(STYLE_PICA_SCRIPT | STYLE_SCRIPT_BIT)
#define STYLE_ELITE_SCRIPT_ITALIC						(STYLE_ELITE_SCRIPT | STYLE_SCRIPT_BIT)
#define STYLE_COMPRESSED_SCRIPT_ITALIC					(STYLE_COMPRESSED_SCRIPT | STYLE_SCRIPT_BIT)
#define STYLE_EXPANDED_PICA_SCRIPT_ITALIC				(STYLE_EXPANDED_PICA_SCRIPT | STYLE_SCRIPT_BIT)
#define STYLE_EXPANDED_ELITE_SCRIPT_ITALIC				(STYLE_EXPANDED_ELITE_SCRIPT | STYLE_SCRIPT_BIT)
#define STYLE_EXPANDED_COMPRESSED_SCRIPT_ITALIC			(STYLE_EXPANDED_COMPRESSED_SCRIPT | STYLE_SCRIPT_BIT)
#define STYLE_PICA_EMPHASIZED							(STYLE_PICA | STYLE_EMPHASIZED_BIT)
#define STYLE_EXPANDED_PICA_EMPHASIZED					(STYLE_EXPANDED_PICA | STYLE_EMPHASIZED_BIT)
#define STYLE_PICA_SCRIPT_EMPHASIZED					(STYLE_PICA_SCRIPT | STYLE_EMPHASIZED_BIT)
#define STYLE_EXPANDED_PICA_SCRIPT_EMPHASIZED			(STYLE_EXPANDED_PICA_SCRIPT | STYLE_EMPHASIZED_BIT)
#define STYLE_PICA_ITALIC_EMPHASIZED					(STYLE_PICA_ITALIC | STYLE_EMPHASIZED_BIT)
#define STYLE_EXPANDED_PICA_ITALIC_EMPHASIZED			(STYLE_EXPANDED_PICA_ITALIC | STYLE_EMPHASIZED_BIT)
#define STYLE_PICA_SCRIPT_ITALIC_EMPHASIZED				(STYLE_PICA_SCRIPT_ITALIC | STYLE_EMPHASIZED_BIT)
#define STYLE_EXPANDED_PICA_SCRIPT_ITALIC_EMPHASIZED	(STYLE_EXPANDED_PICA_SCRIPT_ITALIC | STYLE_EMPHASIZED_BIT)

#define PITCH_PICA										0
#define PITCH_ELITE										1
#define PITCH_COMPRESSED								2

#define MODE_US											0
#define MODE_FRANCE										1
#define MODE_GERMANY									2
#define MODE_UK											3
#define MODE_DENMARK									4
#define MODE_SWEDEN										5
#define MODE_ITALY										6
#define MODE_SPAIN										7
#define MODE_JAPAN										8

static float horizWidths[64] = {7.2,6.0,7.2,7.2,14.4,12.0,14.4,7.2,
								7.2,6.0,7.2,0.0,14.4,12.0,14.4,0.0,
								7.2,6.0,7.2,7.2,14.4,12.0,14.4,7.2,
								7.2,6.0,7.2,0.0,14.4,12.0,14.4,0.0,
								7.2,6.0,7.2,0.0,14.4,12.0,14.4,0.0,
								7.2,6.0,7.2,0.0,14.4,12.0,14.4,0.0,
								7.2,6.0,7.2,0.0,14.4,12.0,14.4,0.0,
								7.2,6.0,7.2,0.0,14.4,12.0,14.4,0.0};

static float graphWidth[9] = {1.2,0.6,0.6,0.3,0.9,1.0,0.8,1.2,0.6};	

static float propCharWidth[256] = {
	 12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 12*(7.2/12),5*(7.2/12),8*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),5*(7.2/12),
	 6*(7.2/12),6*(7.2/12),12*(7.2/12),12*(7.2/12),7*(7.2/12),12*(7.2/12),6*(7.2/12),10*(7.2/12),
	 12*(7.2/12),8*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 12*(7.2/12),12*(7.2/12),6*(7.2/12),6*(7.2/12),10*(7.2/12),12*(7.2/12),10*(7.2/12),12*(7.2/12),
	 12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 8*(7.2/12),11*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 10*(7.2/12),12*(7.2/12),10*(7.2/12),10*(7.2/12),10*(7.2/12),10*(7.2/12),12*(7.2/12),12*(7.2/12),
	 5*(7.2/12),12*(7.2/12),11*(7.2/12),11*(7.2/12),11*(7.2/12),12*(7.2/12),10*(7.2/12),11*(7.2/12),
	 12*(7.2/12),10*(7.2/12),9*(7.2/12),10*(7.2/12),10*(7.2/12),12*(7.2/12),11*(7.2/12),12*(7.2/12),
	 11*(7.2/12),11*(7.2/12),11*(7.2/12),12*(7.2/12),11*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 10*(7.2/12),12*(7.2/12),10*(7.2/12),9*(7.2/12),5*(7.2/12),9*(7.2/12),12*(7.2/12),12*(7.2/12),
	 12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 12*(7.2/12),5*(7.2/12),11*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),10*(7.2/12),
	 8*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 8*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),11*(7.2/12),
	 12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 12*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),6*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),
	 12*(7.2/12),11*(7.2/12),10*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12),10*(7.2/12),12*(7.2/12),
	 12*(7.2/12),11*(7.2/12),12*(7.2/12),12*(7.2/12),11*(7.2/12),12*(7.2/12),12*(7.2/12),12*(7.2/12)};


static unsigned short countryMap[9][128] = {
	// US
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 
	 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f},
	// France
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	 0xe0, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0xb0, 0xe7, 0xa7, 0x5e, 0x5f, 
	 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0xe9, 0xf9, 0xe8, 0xa8, 0x7f}, 
	// Germany
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	 0xa7, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0xc5, 0xd6, 0xdc, 0x5e, 0x5f, 
	 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0xe4, 0xf6, 0xfc, 0xdf, 0x7f}, 
	// UK
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	 0x20, 0x21, 0x22, 0xa3, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 
	 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f}, 
	// Denmark
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0xc6, 0xd8, 0xc5, 0x5e, 0x5f, 
	 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0xe6, 0xf8, 0xe5, 0x7e, 0x7f}, 
	// Sweeden
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	 0x20, 0x21, 0x22, 0x23, 0xa4, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	 0xc9, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0xc4, 0xd6, 0xc5, 0xdc, 0x5f, 
	 0xe9, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0xe4, 0xf6, 0xe5, 0xfc, 0x7f}, 
	// Italy
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0xb0, 0x5c, 0xe9, 0x5e, 0x5f, 
	 0xf9, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0xe0, 0xf2, 0xe8, 0xec, 0x7f}, 
	// Spain
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	 0x20, 0x21, 0x22, 0x20ac, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0xa1, 0xd1, 0xbf, 0x5e, 0x5f, 
	 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0xa8, 0xf1, 0x7d, 0x7e, 0x7f}, 
	// Japan
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0xa5, 0x5d, 0x5e, 0x5f, 
	 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f} 
};

static unsigned short controlCharMap[32] = 
	{0xe0,0xe8,0xf9,0xf2,0xec,0xb0,0xb3,0xa1,0xbf,0xd1,0xf1,0xa4,0x20ac,0xc5,0xe5,0xc7,
	 0xa7,0xdf,0xc6,0xe6,0xd8,0xf8,0xa8,0xa4,0xd6,0xdc,0xe4,0xf6,0xfc,0xc9,0xe9,0xa5};

EPSON_PREF prefsEpson;

// Other constants
#define FX_LEFT_PRINT_EDGE			10.8

@implementation EpsonFX80Simulator
static EpsonFX80Simulator *sharedInstance = nil;

+ (EpsonFX80Simulator *)sharedInstance {
    return sharedInstance ? sharedInstance : [[self alloc] init];
}

- (id)init {
	float aFontMatrix [6];
	
    if (sharedInstance) {
		[self dealloc];
    } else {
        [super init];
	}
	
    sharedInstance = self;
	
	aFontMatrix [1] = aFontMatrix [2] = aFontMatrix [4] = aFontMatrix [5] = 0.0;  // Always
	aFontMatrix [3] = 12.0;
	
	aFontMatrix [0] = 12.001;
	styles[STYLE_PICA] = [NSFont fontWithName : @"Courier" matrix : aFontMatrix];
	
	aFontMatrix [0] = 10.0;
	styles[STYLE_ELITE] = [NSFont fontWithName : @"Courier" matrix : aFontMatrix];

	aFontMatrix [0] = 120/17.16;
	styles[STYLE_COMPRESSED] = [NSFont fontWithName : @"Courier" matrix : aFontMatrix];
	
	styles[STYLE_PROPORTIONAL] = styles[STYLE_PICA];
	
	aFontMatrix [0] = 24.0;
	aFontMatrix [3] = 12.0;
	styles[STYLE_EXPANDED_PICA] = [NSFont fontWithName : @"Courier" matrix : aFontMatrix];
	
	aFontMatrix [0] = 20.0;
	styles[STYLE_EXPANDED_ELITE] = [NSFont fontWithName : @"Courier" matrix : aFontMatrix];
	
	aFontMatrix [0] = 240.0/17.16;
	styles[STYLE_EXPANDED_COMPRESSED] = [NSFont fontWithName : @"Courier" matrix : aFontMatrix];
	
	styles[STYLE_EXPANDED_PROPORTIONAL] = styles[STYLE_EXPANDED_PICA];
	
	aFontMatrix [3] = 6.0;
	
	aFontMatrix [0] = 12.0;
	styles[STYLE_PICA_SCRIPT] = [NSFont fontWithName : @"Courier" matrix : aFontMatrix];
	
	aFontMatrix [0] = 10.0;
	styles[STYLE_ELITE_SCRIPT] = [NSFont fontWithName : @"Courier" matrix : aFontMatrix];
	
	aFontMatrix [0] = 120/17.16;
	styles[STYLE_COMPRESSED_SCRIPT] = [NSFont fontWithName : @"Courier" matrix : aFontMatrix];
	
	aFontMatrix [0] = 24.0;
	styles[STYLE_EXPANDED_PICA_SCRIPT] = [NSFont fontWithName : @"Courier" matrix : aFontMatrix];
	
	aFontMatrix [0] = 20.0;
	styles[STYLE_EXPANDED_ELITE_SCRIPT] = [NSFont fontWithName : @"Courier" matrix : aFontMatrix];
	
	aFontMatrix [0] = 240/17.16;
	styles[STYLE_EXPANDED_COMPRESSED_SCRIPT] = [NSFont fontWithName : @"Courier" matrix : aFontMatrix];
	
	styles[STYLE_PICA_ITALIC] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_PICA] toHaveTrait : NSItalicFontMask];
	styles[STYLE_ELITE_ITALIC] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_ELITE] toHaveTrait : NSItalicFontMask];
	styles[STYLE_COMPRESSED_ITALIC] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_COMPRESSED] toHaveTrait : NSItalicFontMask];
	styles[STYLE_PROPORTIONAL_ITALIC] = styles[STYLE_PICA_ITALIC];
	styles[STYLE_EXPANDED_PICA_ITALIC] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_EXPANDED_PICA] toHaveTrait : NSItalicFontMask];
	styles[STYLE_EXPANDED_ELITE_ITALIC] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_EXPANDED_ELITE] toHaveTrait : NSItalicFontMask];
	styles[STYLE_EXPANDED_COMPRESSED_ITALIC] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_EXPANDED_COMPRESSED] toHaveTrait : NSItalicFontMask];
	styles[STYLE_EXPANDED_PROPORTIONAL_ITALIC] = styles[STYLE_EXPANDED_PICA_ITALIC];
	styles[STYLE_PICA_SCRIPT_ITALIC] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_PICA_SCRIPT] toHaveTrait : NSItalicFontMask];
	styles[STYLE_ELITE_SCRIPT_ITALIC] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_ELITE_SCRIPT] toHaveTrait : NSItalicFontMask];
	styles[STYLE_COMPRESSED_SCRIPT_ITALIC] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_COMPRESSED_SCRIPT] toHaveTrait : NSItalicFontMask];
	styles[STYLE_EXPANDED_PICA_SCRIPT_ITALIC] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_EXPANDED_PICA_SCRIPT] toHaveTrait : NSItalicFontMask];
	styles[STYLE_EXPANDED_ELITE_SCRIPT_ITALIC] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_EXPANDED_ELITE_SCRIPT] toHaveTrait : NSItalicFontMask];
	styles[STYLE_EXPANDED_COMPRESSED_SCRIPT_ITALIC] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_EXPANDED_COMPRESSED_SCRIPT] toHaveTrait : NSItalicFontMask];
	
	styles[STYLE_PICA_EMPHASIZED] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_PICA] toHaveTrait : NSBoldFontMask];
	styles[STYLE_EXPANDED_PICA_EMPHASIZED] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_EXPANDED_PICA] toHaveTrait : NSBoldFontMask];
	styles[STYLE_PICA_SCRIPT_EMPHASIZED] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_PICA_SCRIPT] toHaveTrait : NSBoldFontMask];
	styles[STYLE_EXPANDED_PICA_SCRIPT_EMPHASIZED] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_EXPANDED_PICA_SCRIPT] toHaveTrait : NSBoldFontMask];
	styles[STYLE_PICA_ITALIC_EMPHASIZED] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_PICA_ITALIC] toHaveTrait : NSBoldFontMask];
	styles[STYLE_EXPANDED_PICA_ITALIC_EMPHASIZED] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_EXPANDED_PICA_ITALIC] toHaveTrait : NSBoldFontMask];
	styles[STYLE_PICA_SCRIPT_ITALIC_EMPHASIZED] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_PICA_SCRIPT_ITALIC] toHaveTrait : NSBoldFontMask];
	styles[STYLE_EXPANDED_PICA_SCRIPT_ITALIC_EMPHASIZED] = [[NSFontManager sharedFontManager] convertFont : styles[STYLE_EXPANDED_PICA_SCRIPT_ITALIC] toHaveTrait : NSBoldFontMask];
			
	printBuffer = [[PrintableString alloc] init];
	[printBuffer retain];
	
	[self reset];

    return sharedInstance;
}

- (void)printChar:(char) character
{
	switch (state)
	{
		case FX_IDLE:
			[self idleState:character];
			break;
		case FX_GOT_ESC:
			[self escState:character];
			break;
		case FX_GOT_MASTER_MODE:
			[self masterModeState:character];
			break;
		case FX_GOT_GRAPH_MODE:
			[self graphModeState:character];
			break;
		case FX_GOT_GRAPH_MODE_TYPE:
			[self graphModeTypeState:character];
			break;
		case FX_GOT_NINE_PIN_GRAPH:
			[self ninePinGraphState:character];
			break;
		case FX_GOT_NINE_PIN_GRAPH_TYPE:
			[self ninePinGraphTypeState:character];
			break;
		case FX_GOT_GRAPH_CNT1:
			[self graphCnt1State:character];
			break;
		case FX_IN_GRAPH_MODE:
			[self inGraphModeState:character];
			break;
		case FX_GOT_UNDER_MODE:
			[self underModeState:character];
			break;
		case FX_GOT_VERT_TAB_CHAN:
			[self vertTabChanState:character];
			break;
		case FX_GOT_SUPER_LINE_SPACE:
			[self superLineSpaceState:character];
			break;
		case FX_GOT_GRAPH_REDEF:
			[self graphRedefState:character];
			break;
		case FX_GOT_GRAPH_REDEF_CODE:
			[self grapRedefCodeState:character];
			break;
		case FX_GOT_LINE_SPACE:
			[self lineSpaceState:character];
			break;
		case FX_GOT_VERT_TAB_SET:
			[self vertTabSetState:character];
			break;
		case FX_GOT_FORM_LENGTH_LINE:
			[self formLengthLineState:character];
			break;
		case FX_GOT_FORM_LENGTH_INCH:
			[self formLengthInchState:character];
			break;
		case FX_GOT_HORIZ_TAB_SET:
			[self horizTabSetState:character];
			break;
		case FX_GOT_PRINT_CTRL_CODES:
			[self printCtrlCodesState:character];
			break;
		case FX_GOT_IMMED_LF:
			[self immedLfState:character];
			break;
		case FX_GOT_SKIP_OVER_PERF:
			[self skipOverPerfState:character];
			break;
		case FX_GOT_RIGHT_MARGIN:
			[self rightMarginState:character];
			break;
		case FX_GOT_INTER_CHAR_SET:
			[self interCharSetState:character];
			break;
		case FX_GOT_SCRIPT_MODE:
			[self scriptMode:character];
			break;
		case FX_GOT_EXPAND_MODE:
			[self expandMode:character];
			break;
		case FX_GOT_VERT_TAB_CHAN_SET:
			[self vertTabChanSetState:character];
			break;
		case FX_GOT_VERT_TAB_CHAN_NUM:
			[self vertTabChanNumState:character];
			break;
		case FX_GOT_IMMED_MODE:
			[self immedModeState:character];
			break;
		case FX_GOT_IMMED_REV_LF:
			[self immedRevLfState:character];
			break;
		case FX_GOT_LEFT_MARGIN:
			[self leftMarginState:character];
			break;
		case FX_GOT_PROPORTIONAL_MODE:
			[self proportionalModeState:character];
			break;
		case FX_GOT_UNIMP_ESC_1:
			[self unimpEsc1State:character];
			break;
	}
}

- (void)addChar:(unsigned short)unicharacter:(bool)italic
{
	NSAttributedString *newString;
	NSFont *theFont;
	float currRightMargin;
	
	if (italic == YES)
		theFont = styles[style | STYLE_ITALIC_BIT];
	else
		theFont = styles[style];

	newString = [NSAttributedString alloc];
	if (superscriptMode)
		{
		if (underlineMode)
			{
			[newString initWithString:[NSString stringWithCharacters:&unicharacter 
										length:1]
				   attributes:[NSDictionary dictionaryWithObjectsAndKeys:
							   theFont, NSFontAttributeName,
							   [NSNumber numberWithInt:1],NSUnderlineStyleAttributeName,
							   [NSNumber numberWithFloat:3.5],NSBaselineOffsetAttributeName,
							   nil]];
			}
		else
			{
			[newString initWithString:[NSString stringWithCharacters:&unicharacter 
										length:1]
				   attributes:[NSDictionary dictionaryWithObjectsAndKeys:
							   theFont, NSFontAttributeName,
							   [NSNumber numberWithFloat:3.5],NSBaselineOffsetAttributeName,
							   nil]];
			}
		}
	else
		{
		if (underlineMode)
			{
			[newString initWithString:[NSString stringWithCharacters:&unicharacter 
										length:1]
				   attributes:[NSDictionary dictionaryWithObjectsAndKeys:
							   theFont, NSFontAttributeName,
							   [NSNumber numberWithInt:1],NSUnderlineStyleAttributeName,
							   nil]];
			}
		else
			{
			[newString initWithString:[NSString stringWithCharacters:&unicharacter 
										length:1]
				   attributes:[NSDictionary dictionaryWithObjectsAndKeys:
							   theFont, NSFontAttributeName,
							   nil]];
			}
		}
	[printBuffer appendAttributedString:newString];
	//	Increment horizontal position by character width;
	if ((style & STYLE_BASE_MASK) == STYLE_PROPORTIONAL)
		{
		float width;
		if (unicharacter >= 0x100)
			width = 7.2;
		else
			width = propCharWidth[unicharacter];
				
		nextHorizPosition += width;
		if (style & STYLE_EXPANDED_BIT)
			propCharSetback = (horizWidth - 2*width)/2;
		else
			propCharSetback = (horizWidth - width)/2;
		}
	else
		{
		nextHorizPosition += horizWidth;
		}

	[newString release];
	
	if (pitch == PITCH_COMPRESSED)
		currRightMargin = compressedRightMargin;
	else
		currRightMargin = rightMargin;
				
	if (nextHorizPosition >= currRightMargin) 
		{
			[self executeLineFeed:lineSpacing];            // move to next line
			startHorizPosition = leftMargin;		//  move to left margin;
			nextHorizPosition = leftMargin;		//  move to left margin;
		}
			
}

- (void)idleState:(char) character
{
	int length;
	int i;
	bool foundTab = NO;
	float currRightMargin;
	unsigned short unicharacter = (unsigned short) ((unsigned char) character);

	if (eighthBitZero == YES)
		unicharacter &= 0x7F;
	else if (eighthBitOne == YES)
		unicharacter |= 0x80;
	
	if ((unicharacter >= 0x20) &&
		 (unicharacter <= 0x7E))
		{
		if (countryMode != MODE_US)
			{
			unicharacter = countryMap[countryMode][unicharacter];
			}
	
		if ((unicharacter == 0x0030) && (slashedZeroMode == YES))
			unicharacter = 0x00d8;
		[self addChar:unicharacter:NO];
		if (immedMode == YES || ((style & STYLE_BASE_MASK) == STYLE_PROPORTIONAL))
			[self emptyPrintBuffer:doubleStrikeMode];
		}
	else if ((unicharacter >= 0xA0) &&
		 (unicharacter <= 0xFE))
		{
		if (countryMode != MODE_US)
			{
			unicharacter = countryMap[countryMode][unicharacter & 0x7F];
			}
		[self addChar:(unicharacter & 0x7F):YES];
		if (immedMode == YES || ((style & STYLE_BASE_MASK) == STYLE_PROPORTIONAL))
			[self emptyPrintBuffer:doubleStrikeMode];
		}
	else if ((italicInterMode == YES) &&
              (unicharacter >= 0x80) &&
		      (unicharacter <= 0x9F))
		{
		[self addChar:controlCharMap[(unicharacter & 0x7F)]:YES];
		if (immedMode == YES || ((style & STYLE_BASE_MASK) == STYLE_PROPORTIONAL))
			[self emptyPrintBuffer:doubleStrikeMode];
		}
	else if ((italicInterMode == YES) &&
              (unicharacter == 0xFF))
		{
		[self addChar:0x00d8:YES];
		if (immedMode == YES || ((style & STYLE_BASE_MASK) == STYLE_PROPORTIONAL))
			[self emptyPrintBuffer:doubleStrikeMode];
		}
	else 
		{
		switch(unicharacter)
		{
			case 0x07: // BEL
			case 0x87:
				// Ring the Bell
				break;
			case 0x08: // BS
			case 0x88:
				[self emptyPrintBuffer:doubleStrikeMode];
				startHorizPosition -= horizWidth;
				nextHorizPosition = startHorizPosition;
				break;
			case 0x09: // HT
			case 0x89:
				[self emptyPrintBuffer:doubleStrikeMode];
				if (pitch == PITCH_COMPRESSED)
					currRightMargin = compressedRightMargin;
				else
					currRightMargin = rightMargin;

				for (i=0;i<horizTabCount;i++)
					{
					if (startHorizPosition > horizTabs[i])
						continue;
					if (horizTabs[i] < rightMargin)
						{
						startHorizPosition = horizTabs[i];
						nextHorizPosition = startHorizPosition;
						}
					else
						{
						startHorizPosition = leftMargin;		//  move to left margin;
						nextHorizPosition = leftMargin;		//  move to left margin;
						[self executeLineFeed:lineSpacing];            // move to next line
						}
					foundTab = YES;
					break;
					}
				if (foundTab == NO)
					{
					startHorizPosition = leftMargin;		//  move to left margin;
					nextHorizPosition = leftMargin;		//  move to left margin;
					[self executeLineFeed:lineSpacing];            // move to next line
					}
				break;
			case 0x0A: // LF
			case 0x8A:
				[self executeLineFeed:lineSpacing];            // move to next line
				break;
			case 0x0B: // VT
			case 0x8B:
				[self emptyPrintBuffer:doubleStrikeMode];
				for (i=0;i<vertTabCount[vertTabChan];i++)
					{
					if (vertPosition >= vertTabs[vertTabChan][i])
						continue;
					[self executeLineFeed:(vertTabs[vertTabChan][i] - vertPosition)];
					expandedTempMode = NO;
					foundTab = YES;
					break;
					}
				// If we don't find a tab, it's the same as a form feed.
				if (foundTab == NO)
					{
					[self executeFormFeed];
					}
				break;
			case 0x0C: // FF
			case 0x8C:
				[self emptyPrintBuffer:doubleStrikeMode];
				[self executeFormFeed];
				break;
			case 0x0D: // CR
			case 0x8D:
				[self emptyPrintBuffer:doubleStrikeMode];
				if (autoLineFeed == YES)
					{
					[self executeLineFeed:lineSpacing];            // move to next line
					}
				startHorizPosition = leftMargin;		//  move to left margin;
				nextHorizPosition = leftMargin;		//  move to left margin;
				break;
			case 0x0E: // SO
			case 0x8E:
				[self emptyPrintBuffer:doubleStrikeMode];
				expandedTempMode = YES;
				[self setStyle];
				break;
			case 0x0F: // SI
			case 0x8F:
				[self emptyPrintBuffer:doubleStrikeMode];
				compressedMode = YES;
				[self setStyle];
				break;
			case 0x12: // DC2
			case 0x92:
				[self emptyPrintBuffer:doubleStrikeMode];
				compressedMode = NO;
				[self setStyle];
				break;
			case 0x14: // DC4
			case 0x94:
				[self emptyPrintBuffer:doubleStrikeMode];
				expandedTempMode = NO;
				[self setStyle];
				break;
			case 0x18: // CAN
			case 0x98:
				// Cancel the print buffer
				[self clearPrintBuffer];
				break;
			case 0x1B: // ESC
			case 0x9B:
				state = FX_GOT_ESC;
				break;
			case 0x7F: // DEL:
			case 0xFF:
				// Delete the last character in the text buffer
				length = [[printBuffer string] length];
				if (length != 0)
					{
					NSRange range=NSMakeRange(length-1,1);
					[printBuffer deleteCharactersInRange:range];
					}
				state = FX_IDLE;
				break;
			default:
				if ((printCtrlCodesMode == YES) &&
					(unicharacter <= 0x1F))
					{
					[self addChar:controlCharMap[unicharacter]:NO];
					if (immedMode == YES)
						[self emptyPrintBuffer:doubleStrikeMode];
					}
				break;
			}
		}
		
}
- (void)escState:(char) character
{
	switch(character)
		{
		case 0x21:
			state = FX_GOT_MASTER_MODE;
			break;
		case 0x23:
			// Accepts 8th bit as is.
			eighthBitZero = NO;
			eighthBitOne = NO;
			state = FX_IDLE;
			break;
		case 0x2A:
			state = FX_GOT_GRAPH_MODE;
			[self emptyPrintBuffer:doubleStrikeMode];
			break;
		case 0x2D:
			state = FX_GOT_UNDER_MODE;
			break;
		case 0x2F:
			state = FX_GOT_VERT_TAB_CHAN;
			break;
		case 0x30:
			// Set line spacing to 1/8 inch.
			lineSpacing = 9.0;
			state = FX_IDLE;
			break;
		case 0x31:
			// Set line spacing to 7/72 inch.
			lineSpacing = 7.0;
			state = FX_IDLE;
			break;
		case 0x32:
			// Set line spacing to 1/6 inch.
			lineSpacing = 12.0;
			state = FX_IDLE;
			break;
		case 0x33:
			state = FX_GOT_SUPER_LINE_SPACE;
			break;
		case 0x34:
			// Turn italic printing on
			italicMode = YES;
			[self setStyle];
			state = FX_IDLE;
			break;
		case 0x35:
			// Turn italic printing off
			italicMode = NO;
			[self setStyle];
			state = FX_IDLE;
			break;
		case 0x36:
			italicInterMode = YES;
			state = FX_IDLE;
			break;
		case 0x37:
			italicInterMode = NO;
			state = FX_IDLE;
			break;
		case 0x3D:
			// Sets eighth bit to zero.
			eighthBitZero = YES;
			eighthBitOne = NO;
			state = FX_IDLE;
			break;
		case 0x3E:
			// Sets eighth bit to one.
			eighthBitOne = YES;
			eighthBitZero = NO;
			state = FX_IDLE;
			break;
		case 0x3F:
			state = FX_GOT_GRAPH_REDEF;
			break;
		case 0x40:
			[self clearPrintBuffer];
			[self reset];
			state = FX_IDLE;
			break;
		case 0x41:
			state = FX_GOT_LINE_SPACE;
			break;
		case 0x42:
			state = FX_GOT_VERT_TAB_SET;
			vertTabCount[0] = 0;
			break;
		case 0x43:
			state = FX_GOT_FORM_LENGTH_LINE;
			break;
		case 0x44:
			state = FX_GOT_HORIZ_TAB_SET;
			horizTabCount = 0;
			break;
		case 0x45:
			emphasizedMode = YES;
			[self setStyle];
			state = FX_IDLE;
			break;
		case 0x46:
			emphasizedMode = NO;
			[self setStyle];
			state = FX_IDLE;
			break;
		case 0x47:
			[self emptyPrintBuffer:doubleStrikeMode];
			doubleStrikeWantedMode = YES;
			[self setStyle];
			state = FX_IDLE;
			break;
		case 0x48:
			[self emptyPrintBuffer:doubleStrikeMode];
			doubleStrikeWantedMode = NO;
			[self setStyle];
			state = FX_IDLE;
			break;
		case 0x49:
			state = FX_GOT_PRINT_CTRL_CODES;
			break;
		case 0x4A:
			state = FX_GOT_IMMED_LF;
			break;
		case 0x4B:
			state = FX_GOT_GRAPH_MODE_TYPE;
			graphMode = kGraphMode;
			[self emptyPrintBuffer:doubleStrikeMode];
			break;
		case 0x4C:
			state = FX_GOT_GRAPH_MODE_TYPE;
			graphMode = lGraphMode;
			[self emptyPrintBuffer:doubleStrikeMode];
			break;
		case 0x4D:
			[self emptyPrintBuffer:doubleStrikeMode];
			eliteMode = YES;
			[self setStyle];
			state = FX_IDLE;
			break;
		case 0x4E:
			state = FX_GOT_SKIP_OVER_PERF;
			break;
		case 0x4F:
			skipOverPerf = 0;
			state = FX_IDLE;
			break;
		case 0x50:
			[self emptyPrintBuffer:doubleStrikeMode];
			eliteMode = NO;
			[self setStyle];
			state = FX_IDLE;
			break;
		case 0x51:
			state = FX_GOT_RIGHT_MARGIN;
			break;
		case 0x52:
			state = FX_GOT_INTER_CHAR_SET;
			break;
		case 0x53:
			state = FX_GOT_SCRIPT_MODE;
			break;
		case 0x54:
			superscriptMode = NO;
			subscriptMode = NO;
			[self setStyle];  /* Fix bug where script wasn't turning off */
			state = FX_IDLE;
			break;
		case 0x55:
			state = FX_GOT_UNIMP_ESC_1;
			break;
		case 0x57:
			state = FX_GOT_EXPAND_MODE;
			break;
		case 0x59:
			state = FX_GOT_GRAPH_MODE_TYPE;
			[self emptyPrintBuffer:doubleStrikeMode];
			graphMode = yGraphMode;
			break;
		case 0x5A:
			state = FX_GOT_GRAPH_MODE_TYPE;
			[self emptyPrintBuffer:doubleStrikeMode];
			graphMode = zGraphMode;
			break;
		case 0x5E:
			state = FX_GOT_NINE_PIN_GRAPH;
			[self emptyPrintBuffer:doubleStrikeMode];
			break;
		case 0x62:
			state = FX_GOT_VERT_TAB_CHAN_SET;
			break;
		case 0x69:
			state = FX_GOT_IMMED_MODE;
			break;
		case 0x6A:
			state = FX_GOT_IMMED_REV_LF;
			break;
		case 0x6C:
			state = FX_GOT_LEFT_MARGIN;
			break;
		case 0x70:
			state = FX_GOT_PROPORTIONAL_MODE;
			break;
		case 0x73:
			state = FX_GOT_UNIMP_ESC_1;
			break;
		default:
			state = FX_IDLE;
		}
}

- (void)masterModeState:(char) character
{
	unsigned char n = (unsigned char) character;

	switch(n)
		{
		case 0:
			eliteMode = NO;
			expandedMode = NO;
			compressedMode = NO;
			proportionalMode = NO;
			doubleStrikeWantedMode = NO;
			emphasizedMode = NO;
			break;
		case 1:
			eliteMode = YES;
			expandedMode = NO;
			compressedMode = NO;
			proportionalMode = NO;
			doubleStrikeWantedMode = NO;
			emphasizedMode = NO;
			break;
		case 4:
			eliteMode = NO;
			expandedMode = NO;
			compressedMode = YES;
			proportionalMode = NO;
			doubleStrikeWantedMode = NO;
			emphasizedMode = NO;
			break;
		case 8:
			eliteMode = NO;
			expandedMode = NO;
			compressedMode = NO;
			proportionalMode = NO;
			doubleStrikeWantedMode = NO;
			emphasizedMode = YES;
			break;
		case 16:
			eliteMode = NO;
			expandedMode = NO;
			compressedMode = NO;
			proportionalMode = NO;
			doubleStrikeWantedMode = YES;
			emphasizedMode = NO;
			break;
		case 17:
			eliteMode = YES;
			expandedMode = NO;
			compressedMode = NO;
			proportionalMode = NO;
			doubleStrikeWantedMode = YES;
			emphasizedMode = NO;
			break;
		case 20:
			eliteMode = NO;
			expandedMode = NO;
			compressedMode = YES;
			proportionalMode = NO;
			doubleStrikeWantedMode = YES;
			emphasizedMode = NO;
			break;
		case 24:
			eliteMode = NO;
			expandedMode = NO;
			compressedMode = NO;
			proportionalMode = NO;
			doubleStrikeWantedMode = YES;
			emphasizedMode = YES;
			break;
		case 32:
			eliteMode = NO;
			expandedMode = YES;
			compressedMode = NO;
			proportionalMode = NO;
			doubleStrikeWantedMode = NO;
			emphasizedMode = NO;
			break;
		case 33:
			eliteMode = YES;
			expandedMode = YES;
			compressedMode = NO;
			proportionalMode = NO;
			doubleStrikeWantedMode = NO;
			emphasizedMode = NO;
			break;
		case 36:
			eliteMode = NO;
			expandedMode = YES;
			compressedMode = YES;
			proportionalMode = NO;
			doubleStrikeWantedMode = NO;
			emphasizedMode = NO;
			break;
		case 40:
			eliteMode = NO;
			expandedMode = YES;
			compressedMode = NO;
			proportionalMode = NO;
			doubleStrikeWantedMode = NO;
			emphasizedMode = YES;
			break;
		case 48:
			eliteMode = NO;
			expandedMode = YES;
			compressedMode = NO;
			proportionalMode = NO;
			doubleStrikeWantedMode = YES;
			emphasizedMode = NO;
			break;
		case 49:
			eliteMode = YES;
			expandedMode = YES;
			compressedMode = NO;
			proportionalMode = NO;
			doubleStrikeWantedMode = YES;
			emphasizedMode = NO;
			break;
		case 52:
			eliteMode = NO;
			expandedMode = YES;
			compressedMode = YES;
			proportionalMode = NO;
			doubleStrikeWantedMode = YES;
			emphasizedMode = NO;
			break;
		case 56:
			eliteMode = NO;
			expandedMode = YES;
			compressedMode = NO;
			proportionalMode = NO;
			doubleStrikeWantedMode = YES;
			emphasizedMode = YES;
			break;
		}
	[self setStyle];
	state = FX_IDLE;
}

- (void)graphModeState:(char) character
{
	unsigned char n = (unsigned char) character;

	if (n <= 6)
		graphMode = n;
	state = FX_GOT_GRAPH_MODE_TYPE;
}

- (void)graphModeTypeState:(char) character
{
	unsigned char n = (unsigned char) character;

	graphTotal = n;
	state = FX_GOT_GRAPH_CNT1;
}

- (void)ninePinGraphState:(char) character
{
	state = FX_GOT_NINE_PIN_GRAPH_TYPE;
}

- (void)ninePinGraphTypeState:(char) character
{
	unsigned char n = (unsigned char) character;
	
	if (n == 0)
		graphMode = 7;
	else
		graphMode = 8;
	
	state = FX_GOT_GRAPH_MODE_TYPE;
}

- (void)graphCnt1State:(char) character
{
	unsigned char n = (unsigned char) character;
	
	graphTotal = graphTotal + (n*256);
	if (graphMode > 6)
		graphTotal *= 2;
	graphCount = 0;
	state = FX_IN_GRAPH_MODE;
}

- (void)inGraphModeState:(char) character
{
	unsigned char n = (unsigned char) character;
	PrintableGraphics *graph;
	unsigned bits, length;
	
	graphBuffer[graphCount++] = n;

	if (graphCount == graphTotal)
		{
		NSPoint location = NSMakePoint(startHorizPosition, vertPosition);
		// Add the graphics to the list for printing.
		if (graphMode <= 6)
			{
			length = graphCount;
			bits = 8;
			}
		else
			{
			length = graphCount/2;
			bits = 9;
			}
		graph = [[PrintableGraphics alloc] initWithBytes:((const void *) graphBuffer)
		         length:length width:graphWidth[graphMode] height:1.0 bits:bits];
		[graph setLocation:location];
		[[PrintOutputController sharedInstance] addToPrintArray:graph];
		[graph release];

		startHorizPosition += length * graphWidth[graphMode];
		state = FX_IDLE;
		}
}

- (void)underModeState:(char) character
{
	if (character == 0 || character == '0')
		underlineMode = NO;
	else if (character == 1 || character == '1')
		underlineMode = YES;
	state = FX_IDLE;
}

- (void)vertTabChanState:(char) character
{
	if (character >= 0 && character <= 7)
		vertTabChan = character;
	state = FX_IDLE;
}

- (void)superLineSpaceState:(char) character
{
	unsigned char n = (unsigned char) character;
	
	lineSpacing = ((float) n)/3.0;
	state = FX_IDLE;
}

- (void)graphRedefState:(char) character
{
	graphRedefCode = character;
	state = FX_GOT_GRAPH_REDEF_CODE;
}

- (void)grapRedefCodeState:(char) character
{
	unsigned char n = (unsigned char) character;
	
	if (n <= 6)
		{
		if (graphRedefCode == 0x4B)
			kGraphMode = n;
		else if (graphRedefCode == 0x4C)
			lGraphMode = n;
		else if (graphRedefCode == 0x59)
			yGraphMode = n;
		else if (graphRedefCode == 0x5A)
			zGraphMode = n;
		}
	state = FX_IDLE;
}

- (void)lineSpaceState:(char) character
{
	unsigned char n = (unsigned char) character;
	if (n<85)
		lineSpacing = ((float) n);
	state = FX_IDLE;
}

- (void)vertTabSetState:(char) character
{
	unsigned char n = (unsigned char) character;
	
	if (character == 0 || 
		vertTabCount[0] == 16 ||
		(n*lineSpacing > formLength))
		state = FX_IDLE;
	else
		{
		if (vertTabCount[0] == 0)
			{
			vertTabs[0][0] = n*lineSpacing;
			vertTabCount[0]++;
			}
		else
			{
			if (n*lineSpacing < vertTabs[0][vertTabCount[0]])
				{
				state = FX_IDLE;
				}
			else
				{
				vertTabs[0][vertTabCount[0]] = n*lineSpacing;
				vertTabCount[0]++;
				}
			}
		}
}

- (void)formLengthLineState:(char) character
{
	unsigned char n = (unsigned char) character;
	
	if (n>=1 && n<=127)
		formLength = n*lineSpacing;
	state = FX_IDLE;
}

- (void)formLengthInchState:(char) character
{
	unsigned char n = (unsigned char) character;
	
	if (n>=1 && n<=22)
		formLength = n*72.0;
	state = FX_IDLE;
}

- (void)horizTabSetState:(char) character
{
	unsigned char n = (unsigned char) character;
	float charWidth;
	
	if (pitch == PITCH_PICA)
		charWidth =  7.2;
	else if (pitch == PITCH_ELITE)
		charWidth = 6.0;
	else
		charWidth = (576.0/132.0);
	
	if (character == 0 || 
		horizTabCount == 32)
		state = FX_IDLE;
	else
		{
		if (horizTabCount == 0)
			{
			horizTabs[0] = n*charWidth + leftMargin;
			horizTabCount++;
			}
		else
			{
			if (n < horizTabs[horizTabCount])
				{
				state = FX_IDLE;
				}
			else
				{
				horizTabs[horizTabCount] = n*charWidth + leftMargin;
				horizTabCount++;
				}
			}
		}
}

- (void)printCtrlCodesState:(char) character
{
	if (character == 0 || character == '0')
		printCtrlCodesMode = NO;
	else if (character == 1 || character == '1')
		printCtrlCodesMode = YES;
	state = FX_IDLE;
}

- (void)immedLfState:(char) character
{
	unsigned char n = (unsigned char) character;

	[self executeLineFeed:((float) n)/3.0];            
	state = FX_IDLE;
}

- (void)skipOverPerfState:(char) character
{
	unsigned char n = (unsigned char) character;
	if (n>=1 && n<=127)
		skipOverPerf = n;
	state = FX_IDLE;
}

- (void)rightMarginState:(char) character
{
	unsigned char n = (unsigned char) character;
	
	if (pitch == PITCH_PICA)
		{
		if (n >= 2 && n <= 80)
			{
			rightMargin = 7.2 * n + FX_LEFT_PRINT_EDGE;
			compressedRightMargin = rightMargin;
			}
		}
	else if (pitch == PITCH_ELITE)
		{
		if (n >= 3 && n <= 96)
			{
			rightMargin = 6.0 * n + FX_LEFT_PRINT_EDGE;
			compressedRightMargin = rightMargin;
			}
		}
	else if (pitch == PITCH_COMPRESSED)
		{
		if (n >= 4 && n <= 137)
			{
			rightMargin = (72/17.16) * n + FX_LEFT_PRINT_EDGE;
			compressedRightMargin = rightMargin;
			}
		}
	else
		{
		state = FX_IDLE;
		return;
		}
		
	state = FX_IDLE;
}

- (void)interCharSetState:(char) character
{
	unsigned char n = (unsigned char) character;
	
	if (n <= MODE_JAPAN)
		countryMode = n;
	state = FX_IDLE;
}

- (void)scriptMode:(char) character
{
	if (character == 0 || character == '0')
		{
		superscriptMode = YES;
		subscriptMode = NO;
		[self setStyle];
		}
	else if (character == 1 || character == '1')
		{
		subscriptMode = YES;
		superscriptMode = NO;
		[self setStyle];
		}
	state = FX_IDLE;
}

- (void)expandMode:(char) character
{
	if (character == 0 || character == '0')
		{
		expandedMode = NO;
		expandedTempMode = NO;
		[self setStyle];
		}
	else if (character == 1 || character == '1')
		{
		expandedMode = YES;
		[self setStyle];
		}
	state = FX_IDLE;
}

- (void)vertTabChanSetState:(char) character
{
	unsigned char n = (unsigned char) character;
	if (n<8)
		{
		vertTabChanSetNum = n;
		state = FX_GOT_VERT_TAB_CHAN_NUM;
		}
	else
		{
		state = FX_IDLE;
		}
}

- (void)vertTabChanNumState:(char) character
{
	unsigned char n = (unsigned char) character;
	
	if (character == 0 || 
		vertTabCount[vertTabChanSetNum] == 16 ||
		(n*lineSpacing > formLength))
		state = FX_IDLE;
	else
		{
		if (vertTabCount[vertTabChanSetNum] == 0)
			{
			vertTabs[vertTabChanSetNum][0] = n*lineSpacing;
			vertTabCount[vertTabChanSetNum]++;
			}
		else
			{
			if (n*lineSpacing < vertTabs[vertTabChanSetNum][vertTabCount[vertTabChanSetNum]])
				{
				state = FX_IDLE;
				}
			else
				{
				vertTabs[vertTabChanSetNum][vertTabCount[vertTabChanSetNum]] = n*lineSpacing;
				vertTabCount[vertTabChanSetNum]++;
				}
			}
		}
}

- (void)immedModeState:(char) character
{
	if (character == 0 || character == '0')
		immedMode = NO;
	else if (character == 1 || character == '1')
		immedMode = YES;
}

- (void)immedRevLfState:(char) character
{
	unsigned char n = (unsigned char) character;

	[self executeLineFeed:((float) n)/-3.0];            
	state = FX_IDLE;
}

- (void)leftMarginState:(char) character
{
	unsigned char n = (unsigned char) character;
	
	[self clearPrintBuffer];

	if (pitch == PITCH_PICA)
		{
		if (n < 79)
			leftMargin = 7.2 * n + FX_LEFT_PRINT_EDGE;
		}
	else if (pitch == PITCH_ELITE)
		{
		if (n < 93)
			leftMargin = 6.0 * n + FX_LEFT_PRINT_EDGE;
		}
	else if (pitch == PITCH_COMPRESSED)
		{
		if (n < 133)
			leftMargin = (576.0/132.0) * n + FX_LEFT_PRINT_EDGE;
		}
	else
		{
		state = FX_IDLE;
		return;
		}
		
	[self resetHorizTabs];

	state = FX_IDLE;
}

- (void)proportionalModeState:(char) character
{
	if (character == 0 || character == '0')
		{
		proportionalMode = NO;
		[self setStyle];
		}
	else if (character == 1 || character == '1')
		{
		proportionalMode = YES;
		[self setStyle];
		}
	state = FX_IDLE;
}

- (void)unimpEsc1State:(char) character
{
	state = FX_IDLE;
}

-(void)reset
{
	int i,j;

	state = FX_IDLE;
	rightMargin = 576.0 + FX_LEFT_PRINT_EDGE;
	compressedRightMargin = (132.0*(72.0/17.16)) + FX_LEFT_PRINT_EDGE;
	leftMargin = 0.0 + FX_LEFT_PRINT_EDGE;
	startHorizPosition = leftMargin;
	nextHorizPosition = leftMargin;
	
	immedMode = NO;
	if (prefsEpson.autoLinefeed)
		autoLineFeed = YES;
	else
		autoLineFeed = NO;
	if (prefsEpson.printSlashedZeros) 
		slashedZeroMode = YES;
	else 
		slashedZeroMode = NO;
	lineSpacing = 12.0;
	formLength = 72.0*prefsEpson.formLength;
	italicMode = NO;
	italicInterMode = NO;
	printCtrlCodesMode = NO;
	eighthBitZero = NO;
	eighthBitOne = NO;
	if (prefsEpson.printWeight)
		emphasizedMode = YES;
	else
		emphasizedMode = NO;
	doubleStrikeMode = NO;
	doubleStrikeWantedMode = NO;
	eliteMode = NO;
	superscriptMode = NO;
	subscriptMode = NO;
	expandedMode = NO;
	expandedTempMode = NO;
	if (prefsEpson.printPitch)
		compressedMode = YES;
	else
		compressedMode = NO;
	underlineMode = NO;
	proportionalMode = NO;
	graphMode = 0;
	kGraphMode =  0;
	lGraphMode =  1;
	yGraphMode =  2;
	zGraphMode =  3;
	vertTabChan = 0;
	for (i=0;i<8;i++)
		{
		vertTabCount[i] = 16;
		for (j=0;j<16;j++)
			{
			vertTabs[i][j] = (j*2+1) * lineSpacing;
			}
		}
	countryMode = prefsEpson.charSet;
	if (prefsEpson.autoSkip)
		skipOverPerf = 6;
	else
		skipOverPerf = 0;
	if (prefsEpson.splitSkip)
		splitPerfSkip = YES;
	else
		splitPerfSkip = NO;
	if (splitPerfSkip == NO)
		vertPosition = 0.0;
	else
		vertPosition = (skipOverPerf / 2) * lineSpacing;
		
	[self resetHorizTabs];
	[self setStyle];
	
}

-(void)resetHorizTabs
{
	float nextTab = leftMargin;
	float charWidth;
	horizTabCount = 0;
    float currRightMargin;
	
	if (pitch == PITCH_PICA)
		charWidth =  7.2;
	else if (pitch == PITCH_ELITE)
		charWidth = 6.0;
	else
		charWidth = (72/17.16);
	
	if (pitch == PITCH_COMPRESSED)
		currRightMargin = compressedRightMargin;
	else
		currRightMargin = rightMargin;
				
	do
		{
		nextTab += 8.0 * charWidth;
		if (nextTab < currRightMargin)
			horizTabs[horizTabCount++] = nextTab;
		} while (nextTab < currRightMargin);
}

-(void)setStyle
{
	bool totalExpandedMode = expandedMode | expandedTempMode;
	
	if (eliteMode == YES)
		{
		style = STYLE_ELITE;
		pitch = PITCH_ELITE;
		}
	else if (proportionalMode == YES)
		{
		style = STYLE_PROPORTIONAL;
		pitch = PITCH_PICA;
		}
	else if (emphasizedMode == YES)
		{
		style = STYLE_PICA | STYLE_EMPHASIZED_BIT;
		pitch = PITCH_PICA;
		}
	else if (compressedMode == YES)
		{
		style = STYLE_COMPRESSED;
		pitch = PITCH_COMPRESSED;
		}
	else
		{
		style = STYLE_PICA;
		pitch = PITCH_PICA;
		}

	if (totalExpandedMode)
		style |= STYLE_EXPANDED_BIT;
		
	if (italicMode)
		style |= STYLE_ITALIC_BIT;
	
	if (!proportionalMode)
		{
		doubleStrikeMode = doubleStrikeWantedMode;
		if (superscriptMode || subscriptMode)
			style |= STYLE_SCRIPT_BIT;
		}
	else
		doubleStrikeMode = FALSE;
	
	horizWidth = horizWidths[style];
}

-(void)emptyPrintBuffer:(bool)doubleStrike
{
	NSPoint location;
	if ([[printBuffer string] length] == 0)
		return;
	if ((style & STYLE_BASE_MASK) == STYLE_PROPORTIONAL)
		location = NSMakePoint(startHorizPosition - propCharSetback, vertPosition-5.0);
	else
		location = NSMakePoint(startHorizPosition, vertPosition-5.0);
		
	[printBuffer setLocation:location];
	[[PrintOutputController sharedInstance] addToPrintArray:printBuffer];
	if (doubleStrike == YES)
		[[PrintOutputController sharedInstance] addToPrintArray:printBuffer];
	[printBuffer release];
	printBuffer = [[PrintableString alloc] init];
	
	startHorizPosition = nextHorizPosition;
}

-(void)clearPrintBuffer
{
	[printBuffer release];
	printBuffer = [[PrintableString alloc] init];
	
	nextHorizPosition = startHorizPosition;
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

-(void)executePenChange
{
}

-(NSColor *)getPenColor
{
	return(nil);
}

-(void)executeLineFeed:(float) amount
{
	float posOnPage;
	int linesToSkip;
	
	[self emptyPrintBuffer:doubleStrikeMode];
	vertPosition += amount;
	if (vertPosition < 0.0)
		vertPosition = 0.0;
	expandedTempMode = NO;
	[self setStyle];
	// Skip over the paper perferation.
	if (skipOverPerf)
		{
		posOnPage = vertPosition -  (((int) (vertPosition/formLength)) * formLength);
		if (splitPerfSkip == NO)
			linesToSkip = skipOverPerf;
		else
			linesToSkip = skipOverPerf / 2;
			
		if (posOnPage >= (formLength - (linesToSkip * lineSpacing)))
			[self executeFormFeed];
		}
}

-(void)executeFormFeed
{
	vertPosition = (((int) (vertPosition/formLength)) + 1) * formLength;
	if (splitPerfSkip)
		vertPosition += (skipOverPerf / 2) * lineSpacing;
}

-(void)topBlankForm
{
	if (splitPerfSkip == NO)
		vertPosition = 0.0;
	else
		vertPosition = (skipOverPerf / 2) * lineSpacing;
	startHorizPosition = leftMargin;
	nextHorizPosition = leftMargin;
}

-(bool)isAutoPageAdjustEnabled
{
	return(NO);
}

@end

