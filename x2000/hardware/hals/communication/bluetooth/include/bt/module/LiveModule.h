#ifndef LIVE_MODULE_H_
#define LIVE_MODULE_H_

#include <stdio.h>
#include <protocol/SyncModule.h>

#define IP_ADDRESS "glass.ingenic.com"
#define SERVER_PORT 5080
#define PARAMETER_GLASS_UID "GlassUid"
#define PARAMETER_TYPE  "Type"
#define PARAMETER_ERROR_CODE  "ErrorCode"
#define PARAMETER_ERROR_MESSAGE  "ErrorMessage"
#define PARAMETER_PUSH_URL  "PushUrl"
#define PARAMETER_TALK_URL  "TalkUrl"
#define PARAMETER_STATUS "Status"
#define PARAMETER_CMP_UID "CmpUid"

enum RTMP_STATUS{
    RTMP_STATUS_NONE = 1,
    RTMP_STATUS_START,
    RTMP_STATUS_ERROR,
    RTMP_STATUS_STOP
};

namespace android {
class LiveModule : public SyncModule {
 public:
     static LiveModule* getInstance(RefBase* protocol);
     static void sendURL(string & url);
     static void sendRtmpStatus(int status);
     static void sendCameraStatus(string & status);
     virtual ~LiveModule();
 private:
    LiveModule(RefBase* protocol);
    static LiveModule* sInstance;
    void onRetrive(const sp<SyncData> & data);
    void handleRTSPLive(int oper);
    void setLiveAudioMode(int mode);
    void getRtmpUrl();
    void handleTencentLive();
    void handleRTMPLive(string url);
    pthread_t mGetRtmpUrlThread;
    static void *GetRtmpUrlThreadWrapper(void *me);
    void GetRtmpUrlThreadFunc();
    void startRtmpLive(string rtmpurl, int mode);
};
}
#endif  // LIVE_MODULE_H_
