#include "ingenic_oss_input.h"

static struct hw_device_t *ingenicAudioDev = NULL;
static struct audio_stream_in *inAudioStream = NULL;

int load_ingenic_audio_oss(){
    int ret = 0;
    struct hw_module_t *audioModule = NULL;
      /*open audio oss*/
    if (hw_get_module(AUDIO_HARDWARE_MODULE_ID,
		      (const hw_module_t **)&audioModule) < 0) {
	printf("ERROR:Could not load audio HAL module\n");
	return -1;
    }

      //struct hw_device_t *dev;
    return audioModule->methods->open(audioModule, AUDIO_HARDWARE_INTERFACE,
				      (struct hw_device_t**)&ingenicAudioDev);
}

int open_ingenic_audio_oss_input(int sample_rate){
    int ret = 0;
    if(ingenicAudioDev == NULL)
	return -1;
    
    struct audio_hw_device *audioDev = (struct audio_hw_device *)ingenicAudioDev;
     struct audio_config inconfig = {
        8000,
        AUDIO_CHANNEL_IN_MONO,
        AUDIO_FORMAT_PCM_16_BIT,
    };

    /* if(current_hs_bcs==BSA_SCO_CODEC_MSBC) */
    /* 	inconfig.sample_rate = 16000; */

    inconfig.sample_rate = sample_rate;
    ret = audioDev->open_input_stream(audioDev,0,0,&inconfig,&inAudioStream);
    if(ret != 0)
    	return ret;
    ret = (inAudioStream->common).set_device(&(inAudioStream->common),AUDIO_DEVICE_IN_BUILTIN_MIC);
    printf("inAudioStream set device ret = %d\n", ret);
    return ret;
}

int close_ingenic_audio_oss_input(){
    if(ingenicAudioDev == NULL)
	return -1;
    struct audio_hw_device *audioDev = (struct audio_hw_device *)ingenicAudioDev;
    audioDev->close_input_stream(audioDev,inAudioStream);
    inAudioStream = NULL;
    return 0;
}

int ingenic_audio_oss_read(char* ref_buf,int size){
    if(ingenicAudioDev == NULL || inAudioStream == NULL){
	printf("Error:audio oss is not prepared.");
	return -1;
    }

    return inAudioStream->read(inAudioStream, ref_buf, size);
}
