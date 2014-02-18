/****************************************************************************
File    : mac_rdevice.h
@(#) #SY# Atari800Win PLus
@(#) #IS# R: device public methods and objects prototypes
@(#) #BY# Daniel Noguerol, Piotr Fusik
@(#) #LM# 04.10.2001
*/

#ifndef __RDEVICE_H__
#define __RDEVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int enable_r_patch;
extern int r_device_port;

#define R_DEVICE_BEGIN	0xd180
#define R_TABLE_ADDRESS	0xd180
#define R_PATCH_OPEN	0xd190
#define R_PATCH_CLOS	0xd193
#define R_PATCH_READ	0xd196
#define	R_PATCH_WRIT	0xd199
#define R_PATCH_STAT	0xd19c
#define R_PATCH_SPEC	0xd19f
#define R_DEVICE_END	0xd1a1

/* call in Atari_Exit() */
void RDevice_Exit();

#ifdef __cplusplus
}
#endif

#endif /*__RDEVICE_H__*/
