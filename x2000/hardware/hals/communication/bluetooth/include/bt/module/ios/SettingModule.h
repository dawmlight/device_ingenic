#ifndef SETTING_MODULE_H_
#define SETTING_MODULE_H_

#include <stdio.h>
#include <protocol/SyncModule.h>

namespace android {
class SettingModule : public SyncModule {
 public:
     static SettingModule* getInstance(RefBase* protocol);
     virtual ~SettingModule();
 private:
    SettingModule(RefBase* protocol);
    static SettingModule* sInstance;
    void onRetrive(const sp<SyncData> & data);
    void setVideoWaterMark(int enable);
    void setPictureWaterMark(int enable);
    void setLiveRecord(int enable);
    void setVideoRound(int enable);
    void setVoiceRecognize(int enable);
    void setVideoDuration(int minute);
    void responseVoiceRecognizeState();
    void responseVideoRoundState();
    void responseLiveVideoState();
    void responseVideoResolution(string res);
};
}
#endif  // SETTING_MODULE_H_
