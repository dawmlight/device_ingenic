#ifndef SETTING_MODULE_ANDROID_H_
#define SETTING_MODULE_ANDROID_H_

#include <stdio.h>
#include <protocol/SyncModule.h>
#include "WpaInterface.h"

namespace android {
class SettingModule_Android : public SyncModule {
 public:
     static SettingModule_Android* getInstance(RefBase* protocol);
     virtual ~SettingModule_Android();
 private:
    SettingModule_Android(RefBase* protocol);
    static SettingModule_Android* sInstance;
    WpaInterface* mWpaIf = NULL;
    char* mConnectedSsid = NULL;
    char* mIpAddr = NULL;
    void onRetrive(const sp<SyncData> & data);
    void sendSyncData(int type, string & values);
    void sendSyncData(int type, bool enable);
    void setPictureWaterMark(int enable);
    void setVideoWaterMark(int enable);
    void setLanguage(int type);
    int setLanguageForIos(int type);
    void sendCameraState();
    int takePicture();
    int handleVideoRecorder();
    int sendWifiInfo(int type);
    int getWifiStatus();
    int connectWifi(const char* ssid, const char* pwd);
    void setLiveAudio(int enable);
    void setSubsectionRecord(string & video_duration);
    int onWifiStateChanged(int event);
    static void wifiStateCallBack(WIFI_STATE event);
    void setLoopVideoMode(int enable);
    void setVideoResolution(string & video_resolution);
};
}
#endif  // SETTING_MODULE_ANDROID_H_
