
/* Simple program to take a BMP image and turn it into a C header which can
   be included in your program as an image data array.  You can then call
   GUI_LoadImage() on the data in the header to create a usable SDL surface.
*/

#include <stdio.h>
#include "SDL.h"

main(int argc, char *argv[])
{
	SDL_Surface *bmp;
	int status, i, j;
	Uint8 *spot;

	if ( argv[1] == NULL ) {
		fprintf(stderr, "Usage: %s <image.bmp>\n", argv[0]);
		exit(1);
	}

	if ( SDL_Init(0) < 0 ) {
		fprintf(stderr, "Couldn't init SDL: %s\n", SDL_GetError());
		exit(-1);
	}
	atexit(SDL_Quit);

	status = 0;
	bmp = SDL_LoadBMP(argv[1]);
	if ( bmp ) {
		if ( bmp->format->BitsPerPixel == 8 ) {
			SDL_Palette *palette;

			printf("static int image_w = %d;\n", bmp->w);
			printf("static int image_h = %d;\n", bmp->h);
			printf("static Uint8 image_pal[] = {");
			palette = bmp->format->palette;
			for ( i=0; i<256; ++i ) {
				if ( (i%4) == 0 ) {
					printf("\n    ");
				}
				printf("0x%.2x, ", palette->colors[i].r);
				printf("0x%.2x, ", palette->colors[i].g);
				printf("0x%.2x, ", palette->colors[i].b);
			}
			printf("\n};\n");
			printf("static Uint8 image_data[] = {\n");
			for ( i=0; i<bmp->h; ++i ) {
				spot = (Uint8 *)bmp->pixels + i*bmp->pitch;
				printf("    ");
				for ( j=0; j<bmp->w; ++j ) {
					printf("0x%.2x, ", *spot++);
				}
				printf("\n");
			}
			printf("};\n");
		} else {
			fprintf(stderr,"Only 8-bit BMP images are supported\n");
			status = 3;
		}
		SDL_FreeSurface(bmp);
	} else {
		fprintf(stderr, "Unable to load %s: %s\n", argv[1],
							SDL_GetError());
		status = 2;
	}
	exit(status);
}
