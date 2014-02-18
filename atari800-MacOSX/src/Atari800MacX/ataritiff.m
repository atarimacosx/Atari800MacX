/*
 * ataritiff.c - Atari 336x240 tiff screenshot code
 *
 * Copyright (c) 2004 Mark Grebe
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


#import <Cocoa/Cocoa.h>
#import <stdio.h>
#import <stdlib.h>

#import "antic.h"
#import "atari.h"
#import "screen.h"
#import "config.h"
#import "colours.h"

#define bytesPerLine 384
#define numberLines 240

extern unsigned int Palette32[256];          // 32-bit palette
extern unsigned short Palette16[256];        // 16-bit palette
extern char atari_image_dir[FILENAME_MAX];

char *Find_TIFF_name(void)
{
	int tiff_no = -1;
	static char filename[FILENAME_MAX];
	FILE *fp;

	while (++tiff_no < 1000) {
		sprintf(filename, "%s/atari%03i.tiff", atari_image_dir,tiff_no);
		if ((fp = fopen(filename, "r")) == NULL)
			return filename; /*file does not exist - we can create it */
		fclose(fp);
	}
	return NULL;
}

UBYTE Save_TIFF_file(char *filename)
{
    register unsigned char *fromPtr;
    register unsigned int *toPtr;
    register int i,j;
    register unsigned int *start32;
	register unsigned char *screen;
	ULONG *temp_screen = Screen_atari;
	unsigned char *rgbScreen;
	FILE *fp;
	NSBitmapImageRep *bitmapRep;
	NSData *tiffRep;

	if (filename == NULL || (fp = fopen(filename,"wb")) == NULL)
		return FALSE;

	// Convert the Atari screen into 32bit RGB
	rgbScreen = (unsigned char *) malloc(bytesPerLine * numberLines * 4);
	if (rgbScreen == NULL)
		return FALSE;
    start32 = (unsigned int *) rgbScreen;
    screen = (unsigned char *) temp_screen;
    i = numberLines;
    while (i > 0) {
        j = bytesPerLine;
        toPtr = (unsigned int *) start32;
        fromPtr = screen;
        while (j > 0) {
#ifdef WORDS_BIGENDIAN
            *toPtr++ = Palette32[*fromPtr++]<< 8;
#else
			unsigned int value;
			value = Palette32[*fromPtr++]<< 8;
            *toPtr++ = (value << 24) | 
					   ((value << 8) & 0x00ff0000) |
					   ((value >> 8) & 0x0000ff00) |
					   (value >> 24);
#endif			
            j--;
            }
        screen += bytesPerLine;
        start32 += bytesPerLine;
        i--;
        }

	//  Create a bitmap representation of the screen
    bitmapRep = [NSBitmapImageRep alloc];
	[bitmapRep initWithBitmapDataPlanes:&rgbScreen 
	              pixelsWide:bytesPerLine pixelsHigh:numberLines bitsPerSample:8 
				  samplesPerPixel:3 hasAlpha:NO 
				  isPlanar:NO colorSpaceName:NSCalibratedRGBColorSpace 
				  bytesPerRow:(bytesPerLine*4) bitsPerPixel:32];

	//  Convert it to TIFF
	tiffRep = [bitmapRep TIFFRepresentationUsingCompression:NSTIFFCompressionNone factor:0.0];

	//  And write it to the file 
	fwrite([tiffRep bytes],[tiffRep length],1,fp);

	fclose(fp);
	free(rgbScreen);
	[tiffRep release];
	[bitmapRep release];
	
	return TRUE;
}
