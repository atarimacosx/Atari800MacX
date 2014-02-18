#ifndef EMUIO_H_
#define EMUIO_H_

#include "atari.h" /* UBYTE */

/* Byte write sequence to identify command write */
#define EMUIO_MAGIC1_BYTE					'E'
#define EMUIO_MAGIC2_BYTE					'M'

/* Emulator output commands */
#define EMUIO_CMD_GET_EMU_AND_HOST			0x00
#define EMUIO_CMD_GET_EMU_VERSION			0x01
#define EMUIO_CMD_LIMIT_SPEED				0x02
#define EMUIO_CMD_UNLIMIT_SPEED				0x03

/* Emulator IO return codes */
#define EMUIO_OK							0x00
#define EMUIO_ERR_INVALID_CMD_NUMBER		0x80
#define EMUIO_ERR_CMD_NOT_IMPLEMENTED		0x81
#define EMUIO_ERR_INVALID_PARAM				0x82
#define EMUIO_ERR_MISC						0xF0

/* GetEmuAndHost Emulator values */
#define EMUIO_EMU_ATARI800					0x00
#define EMUIO_EMU_ATARIPLUSPLUS				0x01
#define EMUIO_EMU_ATARI800MACX				0x02
#define EMUIO_EMU_ALTIRRA					0x03
#define EMUIO_EMU_OTHER						0x7F

/* GetEmuAndHost Host values */
#define EMUIO_HOST_WINDOWS					0x00
#define EMUIO_HOST_LINUX					0x01
#define EMUIO_HOST_MACOSX					0x02
#define EMUIO_HOST_OTHER					0x7F

void Emuio_WriteByte(UBYTE byte);
UBYTE Emuio_ReadByte();

#endif /* EMUIO_H_ */
