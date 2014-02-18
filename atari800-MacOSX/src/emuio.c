/*
 * emuio.c - emulation IO 
 *
 * Copyright (C) 2010 Atari800 development team (see DOC/CREDITS)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "config.h"
#include "atari.h"
#include "emuio.h"

#define EMUIO_CMD_MAX_VALID_CMD				EMUIO_CMD_UNLIMIT_SPEED

#define WAIT_MAGIC1_STATE	0
#define WAIT_MAGIC2_STATE	1
#define WAIT_CMD_STATE		2
#define WAIT_PARM1_STATE	3
#define WAIT_PARM2_STATE	4
#define ERR_STATE			5
#define CMD_COMPLETE_STATE	6
#define RET1_READY_STATE	7
#define RET2_READY_STATE	8


#ifdef MACOSX
extern int speed_limit;
extern int requestLimitChange;
#endif

static UBYTE cmd, status;
static UBYTE param1, param2;
static UBYTE ret1, ret2;
static int state = WAIT_MAGIC1_STATE;
static int num_params, num_rets;
static int param_count[EMUIO_CMD_MAX_VALID_CMD+1] = 
	{0,0,0,0};
static int ret_count[EMUIO_CMD_MAX_VALID_CMD+1] = 
	{2,2,0,0};

static UBYTE Emuio_ExecuteGetEmuAndHost()
{
#ifdef MACOSX
	ret1 = EMUIO_EMU_ATARI800MACX;
#else
	ret1 = EMUIO_EMU_ATARI800;
#endif
#ifdef WIN32
	ret2 = EMUIO_HOST_WINDOWS;
#else
#ifdef LINUX
	ret2 = EMUIO_HOST_LINUX;
#else
#ifdef MACOSX
	ret2 = EMUIO_HOST_MACOSX;
#else
	ret2 = EMUIO_HOST_OTHER;
#endif
#endif
#endif
	return EMUIO_OK;
}

static UBYTE Emuio_ExecuteGetEmuVersion()
{
#ifdef MACOSX	
	ret1 = 0x04;
	ret2 = 0x31;
#else
	ret1 = 0x02;
	ret2 = 0x02;
#endif
	return EMUIO_OK;
}

static UBYTE Emuio_ExecuteLimitSpeed()
{
#ifdef MACOSX
	if (!speed_limit)
		requestLimitChange = TRUE;
#else
#endif		
	return EMUIO_OK;
}

static UBYTE Emuio_ExecuteUnlimitSpeed()
{
#ifdef MACOSX
	if (speed_limit)
		requestLimitChange = TRUE;
#else
#endif		
	return EMUIO_OK;
}

UBYTE Emuio_ExecuteCommand(UBYTE cmd, UBYTE parm1, UBYTE parm2)
{
	UBYTE status;
	
	switch(cmd) {
		case EMUIO_CMD_GET_EMU_AND_HOST:
			status = Emuio_ExecuteGetEmuAndHost();
			break;
		case EMUIO_CMD_GET_EMU_VERSION:
			status = Emuio_ExecuteGetEmuVersion();
			break;
		case EMUIO_CMD_LIMIT_SPEED:
			status = Emuio_ExecuteLimitSpeed();
			break;
		case EMUIO_CMD_UNLIMIT_SPEED:
			status = Emuio_ExecuteUnlimitSpeed();
			break;
	}
	
	return(status);
}


void Emuio_WriteByte(UBYTE byte)
{
	switch(state)
	{
		case WAIT_MAGIC1_STATE:
		case CMD_COMPLETE_STATE:
		case ERR_STATE:
		case RET1_READY_STATE:
		case RET2_READY_STATE:
			if (byte == EMUIO_MAGIC1_BYTE)
				state = WAIT_MAGIC2_STATE;
			else 
				state = WAIT_MAGIC1_STATE;
			break;
		case WAIT_MAGIC2_STATE:
			if (byte == EMUIO_MAGIC2_BYTE)
				state = WAIT_CMD_STATE;
			else if (byte != EMUIO_MAGIC1_BYTE)
				state = WAIT_MAGIC1_STATE;
			break;
		case WAIT_CMD_STATE:
			if (byte <= EMUIO_CMD_MAX_VALID_CMD)
				{
				num_params = param_count[byte];
				num_rets = ret_count[byte];
				cmd = byte;
				if (num_params == 0) 
					{
					status = Emuio_ExecuteCommand(cmd, 0, 0);
					state = CMD_COMPLETE_STATE;
					}
				else  
					{
					state = WAIT_PARM1_STATE;
					}
				}
			else 
				{
				status = EMUIO_ERR_INVALID_CMD_NUMBER;
				state = ERR_STATE;
				}
			break;
		case WAIT_PARM1_STATE:
			param1 = byte;
			if (num_params == 1)
				{
				status = Emuio_ExecuteCommand(cmd, param1, 0);
				state = CMD_COMPLETE_STATE;
				}
			else  
				{
				state = WAIT_PARM2_STATE;
				}
			break;
		case WAIT_PARM2_STATE:
			param2 = byte;
			status = Emuio_ExecuteCommand(cmd, param1, param2);
			state = CMD_COMPLETE_STATE;
			break;
	}
}

UBYTE Emuio_ReadByte()
{
	UBYTE val;
	
	switch(state)
	{
		case WAIT_MAGIC1_STATE:
		case WAIT_MAGIC2_STATE:
		case WAIT_CMD_STATE:
		case WAIT_PARM1_STATE:
		case WAIT_PARM2_STATE:
			state = WAIT_MAGIC1_STATE;
			val  =0xFF;
			break;
		case ERR_STATE:
			state = WAIT_MAGIC1_STATE;
			val = status;
			break;
		case CMD_COMPLETE_STATE:
			if (num_rets == 0)
				state = WAIT_MAGIC1_STATE;
			else
				state = RET1_READY_STATE;
			val = status;
			break;
		case RET1_READY_STATE:
			if (num_rets == 1)
				state = WAIT_MAGIC1_STATE;
			else
				state = RET2_READY_STATE;
			val = ret1;
			break;
		case RET2_READY_STATE:
			state = WAIT_MAGIC1_STATE;
			val = ret2;
			break;
	}
	return(val);
}

