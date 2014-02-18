/* atrUtil.c -  
 *  Part of the Atari Disk Image File Editor Library
 *  Mark Grebe <atarimac@kc.rr.com>
 *  
 * Based on code from:
 *    Atari800 Emulator (atari800.sourceforge.net)
 *    Adir v0.67 (c) 1999-2001 Jindrich Kubec <kubecj@asw.cz>
 *    Atr8fs v0.1  http://www.rho-sigma.de/atari8bit/fs.html
 *
 * Copyright (C) 2004 Mark Grebe
 *
 * Atari Disk Image File Editor Library is free software; 
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atari Disk Image File Editor Library is distributed in the hope 
 * hat it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari Disk Image File Editor Library; if not, write to the 
 * Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/fcntl.h>
#include "atari.h"
#include "atrUtil.h"

void ADos2Host( char* host, unsigned char* aDos )
{
	char name[ 9 ];
	char ext[ 4 ];

	strncpy( name, (const char *) aDos, 8 );
	name[ 8 ] = '\0';

	strncpy( ext, (const char *) aDos + 8, 3 );
	ext[ 3 ] = '\0';

	NameFilter( name, name );
	NameFilter( ext, ext );

	if ( !*name )
		strcpy( name, "out" );

	strcpy( host, name );

	if ( *ext )
	{
		strcat( host, "." );
		strcat( host, ext );
	}
}

void Host2ADos( char* host, unsigned char* aDos )
{
	char *ext;
    int namelen, extlen, i;

    for (i=0;i<strlen(host);i++) 
        {
        if (islower(host[i]))
            host[i] -= 32;
        }

    if ((ext = strchr(host,'.')) == NULL) {
        namelen = strlen(host);
        extlen = 0;
        }
    else {
        *ext = 0;
        ext++;
        namelen = strlen(host);
        extlen = strlen(ext);
    }

    if (namelen <= 8) {
        memcpy(aDos,host,namelen);
        for (i=0;i<8-namelen;i++) 
            aDos[namelen+i] = ' ';
        }
    else
        memcpy(aDos,host,8);

    if (extlen <= 3) {
        memcpy(aDos+8,ext,extlen);
        for (i=0;i<3-extlen;i++) 
            aDos[8+extlen+i] = ' ';
        }
    else
        memcpy(aDos+8,ext,3);
}

void NameFilter( char* host, char* atari )
{
	int len = strlen( atari );

	while( len-- )
	{
		//filter off the inverse chars
		char c = *( atari++ ) & 0x7F;

		//filter spaces
		if ( c == ' ' )
			continue;

		//filter unprintables
		if ( !isprint( c ) )
			c = '_';

		//filter other 'ugly' characters :)
		switch( c )
		{
			case '*':
			case ':':
			case '\"':
			case ',':
			case '.':
			case '|':
			case '?':
			case '/':
			case '\\':
				c = '_';
				break;
		}

		*( host++ ) = c;
	}

	*host = '\0';
}

void AtariLFToHost(unsigned char *buffer, int len)
{
	int i;
	unsigned char *ptr = buffer;

	for (i=0;i<len;i++,ptr++) {
		if (*ptr == 0x9b)
			{
			*ptr = 0x0a;
			}
		}
}

void HostLFToAtari(unsigned char *buffer, int len)
{
	int i;
	unsigned char *ptr = buffer;
	
	for (i=0;i<len;i++,ptr++) {
		if (*ptr == 0x0a)
			*ptr = 0x9b;
		}
}

