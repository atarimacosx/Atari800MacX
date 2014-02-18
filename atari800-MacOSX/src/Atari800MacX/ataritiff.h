#ifndef _ATARITIFF_H_
#define _ATARITIFF_H_

/* Find unused filename for PCX screenshot
   Return pointer to static buffer or NULL on error */
char *Find_TIFF_name(void);

/* Write PCX screenshot to a file
   Return TRUE on OK, FALSE on error */
UBYTE Save_TIFF_file(char *filename);

#endif /* _ATARITIFF_H_ */
