#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include "atari.h"
#include "atrUtil.h"
#include "atrMount.h"
#include "atrDos2.h"
#include "atrErr.h"

static ADosFileEntry fileList[64];

static int mounted = FALSE;
static AtrDiskInfo *info;
static int lfconvert = FALSE;

static void printErr(int status);
static void printNotMountedErr(void);

int main(int argc, char **argv)
{
    char inBuff[1024];
    int done = FALSE;
    char *second;
    UWORD fileCount;
    ULONG freeBytes;
    int i,j;
    char XEName[20];
    char *XENamePtr;
    int status;
    int dosType;
    int readWrite;
    int writeProtect;
    static char month[13][3] = {"  ","JA","FB","MR","AP","MA","JN",
                                "JL","AG","SE","NO","DE"};

    printf("Atari Disk Image Editing Shell\n\n");

    do {
        printf("->");
        fgets(inBuff,1024,stdin);
        inBuff[strlen(inBuff) - 1 ] = 0;
        status = FALSE;

        if (strncmp(inBuff,"mount",5)==0) {
            if (mounted) 
                {
                AtrUnmount(info);
                }

            if((status = AtrMount(&inBuff[6],&dosType, 
                                  &readWrite, &writeProtect,&info)))
                {
                AtrUnmount(info);
                }
            else
                mounted = TRUE;

            switch(dosType) {
                case DOS_UNKNOWN:
                    break;
                case DOS_ATARI:
                    printf("Detected Atari DOS 2.0 compatible image\n");
                    break;
                case DOS_ATARI3:
                    printf("Detected Atari DOS 3.0 compatible image\n");
                    break;
                case DOS_ATARI4:
                    printf("Detected Atari DOS 4.0 compatible image\n");
                    break;
                case DOS_ATARIXE:
                    printf("Detected Atari DOS XE compatible image\n");
                    break;
                case DOS_MYDOS:
                    printf("Detected MYDOS compatible image\n");
                    break;
                case DOS_SPARTA2:
                    printf("Detected SpartaDOS 2 compatible image\n");
                    break;
                }
            }
        else if (strncmp(inBuff,"unmount",7)==0) {
            if (mounted) 
                AtrUnmount(info);
            mounted = FALSE;
            }
        else if (strncmp(inBuff,"exit",4)==0) {
            done = TRUE;
            }
        else if (strncmp(inBuff,"quit",4)==0) {
            done = TRUE;
            }
        else if (strncmp(inBuff,"dir",3)==0) {
            if (!mounted) 
                {
                printNotMountedErr();
                }
            else
                {
                status = AtrGetDir(info, &fileCount, fileList, &freeBytes);
                if (dosType == DOS_SPARTA2) {
                    for (i=0;i<fileCount;i++) {
                        if (fileList[i].flags & 0x20)
                            printf("* %11s %6ld %02d/%02d/%02d %02d:%02d\n",
                                   fileList[i].aname,fileList[i].bytes,
                                   fileList[i].day, fileList[i].month, 
                                   fileList[i].year,
                                   fileList[i].hour, fileList[i].minute);
                        else if (fileList[i].flags & 0x10)
                            printf("  %11s  <DIR>\n",
                                   fileList[i].aname);
                        else
                            printf("  %11s %6ld %02d/%02d/%02d %02d:%02d\n",
                                   fileList[i].aname,fileList[i].bytes,
                                   fileList[i].day, fileList[i].month, 
                                   fileList[i].year,
                                   fileList[i].hour, fileList[i].minute);
                        }
                    printf("\nFree Space = %7.1f KBytes\n",((double) freeBytes)/1024);
                    }
                else if (dosType == DOS_ATARIXE) {
                    for (i=0;i<fileCount;i++) {
                         if (fileList[i].flags & 0x20)
                            printf("* ");
                        else
                            printf("  ");
                        
                        XENamePtr = XEName;
                        for (j=0;j<8;j++) {
                           if (fileList[i].aname[j] != ' ')
                              *XENamePtr++ = fileList[i].aname[j];
                           }
                        *XENamePtr++ = '.';
                        for (j=8;j<11;j++) {
                           if (fileList[i].aname[j] != ' ')
                              *XENamePtr++ = fileList[i].aname[j];
                           }
                        if (fileList[i].flags & 0x10)
                           *XENamePtr++ = '>';
                        *XENamePtr = 0;
                        printf("%-15s  %02d%2s%02d %02d%2s%02d",
                               XEName,
                               fileList[i].createdDay, 
                               month[fileList[i].createdMonth], 
                               fileList[i].createdYear,
                               fileList[i].day, month[fileList[i].month], 

                               fileList[i].year);
                        if (fileList[i].flags & 0x10)
                            printf("       >\n");
                        else
                            printf("  %6ld\n",fileList[i].bytes);
                        }
                   printf("\nFree Space = %7.1f KBytes\n",((double) freeBytes)/1024);
                    }
                else {
                    for (i=0;i<fileCount;i++) {
                        if (fileList[i].flags & 0x20)
                            printf("* %11s %04d\n",fileList[i].aname,
                                   fileList[i].sectors);
                        else if (fileList[i].flags & 0x10)
                            printf(" :%11s %04d\n",fileList[i].aname,
                                   fileList[i].sectors);
                        else
                            printf("  %11s %04d\n",fileList[i].aname,
                                   fileList[i].sectors);
                        }
                    printf("\nFree Space = %7.1f KBytes\n",((double) freeBytes)/1024);
                    }
                }
            }
        else if (strncmp(inBuff,"del",3)==0) {
            if (!mounted) 
                {
                printNotMountedErr();
                }
            else
                {
                status = AtrDeleteFile(info,&inBuff[4]);
                }
            }
        else if (strncmp(inBuff,"cd",2)==0) {
            if (!mounted) 
                {
                printNotMountedErr();
                }
            else
                {
                if (strncmp(&inBuff[3],"..",2) == 0) 
                    status = AtrChangeDir(info,CD_UP,&inBuff[3]);
                else if (strncmp(&inBuff[3],"root",4) == 0) 
                    status = AtrChangeDir(info,CD_ROOT,&inBuff[3]);
                else
                    status = AtrChangeDir(info,CD_NAME,&inBuff[3]);
                }
            }
        else if (strncmp(inBuff,"mkdir",5)==0) {
            if (!mounted) 
                {
                printNotMountedErr();
                }
            else
                {
                status = AtrMakeDir(info,&inBuff[6]);
                }
            }
        else if (strncmp(inBuff,"rmdir",5)==0) {
            if (!mounted) 
                {
                printNotMountedErr();
                }
            else
                {
                status = AtrDeleteDir(info,&inBuff[6]);
                }
            }
        else if (strncmp(inBuff,"lock",3)==0) {
            if (!mounted) 
                {
                printNotMountedErr();
                }
            else
                {
                status = AtrLockFile(info,&inBuff[5],1);
                }
            }
        else if (strncmp(inBuff,"unlock",3)==0) {
            if (!mounted) 
                {
                printNotMountedErr();
                }
            else
                {
                status = AtrLockFile(info,&inBuff[7],0);
                }
            }
        else if (strncmp(inBuff,"rename",3)==0) {
            if (!mounted) 
                {
                printNotMountedErr();
                }
            else
                {
                if ((second = strchr(&inBuff[7],' ')) != NULL) {
                    *second++ = 0;
                    status = AtrRenameFile(info,&inBuff[7],second);
                    }
                }
            }
        else if (strncmp(inBuff,"export",6)==0) {
            if (!mounted) 
                {
                printNotMountedErr();
                }
            else
                {
                if ((second = strchr(&inBuff[7],' ')) != NULL) {
                    *second++ = 0;
                    status = AtrExportFile(info,&inBuff[7],second,lfconvert);
                    }
                }
            }
        else if (strncmp(inBuff,"import",6)==0) {
            if (!mounted) 
                {
                printNotMountedErr();
                }
            else
                {
                status = AtrImportFile(info,&inBuff[7],lfconvert);
                }
            }
        else if (strncmp(inBuff,"lf on",5)==0) 
            lfconvert = TRUE;
        else if (strncmp(inBuff,"lf off",6)==0) 
            lfconvert = FALSE;
        else 
            printf("Unknown Command\n");
        
        if (status)
        	printErr(status);
    } while (!done);

    if (mounted) 
        AtrUnmount(info);
        
    return(0);
}

static void printNotMountedErr(void)
{
    printf("ERROR - No Disk Image is Mounted\n");
}

static void printErr(int status)
{
	printf("ERROR %4x - ",status);
	switch(status) {
        case ADOS_UNKNOWN_FORMAT:
        	printf("Unsupported DOS Format\n");
        	break;
        case ADOS_FILE_NOT_FOUND:
            printf("File not found on image\n");
            break;
        case ADOS_NOT_A_ATR_IMAGE:
            printf("Not a valid image file\n");
            break;
        case ADOS_FILE_LOCKED:
        	printf("File on image is locked\n");
        	break;
        case ADOS_NOT_A_DIRECTORY:
            printf("Not a directory\n");
            break;
        case ADOS_DIR_READ_ERR:
        	printf("Error Reading Directory on image\n");
        	break;
        case ADOS_DIR_WRITE_ERR:
        	printf("Error Writing Directory on image\n");
        	break;
        case ADOS_DIR_NOT_EMPTY:
            printf("Cannot Delete Directory which is not empty\n");
            break;
        case ADOS_FILE_IS_A_DIR:
            printf("Cannot perform operation on a directory\n");
            break;
        case ADOS_DUPLICATE_NAME:
            printf("File/Directory of that name already exists\n");
            break;
        case ADOS_VTOC_READ_ERR:
        	printf("Error Reading Table of Contents on image\n");
        	break;
        case ADOS_VTOC_WRITE_ERR:
        	printf("Error Writing Table of Contents on image\n");
        	break;
        case ADOS_FILE_CORRUPTED:
        	printf("File on image is corrputed\n");
        	break;
        case ADOS_DELETE_FILE_ERR:
        	printf("Unable to delete image file, possibly corrupted\n");
        	break;
        case ADOS_DISK_FULL:
        	printf("Image file does not have enough space\n");
        	break;
        case ADOS_DIR_FULL:
        	printf("Directory on image is full\n");
        	break;
        case ADOS_FILE_WRITE_ERR:
        	printf("Error writing file on image\n");
        	break;
        case ADOS_FILE_READ_ERR:
        	printf("Error reading file on image\n");
        	break;
        case ADOS_MEM_ERR:
        	printf("Memory allocation error\n");
        	break;
        case ADOS_HOST_FILE_NOT_FOUND:
        	printf("File not found on host\n");
        	break;
        case ADOS_HOST_CREATE_ERR:
        	printf("Unable to create file on host\n");
        	break;
        case ADOS_HOST_READ_ERR:
        	printf("Error reading file on host\n");
        	break;
        case ADOS_HOST_WRITE_ERR:
        	printf("Error writing file on host\n");
        	break;
        case ADOS_FUNC_NOT_SUPPORTED:
            printf("Function not support on image DOS type\n");
            break;
        default:
            printf("Unknown Error code \n");
        	break;
	}
}

