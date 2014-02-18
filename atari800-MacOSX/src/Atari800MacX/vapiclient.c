#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include "vapiclient.h"

#define CMD_PORT  5000
#define DATA_PORT 5001

static int insocket, outsocket;
static struct sockaddr_in serveraddr;

void VapiInit(void)
{
	static struct sockaddr_in myaddr;
	
    insocket = socket( PF_INET, SOCK_DGRAM, 0);
	if (insocket < 0) { printf("In sock create error\n"); return; }
   	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons( CMD_PORT );
	serveraddr.sin_addr.s_addr = htonl( 0x7f000001 );
    outsocket = socket( PF_INET, SOCK_DGRAM, 0);
	if (outsocket < 0) { printf("Out sock create error\n"); return; }
    myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons( DATA_PORT );
	myaddr.sin_addr.s_addr = htonl( INADDR_ANY );
	bind(insocket, &myaddr, sizeof(myaddr));

    printf("insocket = %d outsocket = %d\n",insocket,outsocket);
}

int VapiLoadImage(char *filename)
{
    unsigned char status;
    int result;
    unsigned char cmdBuff[520];
    
    cmdBuff[0] = 'F';
    strcpy(&cmdBuff[1],filename);
    
    sendto(outsocket, cmdBuff, strlen(filename)+2, 0, &serveraddr, sizeof(serveraddr));

    /* Read the status */
	result = recvfrom(insocket,&status, 1, 0, NULL, NULL);
    if (status == 'A') 
        return(1);
    else
        return(0);
}

unsigned int VapiSectorRead(int sector, unsigned int time, 
                            unsigned int *count, unsigned char *buffer) 
{
    unsigned char cmdBuff[16];
    unsigned int delay;
    int result;

    /* Sector read command */
    cmdBuff[0] = 'R';
    *((unsigned int *) &cmdBuff[1]) = time;  //time
    *((int *) &cmdBuff[5]) = sector; //sector
    sendto(outsocket, cmdBuff, 9, 0, &serveraddr, sizeof(serveraddr));

    /* Read the status */
	result = recvfrom( insocket, buffer, 520, 0, NULL, NULL);
	delay = *((unsigned int *) buffer);
	*count = *((unsigned int *) &buffer[4]);

    return(delay);
}

unsigned int VapiDriveStatus(unsigned int time, unsigned int *count, 
                             unsigned char *buffer) 
{
    unsigned char cmdBuff[16], status;
    unsigned int delay;
    int result;

    /* Drive status command */
    cmdBuff[0] = 'S';
    *((unsigned int *) &cmdBuff[1]) = 0;  //time
    sendto(outsocket, cmdBuff, 5, 0, &serveraddr, sizeof(serveraddr));

    /* Read the status */
	result = recvfrom( insocket, buffer, 520, 0, NULL, NULL);
	delay = *((unsigned int *) buffer);
	*count = *((unsigned int *) &buffer[4]);
	
    return(delay);
}

void VapiStopServer(void)
{
    sendto(outsocket, "X", 1, 0, &serveraddr, sizeof(serveraddr));
}

int vapimain(void) {
    unsigned int count, delay;
    int i;
    unsigned char dataBuff[520];

    VapiInit();

    if (!VapiLoadImage("Ankh.atx"))
        {
        printf("Failed to load image file\n");
        VapiStopServer();
        return(0);
        }

    /* Sector read command */

    delay = VapiSectorRead(4, 0, &count, dataBuff);
    printf("Sector Delay = %d\n",delay);
    printf("Sector Count = %d\n",count);
    for (i=0;i<10;i++) 
        printf("Data Buff[%d] 0x%x\n",i,dataBuff[i+8]);


    /* Drive status command */

    delay = VapiDriveStatus(100000, &count, dataBuff);
    printf("Status Delay = %d\n",delay);
    printf("Status Count = %d\n",count);
    for (i=0;i<10;i++) 
        printf("Data Buff[%d] 0x%x\n",i,dataBuff[i+8]);

    VapiStopServer();

    return(0);
}
