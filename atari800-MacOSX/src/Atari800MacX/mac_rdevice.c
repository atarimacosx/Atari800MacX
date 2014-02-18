/****************************************************************************
File    : mac_rdevice.c
@(#) #SY# Atari800Win PLus
@(#) #IS# R: device implementation for Win32 platforms
@(#) #BY# Daniel Noguerol, Piotr Fusik
@(#) #LM# 02.12.2001
*/

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "atari.h"
#include "cpu.h"
#include "devices.h"
#include "log.h"
#include "memory.h"

int r_device_port = 8080;

/* following is copied from devices.c */

static char filename[64];

static int Device_isvalid(UBYTE ch)
{
	int valid;

	if (ch < 0x80 && isalnum(ch))
		valid = TRUE;
	else
		switch (ch) {
		case ':':
		case '.':
		case '_':
		case '*':
		case '?':
			valid = TRUE;
			break;
		default:
			valid = FALSE;
			break;
		}

	return valid;
}

static void Device_GetFilename(void)
{
	int bufadr;
	int offset = 0;
	int devnam = TRUE;

	bufadr = (dGetByte(ICBAHZ) << 8) | dGetByte(ICBALZ);

	while (Device_isvalid(dGetByte(bufadr))) {
		int byte = dGetByte(bufadr);

		if (!devnam) {
			if (isupper(byte))
				byte = tolower(byte);

			filename[offset++] = byte;
		}
		else if (byte == ':')
			devnam = FALSE;

		bufadr++;
	}

	filename[offset++] = '\0';
}

/* Original code by Daniel Noguerol <dnoguero@earthlink.net>.

   Changes by Piotr Fusik <fox@scene.pl>:
   - removed fid in xio_34 (was unused)
   - Peek -> dGetByte, Poke -> dPutByte
   - Device_* functions renamed to match devices.c naming convention
   - connection info written to Atari800 log, not c:\\Atari800Log.txt
*/

/**********************************************/
/*             R: stuff                       */
/**********************************************/
static int do_once;

UBYTE r_dtr, r_rts, r_sd;
UBYTE r_dsr, r_cts, r_cd;
UBYTE r_parity, r_stop;
UBYTE r_error, r_in;
UBYTE r_tr = 32, r_tr_to = 32;
unsigned int r_stat;
unsigned int svainit;

char ahost[256];
struct hostent *hent;
unsigned long int haddress;

struct sockaddr_in in;
struct sockaddr_in peer_in;
int myport;
char PORT[256];
char MESSAGE[256];
int newsock;
int len;
char buf[256];
fd_set fd;
char str[256];
int retval;
char buffer[200];
char reading=0;
unsigned char one;
unsigned char bufout[256];
unsigned char bufend = 0;
char temp1;
char temp2;
int bytesread;
int concurrent;
int connected;
int sock;


void xio_34(void)
{
	int temp;

/* Controls handshake lines DTR, RTS, SD */

	temp = dGetByte(ICAX1Z);

	if (temp & 128) {
		if (temp & 64) {
/* turn DTR on */
		} else {
			if (connected != 0) {
				close(newsock);
				connected = 0;
				do_once = 0;
				concurrent = 0;
				bufend = 0;
			}
		}
	}
	regA = 1;
	regY = 1;
	ClrN;

	dPutByte(747,0);
}

void xio_36(void)
{
/* Sets baud, stop bits, and ready monitoring. */

	r_cd = dGetByte(ICAX2Z);


	regA = 1;
	regY = 1;
	ClrN;
	dPutByte(747,0);

}



void xio_38(void)
{
/* Translation and parity */

	r_tr = dGetByte(ICAX1Z);
	r_tr_to = dGetByte(ICAX2Z);

	regA = 1;
	regY = 1;
	ClrN;
	dPutByte(747,0);

}

void xio_40(void)
{

/* Sets concurrent mode.  Also checks for dropped carrier. */

/*
		if(connected == 0)
			dPutByte(747,0);
*/

	dPutByte(747,0);
	regA = 1;
	regY = 1;
	ClrN;
	concurrent = 1;


}


void Device_ROPEN(void)
{
int temp, mode;
u_long argp = 1L;
static unsigned long int HOST;
static int PORT;
char port[] = "        \n";
struct hostent *hostlookup;

/* open #1,8,23,"R:cth.tzo.com" =
   open port 23 of cth.tzo.com
*/
	temp = dGetByte(ICAX1Z);
	mode = dGetByte(0x30a);
	PORT = (int) dGetByte(regX + 0x340 + 11);
	if (temp == 8 && PORT > 0)
	{
		Aprint("R: request to dial-out intercepted.");
		Device_GetFilename();
		strcpy(ahost, filename);
		Aprint(ahost);
                sprintf(port,"%5d   \n",PORT);
		hostlookup = gethostbyname(ahost);
		if (hostlookup == NULL)
		{
			Aprint("ERROR! Cannot resolve %s.", ahost);
			dPutByte(747,0);
			regA = 170;
			regY = 170;
			SetN;
			return;
		}
		Aprint("Resolve successful.");

		HOST = ((struct in_addr *)hostlookup->h_addr)->s_addr;
		Aprint("Got HOST.\n");

		memset(&in, 0, sizeof(struct sockaddr_in));

		if ((newsock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			Aprint("Socket connect error.");
			dPutByte(747, 0);
			regA = 170;
			regY = 170;
			SetN;
			return;
		}
		in.sin_family = AF_INET;
		in.sin_port = htons(PORT);
		in.sin_addr.s_addr = HOST;

		Aprint("Ready to connect.");

		retval = connect(newsock, (struct sockaddr *)&in, sizeof(struct sockaddr_in));

		Aprint("Got retval.");

		if (retval == -1)
		{
			Aprint("Connect error.");
			dPutByte(747, 0);
			regA = 170;
			regY = 170;
			SetN;
			return;
		}

		Aprint("Successful connect.");
		ioctl(sock, FIONBIO, &argp);
		connected = 1;
		concurrent = 1;
		dPutByte(747, 0);
		regA = 1;
		regY = 1;
		ClrN;
		return;

	}

	dPutByte(747,0);
	regA = 1;
	regY = 1;
	ClrN;

}

void Device_RCLOS(void)
{
	dPutByte(747,0);
	bufend = 0;
	regA = 1;
	regY = 1;
	ClrN;
	concurrent = 0;
}

void Device_RREAD(void)
{
	int j;


	if (bufend > 0) {
		regA = bufout[1];
		regY = 1;
		ClrN;

		bufend--;
		dPutByte(747,bufend);



		j = 1;

		for (j = 1; j <= bufend; j++) {
			bufout[j] = bufout[j+1];
		}

		/* translation */
		/* heavy */

		if (r_tr == 32) { /* no translation */
		} else {
			/* light translation */
			if (regA == 13) {
				regA = 155;
			} else if (regA == 10) {
				regA = 32;
			}
		}

		return;
	}
}



void Device_RWRIT(void)
{
	if (connected != 0) {
			if (r_tr == 32) { /* no translation */
			} else {
				if (regA == 155) {
					regA = 13;
					retval = send(newsock, &regA, 1, 0);
					regA = 1;
					regY = 1;
					ClrN;
					dPutByte(749, 0);
					return;
				}
			}
			if (regA == 255)
				retval = send(newsock, &regA, 1, 0); /* IAC escape sequence */


		retval = send(newsock, &regA, 1, 0); /* returns -1 if disconnected */
		if (retval == -1) {
			Aprint("Error on R: write.");
			dPutByte(749,0);
			regA = 1;
			regY = 1;
			ClrN;


		} else {
			dPutByte(749,0);
			regA = 1;
			regY = 1;
			ClrN;
		}
		dPutByte(749,0); /* bytes waiting to be sent */
		return;
	}
	dPutByte(749,0);
	regA = 1;
	regY = 1;
	ClrN;


}

void Device_RSTAT(void)
{
//	FILE *fd;
//	unsigned int st;

	u_long argp = 1L;
	struct tm *datetime;
	time_t lTime;
        int nWSAerror;

/* are we connected? */
	regA = 1;
	regY = 1;
	ClrN;

	if (connected == 0) {

		if (do_once == 0) {

			// Set up the listening port.
			do_once = 1;
			memset ( &in, 0, sizeof ( struct sockaddr_in ) );
			sock = socket(AF_INET, SOCK_STREAM, 0);
			if (sock == -1) {
				Aprint("Unable to create socket!");
			}
			in.sin_family = AF_INET;
			in.sin_addr.s_addr = INADDR_ANY;
			in.sin_port = htons(r_device_port);
			nWSAerror = bind(sock, (struct sockaddr *) &in, sizeof(struct sockaddr_in));
			if (nWSAerror == -1) {
				Aprint("Unable to bind to socket! - Error = %d",errno);
			}
			nWSAerror = listen(sock, 5);
			if (nWSAerror == -1) {
				Aprint("Unable to listen to socket!");
			}
			retval = ioctl(sock, FIONBIO, &argp);
			len = sizeof ( struct sockaddr_in );
			connected = 0;
			bufend = 0;
			Aprint("Socket is listening.");

		}

		newsock = accept( sock, (struct sockaddr *)&peer_in, (socklen_t *) &len );
		if (newsock != -1) {
//			Aprint("Connected.");

			retval = ioctl(newsock, FIONBIO, &argp);
			connected = 1;

			// add to log
#if 0	/* original code by Daniel */
			fd = fopen("c:\\Atari800Log.txt", "a");
			time(&lTime);
			datetime = localtime(&lTime);
			fprintf(fd, "Connected ");
			fprintf(fd, "%02u", datetime->tm_hour);
			fprintf(fd, ":");
			fprintf(fd, "%02u", datetime->tm_min);
			fprintf(fd, ":");
			fprintf(fd, "%02u", datetime->tm_sec);
			fprintf(fd, "  -  ");
			fprintf(fd, "%02u/", datetime->tm_mon + 1);
			fprintf(fd, "%02u/", datetime->tm_mday);
			fprintf(fd, "%02u  -  ", datetime->tm_year % 100);
			fprintf(fd, "From %s\r\n", _INET_NTOA(peer_in.sin_addr));
			fclose(fd);
#else	/* Piotr's code */
			time(&lTime);
			datetime = localtime(&lTime);
			Aprint("Connected %02u:%02u:%02u - %02u/%02u/%02u - From %s",
				datetime->tm_hour,
				datetime->tm_min,
				datetime->tm_sec,
				datetime->tm_mon + 1,
				datetime->tm_mday,
				datetime->tm_year % 100,
				inet_ntoa(peer_in.sin_addr)
			);
#endif

			strcat((char *)bufout,"\r\n_CONNECT 2400\r\n");
			bufend = 17;
			dPutByte(747,17);
			close(sock);
			return;
		}

	}
	if (concurrent == 1) {
		bytesread = recv(newsock, &one, 1, 0);
		if (bytesread > 0) {
			if (one == 255) {
				while ((bytesread = recv(newsock, &one, 1, 0))==0) {
				}
				if (one == 255) {
					bufend++;
					bufout[bufend] = one;
					dPutByte(747,bufend);
					regA = 1;
					regY = 1;
					ClrN;
					return;
				} else {
					while ((bytesread = recv(newsock, &one, 1, 0))==0) {
					}

					regA = 1;
					regY = 1;
					ClrN;
					return;
				}
			} else {
				bufend++;
				bufout[bufend] = one;
				dPutByte(747,bufend);
				regA = 1;
				regY = 1;
				ClrN;
				return;
			}

		}
	} else {
		if (concurrent == 0 && connected == 1) {
			dPutByte(747,12);
			regA = 1;
			regY = 1;
			ClrN;
			return;
		}
	}


}

void Device_RSPEC(void)
{
	r_in = dGetByte(ICCOMZ);
/*
	Aprint( "R: device special" );

	Aprint("ICCOMZ =");
	Aprint("%d",r_in);
	Aprint("^^ in ICCOMZ");
*/

	switch (r_in) {
	case 34:
		xio_34();
		break;
	case 36:
		xio_36();
		break;
	case 38:
		xio_38();
		break;
	case 40:
		xio_40();
		break;
	default:
		Aprint("Unsupported XIO #.");
		break;
	}
/*
	regA = 1;
	regY = 1;
	ClrN;
*/

}

//---------------------------------------------------------------------------
// R Device INIT vector - called from Atari OS Device Handler Address Table
//---------------------------------------------------------------------------
void Device_RINIT(void)
{
  regA = 1;
  regY = 1;
  ClrN;
}

int enable_r_patch = 1;

/* call in Atari_Exit() */
void RDevice_Exit()
{
    if (connected) {
         close(newsock);
         close(sock);
         connected = 0;
    }
}
