/*****************************************************************************
 **
 **  Name:           ingenic_oss_input.h
 **
 **  Description:    get audio pcm from oss device
 **
 *****************************************************************************/

/* idempotency */
#ifndef INGENIC_OSS_INPUT_H_
#define INGENIC_OSS_INPUT_H_

#include <hardware/hardware.h>
#include <hardware/audio.h>

/*******************************************************************************
 **
 ** Function         load_ingenic_audio_oss
 **
 ** Description      load oss module.just need be called one time for a process
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int load_ingenic_audio_oss();

/*******************************************************************************
 **
 ** Function         open_ingenic_audio_oss_input
 **
 ** Description      open oss input stream.and set some param
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int open_ingenic_audio_oss_input(int sample_rate);

/*******************************************************************************
 **
 ** Function         close_ingenic_audio_oss_input
 **
 ** Description      close oss input stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int close_ingenic_audio_oss_input();

/*******************************************************************************
 **
 ** Function         ingenic_audio_oss_read
 **
 ** Description      get pcm data from oss
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int ingenic_audio_oss_read(char* ref_buf,int size);

#endif //INGENIC_OSS_INPUT_H_
