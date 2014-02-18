#ifndef _MAC_COLOURS_H_
#define _MAC_COLOURS_H_

extern int colortable[256];

void Colours_Format(int black, int white, int colors);
void Colours_Generate(int black, int white, int colors, int shift);
int read_palette(char *filename, int adjust_palette);

#endif /* _MAC_COLOURS_H_ */
