/*****************************************************************************
 **
 **  Name:           ingenic_oss_output.h
 **
 **  Description:    send audio pcm to oss device by AudioSystem
 **
 *****************************************************************************/
#ifndef INGENIC_OSS_OUTPUT_H_
#define INGENIC_OSS_OUTPUT_H_

/*******************************************************************************
 **
 ** Function         open_ingenic_audio_oss_output
 **
 ** Description      connect audio_socket server which create in libmedia/AudioReceiver.cpp
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
void open_ingenic_audio_oss_output(char * mode,int mode_size);

/*******************************************************************************
 **
 ** Function         close_ingenic_audio_oss_output
 **
 ** Description      close audio_socket connect
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
void close_ingenic_audio_oss_output();

/*******************************************************************************
 **
 ** Function         ingenic_audio_oss_write
 **
 ** Description      write pcm data to server which create in libmedia/AudioReceiver.cpp
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int ingenic_audio_oss_write(const char* data,int size);

#endif // INGENIC_OSS_OUTPUT_H_
